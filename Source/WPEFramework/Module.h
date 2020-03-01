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

#ifndef __MODULE_FRAMEWORK_APPLICATION_H
#define __MODULE_FRAMEWORK_APPLICATION_H

#ifndef MODULE_NAME
#define MODULE_NAME Application
#endif

#include "../core/core.h"
#include "../cryptalgo/cryptalgo.h"
#include "../plugins/plugins.h"
#include "../tracing/tracing.h"
#include "../websocket/websocket.h"

#undef EXTERNAL
#define EXTERNAL

#ifndef TREE_REFERENCE
#define TREE_REFERENCE engineering_build_for_debug_purpose_only
#endif

#endif // __MODULE_FRAMEWORK_APPLICATION_H
