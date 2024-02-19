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
// @stubgen:include <plugins/ISubSystem.h>
// @stubgen:include <com/IIteratorType.h>

namespace WPEFramework {

namespace Exchange {

namespace Controller {

    /* @json 1.0.0 */
    struct EXTERNAL ISystemManagement : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SYSTEM_MANAGEMENT };

        // @alt:deprecated harakiri
        // @brief Reboots the device
        virtual Core::hresult Reboot() = 0;

        // @brief Removes contents of a directory from the persistent storage.
        // @param path : Path of the directory
        virtual Core::hresult Delete(const string& path) = 0;

        // @brief Creates a clone of given plugin to requested new callsign
        // @param callsign: Callsign of the plugin
        // @param newcallsign: New callsign for the plugin
        virtual Core::hresult Clone(const string& callsign, const string& newcallsign, string& response /* @out */) = 0;

        // @property
        // @brief Provides the value of request environment variable.
        // @return Environment value
        virtual Core::hresult Environment(const string& variable /* @index */, string& value /* @out */ ) const = 0;
    };

    /* @json 1.0.0 */
    struct EXTERNAL IDiscovery : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_DISCOVERY };

        struct Data {
            struct DiscoveryResult {
                string Locator /* @brief Locator for the discovery */;
                uint32_t Latency /* @brief Latency for the discovery */;
                string Model /* @optional @brief Model */;
                bool Secure /* @brief Secure or not*/;
            };

            using IDiscoveryResultsIterator = RPC::IIteratorType<Data::DiscoveryResult, RPC::ID_CONTROLLER_DISCOVERY_DISCOVERYRESULTS_ITERATOR>;
        };

        // @brief Starts the network discovery. Use this method to initiate SSDP network discovery process.
        // @param ttl: Time to live, parameter for SSDP discovery
        virtual Core::hresult StartDiscovery(const uint8_t ttl) = 0;

        // @property
        // @brief Provides SSDP network discovery results.
        // @return SSDP: Network discovery results
        virtual Core::hresult DiscoveryResults(Data::IDiscoveryResultsIterator*& results /* @out */) const = 0;
    };

    /* @json 1.0.0 @uncompliant:extended */
    struct EXTERNAL IConfiguration : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_CONFIGURATION };

        // @alt storeconfig
        // @brief Stores: The configuration to persistent memory
        virtual Core::hresult Persist() = 0;

        // @property
        // @brief Provides configuration value of a request service.
        virtual Core::hresult Configuration(const string& callsign /* @index @optional */, string& configuration /* @out @opaque */) const = 0;
        virtual Core::hresult Configuration(const string& callsign /* @index */, const string& configuration /* @opaque */) = 0;
    };

    /* @json 1.0.0 */
    struct EXTERNAL ILifeTime : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_LIFETIME };

        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_LIFETIME_NOTIFICATION };

            // @brief Notifies a plugin state change
            // @param callsign: Plugin callsign
            // @param state: New state of the plugin
            // @param reason: Reason of state change
            virtual void StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason) = 0;
        };

        virtual Core::hresult Register(INotification* sink) = 0;
        virtual Core::hresult Unregister(INotification* sink) = 0;

        // @brief Activates a plugin, i.e. move from Deactivated, via Activating to Activated state
        // @param callsign: Callsign of plugin to be activated
        virtual Core::hresult Activate(const string& callsign) = 0;

        // @brief Deactivates a plugin, i.e. move from Activated, via Deactivating to Deactivated state
        // @param callsign: Callsign of plugin to be deactivated
        virtual Core::hresult Deactivate(const string& callsign) = 0;

        // @brief Sets a plugin unavailable for interaction.
        // @param callsign: Callsign of plugin to be set as unavailable
        virtual Core::hresult Unavailable(const string& callsign) = 0;

        // @brief Sets a plugin in Hibernate state
        // @param callsign: Callsign of plugin to be set as hibernate
        // @param timeout: Timeout to hibernate
        virtual Core::hresult Hibernate(const string& callsign, const uint32_t timeout) = 0;

        // @brief Suspends a plugin
        // @param callsign: Callsign of plugin to be suspended
        virtual Core::hresult Suspend(const string& callsign) = 0;

        // @brief Resumes a plugin
        // @param callsign: Callsign of plugin to be resumed
        virtual Core::hresult Resume(const string& callsign) = 0;
    };

    struct EXTERNAL IShells : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SHELLS };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_SHELLS_NOTIFICATION };

            // @brief Notifies the creation of a Shell (Startup or Clone)
            virtual void Created(const string& callsign, PluginHost::IShell* plugin /* @in */) = 0;
            // @brief Notifies the destruction of a shell (Destroy)
            virtual void Destroy(const string& callsign, PluginHost::IShell* plugin /* @in */) = 0;
        };

        // Pushing notifications to interested sinks
        virtual Core::hresult Register(INotification* sink) = 0;
        virtual Core::hresult Unregister(INotification* sink) = 0;
    };

    /* @json 1.0.0 */
    struct EXTERNAL IMetadata : virtual public Core::IUnknown {

        enum { ID = RPC::ID_CONTROLLER_METADATA };

        struct Data {
            struct Version {
                string Hash /* @brief SHA256 hash identifying the source code */;
                uint8_t Major /* @brief Major number */;
                uint8_t Minor /* @brief Minor number */;
                uint8_t Patch /* @brief Patch number */;
            };

            struct Subsystem {
                PluginHost::ISubSystem::subsystem Subsystem /* @brief Subsystem name */;
                bool Active /* @brief Denotes if currently active */;
            };

            struct CallStack {
                Core::instance_id Address /* @brief Address */;
                string Module /* @brief Module name */;
                string Function /* @optional @brief Function name */;
                uint32_t Line /* @optional @brief Line number */;
            };

            struct Thread {
                Core::instance_id Id /* @brief Thread Id */;
                string Job /* @brief Job name */;
                uint32_t Runs /* @brief Number of runs */;
            };

            struct Proxy {
                uint32_t Interface /* @brief Interface ID */;
                Core::instance_id Instance /* @brief Instance ID */;
                uint32_t Count /* @brief Reference count */;
            };

            struct Link {
                enum state : uint8_t {
                    CLOSED,
                    WEBSERVER /* @text WebServer */,
                    WEBSOCKET /* @text WebSocket */,
                    RAWSOCKET /* @text RawSocket */,
                    COMRPC /* @text COMRPC */,
                    SUSPENDED
                };

                string Remote /* @brief IP address (or FQDN) of the other side of the connection */;
                state State /* @brief State of the link */;
                string Name /* @optional @brief Name of the connection */;
                uint32_t Id /* @brief A unique number identifying the connection */;
                bool Activity /* @brief Denotes if there was any activity on this connection */;
            };

            struct Service {
                enum state : uint32_t {
                    UNAVAILABLE = PluginHost::IShell::UNAVAILABLE,
                    DEACTIVATED = PluginHost::IShell::DEACTIVATED,
                    DEACTIVATION = PluginHost::IShell::DEACTIVATION,
                    ACTIVATED = PluginHost::IShell::ACTIVATED,
                    ACTIVATION = PluginHost::IShell::ACTIVATION,
                    DESTROYED = PluginHost::IShell::DESTROYED,
                    PRECONDITION = PluginHost::IShell::PRECONDITION,
                    HIBERNATED = PluginHost::IShell::HIBERNATED,
                    SUSPENDED,
                    RESUMED
                };

                string Callsign /* @brief Plugin callsign */;
                string Locator /* @brief Shared library path */;
                string ClassName /* @brief Plugin class name */;
                string Module /* @brief Module name */;
                state State /* @brief Current state */;
                PluginHost::IShell::startmode StartMode /* @brief Startup mode */;
                bool Resumed /* @brief Determines if the plugin is to be activated in resumed or suspended mode */;
                Data::Version Version /* @brief Version */;

                string Communicator /* @optional @brief Communicator */;

                string PersistentPathPostfix /* @optional @brief Postfix of persistent path*/;
                string VolatilePathPostfix /* @optional @brief Postfix of volatile path*/;
                string SystemRootPath /* @optional @brief Path of system root */;

                string Precondition /* @opaque @optional @brief Activation conditons */;
                string Termination /* @opaque @optional @brief Deactivation conditions */;

                string Configuration /* @opaque @optional @brief Plugin configuration */;

                uint16_t Observers /* @optional @brief Number or observers */;
                uint32_t ProcessedRequests /* @optional @brief Number of API requests that have been processed by the plugin */;
                uint32_t ProcessedObjects /* @optional @brief Number of objects that have been processed by the plugin */;
            };

            using ISubsystemsIterator = RPC::IIteratorType<Data::Subsystem, RPC::ID_CONTROLLER_METADATA_SUBSYSTEMS_ITERATOR>;
            using ICallStackIterator = RPC::IIteratorType<Data::CallStack, RPC::ID_CONTROLLER_METADATA_CALLSTACK_ITERATOR>;
            using IThreadsIterator = RPC::IIteratorType<Data::Thread, RPC::ID_CONTROLLER_METADATA_THREADS_ITERATOR>;
            using IPendingRequestsIterator = RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>;
            using ILinksIterator = RPC::IIteratorType<Data::Link, RPC::ID_CONTROLLER_METADATA_LINKS_ITERATOR>;
            using IProxiesIterator = RPC::IIteratorType<Data::Proxy, RPC::ID_CONTROLLER_METADATA_PROXIES_ITERATOR>;
            using IServicesIterator = RPC::IIteratorType<Data::Service, RPC::ID_CONTROLLER_METADATA_SERVICES_ITERATOR>;
        };

        // @property @alt:deprecated status
        // @brief Provides status of a service, including their configurations
        virtual Core::hresult Services(const string& callsign /* @index @optional */, Data::IServicesIterator*& services /* @out @extract */) const = 0;

        // @property
        // @brief Provides active connections details
        virtual Core::hresult Links(Data::ILinksIterator*& links /* @out */) const = 0;

        // @property
        // @brief Provides details of a proxy
        virtual Core::hresult Proxies(const uint32_t linkId /* @index */, Data::IProxiesIterator*& proxies /* @out */) const = 0;

        // @property
        // @brief Provides status of subsystems
        virtual Core::hresult Subsystems(Data::ISubsystemsIterator*& subsystems /* @out */) const = 0;

        // @property
        // @brief Provides version and hash of WPEFramework
        virtual Core::hresult Version(Data::Version& version /* @out */) const = 0;

        // @property
        // @brief Provides information on workerpool threads
        virtual Core::hresult Threads(Data::IThreadsIterator*& threads /* @out */) const = 0;

        // @property
        // @brief Provides information on pending requests
        virtual Core::hresult PendingRequests(Data::IPendingRequestsIterator*& requests /* @out */) const = 0;

        // @property
        // @brief Provides callstack associated with the given thread
        virtual Core::hresult CallStack(const uint8_t thread /* @index */, Data::ICallStackIterator*& callstack /* @out */) const = 0;
    };
}

} // namespace Exchange

}
