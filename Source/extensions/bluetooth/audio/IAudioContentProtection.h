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

    struct IAudioContentProtection {

        enum protectiontype {
            SCMS_T
        };

        virtual ~IAudioContentProtection() = default;

        virtual protectiontype Type() const = 0;

        virtual uint32_t Configure(const string& format) = 0;
        virtual void Configuration(string& format) const = 0;

        virtual uint32_t Protect(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                 const uint16_t maxOutBufferSize, uint8_t outBuffer) const = 0;

        virtual uint32_t Unprotect(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                   const uint16_t maxOutBufferSize, uint8_t outBuffer) const = 0;

        virtual uint32_t Serialize(const bool capabilities, uint8_t buffer[], const uint16_t length ) const = 0;
    };

} // namespace A2DP

} // namespace Bluetooth

}
