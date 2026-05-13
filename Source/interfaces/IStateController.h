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

#include "IShell.h"

#include <com/ICOM.h>

namespace Thunder {

namespace PluginHost {

    // This interface gives direct access to change occuring on the remote object
    // @json 1.0.0 @text:legacy_lowercase @prefix:statecontrol
    struct EXTERNAL IStateController : virtual public Core::IUnknown {

        enum { ID = RPC::ID_STATE_CONTROLLER };

        enum command : uint8_t {
            SUSPEND,
            RESUME
        };

        enum state : uint8_t {
            UNKNOWN,
            SUSPENDED,
            RESUMED
        };

        // @event
        struct INotification : virtual public Core::IUnknown {

            enum { ID = RPC::ID_STATE_CONTROLLER_NOTIFICATION };

            // @brief Signals a state change of the service
            // @param state: Current state of the service
            virtual void StateChanged(const state state) = 0;
        };

        virtual Core::hresult Register(INotification* const notification) = 0;
        virtual Core::hresult Unregister(const INotification* const notification) = 0;

        // @property
        // @brief Running state of the service
        virtual Core::hresult State(state& state /* @out */) const = 0;

        // @brief Request a change to the specified state
        // @param state: State to which it is requested to change
        virtual Core::hresult Request(const command state) = 0;
    };

} // namespace PluginHost

}
