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

#include "URL.h"
#include "JSONWebToken.h"
#include "JSONRPCLink.h"
#include "WebLink.h"
#include "WebRequest.h"
#include "WebResponse.h"
#include "WebSerializer.h"
#include "WebSocketLink.h"
#include "WebTransfer.h"
#include "WebTransform.h"

#ifdef __WINDOWS__
#pragma comment(lib, "websocket.lib")
#endif

WPEFRAMEWORK_NESTEDNAMESPACE_COMPATIBILIY(Web)
WPEFRAMEWORK_NESTEDNAMESPACE_COMPATIBILIY(JSONRPC)
