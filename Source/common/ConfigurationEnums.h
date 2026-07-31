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
            REQUESTED /* @text Requested */,
            AUTOMATIC /* @text Automatic */,
            FAILURE /* @text Failure */,
            MEMORY_EXCEEDED /* @text MemoryExceeded */,
            STARTUP /* @text Startup */,
            SHUTDOWN /* @text Shutdown */,
            CONDITIONS /* @text Conditions */,
            WATCHDOG_EXPIRED /* @text WatchdogExpired */,
            INITIALIZATION_FAILED /* @text InitializationFailed */,
            INSTANTIATION_FAILED /* @text InstantiationFailed */
        };

        // @encode:text
        enum class startmode : uint8_t {
            UNAVAILABLE /* @text Unavailable */,
            DEACTIVATED /* @text Deactivated */,
            ACTIVATED /* @text Activated */
        };

        // @encode:text
        enum class subsystem : uint32_t {
            PLATFORM /* @text Platform */,
            SECURITY /* @text Security */,
            NETWORK /* @text Network */,
            IDENTIFIER /* @text Identifier */,
            GRAPHICS /* @text Graphics */,
            INTERNET /* @text Internet */,
            LOCATION /* @text Location */,
            TIME /* @text Time */,
            PROVISIONING /* @text Provisioning */,
            DECRYPTION /* @text Decryption */,
            WEBSOURCE /* @text WebSource */,
            STREAMING /* @text Streaming */,
            BLUETOOTH /* @text Bluetooth */,
            CRYPTOGRAPHY /* @text Cryptography */,
            INSTALLATION /* @text Installation */
        };
    }
}
}