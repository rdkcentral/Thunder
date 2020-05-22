/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Module.h"
#include "SDPSocket.h"


namespace WPEFramework {

namespace Bluetooth {

    enum sizetype {
        SIZE_8 = 0,
        SIZE_16 = 1,
        SIZE_32 = 2,
        SIZE_64 = 3,
        SIZE_128 = 4,
        SIZE_U8_FOLLOWS = 5,
        SIZE_U16_FOLLOWS = 6,
        SIZE_U32_FOLLOWS = 7
    };

    void SDPSocket::Payload::PushDescriptor(const elementtype type, const uint32_t size)
    {
        ASSERT(Free() >= 1);

        uint8_t* buffer = &_buffer[_writerOffset];
        uint32_t offset = 0;

        buffer[offset++] = (type | SIZE_8);

        switch (type) {
            case NIL:
                ASSERT(size == 0);
                // Exception: even if size descriptor says BYTE for NIL type,
                // actually there's no data following.
                break;
            case BOOL:
                ASSERT(size == 1);
                break;
            case INT:
            case UINT:
                if (size == 1) {
                    // already set
                } else if (size == 2) {
                    buffer[0] |= SIZE_16;
                } else if (size == 4) {
                    buffer[0] |= SIZE_32;
                } else if (size == 8) {
                    buffer[0] |= SIZE_64;
                } else {
                    ASSERT(false && "Invalid INT size");
                }
                break;
            case UUID:
                if (size == 2) {
                    buffer[0] |= SIZE_16;
                } else if (size == 4) {
                    buffer[0] |= SIZE_32;
                } else if (size == 16) {
                    buffer[0] |= SIZE_128;
                } else {
                    ASSERT(false && "Invalid UUID size");
                }
                break;
            case TEXT:
            case SEQ:
            case ALT:
            case URL:
                if (size <= 0xFF) {
                    ASSERT(Free() >= 1);
                    buffer[0] |= SIZE_U8_FOLLOWS;
                } else if (size <= 0xFFFF) {
                    ASSERT(Free() >= 2);
                    buffer[0] |= SIZE_U16_FOLLOWS;
                    buffer[offset++] = (size >> 8);
                } else {
                    ASSERT(Free() >= 4);
                    buffer[0] |= SIZE_U32_FOLLOWS;
                    buffer[offset++] = (size >> 24);
                    buffer[offset++] = (size >> 16);
                    buffer[offset++] = (size >> 8);
                }
                buffer[offset++] = size;
                break;
        }

        _writerOffset += offset;
    }

    uint8_t SDPSocket::Payload::PopDescriptor(elementtype& type, uint32_t& size) const
    {
        uint8_t offset = 0;
        uint8_t t = _buffer[_readerOffset + offset++];

        switch (t & 7) {
        case SIZE_8:
            size = 1;
            break;
        case SIZE_16:
            size = 2;
            break;
        case SIZE_32:
            size = 4;
            break;
        case SIZE_64:
            size = 8;
            break;
        case SIZE_128:
            size = 16;
            break;
        case SIZE_U8_FOLLOWS:
            size = _buffer[_readerOffset + offset++];
            break;
        case SIZE_U16_FOLLOWS:
            size = (_buffer[_readerOffset + offset++] << 8);
            size |= _buffer[_readerOffset + offset++];
            break;
        case SIZE_U32_FOLLOWS:
            size = (_buffer[_readerOffset + offset++] << 24);
            size |= (_buffer[_readerOffset + offset++] << 16);
            size |= (_buffer[_readerOffset + offset++] << 8);
            size |= _buffer[_readerOffset + offset++];
            break;
        default:
            TRACE_L1("SDP: Unexpected descriptor size [0x%01x]", (t & 7));
            size = 0;
            break;
        }

        type = static_cast<elementtype>(t & 0xF8);
        if (type == NIL) {
            size = 0;
        }

        return (offset);
    }

