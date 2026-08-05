/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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
namespace Plugin {

    namespace Configuration {
        // @encode:text
        enum class reason : uint8_t {
            REQUESTED,
            AUTOMATIC,
            FAILURE,
            MEMORY_EXCEEDED,
            STARTUP,
            SHUTDOWN,
            CONDITIONS,
            WATCHDOG_EXPIRED,
            INITIALIZATION_FAILED,
            INSTANTIATION_FAILED
        };

        // @encode:text
        enum class startmode : uint8_t {
            UNAVAILABLE,
            DEACTIVATED,
            ACTIVATED
        };

        // @encode:text
        enum class subsystem : uint32_t {
            PLATFORM,
            SECURITY,
            NETWORK,
            IDENTIFIER,
            GRAPHICS,
            INTERNET,
            LOCATION,
            TIME,
            PROVISIONING,
            DECRYPTION,
            WEBSOURCE /* @text WebSource */,
            STREAMING,
            BLUETOOTH,
            CRYPTOGRAPHY,
            INSTALLATION
        };
    }
}
}