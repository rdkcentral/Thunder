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

#include "Module.h"
 
namespace Thunder {
namespace PluginHost {

    struct IRemoteInstantiation : virtual public Core::IUnknown {

        enum { ID = RPC::IDS::ID_REMOTE_INSTANTIATION };

        ~IRemoteInstantiation() override = default;

        // Overview:
        //          HOST                                      TARGET
        //
        //      +------------+                            +------------+
        //      | WEBSERVER' | [OOP]                      | WEBSERVER' | [OOH]
        //      +-----+------+                            +--+---------+
        //            |                                      |   ^
        //            |+-------------------------------------+   |
        //            ||                                         |
        //      +-----++-----+                                SPAWNED
        //      | WEBSERVER  |                                   |
        //  +---+------------+---+                     +---------+---------+
        //  |     Thunder [A]    |                     |    Thunder [B]    |
        //  +--------------------+                     +-------------------+
        virtual uint32_t Instantiate(
            const uint32_t requestId,
            const string& libraryName,
            const string& className,
            const string& callsign,
            const uint32_t interfaceId,
            const uint32_t version,
            const string& user,
            const string& group,
            const string& systemRootPath,
            const uint8_t threads,
            const int8_t priority,
            const string configuration) = 0;
    };
}
}
