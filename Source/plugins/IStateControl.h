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

#include "Module.h"

#ifndef __ISTATECONTROL_H
#define __ISTATECONTROL_H

#include "IShell.h"

namespace WPEFramework {
namespace PluginHost {

    // This interface gives direct access to change occuring on the remote object
    struct IStateControl
        : virtual public Core::IUnknown {

        enum {
            ID = RPC::ID_STATECONTROL
        };

        enum command {
            SUSPEND = 0x0001,
            RESUME = 0x0002
        };

        enum state {
            UNINITIALIZED = 0x0000,
            SUSPENDED = 0x0001,
            RESUMED = 0x0002,
            EXITED = 0x0003
        };

        struct INotification
            : virtual public Core::IUnknown {
            enum {
                ID = RPC::ID_STATECONTROL_NOTIFICATION
            };

            virtual ~INotification()
            {
            }

            virtual void StateChange(const IStateControl::state state) = 0;
        };

        static const TCHAR* ToString(const state value);
        static const TCHAR* ToString(const command value);

        virtual ~IStateControl()
        {
        }

        virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
        virtual state State() const = 0;
        virtual uint32_t Request(const command state) = 0;

        virtual void Register(IStateControl::INotification* notification) = 0;
        virtual void Unregister(IStateControl::INotification* notification) = 0;
    };
}

namespace Core {

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::IStateControl::command>*
    EnumerateType<PluginHost::IStateControl::command>::Table(const uint16_t);

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::IStateControl::state>*
    EnumerateType<PluginHost::IStateControl::state>::Table(const uint16_t);

} // namespace PluginHost
} // namespace WPEFramework

#endif // __ISTATECONTROL_H
