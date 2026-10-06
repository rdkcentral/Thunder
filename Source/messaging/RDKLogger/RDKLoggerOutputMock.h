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

#include "../Module.h"
#include "../ExternalOutput.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	THUNDER_EXTERNAL_LOG_LEVEL_FATAL  = 0,
	THUNDER_EXTERNAL_LOG_LEVEL_ERROR  = 1,
	THUNDER_EXTERNAL_LOG_LEVEL_WARN   = 2,
	THUNDER_EXTERNAL_LOG_LEVEL_NOTICE = 3,
	THUNDER_EXTERNAL_LOG_LEVEL_INFO   = 4,
	THUNDER_EXTERNAL_LOG_LEVEL_TRACE  = 5
} ThunderExternalLogLevel;

EXTERNAL void ThunderExternalOutputMock_Reset(void);
EXTERNAL void ThunderExternalOutputMock_SetInitializeSucceeds(bool succeeds);
EXTERNAL void ThunderExternalOutputMock_SetEnabled(bool enabled);
EXTERNAL uint32_t ThunderExternalOutputMock_InitializeCount(void);
EXTERNAL uint32_t ThunderExternalOutputMock_DeinitializeCount(void);
EXTERNAL uint32_t ThunderExternalOutputMock_QueryCount(void);
EXTERNAL uint32_t ThunderExternalOutputMock_QueryCountFor(const char* module, ThunderExternalLogLevel level);
EXTERNAL uint32_t ThunderExternalOutputMock_SubmitCount(void);
EXTERNAL const char* ThunderExternalOutputMock_LastModule(void);
EXTERNAL ThunderExternalLogLevel ThunderExternalOutputMock_LastLevel(void);
EXTERNAL const char* ThunderExternalOutputMock_LastPayload(void);

#ifdef __cplusplus
}
#endif
