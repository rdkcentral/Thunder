/*
 * Copyright 2024 RDK Management
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

// Module definition for the thunder_test_support static library.
// MODULE_NAME is set to ThunderTestRuntime via -D in CMakeLists.txt,
// which overrides Source/Thunder/Module.h's default of "Application".
// This ensures all server sources and the MODULE_NAME_DECLARATION below
// use the same symbol, and trace output shows "ThunderTestRuntime".

#ifndef MODULE_NAME
#define MODULE_NAME ThunderTestRuntime
#endif

#include <core/core.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)
