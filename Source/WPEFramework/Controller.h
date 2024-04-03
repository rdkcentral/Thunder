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

#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include "Module.h"
#include "PluginServer.h"
#include "Probe.h"
#include "IController.h"
#include "PostMortem.h"

#include <plugins/json/JsonData_Events.h>
#include <plugins/json/JEvents.h>

namespace WPEFramework {
namespace Plugin {

    PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE) // for now we must support the deprecated interface for backwards compatibility reasons 
    class Controller
        : public PluginHost::IPlugin
        , public PluginHost::IWeb
        , public PluginHost::JSONRPC
        , public Exchange::Controller::IConfiguration
        , public Exchange::Controller::IDiscovery
        , public Exchange::Controller::ISystem
        , public Exchange::Controller::IMetadata
        , public Exchange::Controller::ILifeTime
        , public Exchange::Controller::IShells
        , public Exchange::Controller::ISubsystems
        , public Exchange::Controller::IEvents {
    private:
        using Resumes = std::vector<string>;
        using ExternalSubSystems = std::vector<PluginHost::ISubSystem::subsystem>;
        using LifeTimeNotifiers = std::vector<Exchange::Controller::ILifeTime::INotification*>;

        class Sink 
            : public PluginHost::IPlugin::INotification
            , public PluginHost::ISubSystem::INotification {
        private:
            class Job {
            public:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

                Job(Controller& parent) : _parent(parent) { }
                ~Job() = default;

            public:
                void Dispatch() {
                    _parent.SubSystems();
                }

            private:
                Controller& _parent;
            };

        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            Sink(Controller& parent)
                : _parent(parent)
                , _job(parent) {
            }
            ~Sink() override {
                _job.Revoke();
            }

        private:
            void Updated() override
            {
                _job.Submit();
            }
            void Activated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.NotifyStateChange(callsign, PluginHost::IShell::ACTIVATED, plugin->Reason());

                // Make sure the resumes 
                _parent.StartupResume(callsign, plugin);

            }
            void Deactivated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.NotifyStateChange(callsign, PluginHost::IShell::DEACTIVATED, plugin->Reason());
            }
            void Unavailable(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.NotifyStateChange(callsign, PluginHost::IShell::UNAVAILABLE, plugin->Reason());
            }

