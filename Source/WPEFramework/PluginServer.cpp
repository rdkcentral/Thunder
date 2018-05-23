#include "PluginServer.h"
#include "Controller.h"

#ifndef __WIN32__
#include <syslog.h>
#endif

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Core::ProcessInfo::scheduler)

{ Core::ProcessInfo::BATCH,      _TXT("Batch")},
{ Core::ProcessInfo::IDLE,       _TXT("Idle") },
{ Core::ProcessInfo::FIFO,       _TXT("FIFO") },
{ Core::ProcessInfo::ROUNDROBIN, _TXT("RoundRobin") },
{ Core::ProcessInfo::OTHER,      _TXT("Other") },

ENUM_CONVERSION_END(Core::ProcessInfo::scheduler)


ENUM_CONVERSION_BEGIN(PluginHost::InputHandler::type)

{ PluginHost::InputHandler::DEVICE,  _TXT("device")},
{ PluginHost::InputHandler::VIRTUAL, _TXT("virtual") },

ENUM_CONVERSION_END(PluginHost::InputHandler::type)

namespace PluginHost {
/* static */ Core::ProxyType<Web::Response> Server::Channel::_missingCallsign(Core::ProxyType<Web::Response>::Create());
/* static */ Core::ProxyType<Web::Response> Server::Channel::WebRequestJob::_missingResponse(Core::ProxyType<Web::Response>::Create());
/* static */ Core::ProxyType<Web::Response> Server::Service::_missingHandler(Core::ProxyType<Web::Response>::Create());
/* static */ Core::ProxyType<Web::Response> Server::Service::_unavailableHandler(Core::ProxyType<Web::Response>::Create());

/* static */ Core::ProxyPoolType<Server::Channel::WebRequestJob> Server::Channel::_webJobs(2);
/* static */ Core::ProxyPoolType<Server::Channel::JSONElementJob> Server::Channel::_jsonJobs(2);
/* static */ Core::ProxyPoolType<Server::Channel::TextJob> Server::Channel::_textJobs(2);

#ifdef __WIN32__
/* static */ const TCHAR* Server::ConfigFile = _T("C:\\Projects\\PluginHost.json");
#else
/* static */ const TCHAR* Server::ConfigFile = _T("/etc/" EXPAND_AND_QUOTE(INSTALL_COMMON_PREFIX) "/config.json");
#endif

/* static */ const TCHAR* Server::PluginOverrideFile = _T("PluginHost/override.json");
/* static */ const TCHAR* Server::PluginConfigDirectory = _T("plugins");
/* static */ const TCHAR* Server::CommunicatorConnector = _T("COMMUNICATOR_CONNECTOR");


static const TCHAR _defaultControllerCallsign[] = _T("Controller");

static Core::NodeId DetermineAccessor(const Server::Config& configuration, Core::NodeId& accessor) {
    Core::NodeId result(configuration.Binding.Value().c_str());

    if (configuration.Interface.Value().empty() == false) {
        Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(configuration.Interface.Value());

        if (selectedNode.IsValid() == true) {
            accessor = selectedNode;
            result = accessor;
        }
    } else if (result.IsAnyInterface() == true) {
        Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(configuration.Interface.Value());

        if (selectedNode.IsValid() == true) {
            accessor = selectedNode;
        }
    } else {
        accessor = result;
    }

    if (accessor.IsValid() == false) {

        // Let's go fr the default and make the best of it :-)
        struct sockaddr_in value;

        value.sin_addr.s_addr = 0;
        value.sin_family = AF_INET;
        value.sin_port = htons(configuration.Port.Value());

        result = value;
        
        TRACE_L1("Invalid config information could not resolve to a proper IP set to: (%s:%d)", result.HostAddress().c_str(), result.PortNumber());
    } else {
        result.PortNumber(configuration.Port.Value());

        string URL(_T("http://"));

        URL += accessor.HostAddress();
        URL += ':' + Core::NumberType<uint32_t>(configuration.Port.Value()).Text();

        accessor.PortNumber(configuration.Port.Value());

        SYSLOG(Startup, (_T("Accessor: %s"), URL.c_str()));
        SYSLOG(Startup, (_T("Interface IP: %s"), result.HostAddress().c_str()));
    }

    return (result);
}

void Server::ChannelMap::GetMetaData(Core::JSON::ArrayType<MetaData::Channel>& metaData) const {

    Core::SocketServerType<Channel>::Iterator index(Core::SocketServerType<Channel>::Clients());

    while (index.Next() == true) {

        MetaData::Channel newInfo;
        // Let the Channel,report it's metadata

        Core::ProxyType<Channel> client(index.Client());

        newInfo.ID = client->Id();

        newInfo.Activity = client->HasActivity();
        newInfo.Remote = client->RemoteId();
        newInfo.JSONState = (client->IsWebSocket() ? ((client->State() != PluginHost::Channel::RAW) ? MetaData::Channel::RAWSOCKET : MetaData::Channel::WEBSOCKET) : (client->IsWebServer() ? MetaData::Channel::WEBSERVER : MetaData::Channel::SUSPENDED));
        string name = client->Name();

        if (name.empty() == false) {
            newInfo.Name = name;
        }

        metaData.Add(newInfo);
    }
}

void Server::ServiceMap::Destroy() {
    _adminLock.Lock();

    std::map<const string, Core::ProxyType<Service> >::iterator index(_services.end());

    TRACE_L1("Deactivating %d plugins.", static_cast<uint32_t>(_services.size()));

    // First, move them all to deactivated
    do {
        index--;

        ASSERT(index->second.IsValid());

        index->second->Deactivate(PluginHost::IShell::SHUTDOWN);

        Core::ServiceAdministrator::Instance().FlushLibraries();

    } while (index != _services.begin());

    TRACE_L1("Destructing %d plugins.", static_cast<uint32_t>(_services.size()));
    // Now release them all, once they are deactivated..
    index = _services.begin();

    while (index != _services.end()) {
        Core::ProxyType<Service> service(index->second);

        ASSERT(service.IsValid());

        index = _services.erase(index);

        service.Release();
    }

    _adminLock.Unlock();

    ASSERT(_notifiers.size() == 0);

    std::list<uint32_t> pidList;

    // See if there are still pending processes, we need to shoot and kill..
    Processes(pidList);

    if (pidList.size() > 0) {
        uint16_t waitSlots = 50; /* each slot is 100ms, so we wait for 5 Seconds for all processes to terminte !!! */

        std::list<uint32_t>::iterator index(pidList.begin());

        while (index != pidList.end()) {
            _processAdministrator.Destroy(*index);

            SYSLOG(Shutdown, (_T("Process [%d] requested to be shutdown."), *index));

            index++;
        }

        // Now it is time to wait, for a certain amount of time to see
        // if all processes are killed.
        while ((waitSlots-- != 0) && (pidList.size() > 0)) {
            SleepMs(100);

            // Now chek the process in the list if they are still alive..
            std::list<uint32_t>::iterator check(pidList.begin());

            while (check != pidList.end()) {
                Core::ProcessInfo process(*check);

                if (process.IsActive() == false) {
                    check = pidList.erase(check);
                } else {
                    check++;
                }
            }
        }

        if (pidList.size() > 0) {
            index = pidList.begin();

            while (index != pidList.end()) {
                SYSLOG(Shutdown, (_T("Process [%d] could not be destroyed in time."), *index));
                index++;
            }
        }
    }
}

/* virtual */ void* Server::Service::QueryInterface(const uint32_t id) {

    void* result = nullptr;
    if ((id == Core::IUnknown::ID) || (id == PluginHost::IShell::ID)) {
        AddRef();
        result = this;
    } else {

        Lock();

        if (_handler != nullptr) {

            result = _handler->QueryInterface(id);
        }

        Unlock();
    }

    return (result);
}

/* virtual */ void* Server::Service::QueryInterfaceByCallsign(const uint32_t id, const string& name) {
    return (_administrator.QueryInterfaceByCallsign(id, name));
}

/* virtual */ void Server::Service::Register(IPlugin::INotification* sink) {
    _administrator.Register(sink);
}

/* virtual */ void Server::Service::Unregister(IPlugin::INotification* sink) {
    _administrator.Unregister(sink);
}

// Use the base framework (webbridge) to start/stop processes and the service in side of the given binary.
/* virtual */ void* Server::Service::Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version, uint32_t& pid, const string& locator) {
    return (_administrator.Instantiate(waitTime, className, interfaceId, version, pid, locator, ClassName(), Callsign()));
}

