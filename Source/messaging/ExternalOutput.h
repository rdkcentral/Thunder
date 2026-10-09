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

// Internal Messaging/backend contract. This header is not part of the installed SDK.

#include <stdbool.h>

#ifdef __cplusplus
    #include "Module.h"
    #define THUNDER_MESSAGING_API EXTERNAL
#else
    #if defined(_WIN32) || defined(__CYGWIN__)
        #if defined(MESSAGING_EXPORTS)
            #define THUNDER_MESSAGING_API __declspec(dllexport)
        #else
            #define THUNDER_MESSAGING_API __declspec(dllimport)
        #endif
    #elif defined(__GNUC__)
        #define THUNDER_MESSAGING_API __attribute__((visibility("default")))
    #else
        #define THUNDER_MESSAGING_API
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THUNDER_EXTERNAL_MESSAGE_ASSERT             = 0,
    THUNDER_EXTERNAL_MESSAGE_REPORTING          = 1,
    THUNDER_EXTERNAL_MESSAGE_OPERATIONAL_STREAM = 2,
    THUNDER_EXTERNAL_MESSAGE_TELEMETRY          = 3,
    THUNDER_EXTERNAL_MESSAGE_TRACING            = 4,
    THUNDER_EXTERNAL_MESSAGE_LOGGING            = 5
} ThunderMessageType;

THUNDER_MESSAGING_API bool ThunderExternalOutput_Initialize(void);
THUNDER_MESSAGING_API bool ThunderExternalOutput_IsEnabled(const char* module, ThunderMessageType type, const char* category);
THUNDER_MESSAGING_API void ThunderExternalOutput_Submit(const char* module, ThunderMessageType type, const char* category, const char* payload);
THUNDER_MESSAGING_API void ThunderExternalOutput_Deinitialize(void);

#ifdef __cplusplus
}
#endif