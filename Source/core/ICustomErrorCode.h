/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

/*
    This file contains the interface that a library can implement in case the "custom error codes" feature is used in Thunder and code to string comversion is desired
*/

#pragma once

#undef EXTERNAL

#if defined(WIN32) || defined(_WINDOWS) || defined(__CYGWIN__) || defined(_WIN64)
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifndef TCHAR
#ifdef _UNICODE
#define TCHAR wchar_t
#else
#define TCHAR char
#endif
#endif

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

EXTERNAL const TCHAR* CustomCodeToSting(const int32_t code);

#ifdef __cplusplus
} // extern "C"
#endif 
