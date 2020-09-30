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
#include "Module.h"

// @stubgen:skip

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a switchboard
    struct EXTERNAL ISwitchBoard : virtual public Core::IUnknown {
        enum { ID = ID_SWITCHBOARD };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_SWITCHBOARD_NOTIFICATION };

            // Signal which callsign has been switched on
            virtual void Activated(const string& callsign) = 0;
        };

        virtual void Register(INotification* notification) = 0;
        virtual void Unregister(INotification* notification) = 0;

        virtual bool IsActive(const string& callsign) const = 0;
        virtual uint32_t Activate(const string& callsign) = 0;
        virtual uint32_t Deactivate(const string& callsign) = 0;
    };
}
}