/* virtual */ void Server::Service::Register(RPC::IRemoteProcess::INotification* sink) {
    _administrator.Register(sink);
}

/* virtual */ void Server::Service::Unregister(RPC::IRemoteProcess::INotification* sink) {
    _administrator.Unregister(sink);
}

/* virtual */ RPC::IRemoteProcess* Server::Service::RemoteProcess(const uint32_t pid) {
    return (_administrator.RemoteProcess(pid));
}

// Methods to stop/start/update the service.
uint32_t Server::Service::Activate(const PluginHost::IShell::reason why) {

    uint32_t result = Core::ERROR_NONE;

    Lock();

    IShell::state currentState(State());

    if (currentState == IShell::ACTIVATION) {
        result = Core::ERROR_INPROGRESS;
    } else if ((currentState == IShell::DEACTIVATION) || (currentState == IShell::DESTROYED)) {
        result = Core::ERROR_ILLEGAL_STATE;
    } else if ( (currentState == IShell::DEACTIVATED) || (currentState == IShell::PRECONDITION) ) {

        // Load the interfaces, If we did not load them yet...
        if (_handler == nullptr) {
            AquireInterfaces();
        }

        const string callSign(PluginHost::Service::Configuration().Callsign.Value());
        const string className(PluginHost::Service::Configuration().ClassName.Value());

        if (_handler == nullptr) {
            SYSLOG(Startup, (_T("Activation of plugin [%s]:[%s], failed. Error [Could not be instantiated]"), className.c_str(), callSign.c_str()));
            result = Core::ERROR_UNAVAILABLE;

        // See if the preconditions have been met..
        } else if (_precondition.IsMet() == false) {
            SYSLOG(Startup, (_T("Activation of plugin [%s]:[%s], postponed, preconditions have not been met, yet."), className.c_str(), callSign.c_str()));
            result = Core::ERROR_PENDING_CONDITIONS;
            _reason = why;
            State(PRECONDITION);

            if (Trace::TraceType<Activity, &Core::System::MODULE_NAME>::IsEnabled() == true) {
                string feedback;
                uint8_t index = 1;
                uint32_t delta (_precondition.Delta(_administrator.SubSystemInfo()));

                while (delta != 0) {
                    if ((delta & 0x01) != 0) {
                       if (feedback.empty() == false) {
                           feedback += ',';
                       } 

                       PluginHost::ISubSystem::subsystem element (static_cast<PluginHost::ISubSystem::subsystem>(index));
                       feedback += string(Core::EnumerateType<PluginHost::ISubSystem::subsystem>(element).Data());
                    }

                    delta = (delta >> 1);
                    index++;
                }

                Activity information (_T("Delta preconditions: %s"), feedback.c_str());
                Trace::TraceType<Activity, &Core::System::MODULE_NAME> traceData (information);
		Trace::TraceUnit::Instance().Trace(__FILE__,__LINE__, className.c_str(), &traceData);
            }
        }
        else {

            State(ACTIVATION);
            _administrator.StateChange(this);

            Unlock();

            TRACE(Activity, (_T("Activation plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));

            // Fire up the interface. Let it handle the messages.
            ErrorMessage(_handler->Initialize(this));


            if (HasError() == true) {
                result = Core::ERROR_GENERAL;

                SYSLOG(Startup, (_T("Activation of plugin [%s]:[%s], failed. Error [%s]"), className.c_str(), callSign.c_str(), ErrorMessage().c_str()));

                Lock();
                ReleaseInterfaces();
                State(DEACTIVATED);
                _administrator.StateChange(this);
            } else {
		const string webUI (PluginHost::Service::Configuration().WebUI.Value());
                if ((PluginHost::Service::Configuration().WebUI.IsSet()) || (webUI.empty() == false) ) {
                    EnableWebServer(webUI, EMPTY_STRING);
                }

                SYSLOG(Startup, (_T("Activated plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));
                Lock();
                State(ACTIVATED);
                _administrator.StateChange(this);
                _administrator.Notification(_T("{\"callsign\":\"") + callSign +
                                            _T("\",\"state\":\"activated\",\"reason\":\"") +
                                            IShell::ToString(why) + _T("\"}"));
            }
        }
    }

    Unlock();

    return (result);
}

uint32_t Server::Service::Deactivate(const reason why) {

    uint32_t result = Core::ERROR_NONE;

    Lock();

    IShell::state currentState(State());

    if (currentState == IShell::DEACTIVATION) {
        result = Core::ERROR_INPROGRESS;
    } else if ((currentState == IShell::ACTIVATION) || (currentState == IShell::DESTROYED)) {
        result = Core::ERROR_ILLEGAL_STATE;
    } else if ((currentState == IShell::ACTIVATED) || (currentState == IShell::PRECONDITION)) {

        ASSERT(_handler != nullptr);

        const string className(PluginHost::Service::Configuration().ClassName.Value());
        const string callSign(PluginHost::Service::Configuration().Callsign.Value());

        _reason = why;

        if (currentState == IShell::ACTIVATED) {
            State(DEACTIVATION);
            _administrator.StateChange(this);

            Unlock();

            // If we enabled the webserver, we should also disable it.
            if ( (PluginHost::Service::Configuration().WebUI.IsSet()) || (PluginHost::Service::Configuration().WebUI.Value().empty() == false) ) {
                DisableWebServer();
            }

            TRACE(Activity, (_T("Deactivation plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));

            _handler->Deinitialize(this);

            Lock();
        }

        SYSLOG(Shutdown, (_T("Deactivated plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));

        TRACE(Activity, (Trace::Format(_T("Deactivate plugin [%s]:[%s]"), className.c_str(), callSign.c_str())));
 
        State(DEACTIVATED);

        _administrator.StateChange(this);
        _administrator.Notification(_T("{\"callsign\":\"") + callSign +
                                    _T("\",\"state\":\"deactivated\",\"reason\":\"") +
                                    IShell::ToString(why) + _T("\"}"));

        if (State() != ACTIVATED) {
            // We have no need for his module anymore..
            ReleaseInterfaces();
        }
    }

    Unlock();

    return (result);
}

/* virtual */ ISubSystem* Server::Service::SubSystems() {
    return (_administrator.SubSystemsInterface());
}

/* virtual */ void Server::Service::Notify(const string& message) {
    const string fullMessage("{\"callsign\":\"" + PluginHost::Service::Callsign() + "\", \"data\": " + message + " }");

    // Notify the base class and the subscribers
    PluginHost::Service::Notification(message);

    _administrator.Notification(fullMessage);
}

Core::ProxyType<PluginHost::Server::Service> Server::ServiceMap::FromLocator(const string& identifier, bool& correctHeader) {
	Core::ProxyType<PluginHost::Server::Service> result;
	const string& locator(_webbridgeConfig.WebPrefix());

	// Check the header (prefix part)
	correctHeader = (identifier.compare(0, locator.length(), locator.c_str()) == 0);

	// Yippie the path prefix keyword is found correctly.
	if ((correctHeader == true) && (identifier.length() > (locator.length() + 1))) {
		size_t length;
		uint32_t offset = locator.length() + 1; /* skip the slash after */

		const string callSign(identifier.substr(offset, ((length = identifier.find_first_of('/', offset)) == string::npos ? string::npos : length - offset)));

		_adminLock.Lock();

		std::map<const string, Core::ProxyType<Service> >::iterator index(_services.find(callSign));

		if (index != _services.end()) {
			result = index->second;
		}

		_adminLock.Unlock();
	}

	return (result);
}

Server::Channel::Channel(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Channel>* parent)
    : PluginHost::Channel(connector, remoteId)
    , _parent(static_cast<ChannelMap&>(*parent).Parent()) {
    TRACE(Activity, (_T("Construct a link with ID: [%d] to [%s]"), Id(), remoteId.QualifiedName().c_str()));
}

/* virtual */ Server::Channel::~Channel() {
    TRACE(Activity, (_T("Destruct a link with ID [%d] to [%s]"), Id(), RemoteId().c_str()));

    // If we are still atatched to a service, detach, we are out of scope...
    if (_service.IsValid() == true) {
        _service->Unsubscribe(*this);

        _service.Release();
    }

    Close(0);
}

static string DetermineProperModel(Core::JSON::String& input)
{
    string result;

    if (input.IsSet()) {
        result = input.Value();
    }
    else if (Core::SystemInfo::GetEnvironment(_T("MODEL_NAME"), result) == false) {
        result = "UNKNOWN";
    }
    return (result);
}

#ifdef __WIN32__
#pragma warning( disable : 4355 )
#endif

Server::Server(Server::Config& configuration, ISecurity* securityHandler, const bool background)
    : _accessor()
    , _dispatcher(PluginHost::WorkerPool::Instance(configuration.Process.IsSet() ? configuration.Process.StackSize.Value() : 0))
    , _connections(*this, DetermineAccessor(configuration, _accessor), configuration.IdleTime)
    , _config(configuration.Version.Value(),
              DetermineProperModel(configuration.Model),
              background,
              configuration.Prefix.Value(),
			  configuration.VolatilePath.Value(),
              configuration.PersistentPath.Value(),
              configuration.DataPath.Value(),
              configuration.SystemPath.Value(),
              configuration.ProxyStubPath.Value(),
              configuration.Signature.Value(),
              _accessor,
              Core::NodeId(configuration.Communicator.Value().c_str()),
              configuration.Redirect.Value(),
              securityHandler)
    , _services(*this, _config, configuration.Process.IsSet() ? configuration.Process.StackSize.Value() : 0)
    , _controller() {
    if (configuration.Process.IsSet () == true) {

        Core::ProcessInfo myself;

        if (configuration.Process.OOMAdjust.IsSet() == true) {
            myself.Priority(configuration.Process.OOMAdjust.Value());
        }

        if (configuration.Process.Priority.IsSet()) {
            myself.Priority(configuration.Process.Priority.Value());
        }

        if (configuration.Process.Policy.IsSet()) {
            myself.Policy(configuration.Process.Policy.Value());
        }
    }

	// See if the persitent path for our-selves exist, if not we will create it :-)
	Core::File persistentPath(_config.PersistentPath() + _T("PluginHost"));

	if (persistentPath.IsDirectory() == false) {
		Core::Directory(persistentPath.Name().c_str()).Create();
	}

	Core::JSON::ArrayType<Plugin::Config>::Iterator index = configuration.Plugins.Elements();

	// First register all services, than if we got them, start "activating what is required.
    // Whatever plugin is needed, we at least have our MetaData plugin available (as the first entry :-).
    Plugin::Config metaDataConfig;

    metaDataConfig.ClassName = Core::ClassNameOnly(typeid(Plugin::Controller).name()).Text();

    while (index.Next() == true) {
        Plugin::Config& entry(index.Current());

        if ((entry.ClassName.Value().empty() == true) && (entry.Locator.Value().empty() == true)) {

            // This is a definition/configuration for the Controller or an incorrect entry :-).
            // Read and define the Controller.
            if (metaDataConfig.Callsign.Value().empty() == true) {
                // Oke, this is the first time we "initialize" it.
                metaDataConfig.Callsign = (entry.Callsign.Value().empty() == true ? string(_defaultControllerCallsign) : entry.Callsign.Value());
                metaDataConfig.Configuration = entry.Configuration;
            } else {
                // Let's raise an error, this is a bit strange, again, the controller is initialized !!!
#ifndef __WIN32__
                if (background == true) {
                    syslog(LOG_NOTICE, "Configuration error. Controller is defined mutiple times [%s].\n", entry.Callsign.Value().c_str());
                } else
#endif
                {
                    fprintf(stdout, "Configuration error. Controller is defined mutiple times [%s].\n", entry.Callsign.Value().c_str());
                }
            }
        } else {
            _services.Insert(entry);
        }
    }

    if (metaDataConfig.Callsign.Value().empty() == true) {
        // Oke, this is the first time we "initialize" it.
        metaDataConfig.Callsign = string(_defaultControllerCallsign);
    }

	// Get the configuration from the persistent location.
	_services.Load();

    // Create input handle
    _inputHandler.Initialize(configuration.Input.Type.Value(), configuration.Input.Locator.Value());

    // Initialize static message.
    Service::Initialize();
    Channel::Initialize(_config.WebPrefix());
 
    // Add the controller as a service to the services.
    _controller = _services.Insert(metaDataConfig);
    _controller->Activate(PluginHost::IShell::STARTUP);
    _controller->ClassType<Plugin::Controller>()->SetServer(this);
    _controller->ClassType<Plugin::Controller>()->AddRef();
    _controllerName = _controller->Callsign();

    // Right we have the shells for all possible services registered, time to activate what is needed :-)
    ServiceMap::Iterator iterator(_services.Services());

    while (iterator.Next() == true) {

        Core::ProxyType<Service> service(*iterator);

        if (service->AutoStart() == true) {

            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(&(*service), PluginHost::IShell::ACTIVATED, PluginHost::IShell::STARTUP));
        } else {
            SYSLOG(Startup, (_T("Activation of plugin [%s]:[%s] blocked"), service->ClassName().c_str(), service->Callsign().c_str()));
        }
    }
}

#ifdef __WIN32__
#pragma warning( default : 4355 )
#endif

Server::~Server() {
    Plugin::Controller* destructor(_controller->ClassType<Plugin::Controller>());
    _dispatcher.Block();
    _connections.Close(Core::infinite);
    destructor->Stopped();
    _services.Destroy();
    destructor->Release();
    _inputHandler.Deinitialize();
    _dispatcher.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);
}

}
}
