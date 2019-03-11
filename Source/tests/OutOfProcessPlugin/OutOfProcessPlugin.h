#ifndef __OUTOFPROCESSTEST_H
#define __OUTOFPROCESSTEST_H

#include "Module.h"
#include <interfaces/IBrowser.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class OutOfProcessPlugin : public PluginHost::IPluginExtended, public PluginHost::IWeb {
    private:
        OutOfProcessPlugin(const OutOfProcessPlugin&);
        OutOfProcessPlugin& operator=(const OutOfProcessPlugin&);

        class Notification : 
			public Exchange::IBrowser::INotification,
			public PluginHost::IStateControl::INotification,
            public RPC::IRemoteProcess::INotification {
        private:
            Notification();
            Notification(const Notification&);
            Notification& operator=(const Notification&);

        public:
			explicit Notification(OutOfProcessPlugin* parent)
				: _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
				TRACE_L1("WebServer::Notification destructed. Line: %d", __LINE__);
            }

        public:
            virtual void LoadFinished(const string& URL)
            {
                _parent.LoadFinished(URL);
            }
            virtual void URLChanged(const string& URL)
            {
                _parent.URLChanged(URL);
            }
            virtual void Hidden(const bool hidden)
            {
                _parent.Hidden(hidden);
            }
            virtual void StateChange(const PluginHost::IStateControl::state value)
            {
                _parent.StateChange(value);
            }
			virtual void Activated(RPC::IRemoteProcess* /* process */) 
			{
			}
			virtual void Deactivated(RPC::IRemoteProcess* process)
            {
				_parent.Deactivated(process);
			}
			virtual void Closure() {
			}

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IBrowser::INotification)
            INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
            INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            END_INTERFACE_MAP

        private:
            OutOfProcessPlugin& _parent;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(true)
            {
                Add(_T("outofprocess"), &OutOfProcess);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean OutOfProcess;
        };

    public:
        class Data : public Core::JSON::Container {
        private:
            Data(const Data&);
            Data& operator=(const Data&);

        public:
            Data()
                : Core::JSON::Container()
                , URL()
                , FPS()
                , Suspended(false)
                , Hidden(false)
            {
                Add(_T("url"), &URL);
                Add(_T("fps"), &FPS);
                Add(_T("suspended"), &Suspended);
                Add(_T("hidden"), &Hidden);
            }

            Data(const string& url)
                : Core::JSON::Container()
                , URL()
                , FPS()
                , Suspended(false)
                , Hidden(false)
            {
                Add(_T("url"), &URL);
                Add(_T("fps"), &FPS);
                Add(_T("suspended"), &Suspended);
                Add(_T("hidden"), &Hidden);
                URL = url;
            }
            ~Data()
            {
            }

        public:
            Core::JSON::String URL;
            Core::JSON::DecUInt32 FPS;
            Core::JSON::Boolean Suspended;
            Core::JSON::Boolean Hidden;
        };

    public:
		#ifdef __WIN32__ 
		#pragma warning( disable : 4355 )
		#endif
		OutOfProcessPlugin()
            : _adminLock()
            , _skipURL(0)
            , _webPath("")
            , _service(nullptr)
            , _browser(nullptr)
            , _memory(nullptr)
            , _notification(Core::Service<Notification>::Create<Notification>(this))
            , _state(PluginHost::IStateControl::UNINITIALIZED)
			, _subscriber(nullptr)
            , _hidden(false)
        {
        }
		#ifdef __WIN32__ 
		#pragma warning( default : 4355 )
		#endif
		virtual ~OutOfProcessPlugin()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(OutOfProcessPlugin)
			INTERFACE_ENTRY(IPlugin)
			INTERFACE_ENTRY(IPluginExtended)
	        INTERFACE_ENTRY(IWeb)
		    INTERFACE_AGGREGATE(Exchange::IBrowser, _browser)
			INTERFACE_AGGREGATE(PluginHost::IStateControl, _browser)
			INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

		// ================================== CALLED ON COMMUNICATION THREAD =====================================
		// Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
		// Whenever the channel is closed, it is reported via the detach method.
		virtual bool Attach(PluginHost::Channel& channel);
		virtual void Detach(PluginHost::Channel& channel);

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        void LoadFinished(const string& URL);
        void URLChanged(const string& URL);
        void Hidden(const bool hidden);
        void StateChange(const PluginHost::IStateControl::state value);
        void Deactivated(RPC::IRemoteProcess* process);

    private:
        Core::CriticalSection _adminLock;
        uint8_t _skipURL;
		uint32_t _pid;
        string _webPath;
        PluginHost::IShell* _service;
        Exchange::IBrowser* _browser;
        Exchange::IMemory* _memory;
        Notification* _notification;
        PluginHost::IStateControl::state _state;
		PluginHost::Channel* _subscriber;
        bool _hidden;
    };
}
}

#endif // __OUTOFPROCESSTEST_H
