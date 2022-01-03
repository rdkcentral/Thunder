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
#include "json/JsonData_Controller.h"

namespace WPEFramework {
namespace Plugin {

    using JSONCallstack =  Web::JSONBodyType < Core::JSON::ArrayType < Core::JSON::String > >;

    class Controller : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        class Sink : public PluginHost::IPlugin::INotification,
                     public PluginHost::ISubSystem::INotification {
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
            virtual ~Sink() {
                _job.Revoke();
            }

        private:
            void Updated() override
            {
                _job.Submit();
            }
            void Activated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Activated(callsign, plugin);
            }
            void Deactivated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Deactivated(callsign, plugin);
            }
            void Unavailable(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Unavailable(callsign, plugin);
            }

            BEGIN_INTERFACE_MAP(Sink)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            Controller& _parent;
            Core::WorkerPool::JobType<Job> _job;
        };

        // GET -> URL /<MetaDataCallsign>/Plugin/<Callsign>
        // PUT -> URL /<MetaDataCallsign>/Configure
        // PUT -> URL /<MetaDataCallsign>/Activate/<Callsign>
        // PUT -> URL /<MetaDataCallsign>/Deactivate/<Callsign>
        // DELETE -> URL /<MetaDataCallsign>/Plugin/<Callsign>
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
            {
                Add(_T("probe"), &Probe);
                Add(_T("resumes"), &Resumes);
                Add(_T("subsystems"), &SubSystems);
            }
            ~Config()
            {
            }

        public:
            ProbeConfig Probe;
            Core::JSON::ArrayType<Core::JSON::String> Resumes;
            Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> SubSystems;
        };

    private:
        Controller(const Controller&);
        Controller& operator=(const Controller&);

    protected:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
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
        {
            RegisterAll();
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

    public:
        virtual ~Controller()
        {
            UnregisterAll();
            SetServer(nullptr);
        }
        inline void Notification(const PluginHost::Server::ForwardMessage& message)
        {
            Notify("all", message);
        }

        inline void SetServer(PluginHost::Server* pluginServer)
        {
            ASSERT((_pluginServer == nullptr) ^ (pluginServer == nullptr));

            _pluginServer = pluginServer;

            // Attach to the SubSystems, we propagate the changes.
            PluginHost::ISubSystem* subSystems(_service->SubSystems());

            ASSERT(subSystems != nullptr);

            if (subSystems != nullptr) {
                if (pluginServer != nullptr) {
                    subSystems->Register(&_systemInfoReport);
                } else {
                    subSystems->Unregister(&_systemInfoReport);
                }
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
        virtual const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a request is received, it might carry some additional data in the body. This method allows
        // the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        // in the body of the request.
        virtual void Inbound(Web::Request& request) override;

        // If everything is received correctly, the request is passed to us, on a thread from the thread pool, to
        // do our thing and to return the result in the response object. Here the actual specific module work,
        // based on a a request is handled.
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        //  IUnknown methods
        // -------------------------------------------------------------------------------------------------------
        BEGIN_INTERFACE_MAP(Controller)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        //  IDispatch methods
        // -------------------------------------------------------------------------------------------------------
        Core::ProxyType<Core::JSONRPC::Message> Invoke(const string& token, const uint32_t channelId, const Core::JSONRPC::Message& inbound) override;

        inline Core::ProxyType<PluginHost::Server::Service> FromIdentifier(const string& callsign) const
        {
            Core::ProxyType<PluginHost::Server::Service> service;

            _pluginServer->Services().FromIdentifier(callsign, service);

            return (service);
        }
		void WorkerPoolMetaData(PluginHost::MetaData::Server& data) const
		{
            const Core::WorkerPool::Metadata& snapshot = Core::WorkerPool::Instance().Snapshot();

            data.PendingRequests = snapshot.Pending;
            data.PoolOccupation = snapshot.Occupation;

            for (uint8_t teller = 0; teller < snapshot.Slots; teller++) {
                // Example of why copy-constructor and assignment constructor should be equal...
                Core::JSON::DecUInt32 newElement;
                newElement = snapshot.Slot[teller];
                data.ThreadPoolRuns.Add(newElement);
            }
		}
        void SubSystems();
        void SubSystems(Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>>::ConstIterator& index);
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index) const;
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void Activated(const string& callsign, PluginHost::IShell* plugin);
        void Deactivated(const string& callsign, PluginHost::IShell* plugin);
        void Unavailable(const string& callsign, PluginHost::IShell* plugin);

        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_suspend(const JsonData::Controller::ActivateParamsInfo& params);
        uint32_t endpoint_resume(const JsonData::Controller::ActivateParamsInfo& params);
        uint32_t endpoint_activate(const JsonData::Controller::ActivateParamsInfo& params);
        uint32_t endpoint_clone(const JsonData::Controller::CloneParamsInfo& params, Core::JSON::String& response);
        uint32_t endpoint_deactivate(const JsonData::Controller::ActivateParamsInfo& params);
        uint32_t endpoint_unavailable(const JsonData::Controller::ActivateParamsInfo& params);
        uint32_t endpoint_startdiscovery(const JsonData::Controller::StartdiscoveryParamsData& params);
        uint32_t endpoint_storeconfig();
        uint32_t endpoint_delete(const JsonData::Controller::DeleteParamsData& params);
        uint32_t endpoint_harakiri();
        uint32_t get_callstack(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_status(const string& index, Core::JSON::ArrayType<PluginHost::MetaData::Service>& response) const;
        uint32_t get_links(Core::JSON::ArrayType<PluginHost::MetaData::Channel>& response) const;
        uint32_t get_processinfo(PluginHost::MetaData::Server& response) const;
        uint32_t get_subsystems(Core::JSON::ArrayType<JsonData::Controller::SubsystemsParamsData>& response) const;
        uint32_t get_discoveryresults(Core::JSON::ArrayType<PluginHost::MetaData::Bridge>& response) const;
        uint32_t get_environment(const string& index, Core::JSON::String& response) const;
        uint32_t get_configuration(const string& index, Core::JSON::String& response) const;
        uint32_t set_configuration(const string& index, const Core::JSON::String& params);
        uint32_t get_version(Core::JSON::String& response) const;
        uint32_t set_version(const Core::JSON::String& params);
        uint32_t get_prefix(Core::JSON::String& response) const;
        uint32_t set_prefix(const Core::JSON::String& params);
        uint32_t get_idletime(Core::JSON::DecUInt16& response) const;
        uint32_t set_idletime(const Core::JSON::DecUInt16& params);
        uint32_t get_latitude(Core::JSON::DecSInt32& response) const;
        uint32_t set_latitude(const Core::JSON::DecSInt32& params);
        uint32_t get_longitude(Core::JSON::DecSInt32& response) const;
        uint32_t set_longitude(const Core::JSON::DecSInt32& params);
        void event_all(const string& callsign, const Core::JSON::String& data);
        void event_statechange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason);

    private:
        Core::CriticalSection _adminLock;
        uint8_t _skipURL;
        string _webPath;
        PluginHost::Server* _pluginServer;
        PluginHost::IShell* _service;
        Probe* _probe;
        Core::Sink<Sink> _systemInfoReport;
        std::list<string> _resumes;
        uint32_t _lastReported;
    };
}
}

#endif // __CONTROLLER_H
