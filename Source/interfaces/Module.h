/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#ifndef MODULE_NAME
#define MODULE_NAME Interfaces
#endif

#include <core/core.h>

// These are interfaces offered by Thunder and used in the Plugins. Make them
// available on interfaces taht are exposed cross plugins.
#include <plugins/IPlugin.h>
#include <plugins/ISubSystem.h>
#include <plugins/IShell.h>
#include <plugins/IStateControl.h>

// All identifiers to identify an interface are allocated in this same directory
// in the file calls Ids.h, please extend it with your requried interface number
// if you are creating a new interface.
#include "Ids.h"

#if defined(PROXYSTUB_BUILDING)
#include <com/com.h>
#endif


