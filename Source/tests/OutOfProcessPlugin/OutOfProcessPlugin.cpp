#include "OutOfProcessPlugin.h"

namespace WPEFramework {

namespace OutOfProcessPlugin {

    // An implementation file needs to implement this method to return an operational browser, wherever that would be :-)
    extern Exchange::IMemory* MemoryObserver(const uint32_t pid);
}

namespace Plugin {

    SERVICE_REGISTRATION(OutOfProcessPlugin, 1, 0);

	static Core::ProxyPoolType< OutOfProcessPlugin::Data > jsonDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<OutOfProcessPlugin::Data> > jsonBodyDataFactory(4);
    static Core::ProxyPoolType<Web::TextBody> textFactory(4);

    static const char SampleData[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789=!";

    /* encapsulated class Thread  */
    /* virtual */ const string OutOfProcessPlugin::Initialize(PluginHost::IShell* service)
    {
        string message;
        Config config;

        ASSERT(_browser == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_service == nullptr);

        _pid = 0;
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        _service->EnableWebServer(_T("UI"), EMPTY_STRING);
        _service->Register(static_cast<RPC::IRemoteProcess::INotification*>(_notification));

        config.FromString(_service->ConfigLine());

        if (config.OutOfProcess.Value() == true) {
            _browser = _service->Instantiate<Exchange::IBrowser>(Core::infinite, _T("OutOfProcessImplementation"), static_cast<uint32_t>(~0), _pid, service->Locator());
        }
        else {
            _browser = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IBrowser>(Core::Library(), _T("OutOfProcessImplementation"), static_cast<uint32_t>(~0));
        }

        if (_browser == nullptr) {
            _service->Unregister(static_cast<RPC::IRemoteProcess::INotification*>(_notification));
            _service = nullptr;
            message = _T("OutOfProcessPlugin could not be instantiated.");
        }
        else {
            _browser->Register(_notification);

            PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            ASSERT(stateControl != nullptr);

            stateControl->Configure(_service);
            stateControl->Register(_notification);
            stateControl->Release();

            _memory = WPEFramework::OutOfProcessPlugin::MemoryObserver(_pid);
            ASSERT(_memory != nullptr);
        }

        return message;
    }

    /* virtual */ void OutOfProcessPlugin::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);
        ASSERT(_browser != nullptr);

        _service->DisableWebServer();
        _service->Unregister(static_cast<RPC::IRemoteProcess::INotification*>(_notification));
        _browser->Unregister(_notification);
        _memory->Release();

        PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

        if (stateControl != nullptr) {

            // No longer a need for the notfications..
            stateControl->Unregister(_notification);
            stateControl->Release();
        }

        // Stop processing of the browser:
        if (_browser->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT (_pid != 0);
            TRACE_L1("OutOfProcess Plugin is not properly destructed. PID: %d", _pid);

            RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

            // The process can disappear in the meantime...
            if (process != nullptr) {
                process->Terminate();
                process->Release();
            }
        }

        _memory = nullptr;
        _browser = nullptr;
        _service = nullptr;
    }

    /* virtual */ string OutOfProcessPlugin::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }
	/* virtual */ bool OutOfProcessPlugin::Attach(PluginHost::Channel& channel) {
		if (_subscriber == nullptr) {
			_subscriber = &channel;
			return (true);
		}
		return (false);
	}

	/* virtual */ void OutOfProcessPlugin::Detach(PluginHost::Channel& channel) {
		if ((_subscriber != nullptr) && (_subscriber == &channel)) {
			_subscriber = nullptr;
		}
	}

    /* virtual */ void OutOfProcessPlugin::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_PUT) {
            request.Body(jsonBodyDataFactory.Element());
        }
        else if (request.Verb == Web::Request::HTTP_POST) {
            request.Body(textFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> OutOfProcessPlugin::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received request"))));

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length() - _skipURL)), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Unknown error";

        if (_browser != nullptr) {

			PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            ASSERT(stateControl != nullptr);

            if (request.Verb == Web::Request::HTTP_GET) {
				PluginHost::IStateControl::state currentState = stateControl->State();
                Core::ProxyType<Web::JSONBodyType<OutOfProcessPlugin::Data> > body(jsonBodyDataFactory.Element());
                body->URL = _browser->GetURL();
                body->FPS = _browser->GetFPS();
                body->Suspended = (currentState == PluginHost::IStateControl::SUSPENDED);
                body->Hidden = _hidden;
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
                result->Body<Web::JSONBodyType<OutOfProcessPlugin::Data> >(body);
            }
            else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";

                // We might be receiving a plugin download request.
                if (index.Remainder() == _T("Suspend")) {
                    stateControl->Request(PluginHost::IStateControl::SUSPEND);
                } else if (index.Remainder() == _T("Resume")) {
                    stateControl->Request(PluginHost::IStateControl::RESUME);
                } else if (index.Remainder() == _T("Hide")) {
                    _browser->Hide(true);
                } else if (index.Remainder() == _T("Show")) {
                    _browser->Hide(false);
				} else if (index.Remainder() == _T("Notify4K")) {
					string message;
					for (uint32_t teller = 0; teller < ((4 * 1024) + 64); teller++) {
						message += SampleData[teller % 64];
					}
					_service->Notify(message);
				}
				else if ( (index.Remainder() == _T("JSONPop")) && (_subscriber != nullptr) ) {
					Core::ProxyType<Data> info(jsonDataFactory.Element());

					info->URL = _T("ws://<My IP>/JSONPop");
					_subscriber->Submit(Core::proxy_cast<Core::JSON::IElement>(info));
				} else if ( (index.Remainder() == _T("URL")) && (request.HasBody() == true) && (request.Body<Web::TextBody>()->empty() == false) ) {
                    _browser->SetURL(*(request.Body<Web::TextBody>()));
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Unknown error";
                }
            }
            stateControl->Release();
        }

        return result;
    }



    void OutOfProcessPlugin::LoadFinished(const string& URL)
    {
        Data info(URL);
        string message;
        info.ToString(message);

        _service->Notify(message);
    }

    void OutOfProcessPlugin::URLChanged(const string& URL)
    {

        Data info(URL);
        string message;
        info.ToString(message);
        TRACE_L1("Received a new URL: %s", message.c_str());
        TRACE_L1("URL length: %u", static_cast<uint32_t>(message.length()));

        _service->Notify(message);
    }

    void OutOfProcessPlugin::Hidden(const bool hidden)
    {
        string message = "{ \"hidden\": \"" + string(hidden ? _T("true") : _T("false")) + "\" }";

        _service->Notify(message);
    }

    void OutOfProcessPlugin::StateChange(const PluginHost::IStateControl::state value)
    {
        string message = "{ \"state\": \"test\" }";

        _service->Notify(message);
    }

    void OutOfProcessPlugin::Deactivated(RPC::IRemoteProcess* process)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_pid == process->Id()) {

			ASSERT(_service != nullptr);

			PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
}
} // namespace Plugin