            BEGIN_INTERFACE_MAP(Sink)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            Controller& _parent;
            Core::WorkerPool::JobType<Job> _job;
        };

        // GET -> URL /<MetadataCallsign>/Plugin/<Callsign>
        // PUT -> URL /<MetadataCallsign>/Configure
        // PUT -> URL /<MetadataCallsign>/Activate/<Callsign>
        // PUT -> URL /<MetadataCallsign>/Deactivate/<Callsign>
        // DELETE -> URL /<MetadataCallsign>/Plugin/<Callsign>
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            class ProbeConfig : public Core::JSON::Container {
            private:
                ProbeConfig(const ProbeConfig&) = delete;
                ProbeConfig& operator=(const ProbeConfig&) = delete;

            public:
                ProbeConfig()
                    : Core::JSON::Container()
                    , TTL(1)
                    , Node(_T("239.255.255.0:1900"))
                {
                    Add(_T("ttl"), &TTL);
                    Add(_T("node"), &Node);
                }
                ~ProbeConfig() override = default;

            public:
                Core::JSON::DecUInt8 TTL;
                Core::JSON::String Node;
            };

        public:
            Config()
                : Core::JSON::Container()
                , Probe()
                , Resumes()
                , SubSystems()
                , Ui(true)
            {
                Add(_T("probe"), &Probe);
                Add(_T("resumes"), &Resumes);
                Add(_T("subsystems"), &SubSystems);
                Add(_T("ui"), &Ui);
            }
            ~Config()
            {
            }

        public:
            ProbeConfig Probe;
            Core::JSON::ArrayType<Core::JSON::String> Resumes;
            Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> SubSystems;
            Core::JSON::Boolean Ui;
        };

    private:
        Controller(const Controller&);
        Controller& operator=(const Controller&);

    protected:
        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            Controller()
            : _adminLock()
            , _skipURL(0)
            , _webPath()
            , _pluginServer(nullptr)
            , _service(nullptr)
            , _probe(nullptr)
            , _systemInfoReport(*this)
            , _resumes()
            , _lastReported()
            , _externalSubsystems()
            , _lifeTimeObservers()
        {
        }
        POP_WARNING()

    public:
        virtual ~Controller()
        {
            // Attach to the SubSystems, we propagate the changes.
            PluginHost::ISubSystem* subSystems(_service->SubSystems());

            ASSERT(subSystems != nullptr);

            if (subSystems != nullptr) {
                subSystems->Unregister(&_systemInfoReport);
                subSystems->Release();
            }
        }

        void Notification(const string& callsign, const string& message)
        {
            ASSERT(callsign.empty() == false);
            ASSERT(message.empty() == false);

            Exchange::Controller::JEvents::Event::ForwardMessage(*this, callsign, message);
        }

        void Notification(const string& callsign, const string& event, const string& params)
        {
            ASSERT(callsign.empty() == false);
            ASSERT(event.empty() == false);

            Exchange::Controller::IEvents::INotification::Event data{event, params};
            Exchange::Controller::JEvents::Event::ForwardEvent(*this, callsign, data);
        }

        inline void SetServer(PluginHost::Server* pluginServer, const std::vector<PluginHost::ISubSystem::subsystem>& externalSubsystems)
        {
            ASSERT((_pluginServer == nullptr) && (pluginServer != nullptr));

            _pluginServer = pluginServer;

            // Attach to the SubSystems, we propagate the changes.
            PluginHost::ISubSystem* subSystems(_service->SubSystems());

            ASSERT(subSystems != nullptr);

            if (subSystems != nullptr) {
                uint32_t entries = ~0;

                for (const PluginHost::ISubSystem::subsystem& entry : externalSubsystems) {
                    if (std::find(_externalSubsystems.begin(), _externalSubsystems.end(), entry) != _externalSubsystems.end()) {
                        Core::EnumerateType<PluginHost::ISubSystem::subsystem> name(entry);
                        SYSLOG(Logging::Startup, (Core::Format(_T("Subsystem [%s] was already signalled by plugin metadata. No need as Controller config!!!"), name.Data())));
                    }
                    else {
                        _externalSubsystems.emplace_back(entry);
                    }
                }

                // Insert the subsystems found from the Plugins configured to use..
                for (const PluginHost::ISubSystem::subsystem& entry : _externalSubsystems) {
                    entries &= ~(1 << entry);
                }

                uint8_t setFlag = 0;
                while (setFlag < PluginHost::ISubSystem::END_LIST) {
                    if ((entries & (1 << setFlag)) != 0) {
                        TRACE_L1("Setting the default SubSystem: %d", setFlag);
                        subSystems->Set(static_cast<PluginHost::ISubSystem::subsystem>(setFlag), nullptr);
                    }
                    setFlag++;
                }

                subSystems->Register(&_systemInfoReport);
                subSystems->Release();
            }
        }
        inline uint32_t Stopped()
        {
            return (Core::ERROR_NONE);
        }

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a request is received, it might carry some additional data in the body. This method allows
        // the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        // in the body of the request.
        void Inbound(Web::Request& request) override;

        // If everything is received correctly, the request is passed to us, on a thread from the thread pool, to
        // do our thing and to return the result in the response object. Here the actual specific module work,
        // based on a a request is handled.
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        //  Controller methods
        // -------------------------------------------------------------------------------------------------------
        // ISystem overrides
        Core::hresult Delete(const string& path) override;
        Core::hresult Reboot() override;
        Core::hresult Environment(const string& variable, string& value) const override;
        Core::hresult Clone(const string& callsign, const string& newcallsign, string& response) override;

        // IDiscovery overrides
        Core::hresult StartDiscovery(const uint8_t ttl) override;
        Core::hresult DiscoveryResults(IDiscovery::Data::IDiscoveryResultsIterator*& results) const override;

        // IConfiguration overrides
        Core::hresult Persist() override;
        Core::hresult Configuration(const string& callsign, string& configuration) const override;
        Core::hresult Configuration(const string& callsign, const string& configuration) override;

        // ILifeTime overrides
        Core::hresult Register(Exchange::Controller::ILifeTime::INotification* notification) override;
        Core::hresult Unregister(Exchange::Controller::ILifeTime::INotification* notification) override;
        Core::hresult Activate(const string& callsign) override;
        Core::hresult Deactivate(const string& callsign) override;
        Core::hresult Unavailable(const string& callsign) override;
        Core::hresult Suspend(const string& callsign) override;
        Core::hresult Resume(const string& callsign) override;
        Core::hresult Hibernate(const string& callsign, const uint32_t timeout) override;

        // ISubsystems overrides
        Core::hresult Subsystems(ISubsystems::ISubsystemsIterator*& outSubsystems) const override;

        // IMetadata overrides
        Core::hresult Links(IMetadata::Data::ILinksIterator*& links) const override;
        Core::hresult Proxies(const uint32_t linkId, IMetadata::Data::IProxiesIterator*& proxies) const override;
        Core::hresult Services(const string& callsign, IMetadata::Data::IServicesIterator*& services) const override;
        Core::hresult CallStack(const uint8_t threadId, IMetadata::Data::ICallStackIterator*& callstack) const override;
        Core::hresult Threads(IMetadata::Data::IThreadsIterator*& threads) const override;
        Core::hresult PendingRequests(IMetadata::Data::IPendingRequestsIterator*& requests) const override;
        Core::hresult Version(IMetadata::Data::Version& version) const override;

        // IShells overrides
        Core::hresult Register(Exchange::Controller::IShells::INotification* sink) override;
        Core::hresult Unregister(Exchange::Controller::IShells::INotification* sink) override;

        //  IUnknown methods
        // -------------------------------------------------------------------------------------------------------
        BEGIN_INTERFACE_MAP(Controller)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::Controller::IConfiguration)
            INTERFACE_ENTRY(Exchange::Controller::IDiscovery)
            INTERFACE_ENTRY(Exchange::Controller::ISystem)
            INTERFACE_ENTRY(Exchange::Controller::IMetadata)
            INTERFACE_ENTRY(Exchange::Controller::ILifeTime)
            INTERFACE_ENTRY(Exchange::Controller::IShells)
            INTERFACE_ENTRY(Exchange::Controller::ISubsystems)
            INTERFACE_ENTRY(Exchange::Controller::IEvents)
        END_INTERFACE_MAP

    private:
        //  ILocalDispatcher methods
        // -------------------------------------------------------------------------------------------------------
        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response /* @out */) override;

        inline Core::ProxyType<PluginHost::IShell> FromIdentifier(const string& callsign) const
        {
            Core::ProxyType<PluginHost::IShell> service;

            _pluginServer->Services().FromIdentifier(callsign, service);

            return (service);
        }
        void WorkerPoolMetadata(PluginHost::Metadata::Server& data) const {
            _pluginServer->WorkerPool().Snapshot(data);
        }
        void Callstack(const ThreadId id, Core::JSON::ArrayType<PluginHost::CallstackData>& response) const;
        void SubSystems();
        uint32_t Clone(const string& basecallsign, const string& newcallsign);
        void Proxies(Core::JSON::ArrayType<PluginHost::Metadata::COMRPC>& info) const;
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index) const;
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void StartupResume(const string& callsign, PluginHost::IShell* plugin);
        void NotifyStateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason);

    private:
        Core::CriticalSection _adminLock;
        uint8_t _skipURL;
        string _webPath;
        PluginHost::Server* _pluginServer;
        PluginHost::IShell* _service;
        Probe* _probe;
        Core::SinkType<Sink> _systemInfoReport;
        Resumes _resumes;
        uint32_t _lastReported;
        ExternalSubSystems _externalSubsystems;
        LifeTimeNotifiers _lifeTimeObservers;
    };

    POP_WARNING()
}
}

#endif // __CONTROLLER_H