    uint16_t SDPSocket::Command::Response::Deserialize(const uint16_t reqTransactionId, const uint8_t stream[], const uint16_t length)
    {
        uint16_t result = 0;

        printf("SDP received [%d]: ", length);
        for (uint8_t index = 0; index < (length - 1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[length - 1]);

        if (length >= PDU::HEADER_SIZE) {
            Payload header(stream, PDU::HEADER_SIZE);
            uint16_t transactionId;
            uint16_t payloadLength;

            // Pick up the response header
            header.Pop(_type);
            header.Pop(transactionId);
            header.Pop(payloadLength);

            if (reqTransactionId == transactionId) {
                if (length >= (header.Length() + payloadLength)) {
                    Payload parameters((stream + header.Length()), payloadLength);

                    switch(_type) {
                    case PDU::ErrorResponse:
                        parameters.Pop(_status);
                        break;
                    case PDU::ServiceSearchResponse:
                        // In order to tell if the packet is complete we need to parse the payload upfront,
                        // so let's deserialize the data along the way, too.
                        _status = DeserializeServiceSearchResponse(parameters);
                        break;
                    case PDU::ServiceAttributeResponse:
                    case PDU::ServiceSearchAttributeResponse: // same response
                        _status = DeserializeServiceAttributeResponse(parameters);
                        break;
                    default:
                        _status = PDU::DeserializationFailed;
                        break;
                    }

                    result = length;
                } else {
                    TRACE_L1("SDP response too short [%d]", length);
                }
            } else {
                TRACE_L1("Unexpected transaction Id [%d vs %d]", reqTransactionId, transactionId);
            }
        }

        return (result);
    }

    SDPSocket::Command::PDU::errorid SDPSocket::Command::Response::DeserializeServiceSearchResponse(const SDPSocket::Payload& params)
    {
        PDU::errorid result = PDU::DeserializationFailed;

        ASSERT(Type() == PDU::ServiceSearchResponse);

        if (params.Length() >= 5) {
            Payload payload;
            uint16_t totalCount = 0;
            uint16_t currentCount = 0;

            params.Pop(totalCount);

            // Pick up the payload, but not process it yet, wait until the chain of continued packets ends.
            params.Pop(currentCount);
            params.Peek(payload, (currentCount * sizeof(uint32_t)));
            _payload.Push(payload);

            // Get continuation data.
            Payload::Continuation cont;
            params.Pop(cont, _continuationData);

            if (cont == Payload::Continuation::ABSENT) {
                // No more continued packets, process all the concatenated payloads...
                // The payload is a list of DWORD handles.
                _payload.Pop(_handles, (_payload.Length() / sizeof(uint32_t)));
                result = PDU::Success;
            } else {
                result = PDU::PacketContinuation;
            }
        } else {
            TRACE_L1("Truncated payload in ServiceSearchResponse [%d]", params.Length());
        }

        return (result);
    }

    SDPSocket::Command::PDU::errorid SDPSocket::Command::Response::DeserializeServiceAttributeResponse(const SDPSocket::Payload& params)
    {
        PDU::errorid result = PDU::DeserializationFailed;

        ASSERT((Type() == PDU::ServiceAttributeResponse) || (Type() == PDU::ServiceSearchAttributeResponse));

        if (params.Length() >= 2) {
            uint16_t byteCount = 0;
            Payload payload;

            // Pick up the payload, but not process it yet, wait until the chain of continued packets ends.
            params.Pop(byteCount);
            params.Peek(payload, byteCount);
            _payload.Push(payload);

            // Get continuation data.
            Payload::Continuation cont;
            params.Pop(cont, _continuationData);

            if (cont == Payload::Continuation::ABSENT) {
                // No more continued packets, process all the concatenated payloads...
                // The payload is a sequence of attribute:value pairs (where value can be a sequence too).

                _payload.Pop(use_descriptor, [&](const Payload& sequence) {
                    while (sequence.Available() > 2) {
                        uint32_t attribute = 0;
                        Buffer value;

                        // Pick up the pair and store it.
                        sequence.Pop(use_descriptor, attribute);
                        sequence.Pop(use_descriptor, value);
                        _attributes.emplace(std::piecewise_construct,
                                            std::forward_as_tuple(attribute),
                                            std::forward_as_tuple(value));
                    }

                    if (sequence.Available() == 0) {
                        result = PDU::Success;
                    }
                });
            } else {
                result = PDU::PacketContinuation;
            }
        } else {
            TRACE_L1("Truncated payload in ServiceAttributeResponse [%d]", params.Length());
        }

        return (result);
    }

} // namespace Bluetooth

}
