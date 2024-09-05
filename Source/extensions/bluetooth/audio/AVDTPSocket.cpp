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
#include "AVDTPSocket.h"

namespace Thunder {

namespace Bluetooth {

namespace AVDTP {

    uint16_t Signal::Serialize(uint8_t stream[], const uint16_t length) const
    {
        ASSERT(stream != nullptr);

        constexpr uint8_t HeaderSizeSingle = 2;
        constexpr uint8_t HeaderSizeStart = 3;
        constexpr uint8_t HeaderSizeContinue = 1;

        DataRecord msg(stream, length, 0);
        packettype pktType{};

        if (_payload.Length() + HeaderSizeSingle <= length) {

            if (_processedPackets == 0) {
                ASSERT(_expectedPackets == 0);

                pktType = packettype::SINGLE;
                _expectedPackets = 1;
            }
        }
        else {
            // Have to fragment the signal...
            if (_processedPackets == 0) {
                ASSERT(_expectedPackets == 0);

                _expectedPackets = ((_payload.Length() + HeaderSizeStart + (length - HeaderSizeContinue) - 1) / (length - HeaderSizeContinue));
                pktType = packettype::START;
            }
            else if (_processedPackets == (_expectedPackets - 1)) {
                pktType = packettype::END;
            }
            else {
                pktType = packettype::CONTINUE;
            }
        }

        if (_processedPackets != _expectedPackets) {

            if (_processedPackets == 0) {
                TRACE_L1("AVDTP: sending %s", AsString().c_str());
            }

            msg.Push(static_cast<uint8_t>((_label << 4) | (static_cast<uint8_t>(pktType) << 2) | static_cast<uint8_t>(_type)));

            if (pktType == packettype::START) {
                msg.Push(_expectedPackets);
            }

            if ((pktType == packettype::START) || (pktType == packettype::SINGLE)) {
                msg.Push(static_cast<uint8_t>(_id & 0x3F));
            }

            const uint16_t payloadSize = std::min((length - msg.Length()), (_payload.Length() - _offset));

            if (payloadSize != 0) {
                Payload payload((_payload.Data() + _offset), payloadSize);
                msg.Push(payload);
            }

            _processedPackets++;
        }

        return (msg.Length());
    }

    uint16_t Signal::Deserialize(const uint8_t stream[], const uint16_t length)
    {
        ASSERT(stream != nullptr);

        bool truncated = false;
        packettype pktType{};
        DataRecord msg(stream, length);

        if (msg.Available() >= 1) {
            uint8_t octet;
            msg.Pop(octet);

            pktType = static_cast<packettype>((octet >> 2) & 0x3);

            if ((pktType == packettype::START) || (pktType == packettype::SINGLE)) {
                _expectedPackets = 1;
                _label = (octet >> 4);
                _type = static_cast<messagetype>(octet & 0x3);
            }
        }
        else {
            truncated = true;
        }

        if ((truncated == false) && (pktType == packettype::START)) {
            if (msg.Available() >= 1) {
                msg.Pop(_expectedPackets);
            }
            else {
                truncated = true;
            }
        }

        if ((truncated == false) && ((pktType == packettype::START) || (pktType == packettype::SINGLE))) {
            if (msg.Available() >= 1) {

                uint8_t octet;
                msg.Pop(octet);

                _id = static_cast<signalidentifier>(octet & 0x3F);
            }
            else {
                truncated = true;
            }
        }

        if (truncated == false) {
            if ((_type == messagetype::RESPONSE_ACCEPT) || (_type == messagetype::COMMAND)) {
                msg.Pop(_payload, msg.Available());
                _errorCode = errorcode::IN_PROGRESS;
            }
        }
        else {
            TRACE_L1("AVDTP: truncated signal data");
            _errorCode = errorcode::GENERAL_ERROR;
        }

        _processedPackets++;

        if (IsComplete() == true) {

            if (IsValid() == false) {
                TRACE_L1("Broken frame received");
                _errorCode = errorcode::GENERAL_ERROR;
            }
            else if (_type == messagetype::RESPONSE_ACCEPT) {
                _errorCode = errorcode::SUCCESS;
            }
            else if (_type == messagetype::RESPONSE_REJECT) {

                if ((_id == AVDTP_SET_CONFIGURATION) || (_id == AVDTP_RECONFIGURE)
                        || (_id == AVDTP_START) || (_id == AVDTP_SUSPEND)) {

                    // These signal responses also include the endpoint id that failed
                    uint8_t octet{};
                    msg.Pop(octet);
                }

                if (msg.Available() >= sizeof(_errorCode)) {
                    msg.Pop(_errorCode);
                }
                else {
                    TRACE_L1("AVDTP: truncated signal data");
                    _errorCode = errorcode::GENERAL_ERROR;
                }
            }
            else {
                // GENERIC_REJECT or anything else unexpected
                _errorCode = errorcode::GENERAL_ERROR;
            }

            TRACE_L1("AVDTP: received %s; result: %d", AsString().c_str(), _errorCode);
        }

        return (length);
    }

} // namespace AVDTP

} // namespace Bluetooth

}
