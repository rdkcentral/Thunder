#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include "Module.h"
#include "PluginServer.h"
#include "DownloadEngine.h"
#include "Probe.h"

namespace WPEFramework {
namespace Plugin {

    class Controller : public PluginHost::IPlugin, public PluginHost::IWeb {
    private:
        class Downloader : public PluginHost::DownloadEngine {
        private:
            Downloader() = delete;
            Downloader(const Downloader&) = delete;
            Downloader& operator=(const Downloader&) = delete;

        public:
            Downloader(Controller& parent, const string& key)
                : PluginHost::DownloadEngine(key)
                , _parent(parent)
            {
            }
            virtual ~Downloader()
            {
            }

        private:
            virtual void Transfered(const uint32_t result, const string& source, const string& destination) override
            {
                _parent.Transfered(result, source, destination);
            }

        private:
            Controller& _parent;
        };
       class Sink : 
            public PluginHost::IPlugin::INotification,
            public PluginHost::ISubSystem::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            class Job : public Core::IDispatchType<void> {
            private:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

            public:
                Job(Controller* parent) 
                    : _parent(*parent)
                    , _schedule(false)
                {
                    ASSERT(parent != nullptr);
                }
                virtual ~Job()
                {
                }

            public:
                void Schedule() {
                    if (_schedule == false) {
                        _schedule = true;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType< Core::IDispatchType<void> >(*this));
                    }
                }
                virtual void Dispatch()
                {
                    _schedule = false;
                    _parent.SubSystems();
                }

            private:
                Controller& _parent;
                bool _schedule;
            };
 
        public:
            Sink(Controller* parent)
                : _parent(*parent)
                , _decoupled(Core::ProxyType<Job>::Create(parent))
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Sink()
            {
                PluginHost::WorkerPool::Instance().Revoke(_decoupled);
            }

        private:
            virtual void Updated() override
            {
                _decoupled->Schedule();
            }
            virtual void StateChange(PluginHost::IShell* plugin) override 
            {
                _parent.StateChange(plugin);
            }

            BEGIN_INTERFACE_MAP(Sink)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            Controller& _parent;
            Core::ProxyType<Job> _decoupled;
        };


        // GET -> URL /<MetaDataCallsign>/Plugin/<Callsign>
        // PUT -> URL /<MetaDataCallsign>/Configure
        // PUT -> URL /<MetaDataCallsign>/Activate/<Callsign>
        // PUT -> URL /<MetaDataCallsign>/Deactivate/<Callsign>
        // DELETE -> URL /<MetaDataCallsign>/Plugin/<Callsign>
        // PUT -> URL /<MetaDataCallsign>/Download
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , DownloadStore()
                , TTL(1)
                , Resumes()
                , SubSystems()
            {
                Add(_T("downloadstore"), &DownloadStore);
                Add(_T("ttl"), &TTL);
                Add(_T("resumes"), &Resumes);
                Add(_T("subsystems"), &SubSystems);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String DownloadStore;
            Core::JSON::DecUInt8 TTL;
            Core::JSON::ArrayType<Core::JSON::String> Resumes;
            Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> > SubSystems;
        };
        class Download : public Core::JSON::Container {
        private:
            Download(const Download&);
            Download& operator=(const Download&);

        public:
            Download()
                : Core::JSON::Container()
            {
                Add(_T("source"), &Source);
                Add(_T("destination"), &Destination);
                Add(_T("hash"), &Hash);
            }
            ~Download()
            {
            }

        public:
            Core::JSON::String Source;
            Core::JSON::String Destination;
            Core::JSON::String Hash;
        };

    private:
        Controller(const Controller&);
        Controller& operator=(const Controller&);

    protected:
        Controller()
            : _adminLock()
            , _skipURL(0)
            , _webPath()
            , _pluginServer(nullptr)
            , _service(nullptr)
            , _downloader(nullptr)
            , _probe(nullptr)
            , _systemInfoReport(this)
            , _resumes()
            , _lastReported()
        {
        }

    public:
        virtual ~Controller()
        {
            SetServer(nullptr);
        }
        inline void SetServer(PluginHost::Server* pluginServer)
        {
            ASSERT((_pluginServer == nullptr) ^ (pluginServer == nullptr));

            _pluginServer = pluginServer;

            // Attach to the SubSystems, we propagate the changes.
            PluginHost::ISubSystem* subSystems (_service->SubSystems());

            ASSERT (subSystems != nullptr);

            if (subSystems != nullptr) {
                if (pluginServer != nullptr) {
                    subSystems->Register(&_systemInfoReport);
                }
                else {
                    subSystems->Unregister(&_systemInfoReport);
                }
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
        END_INTERFACE_MAP

    private:
        void SubSystems();
        void SubSystems(Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> >::ConstIterator& index);
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index) const;
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void Transfered(const uint32_t result, const string& source, const string& destination);
        void StateChange(PluginHost::IShell* plugin);

    private:
        Core::CriticalSection _adminLock;
        uint8_t _skipURL;
        string _webPath;
        PluginHost::Server* _pluginServer;
        PluginHost::IShell* _service;
        Downloader* _downloader;
        Probe* _probe;
        Core::Sink<Sink> _systemInfoReport;
        std::list<string> _resumes;
	uint32_t _lastReported;
    };
}
}

#endif // __CONTROLLER_H
