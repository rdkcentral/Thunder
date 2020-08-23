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

    // This interface gives direct access to a Netflix instance
    struct EXTERNAL INetflix : virtual public Core::IUnknown {
        enum { ID = ID_NETFLIX };

        enum state : uint16_t {
            PLAYING = 0x0001,
            STOPPED = 0x0002,
            SUSPENDING = 0x0004
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_NETFLIX_NOTIFICATION };

            // Signal changes on the subscribed namespace..
            virtual void StateChange(const INetflix::state state) = 0;
            virtual void Exit(const uint32_t exitCode) = 0;
        };

        virtual void Register(INetflix::INotification* netflix) = 0;
        virtual void Unregister(INetflix::INotification* netflix) = 0;

        virtual string GetESN() const = 0;

        virtual void FactoryReset() = 0;
        virtual void SystemCommand(const string& command) = 0;
        virtual void Language(const string& language) = 0;
        virtual void SetVisible(bool visibility) = 0;
    };
}
}

