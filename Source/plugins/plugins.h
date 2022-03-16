/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
#error "Please define a MODULE_NAME that describes the binary/library you are building."
#endif

// Since this header is included, the code using it is external to Thunder core.
// So therefore it should use the correct Tracing functionality and not TRACE_L# (which are just fancy printfs).
#define CORE_TRACE_NOT_ALLOWED

#include "Module.h"
#include "Config.h"
#include "Channel.h"
#include "Configuration.h"
#include "IPlugin.h"
#include "Shell.h"
#include "IShell.h"
#include "IController.h"
#include "StateControl.h"
#include "IStateControl.h"
#include "SubSystem.h"
#include "ISubSystem.h"
#include "JSONRPC.h"
#include "Request.h"
#include "Service.h"
#include "System.h"
#include "Types.h"
#include "VirtualInput.h"
#include "IVirtualInput.h"

#ifdef __WINDOWS__
#pragma comment(lib, "plugins.lib")
#endif
