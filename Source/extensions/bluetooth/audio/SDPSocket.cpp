/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

namespace Thunder {

namespace Bluetooth {

namespace SDP {

    void Payload::PushDescriptor(const elementtype type, const uint32_t size)
    {
        ASSERT(_buffer != nullptr);
        ASSERT(Free() >= 1);

        uint8_t* buffer = &_buffer[_writerOffset];
        uint32_t offset = 0;

        buffer[offset++] = (type | SIZE_8);

        switch (type) {
        case NIL:
            ASSERT(size == 0);
            // Exception: even if size descriptor says BYTE, for NIL type there's no data following.
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

    uint8_t Payload::ReadDescriptor(elementtype& type, uint32_t& size) const
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

    uint16_t PDU::Serialize(uint8_t stream[], const uint16_t length) const
    {
        ASSERT(stream != nullptr);

        // The request must always fit into MTU!
        ASSERT((HeaderSize + _payload.Length()) <= length);

        DataRecordBE msg(stream, length, 0);

        if (_errorCode != InProgress) {
            msg.Push(_type);
            msg.Push(_transactionId);

            if (Type() == PDU::ErrorResponse) {
                msg.Push<uint16_t>(2);
                msg.Push(_errorCode);
            } else {
                msg.Push<uint16_t>(_payload.Length());
                msg.Push(_payload);
            }

            _errorCode = InProgress;
        }

        if (msg.Length() != 0) {
            TRACE_L1("SDP: sent %s; result: %d", AsString().c_str(), _errorCode);
        }

        return (msg.Length());
    }

    uint16_t PDU::Deserialize(const uint8_t stream[], const uint16_t length)
    {
        ASSERT(stream != nullptr);

        DataRecordBE msg(stream, length);
        bool truncated = false;

        _errorCode = Success;

        if (msg.Available() >= sizeof(_type)) {
            msg.Pop(_type);
        } else {
            truncated = true;
        }

        if (truncated == false) {
            if (msg.Available() >= (sizeof(_transactionId) + sizeof(uint16_t))) {
                msg.Pop(_transactionId);

                uint16_t payloadLength{};
                msg.Pop(payloadLength);

                if (payloadLength > 0) {
                    if (msg.Available() >= payloadLength) {
                        if (Type() == PDU::ErrorResponse) {
                            if (payloadLength >= 2) {
                                msg.Pop(_errorCode);
                            } else {
                                truncated = true;
                            }
                        } else {
                            msg.Pop(_payload, payloadLength);
                        }
                    } else {
                        truncated = true;
                    }
                }
            } else {
                truncated = true;
            }
        }

        if (truncated == true) {
            TRACE_L1("SDP: Message was truncated");
            _errorCode = DeserializationFailed;
        }

        TRACE_L1("SDP: received %s; result: %d", AsString().c_str(), _errorCode);

        return (length);
    }

} // namespace SDP

} // namespace Bluetooth

}
