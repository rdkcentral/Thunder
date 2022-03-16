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

#include <core/core.h>

namespace WPEFramework {
namespace RPC {

    enum IDS {
        ID_COMCONNECTION = 0x00000001,
        ID_COMCONNECTION_NOTIFICATION = 0x00000002,
        ID_TRACEITERATOR = 0x00000003,
        ID_TRACECONTROLLER = 0x00000004,
        ID_STRINGITERATOR = 0x00000005,
        ID_VALUEITERATOR = 0x00000006,
        ID_MONITORABLE_PROCESS = 0x00000007,

        ID_CONTROLLER = 0x00000020,
        ID_PLUGIN = 0x00000021,
        ID_PLUGIN_NOTIFICATION = 0x00000022,
        ID_PLUGINEXTENDED = 0x00000023,
        ID_WEB = 0x00000024,
        ID_WEBSOCKET = 0x00000025,
        ID_DISPATCHER = 0x00000026,
        ID_TEXTSOCKET = 0x00000027,
        ID_CHANNEL = 0x00000028,
        ID_SECURITY = 0x00000029,
        ID_AUTHENTICATE = 0x0000002A,

        ID_SHELL = 0x00000030,
        ID_STATECONTROL = 0x00000031,
        ID_STATECONTROL_NOTIFICATION = 0x00000032,
        ID_SUBSYSTEM = 0x00000033,
        ID_SUBSYSTEM_NOTIFICATION = 0x00000034,
        ID_SUBSYSTEM_INTERNET = 0x00000035,
        ID_SUBSYSTEM_LOCATION = 0x00000036,
        ID_SUBSYSTEM_IDENTIFIER = 0x00000037,
        ID_SUBSYSTEM_TIME = 0x00000038,
        ID_SUBSYSTEM_SECURITY = 0x00000039,
        ID_SUBSYSTEM_PROVISIONING = 0x0000003A,
        ID_SUBSYSTEM_DECRYPTION = 0x0000003B,
        ID_REMOTE_INSTANTIATION = 0x0000003C,
        ID_COMREQUEST_NOTIFICATION = 0x0000003D
    };
}
}
