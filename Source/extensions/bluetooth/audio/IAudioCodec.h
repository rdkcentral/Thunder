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

#pragma once

#include "Module.h"

namespace Thunder {

namespace Bluetooth {

namespace A2DP {

    struct IAudioCodec {

        static constexpr uint8_t MEDIA_TYPE = 0x00; // audio

        enum codectype : uint8_t {
            LC_SBC = 0
        };

        struct StreamFormat {
            uint32_t SampleRate;
            uint16_t FrameRate;
            uint8_t Resolution;
            uint8_t Channels;
        };

        virtual ~IAudioCodec() = default;

        virtual codectype Type() const = 0;

        virtual uint32_t BitRate() const = 0; // bits per second

        virtual uint16_t RawFrameSize() const = 0;
        virtual uint16_t EncodedFrameSize() const = 0;

        virtual uint32_t QOS(const int8_t policy) = 0;

        virtual uint32_t Configure(const StreamFormat& format, const string& settings) = 0;
        virtual uint32_t Configure(const uint8_t stream[], const uint16_t length) = 0;

        virtual void Configuration(StreamFormat& format, string& settings) const = 0;

        virtual uint16_t Encode(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                uint16_t& outBufferSize, uint8_t outBuffer[]) const = 0;

        virtual uint16_t Decode(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                uint16_t& outBufferSize, uint8_t outBuffer[]) const = 0;

        virtual uint16_t Serialize(const bool capabilities, uint8_t stream[], const uint16_t length) const = 0;
    };

} // namespace A2DP

} // namespace Bluetooth

}
