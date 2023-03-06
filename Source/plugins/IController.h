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
namespace Exchange {
namespace IController {

    /* @json */
    struct EXTERNAL ISystemManagement : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SYSTEM_MANAGEMENT };

        ~ISystemManagement() override = default;

        // @alt harakiri
        // @brief Reboot the device
        virtual Core::hresult Reboot() = 0;

        // @brief Removes contents of a directory from the persistent storage.
        virtual Core::hresult Delete(const string& path) = 0;
        // @brief Create a clone of given plugin to requested new callsign
        virtual Core::hresult Clone(const string& callsign, const string& newcallsign, string& response /* @out */) = 0;
        // @property
        // @brief Provides the value of request environment variable.
        // @return Environment value
        virtual Core::hresult Environment(const string& index /* @index */, string& environment /* @out @opaque */ ) const = 0;

    };

    /* @json */
    struct EXTERNAL IDiscovery : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_DISCOVERY };
        ~IDiscovery() override = default;

        // @brief Starts the network discovery. Use this method to initiate SSDP network discovery process.
        // @param TTL (time to live) parameter for SSDP discovery
        virtual Core::hresult StartDiscovery(const uint8_t& ttl) = 0;

        // @property
        // @brief Provides SSDP network discovery results.
        // @return SSDP network discovery results
        virtual Core::hresult DiscoveryResults(string& response /* @out @opaque */) const = 0;
    };

    /* @json */
    // @json @uncompliant:extended
    struct EXTERNAL IConfiguration : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_CONFIGURATION };

        ~IConfiguration() override = default;

        // @alt storeconfig
        // @brief Stores the configuration to persistent memory
        virtual Core::hresult Persist() = 0;

        // @property
        // @brief Provides configuration value of a request service.
        virtual Core::hresult Configuration(const string& callsign /* @index */, string& configuration /* @out @opaque */) const = 0;
        virtual Core::hresult Configuration(const string& callsign /* @index */, const string& configuration /* @opaque */) = 0;
    };

    /* @json */
    struct EXTERNAL ILifeTime : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_LIFETIME };

        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_LIFETIME_NOTIFICATION };
            ~INotification() override = default;
            // @brief Notifies a plugin state change
            virtual void StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason) = 0;
        };

        ~ILifeTime() override = default;

        // Pushing notifications to interested sinks
        virtual Core::hresult Register(INotification* sink) = 0;
        virtual Core::hresult Unregister(INotification* sink) = 0;

        // @brief Activate a plugin, i.e. move from Deactivated, via Activating to Activated state
        virtual Core::hresult Activate(const string& callsign) = 0;
        // @brief Deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state
        virtual Core::hresult Deactivate(const string& callsign) = 0;
        // @brief Set a plugin unavailable for interaction.
        virtual Core::hresult Unavailable(const string& callsign) = 0;
        // @brief Set a plugin in Hibernate state
        virtual Core::hresult Hibernate(const string& callsign, const Core::hresult timeout) = 0;
        // @brief Suspend a plugin
        virtual Core::hresult Suspend(const string& callsign) = 0;
        // @brief Resumes a plugin
        virtual Core::hresult Resume(const string& callsign) = 0;
    };

    /* @json */
    struct EXTERNAL IMetadata : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_METADATA };
        ~IMetadata() override = default;

        // @property
        // @brief Provides currenlty running proxy details
        virtual Core::hresult Proxies(string& response /* @out @opaque */) const = 0;
        // @property
        // @brief Provides status of a plugin, including their configurations
        virtual Core::hresult Status(const string& index /* @index */, string& response /* @out @opaque */) const = 0;
        // @property
        // @brief Provides active connection details
        virtual Core::hresult Links(string& response /* @out @opaque */) const = 0;
        // @property
        // @brief Provides framework process info, like worker pools details
        virtual Core::hresult ProcessInfo(string& response /* @out @opaque */) const = 0;
        // @property
        // @brief Provides currently active subsystem details
        virtual Core::hresult Subsystems(string& response /* @out @opaque */) const = 0;
        // @property
        // @brief Provides version of WPEFramework hash and in human readable
        virtual Core::hresult Version(string& response /* @out @opaque */) const = 0;
        // @property
        // @brief callstack - Information the callstack associated with the given index 0 - <Max number of threads in the threadpool>
        virtual Core::hresult CallStack(const string& index /* @index */, string& callstack /* @out @opaque */) const = 0;
    };
}
} // namespace Exchange
} // namespace WPEFramework
