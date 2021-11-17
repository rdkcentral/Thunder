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

namespace WPEFramework {

namespace Bluetooth {

    uint16_t AVDTPSocket::Command::Response::Deserialize(const uint8_t expectedLabel, const uint8_t stream[], const uint16_t length)
    {
        uint16_t result = 0;

        CMD_DUMP("AVDTP received", stream, length);

        Signal::signalidentifier signalId{};
        Signal::packettype pktType{};
        Signal::messagetype msgType{};
        uint8_t label = 0;
        uint8_t packets = 1;
        Signal signal(stream, length);

        signal.Pop(label, signalId, msgType, pktType, packets);
        if (label == expectedLabel) {
            if (msgType == Signal::RESPONSE_ACCEPT) {
                _status = Signal::SUCCESS;
                signal.Pop(_payload, signal.Available());
            } else if (msgType == Signal::RESPONSE_REJECT) {
                if ((signalId == Signal::AVDTP_SET_CONFIGURATION)
                        || (signalId == Signal::AVDTP_RECONFIGURE)
                        || (signalId == Signal::AVDTP_START)
                        || (signalId == Signal::AVDTP_SUSPEND)) {

                    // These signal responses also include the endpoint id that failed
                    uint8_t failedSeid{};
                    signal.Pop(failedSeid);
                }

                signal.Pop(_status);
            } else {
                // GENERIC_REJECT or anything else unexpected
                _status = Signal::GENERAL_ERROR;
            }

            _type = signalId;
            result = length;
        } else {
            TRACE_L1("Out of order signal label [got %d, expected %d]", label, expectedLabel);
        }

        return (result);
    }

    void AVDTPSocket::Command::Response::ReadDiscovery(const std::function<void(const Buffer&)>& handler) const
    {
        // Split the payload into SEP sections and pass to the handler for deserialisation
        ASSERT(Type() == Signal::signalidentifier::AVDTP_DISCOVER);

        while (_payload.Available() >= 2) {
            Buffer sep;
            _payload.Pop(sep, 2);
            handler(sep);
        }
    }

    void AVDTPSocket::Command::Response::ReadConfiguration(const std::function<void(const uint8_t /* category */, const Buffer&)>& handler) const
    {
        // Split the payload into configuration sections and pass to the handler for deserialisation
        while (_payload.Available() >= 2) {
            uint8_t category{};
            uint8_t length{};
            Buffer config;
            _payload.Pop(category);
            _payload.Pop(length);
            if (length > 0) {
                _payload.Pop(config, length);
            }
            handler(category, config);
        }
    }

} // namespace Bluetooth

}
