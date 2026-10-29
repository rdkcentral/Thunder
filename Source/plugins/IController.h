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

#include "IControllerDeprecated.h"

// @stubgen:include <plugins/IShell.h>

namespace WPEFramework {

namespace Exchange {
namespace Controller {

    /* @json */
    struct EXTERNAL ISystemManagement : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SYSTEM_MANAGEMENT };

        ~ISystemManagement() override = default;

        // @alt harakiri
        // @brief Reboots the device
        // @details Depending on the device this call may not generate a response
        virtual Core::hresult Reboot() = 0;

        // @brief Removes contents of a directory from the persistent storage
        // @param path: Path to the directory within the persisent storage
        virtual Core::hresult Delete(const string& path) = 0;

        // @brief Creates a clone of given plugin with a new callsign
        // @param callsign: Callsign of the plugin
        // @param newcallsign: Callsign for the cloned plugin
        virtual Core::hresult Clone(const string& callsign, const string& newcallsign, string& response /* @out */) = 0;

        // @property
        // @brief Environment variable value
        virtual Core::hresult Environment(const string& index /* @index */, string& environment /* @out */ ) const = 0;
    };

    /* @json */
    struct EXTERNAL IDiscovery : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_DISCOVERY };
        ~IDiscovery() override = default;

        // @brief Starts the SSDP network discovery
        // @param TTL (time to live) parameter for SSDP discovery
        virtual Core::hresult StartDiscovery(const uint8_t& ttl /* @optional @default:1 */) = 0;

        // @property
        // @brief SSDP network discovery results
        virtual Core::hresult DiscoveryResults(string& response /* @out @opaque */) const = 0;
    };

    /* @json */
    // @json @uncompliant:extended
    struct EXTERNAL IConfiguration : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_CONFIGURATION };

        ~IConfiguration() override = default;

        // @alt storeconfig
        // @brief Stores the configuration to persistent memory
        virtual DEPRECATED Core::hresult Persist() = 0;

        // @property
        // @brief Service configuration
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

            // @brief Notifies of a plugin state change
            // @param callsign: Plugin callsign
            // @param state: New state of the plugin
            // @param reason: Reason for state change
            virtual void StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason) = 0;
        };

        ~ILifeTime() override = default;

        // Pushing notifications to interested sinks
        virtual Core::hresult Register(INotification* sink) = 0;
        virtual Core::hresult Unregister(INotification* sink) = 0;

        // @brief Activate a plugin, i.e. move from Deactivated, via Activating to Activated state
        // @param callsign: Callsign of plugin to be activated
        virtual Core::hresult Activate(const string& callsign) = 0;

        // @brief Deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state
        // @param callsign: Callsign of plugin to be deactivated
        virtual Core::hresult Deactivate(const string& callsign) = 0;

        // @brief Set a plugin unavailable for interaction
        // @param callsign: Callsign of plugin to be set as unavailable
        virtual Core::hresult Unavailable(const string& callsign) = 0;

        // @brief Set a plugin in Hibernate state
        // @param callsign: Callsign of plugin to be hibernated
        // @param timeout: Allowed time in ms (e.g. 10000)
        // @retval ERROR_INPROC The plugin is running in-process and thus cannot be hibernated
        virtual Core::hresult Hibernate(const string& callsign, const uint32_t timeout) = 0;

        // @brief Suspend a plugin
        // @param callsign: Callsign of plugin to be suspended
        virtual Core::hresult Suspend(const string& callsign) = 0;

        // @brief Resumes a plugin
        // @param callsign: Callsign of plugin to be resumed
        virtual Core::hresult Resume(const string& callsign) = 0;
    };

    /* @json */
    struct EXTERNAL IMetadata : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_METADATA };
        ~IMetadata() override = default;

        // @property
        // @brief Currently running proxies list
        virtual Core::hresult Proxies(string& response /* @out @opaque */) const = 0;

        // @property
        // @brief Services metadata
        // @details If callsign is omitted, metadata of all services is returned
        virtual Core::hresult Status(const string& index /* @index @optional */, string& response /* @out @opaque */) const = 0;

        // @property
        // @brief Active connection details
        virtual Core::hresult Links(string& response /* @out @opaque */) const = 0;

        // @property
        // @brief Framework process info, like worker pools details
        virtual Core::hresult ProcessInfo(string& response /* @out @opaque */) const = 0;

        // @property
        // @brief Currently active subsystem details
        virtual Core::hresult Subsystems(string& response /* @out @opaque */) const = 0;

        // @property
        // @brief Framework version
        virtual Core::hresult Version(string& response /* @out @opaque */) const = 0;

        // @property
        // @brief Thread callstack information
        virtual Core::hresult CallStack(const string& index /* @index */, string& callstack /* @out @opaque */) const = 0;
    };
}

} // namespace Exchange

} // namespace WPEFramework
