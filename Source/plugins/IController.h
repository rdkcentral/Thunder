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

namespace Thunder {

namespace Exchange {

namespace Controller {

    // @json 1.0.0 @text:legacy_lowercase
    struct EXTERNAL ISystem : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SYSTEM };

        // @alt:deprecated harakiri
        // @brief Reboots the device
        // @details Depending on the device this call may not generate a response.
        virtual Core::hresult Reboot() = 0;

        // @brief Removes contents of a directory from the persistent storage
        // @param path: Path to the directory within the persisent storage
        virtual Core::hresult Delete(const string& path) = 0;

        // @brief Creates a clone of given plugin with a new callsign
        // @param callsign: Callsign of the plugin
        // @param newcallsign: Callsign for the cloned plugin
        virtual Core::hresult Clone(const string& callsign, const string& newcallsign, string& response /* @out */) = 0;

        // @brief Destroy given plugin
        // @param callsign: Callsign of the plugin
        virtual Core::hresult Destroy(const string& callsign) = 0;

        // @property
        // @brief Environment variable value
        virtual Core::hresult Environment(const string& variable /* @index */, string& value /* @out */ ) const = 0;
    };

    // @json 1.0.0 @text:legacy_lowercase
    struct EXTERNAL IDiscovery : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_DISCOVERY };

        struct Data {
            struct DiscoveryResult {
                string Locator /* @brief Locator for the discovery */;
                uint32_t Latency /* @brief Latency for the discovery */;
                Core::OptionalType<string> Model /* @brief Model */;
                bool Secure /* @brief Secure or not */;
            };

            using IDiscoveryResultsIterator = RPC::IIteratorType<Data::DiscoveryResult, RPC::ID_CONTROLLER_DISCOVERY_RESULTS_ITERATOR>;
        };

        // @brief Starts SSDP network discovery
        // @param ttl: Time to live, parameter for SSDP discovery
        virtual Core::hresult StartDiscovery(const Core::OptionalType<uint8_t>& ttl /* @default:1 @restrict:1..255 */) = 0;

        // @property
        // @brief SSDP network discovery results
        virtual Core::hresult DiscoveryResults(Data::IDiscoveryResultsIterator*& results /* @out */) const = 0;
    };

    // @json 1.0.0 @text:legacy_lowercase @uncompliant:extended 
    struct EXTERNAL IConfiguration : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_CONFIGURATION };

        // @alt storeconfig
        // @brief Stores all configuration to the persistent memory
        virtual Core::hresult Persist() = 0;

        // @property
        // @brief Service configuration
        virtual Core::hresult Configuration(const Core::OptionalType<string>& callsign /* @index */, string& configuration /* @out @opaque */) const = 0;
        virtual Core::hresult Configuration(const string& callsign /* @index */, const string& configuration /* @opaque */) = 0;
    };

    // @json 1.0.0 @text:legacy_lowercase
    struct EXTERNAL ILifeTime : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_LIFETIME };

        enum state : uint8_t {
            UNKNOWN,
            SUSPENDED,
            RESUMED
        };

        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_LIFETIME_NOTIFICATION };

            // @brief Notifies of a plugin state change
            // @param callsign: Plugin callsign
            // @param state: New state of the plugin
            // @param reason: Reason for state change
            virtual void StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason) = 0;

            // @brief Notifies of a plugin state change controlled by IStateControl
            // @param callsign: Plugin callsign
            // @param state: New state of the plugin
            // @param reason: Reason for state change
            virtual void StateControlStateChange(const string& callsign, const state& state) = 0;
        };

        virtual Core::hresult Register(INotification* sink) = 0;
        virtual Core::hresult Unregister(INotification* sink) = 0;

        // @brief Activates a plugin
        // @details Use this method to activate a plugin, i.e. move from Deactivated, via Activating to Activated state.
        //          If a plugin is in Activated state, it can handle JSON-RPC requests that are coming in.
        //          The plugin is loaded into memory only if it gets activated.
        // @param callsign: Callsign of plugin to be activated
        virtual Core::hresult Activate(const string& callsign) = 0;

        // @brief Deactivates a plugin
        // @details Use this method to deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state.
        //          If a plugin is deactivated, the actual plugin (.so) is no longer loaded into the memory of the process.
        //          In a Deactivated state the plugin will not respond to any JSON-RPC requests.
        // @param callsign: Callsign of plugin to be deactivated
        virtual Core::hresult Deactivate(const string& callsign) = 0;

        // @brief Makes a plugin unavailable for interaction
        // @details Use this method to mark a plugin as unavailable, i.e. move from Deactivated to Unavailable state.
        //          It can not be started unless it is first deactivated (what triggers a state transition).
        // @param callsign: Callsign of plugin to be set as unavailable
        virtual Core::hresult Unavailable(const string& callsign) = 0;

        // @brief Hibernates a plugin
        // @param callsign: Callsign of plugin to be hibernated
        // @details Use *activate* to wake up a hibernated plugin.
        //          In a Hibernated state the plugin will not respond to any JSON-RPC requests.
        // @param timeout: Allowed time
        // @retval ERROR_INPROC The plugin is running in-process and thus cannot be hibernated
        virtual Core::hresult Hibernate(const string& callsign, const uint32_t timeout) = 0;

        // @brief Suspends a plugin
        // @details This is a more intelligent method, compared to *deactivate*, to move a plugin to a suspended state
        //          depending on its current state. Depending on the *startmode* flag this method will deactivate the plugin
        //          or only suspend the plugin.
        // @param callsign: Callsign of plugin to be suspended
        virtual Core::hresult Suspend(const string& callsign) = 0;

        // @brief Resumes a plugin
        // @details This is a more intelligent method, compared to *activate*, to move a plugin to a resumed state
        //          depending on its current state. If required it will activate and move to the resumed state,
        //          regardless of the flags in the config (i.e. *startmode*, *resumed*)
        // @param callsign: Callsign of plugin to be resumed
        virtual Core::hresult Resume(const string& callsign) = 0;
    };

    struct EXTERNAL IShells : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SHELLS };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_SHELLS_NOTIFICATION };

            virtual void Created(const string& callsign, PluginHost::IShell* plugin /* @in */) = 0;
            virtual void Destroy(const string& callsign, PluginHost::IShell* plugin /* @in */) = 0;
        };

        virtual Core::hresult Register(INotification* sink) = 0;
        virtual Core::hresult Unregister(INotification* sink) = 0;
    };

    // @json 1.0.0 @text:legacy_lowercase @uncompliant:collapsed
    struct EXTERNAL ISubsystems : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_SUBSYSTEMS };

        struct Subsystem {
            PluginHost::ISubSystem::subsystem Subsystem /* @brief Name of the subsystem*/;
            bool Active /* @brief Denotes if the subsystem is currently active */;
        };

        using ISubsystemsIterator = RPC::IIteratorType<Subsystem, RPC::ID_CONTROLLER_SUBSYSTEMS_ITERATOR>;

        // @event @uncompliant:collapsed
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_SUBSYSTEMS_NOTIFICATION };

            // @brief Notifies a subsystem change
            // @param subsystems: Subsystems that have changed
            virtual void SubsystemChange(ISubsystemsIterator* const subsystems /* @in */) = 0;
        };

        // @property
        // @brief Subsystems status
        virtual Core::hresult Subsystems(ISubsystemsIterator*& subsystems /* @out */) const = 0;
    };

    // @json 1.0.0 @text:legacy_lowercase
    struct EXTERNAL IEvents : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_EVENTS };

        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = RPC::ID_CONTROLLER_EVENTS_NOTIFICATION };

            // @text all
            // @brief Notifies all events forwarded by the framework
            // @details The Controller plugin is an aggregator of all the events triggered by a specific plugin.
            //          All notifications send by any plugin are forwarded over the Controller socket as an event.
            // @param event: Name of the message
            // @param callsign: Origin of the message
            // @param params: Contents of the message
            virtual void ForwardMessage(const string& event, const Core::OptionalType<string>& callsign, const Core::OptionalType<string>& params /* @opaque */) = 0;
        };
    };

    // @json 1.0.0 @text:legacy_lowercase
    struct EXTERNAL IMetadata : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER_METADATA };

        struct Data {
            struct Version {
                string Hash /* @brief SHA256 hash identifying the source code @restrict:64..64 */;
                uint8_t Major /* @brief Major number */;
                uint8_t Minor /* @brief Minor number */;
                uint8_t Patch /* @brief Patch number */;
            };

            struct BuildInfo {

                enum systemtype : uint8_t {
                    SYSTEM_WINDOWS /* @text:Windows */,
                    SYSTEM_LINUX /* @text:Linux */,
                    SYSTEM_MACOS /* @text:MacOS */
                };

                enum buildtype : uint8_t {
                    DEBUG,
                    DEBUG_OPTIMIZED,
                    RELEASE_WITH_DEBUG_INFO,
                    RELEASE,
                    PRODUCTION
                };

                enum extensiontype : uint8_t {
                    WARNING_REPORTING = 1,
                    BLUETOOTH = 2,
                    HIBERBATE = 4,
                    PROCESS_CONTAINERS = 8
                };

                systemtype SystemType /* @brief System type */;
                buildtype BuildType /* @brief Build type */;
                Core::OptionalType<extensiontype> Extensions /* @bitmask */;
                bool Messaging /* @brief Denotes whether Messaging is enabled*/;
                bool ExceptionCatching /* @brief Denotes whether there is an exception */;
                bool DeadlockDetection /* @brief Denotes whether deadlock detection is enabled */;
                bool WCharSupport /* Denotes whether there is wchar support */;
                uint8_t InstanceIDBits /* @brief Core instance bits */;
                Core::OptionalType<uint8_t> TraceLevel /* @brief Trace level */;
                uint8_t ThreadPoolCount /* Number of configured threads on the threadpool */;
                uint32_t COMRPCTimeOut /* The number of milliseconds a COMRPC call can take before it is assumed to fail */;
            };

            struct CallStack {
                Core::instance_id Address /* @brief Memory address */;
                string Module /* @brief Module name */;
                Core::OptionalType<string> Function /* @brief Function name */;
                Core::OptionalType<uint32_t> Line /* @brief Line number */;
            };

            struct Thread {
                Core::instance_id Id /* @brief Thread ID */;
                string Job /* @brief Job name */;
                uint32_t Runs /* @brief Number of runs */;
            };

            struct Proxy {
                uint32_t Interface /* @brief Interface ID */;
                string Name /* @brief The fully qualified name of the interface */;
                Core::instance_id Instance /* @brief Instance ID */;
                uint32_t Count /* @brief Reference count */;
                Core::OptionalType<string> Origin /* @brief The Origin of the assocated connection */;
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
                uint32_t Id /* @brief A unique number identifying the connection */;
                bool Activity /* @brief Denotes if there was any activity on this connection */;
                Core::OptionalType<string> Name /* @brief Name of the connection */;
            };

            struct Service {
                enum state : uint16_t {
                    UNAVAILABLE = PluginHost::IShell::UNAVAILABLE,
                    DEACTIVATED = PluginHost::IShell::DEACTIVATED,
                    DEACTIVATION = PluginHost::IShell::DEACTIVATION,
                    ACTIVATED = PluginHost::IShell::ACTIVATED,
                    ACTIVATION = PluginHost::IShell::ACTIVATION,
                    DESTROYED = PluginHost::IShell::DESTROYED,
                    PRECONDITION = PluginHost::IShell::PRECONDITION,
                    HIBERNATED = PluginHost::IShell::HIBERNATED,
                    SUSPENDED = 0x100,
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

                Core::OptionalType<string> Communicator /* @brief Communicator */;

                Core::OptionalType<string> PersistentPathPostfix /* @brief Postfix of persistent path */;
                Core::OptionalType<string> VolatilePathPostfix /* @brief Postfix of volatile path */;
                Core::OptionalType<string> SystemRootPath /* @brief Path of system root */;

                Core::OptionalType<string> Precondition /* @opaque @brief Activation conditons */;
                Core::OptionalType<string> Termination /* @opaque @brief Deactivation conditions */;
                Core::OptionalType<string> Control /* @opaque @brief Conditions controlled by this service */;

                string Configuration /* @opaque @brief Plugin configuration */;

                uint16_t Observers /* @brief Number or observers */;
                Core::OptionalType<uint32_t> ProcessedRequests /* @brief Number of API requests that have been processed by the plugin */;
                Core::OptionalType<uint32_t> ProcessedObjects /* @brief Number of objects that have been processed by the plugin */;
            };

            using ICallStackIterator = RPC::IIteratorType<Data::CallStack, RPC::ID_CONTROLLER_METADATA_CALLSTACK_ITERATOR>;
            using IThreadsIterator = RPC::IIteratorType<Data::Thread, RPC::ID_CONTROLLER_METADATA_THREADS_ITERATOR>;
            using IPendingRequestsIterator = RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>;
            using ILinksIterator = RPC::IIteratorType<Data::Link, RPC::ID_CONTROLLER_METADATA_LINKS_ITERATOR>;
            using IProxiesIterator = RPC::IIteratorType<Data::Proxy, RPC::ID_CONTROLLER_METADATA_PROXIES_ITERATOR>;
            using IServicesIterator = RPC::IIteratorType<Data::Service, RPC::ID_CONTROLLER_METADATA_SERVICES_ITERATOR>;
        };

        // @property @alt:deprecated status
        // @brief Services metadata
        // @details If callsign is omitted, metadata of all services is returned.
        virtual Core::hresult Services(const Core::OptionalType<string>& callsign /* @index */, Data::IServicesIterator*& services /* @out @extract */) const = 0;

        // @property
        // @brief Connections list of Thunder connections 
        virtual Core::hresult Links(Data::ILinksIterator*& links /* @out */) const = 0;

        // @property
        // @brief Proxies list
        virtual Core::hresult Proxies(const Core::OptionalType<string>& linkID /* @index */, Data::IProxiesIterator*& proxies /* @out */) const = 0;

        // @property
        // @brief Framework version
        virtual Core::hresult Version(Data::Version& version /* @out */) const = 0;

        // @property
        // @brief Workerpool threads
        virtual Core::hresult Threads(Data::IThreadsIterator*& threads /* @out */) const = 0;

        // @property
        // @brief Pending requests
        virtual Core::hresult PendingRequests(Data::IPendingRequestsIterator*& requests /* @out */) const = 0;

        // @property
        // @brief Thread callstack
        virtual Core::hresult CallStack(const uint8_t thread /* @index */, Data::ICallStackIterator*& callstack /* @out */) const = 0;
    
        // @property
        // @brief Build information
        virtual Core::hresult BuildInfo(Data::BuildInfo& buildInfo /* @out */) const = 0;
    };

} // namespace Controller

} // namespace Exchange

}
