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

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IPower : virtual public Core::IUnknown {
        enum { ID = ID_POWER };

        enum PCState : uint8_t {
            On = 1, // S0.
            ActiveStandby = 2, // S1.
            PassiveStandby = 3, // S2.
            SuspendToRAM = 4, // S3.
            Hibernate = 5, // S4.
            PowerOff = 6, // S5.
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_POWER_NOTIFICATION };

            virtual void StateChange(const PCState) = 0;
        };

        virtual void Register(IPower::INotification* sink) = 0;
        virtual void Unregister(IPower::INotification* sink) = 0;

        virtual PCState GetState() const = 0;
        virtual uint32_t SetState(const PCState, const uint32_t) = 0;
        virtual void PowerKey() = 0;
    };
}
}

