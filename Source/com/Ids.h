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

namespace Thunder {
namespace RPC {

    enum IDS {
        ID_OFFSET_INTERNAL                         = (Core::IUnknown::ID_OFFSET_INTERNAL),

        ID_COMCONNECTION                           = (ID_OFFSET_INTERNAL + 0x0001),
        ID_COMCONNECTION_NOTIFICATION              = (ID_OFFSET_INTERNAL + 0x0002),
        ID_TRACEITERATOR                           = (ID_OFFSET_INTERNAL + 0x0003),
        ID_TRACECONTROLLER                         = (ID_OFFSET_INTERNAL + 0x0004),
        ID_STRINGITERATOR                          = (ID_OFFSET_INTERNAL + 0x0005),
        ID_VALUEITERATOR                           = (ID_OFFSET_INTERNAL + 0x0006),
        ID_MONITORABLE_PROCESS                     = (ID_OFFSET_INTERNAL + 0x0007),

        ID_CONTROLLER_SYSTEM                       = (ID_OFFSET_INTERNAL + 0x0008),
        ID_CONTROLLER_CONFIGURATION                = (ID_OFFSET_INTERNAL + 0x0009),
        ID_CONTROLLER_DISCOVERY                    = (ID_OFFSET_INTERNAL + 0x000A),
        ID_CONTROLLER_DISCOVERY_DISCOVERYRESULTS_ITERATOR  = (ID_OFFSET_INTERNAL + 0x000B),
        ID_CONTROLLER_LIFETIME                     = (ID_OFFSET_INTERNAL + 0x000C),
        ID_CONTROLLER_LIFETIME_NOTIFICATION        = (ID_OFFSET_INTERNAL + 0x000D),
        ID_CONTROLLER_SHELLS                       = (ID_OFFSET_INTERNAL + 0x000E),
        ID_CONTROLLER_SHELLS_NOTIFICATION          = (ID_OFFSET_INTERNAL + 0x000F),
        ID_CONTROLLER_SUBSYSTEMS                   = (ID_OFFSET_INTERNAL + 0x0010),
        ID_CONTROLLER_SUBSYSTEMS_NOTIFICATION      = (ID_OFFSET_INTERNAL + 0x0011),
        ID_CONTROLLER_SUBSYSTEMS_ITERATOR          = (ID_OFFSET_INTERNAL + 0x0012),
        ID_CONTROLLER_METADATA                     = (ID_OFFSET_INTERNAL + 0x0013),
        ID_CONTROLLER_METADATA_SERVICES_ITERATOR   = (ID_OFFSET_INTERNAL + 0x0014),
        ID_CONTROLLER_METADATA_LINKS_ITERATOR      = (ID_OFFSET_INTERNAL + 0x0015),
        ID_CONTROLLER_METADATA_PROXIES_ITERATOR    = (ID_OFFSET_INTERNAL + 0x0016),
        ID_CONTROLLER_METADATA_THREADS_ITERATOR    = (ID_OFFSET_INTERNAL + 0x0017),
        ID_CONTROLLER_METADATA_CALLSTACK_ITERATOR  = (ID_OFFSET_INTERNAL + 0x0018),
        ID_CONTROLLER_EVENTS                       = (ID_OFFSET_INTERNAL + 0x0019),
        ID_CONTROLLER_EVENTS_NOTIFICATION          = (ID_OFFSET_INTERNAL + 0x0020),

        // Plugin module
        ID_PLUGIN                           = (ID_OFFSET_INTERNAL + 0x0021),
        ID_CONTROLLER                       = (ID_OFFSET_INTERNAL + 0x0020),
        ID_PLUGIN_NOTIFICATION              = (ID_OFFSET_INTERNAL + 0x0022),
        ID_PLUGINEXTENDED                   = (ID_OFFSET_INTERNAL + 0x0023),
        ID_WEB                              = (ID_OFFSET_INTERNAL + 0x0024),
        ID_WEBSOCKET                        = (ID_OFFSET_INTERNAL + 0x0025),
        ID_DISPATCHER                       = (ID_OFFSET_INTERNAL + 0x0026),
        ID_TEXTSOCKET                       = (ID_OFFSET_INTERNAL + 0x0027),
        ID_CHANNEL                          = (ID_OFFSET_INTERNAL + 0x0028),
        ID_SECURITY                         = (ID_OFFSET_INTERNAL + 0x0029),
        ID_AUTHENTICATE                     = (ID_OFFSET_INTERNAL + 0x002A),
        ID_PLUGIN_LIFETIME                  = (ID_OFFSET_INTERNAL + 0x002B),
        ID_COMPOSIT_PLUGIN                  = (ID_OFFSET_INTERNAL + 0x002C),
        ID_COMPOSIT_PLUGIN_CALLBACK         = (ID_OFFSET_INTERNAL + 0x002D),
        ID_DISPATCHER_CALLBACK              = (ID_OFFSET_INTERNAL + 0x002E),

        ID_SHELL                            = (ID_OFFSET_INTERNAL + 0x0030),
        ID_SHELL_COMLINK                    = (ID_OFFSET_INTERNAL + 0x0031),
        ID_SHELL_CONNECTIONSERVER           = (ID_OFFSET_INTERNAL + 0x0032),
        ID_SHELL_CONNECTIONSERVER_NOTIFICATION = (ID_OFFSET_INTERNAL + 0x0033),
        ID_STATECONTROL                     = (ID_OFFSET_INTERNAL + 0x0034),
        ID_STATECONTROL_NOTIFICATION        = (ID_OFFSET_INTERNAL + 0x0035),
        ID_SUBSYSTEM                        = (ID_OFFSET_INTERNAL + 0x0036),
        ID_SUBSYSTEM_NOTIFICATION           = (ID_OFFSET_INTERNAL + 0x0037),
        ID_SUBSYSTEM_INTERNET               = (ID_OFFSET_INTERNAL + 0x0038),
        ID_SUBSYSTEM_LOCATION               = (ID_OFFSET_INTERNAL + 0x0039),
        ID_SUBSYSTEM_IDENTIFIER             = (ID_OFFSET_INTERNAL + 0x003A),
        ID_SUBSYSTEM_TIME                   = (ID_OFFSET_INTERNAL + 0x003B),
        ID_SUBSYSTEM_SECURITY               = (ID_OFFSET_INTERNAL + 0x003C),
        ID_SUBSYSTEM_PROVISIONING           = (ID_OFFSET_INTERNAL + 0x003D),
        ID_SUBSYSTEM_DECRYPTION             = (ID_OFFSET_INTERNAL + 0x003E),
        ID_REMOTE_INSTANTIATION             = (ID_OFFSET_INTERNAL + 0x003F),
        ID_SYSTEM_METADATA                  = (ID_OFFSET_INTERNAL + 0x0040),

        ID_EXTERNAL_INTERFACE_OFFSET        = (ID_OFFSET_INTERNAL + 0x0080),
        ID_EXTERNAL_QA_INTERFACE_OFFSET     = (ID_OFFSET_INTERNAL + 0xA000)
    };
}
}
