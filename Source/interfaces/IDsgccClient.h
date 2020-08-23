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

    struct EXTERNAL IDsgccClient : virtual public Core::IUnknown {
        enum { ID = ID_DSGCC_CLIENT };

        enum state : uint8_t {
            Unknown = 0,
            Ready   = 1,
            Changed = 2,
            Error   = 3
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_DSGCC_CLIENT_NOTIFICATION };

            virtual void StateChange(const state newState) = 0;
        };

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        virtual string GetChannels() const = 0;
        virtual string State() const = 0;
        virtual void Restart() = 0;
        virtual void Callback(IDsgccClient::INotification* callback) = 0;

        virtual void DsgccClientSet(const string& str) = 0;
    };
}
}

