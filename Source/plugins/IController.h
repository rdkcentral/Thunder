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
#include "IShell.h"

// @stubgen:include <plugins/IShell.h>

namespace WPEFramework {
namespace PluginHost {

    // @json
    struct EXTERNAL IController : public virtual Core::IUnknown {

        enum { ID = RPC::ID_CONTROLLER };

        ~IController() override = default;

        virtual Core::hresult Persist() = 0;

        virtual Core::hresult Delete(const string& path) = 0;

        virtual Core::hresult Reboot() = 0;

        virtual Core::hresult Environment(const string& index, string& environment /* @out */ ) const = 0;

        virtual Core::hresult Configuration(const string& callsign, string& configuration /* @out */) const = 0;
        virtual Core::hresult Configuration(const string& callsign, const string& configuration) = 0;

        virtual Core::hresult Clone(const string& basecallsign, const string& newcallsign) = 0;
    };
} // PluginHost

namespace Exchange {
    // @stubgen:omit
    // @json @uncompliant:extended
    struct EXTERNAL IControllerExt : public virtual Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_EXT };

        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_EXT_NOTIFICATION };
            ~INotification() override = default;
            virtual void StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason) = 0;
        };

        ~IControllerExt() override = default;

        // Pushing notifications to interested sinks
        virtual uint32_t Register(IControllerExt::INotification* sink) = 0;
        virtual uint32_t Unregister(IControllerExt::INotification* sink) = 0;

        virtual uint32_t Activate(const string& callsign) = 0;
        virtual uint32_t Deactivate(const string& callsign) = 0;
        virtual uint32_t Unavailable(const string& callsign) = 0;
        virtual uint32_t Suspend(const string& callsign) = 0;
        virtual uint32_t Resume(const string& callsign) = 0;
        virtual uint32_t Hibernate(const string& callsign, const uint32_t timeout) = 0;
        virtual uint32_t StartDiscovery(const uint8_t& ttl) = 0;
        virtual uint32_t StoreConfig() = 0;
        virtual uint32_t Delete(const string& path /* @in @opaque */) = 0;
        virtual uint32_t Harakiri() = 0;
        virtual uint32_t Proxies(string& response /* @out @opaque */) const = 0;
        virtual uint32_t Clone(const string& callsign, const string& newcallsign, string& response /* @out */) = 0;

        // @property
        virtual uint32_t Status(const string& index /* @index */, string& response /* @out @opaque */) const = 0;
        // @property
        virtual uint32_t Links(string& response /* @out @opaque */) const = 0;
        // @property
        virtual uint32_t ProcessInfo(string& response /* @out @opaque */) const = 0;
        // @property
        virtual uint32_t Subsystems(string& response /* @out @opaque */) const = 0;
        // @property
        virtual uint32_t DiscoveryResults(string& response /* @out @opaque */) const = 0;
        // @property
        virtual uint32_t Environment(const string& index /* @index */, string& environment /* @out @opaque */ ) const = 0;
        // @property
        virtual uint32_t Configuration(const string& callsign /* @index */, string& configuration /* @out @opaque */) const = 0;
        virtual uint32_t Configuration(const string& callsign /* @index */, const string& configuration /* @opaque */) = 0;
        // @property
        virtual uint32_t CallStack(const string& index /* @index */, string& callstack /* @out @opaque */) const = 0;
        // @property
        virtual uint32_t Version(string& response /* @out @opaque */) const = 0;
    };
} // namespace Exchange
} // namespace WPEFramework
