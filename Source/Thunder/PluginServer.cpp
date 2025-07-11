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

#include "PluginServer.h"
#include "Controller.h"

#ifndef __WINDOWS__
#include <syslog.h>
#endif

#ifdef PROCESSCONTAINERS_ENABLED
#include "../processcontainers/processcontainers.h"
#endif

#ifdef HIBERNATE_SUPPORT_ENABLED
#include "../extensions/hibernate/hibernate.h"
#endif

namespace Thunder {

ENUM_CONVERSION_BEGIN(Core::ProcessInfo::scheduler)

    { Core::ProcessInfo::BATCH, _TXT("Batch") },
    { Core::ProcessInfo::IDLE, _TXT("Idle") },
    { Core::ProcessInfo::FIFO, _TXT("FIFO") },
    { Core::ProcessInfo::ROUNDROBIN, _TXT("RoundRobin") },
    { Core::ProcessInfo::OTHER, _TXT("Other") },

ENUM_CONVERSION_END(Core::ProcessInfo::scheduler)

ENUM_CONVERSION_BEGIN(PluginHost::InputHandler::type)

    { PluginHost::InputHandler::DEVICE, _TXT("device") },
    { PluginHost::InputHandler::VIRTUAL, _TXT("virtual") },

ENUM_CONVERSION_END(PluginHost::InputHandler::type)

namespace PluginHost {
    //
    // STATIC declarations
    // -----------------------------------------------------------------------------------------------------------------------------------

    /* static */ Core::ProxyType<Web::Response> Server::Channel::_missingCallsign(Core::ProxyType<Web::Response>::Create());
    /* static */ Core::ProxyType<Web::Response> Server::Channel::_incorrectVersion(Core::ProxyType<Web::Response>::Create());
    /* static */ Core::ProxyType<Web::Response> Server::Channel::WebRequestJob::_missingResponse(Core::ProxyType<Web::Response>::Create());
    /* static */ Core::ProxyType<Web::Response> Server::Channel::_unauthorizedRequest(Core::ProxyType<Web::Response>::Create());
    /* static */ Core::ProxyType<Web::Response> Server::Service::_missingHandler(Core::ProxyType<Web::Response>::Create());
    /* static */ Core::ProxyType<Web::Response> Server::Service::_unavailableHandler(Core::ProxyType<Web::Response>::Create());

    /* static */ Core::ProxyPoolType<Server::Channel::WebRequestJob> Server::Channel::_webJobs(2);
    /* static */ Core::ProxyPoolType<Server::Channel::JSONElementJob> Server::Channel::_jsonJobs(2);
    /* static */ Core::ProxyPoolType<Server::Channel::TextJob> Server::Channel::_textJobs(2);

#ifdef __WINDOWS__
    /* static */ const TCHAR* Server::ConfigFile = _T("C:\\Projects\\PluginHost.json");
#else
    /* static */ const TCHAR* Server::ConfigFile = _T("/etc/" EXPAND_AND_QUOTE(NAMESPACE) "/config.json");
#endif

    /* static */ const TCHAR* Server::PluginOverrideFile = _T("PluginHost/override.json");
    /* static */ const TCHAR* Server::PluginConfigDirectory = _T("plugins/");
    /* static */ const TCHAR* Server::CommunicatorConnector = _T("COMMUNICATOR_CONNECTOR");

    static const TCHAR _defaultControllerCallsign[] = _T("Controller");

    class DefaultSecurity : public ISecurity {
    public:
        DefaultSecurity() = delete;
        DefaultSecurity(const DefaultSecurity&) = delete;
        DefaultSecurity& operator=(const DefaultSecurity&) = delete;

        DefaultSecurity(const string& prefix, const string jsonrpcPath, const string& controllerName)
            : _hasSecurity(true)
            , _controllerPath(prefix + '/' + controllerName)
            , _jsonrpcPath(jsonrpcPath)
            , _controllerName(controllerName)
        {
        }
        ~DefaultSecurity()
        {
        }

    public:
        inline void Security(const bool enabled)
        {
            _hasSecurity = enabled;
        }
        // Allow a request to be checked before it is offered for processing.
        virtual bool Allowed(const string& path) const override
        {
            return ((_hasSecurity == false) || (path.substr(0, _controllerPath.length()) == _controllerPath));
        }

        // Allow a request to be checked before it is offered for processing.
        virtual bool Allowed(const Web::Request& request) const override
        {
            bool result = (_hasSecurity == false);

            if (result == false) {
                // If there is security, maybe this is a valid reuest, althoug this
                // validation is not done by an instance issued by the SecurityOfficer.
                result = ((request.Verb == Web::Request::HTTP_GET) && (request.Path.substr(0, _controllerPath.length()) == _controllerPath));

                if ((result == false) && (request.Verb == Web::Request::HTTP_POST) && (request.HasBody() == true) && (request.Path == _jsonrpcPath)) {

                    // Now dig into the message, if the method is for the controller and the method is to get info
                    // it is good to go... Other wise NO
                    Core::ProxyType<const Core::JSONRPC::Message> body = request.Body<const Core::JSONRPC::Message>();

                    if (body.IsValid() == true) {
                        result = CheckMessage(*body);
                    }
                }
            }
            return (result);
        }
        //! Allow a JSONRPC message to be checked before it is offered for processing.
        bool Allowed(const Core::JSONRPC::Message& message) const override
        {
            return ((_hasSecurity == false) || CheckMessage(message));
        }
        string Token(void) const override
        {
            return (EMPTY_STRING);
        }

        //  IUnknown methods
        // -------------------------------------------------------------------------------------------------------
        BEGIN_INTERFACE_MAP(DefaultSecurity)
        INTERFACE_ENTRY(ISecurity)
        END_INTERFACE_MAP

    private:
        bool CheckMessage(const Core::JSONRPC::Message& message) const
        {
            bool result = false;

            if (message.Callsign() == _controllerName) {
                result = (message.Method() == _T("exists"));
            }

            return (result);
        }

    private:
        bool _hasSecurity;
        const string _controllerPath;
        const string _jsonrpcPath;
        const string _controllerName;
    };

    //
    // class Server::WorkerPoolImplementation
    // -----------------------------------------------------------------------------------------------------------------------------------
    void Server::WorkerPoolImplementation::Dispatcher::Dispatch(Core::IDispatch* job) /* override */ {
    #if defined(__CORE_EXCEPTION_CATCHING__) || defined(__CORE_WARNING_REPORTING__)
        string callsign(_T("Callsign Unknown"));
        Channel::Job* rootObject = dynamic_cast<Channel::Job*>(job);
        if (rootObject != nullptr) {
            callsign = rootObject->Callsign();
        }

        WARNING_REPORTING_THREAD_SETCALLSIGN_GUARD(callsign.c_str());
    #endif

    #ifdef __CORE_EXCEPTION_CATCHING__

        try {
            job->Dispatch();
        }
        catch (const std::exception& type) {
            Logging::DumpException(type.what());
        }
        catch (...) {
            Logging::DumpException(_T("Unknown"));
        }
    #else
        job->Dispatch();
    #endif
    }

    void Server::ChannelMap::GetMetadata(Core::JSON::ArrayType<Metadata::Channel> & metaData) const
    {
        Core::SocketServerType<Channel>::Iterator index(Core::SocketServerType<Channel>::Clients());

        while (index.Next() == true) {

            Metadata::Channel newInfo;
            // Let the Channel,report it's metadata

            Core::ProxyType<Channel> client(index.Client());

            newInfo.ID = client->Id();

            newInfo.Activity = client->HasActivity();
            newInfo.Remote = client->RemoteId();
            if (client->IsOpen() == false) {
                newInfo.State = Metadata::Channel::state::CLOSED;
            }
            else {
                newInfo.State = (client->IsWebSocket() ? ((client->State() == PluginHost::Channel::RAW) ? Metadata::Channel::state::RAWSOCKET : Metadata::Channel::state::WEBSOCKET) : (client->IsWebServer() ? Metadata::Channel::state::WEBSERVER : Metadata::Channel::state::SUSPENDED));
            }
            string name = client->Path();

            if (name.empty() == false) {
                newInfo.Name = name;
            }

            metaData.Add(newInfo);
        }
    }

    void Server::ServiceMap::Destroy()
    {
        _adminLock.Lock();

        // First, move them all to deactivated except Controller
        Core::ProxyType<Service> controller (_server.Controller());

        TRACE_L1("Destructing %d plugins", static_cast<uint32_t>(_services.size()));

        while (_services.empty() == false) {

            auto index = _services.begin();

            Core::ProxyType<Service> service(index->second);

            ASSERT(service.IsValid());

            if (index->first.c_str() != controller->Callsign()) {
                _adminLock.Unlock();

                index->second->Deactivate(PluginHost::IShell::SHUTDOWN);

                _adminLock.Lock();
            }

            _services.erase(index);
        }

        _adminLock.Unlock();

        // Now deactivate controller plugin, once other plugins are deactivated
        controller->Deactivate(PluginHost::IShell::SHUTDOWN);

        TRACE_L1("Pending notifiers are %zu", _notifiers.size());
        for (VARIABLE_IS_NOT_USED auto notifier : _notifiers) {
            TRACE_L1("   -->  %s", Core::ClassNameOnly(typeid(*notifier).name()).Text().c_str());
        }

        _processAdministrator.Close(Core::infinite);

        _processAdministrator.Destroy();
    }

    void* Server::Service::QueryInterface(const uint32_t id) /* override */
    {
        void* result = nullptr;
        if (id == Core::IUnknown::ID) {
            AddRef();
            result = static_cast<IUnknown*>(this);
        }
        else if (id == PluginHost::IShell::ID) {
            AddRef();
            result = static_cast<PluginHost::IShell*>(this);
        }
        else if (id == PluginHost::IShell::ICOMLink::ID) {
            AddRef();
            result = static_cast<PluginHost::IShell::ICOMLink*>(this);
        }
        else if (id == PluginHost::IShell::IConnectionServer::ID) {
            AddRef();
            result = static_cast<PluginHost::IShell::IConnectionServer*>(this);
        }
        else if (id == PluginHost::IDispatcher::ID) {
            _pluginHandling.Lock();
            if (_jsonrpc != nullptr) {
                _jsonrpc->AddRef();
                result = _jsonrpc;
            }
            _pluginHandling.Unlock();
        }
        else {
            _pluginHandling.Lock();

            if (_handler != nullptr) {

                result = _handler->QueryInterface(id);
            }

            _pluginHandling.Unlock();
        }

        return (result);
    }

    void* Server::Service::QueryInterfaceByCallsign(const uint32_t id, const string& name) /* override */
    {
        return (_administrator.QueryInterfaceByCallsign(id, name));
    }

    void Server::Service::Register(IPlugin::INotification * sink) /* override */
    {
        _administrator.Register(sink);
    }

    void Server::Service::Unregister(IPlugin::INotification * sink) /* override */
    {
        _administrator.Unregister(sink);
    }

    // Methods to stop/start/update the service.
    Core::hresult Server::Service::Activate(const PluginHost::IShell::reason why) /* override */
    {
        Core::hresult result = Core::ERROR_NONE;

        Lock();

        IShell::state currentState(State());

        if (currentState == IShell::state::ACTIVATION) {
            Unlock();
            result = Core::ERROR_INPROGRESS;
        }
        else if ((currentState == IShell::state::UNAVAILABLE) || (currentState == IShell::state::DEACTIVATION) || (currentState == IShell::state::DESTROYED) ) {
            Unlock();
            result = Core::ERROR_ILLEGAL_STATE;
        } else if (currentState == IShell::state::HIBERNATED) {
            result = Wakeup(3000);
            Unlock();
        } else if ((currentState == IShell::state::DEACTIVATED) || (currentState == IShell::state::PRECONDITION)) {

            // Load the interfaces, If we did not load them yet...
            if (_handler == nullptr) {
                AcquireInterfaces();
            }

            const string callSign(PluginHost::Service::Configuration().Callsign.Value());
            const string className(PluginHost::Service::Configuration().ClassName.Value());

            if (_handler == nullptr) {
                SYSLOG(Logging::Startup, (_T("Loading of plugin [%s]:[%s], failed. Error [%s]"), className.c_str(), callSign.c_str(), ErrorMessage().c_str()));
                result = Core::ERROR_UNAVAILABLE;

                Unlock();

                // See if the preconditions have been met..
            } else if (_precondition.IsMet() == false) {
                SYSLOG(Logging::Startup, (_T("Activation of plugin [%s]:[%s], postponed, preconditions have not been met, yet."), className.c_str(), callSign.c_str()));
                result = Core::ERROR_PENDING_CONDITIONS;
                _reason = why;
                State(PRECONDITION);

                if (Thunder::Messaging::LocalLifetimeType<Activity, &Thunder::Core::System::MODULE_NAME, Thunder::Core::Messaging::Metadata::type::TRACING>::IsEnabled() == true) {
                    string feedback;
                    uint8_t index = 1;
                    uint32_t delta(_precondition.Delta(_administrator.SubSystemInfo().Value()));

                    while (delta != 0) {
                        if ((delta & 0x01) != 0) {
                            if (feedback.empty() == false) {
                                feedback += ',';
                            }

                            PluginHost::ISubSystem::subsystem element(static_cast<PluginHost::ISubSystem::subsystem>(index));
                            feedback += string(Core::EnumerateType<PluginHost::ISubSystem::subsystem>(element).Data());
                        }

                        delta = (delta >> 1);
                        index++;
                    }

                    TRACE(Activity, (_T("Delta preconditions: %s"), feedback.c_str()));
                }

                Unlock();

            } else {

                // Before we dive into the "new" initialize lets see if this has a pending OOP running, if so forcefully kill it now, no time to wait !
                if (_lastId != 0) {
                    _administrator.Destroy(_lastId);
                    _lastId = 0;
                }

                TRACE(Activity, (_T("Activation plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));

                _administrator.Initialize(callSign, this);

                State(ACTIVATION);

                Unlock();

                REPORT_DURATION_WARNING( { ErrorMessage(_handler->Initialize(this)); }, WarningReporting::TooLongPluginState, WarningReporting::TooLongPluginState::StateChange::ACTIVATION, callSign.c_str());

                if (HasError() == true) {
                    result = Core::ERROR_GENERAL;

                    SYSLOG(Logging::Startup, (_T("Activation of plugin [%s]:[%s], failed. Error [%s]"), className.c_str(), callSign.c_str(), ErrorMessage().c_str()));

                    _reason = reason::INITIALIZATION_FAILED;

                    if( _administrator.Configuration().LegacyInitialize() == false ) {
                        REPORT_DURATION_WARNING({ _handler->Deinitialize(this); }, WarningReporting::TooLongPluginState, WarningReporting::TooLongPluginState::StateChange::DEACTIVATION, callSign.c_str());
                    }

                    Lock();
                    ReleaseInterfaces();
                    State(DEACTIVATED);
                    Unlock();

                    _administrator.Deinitialized(callSign, this);

                } else {
                    const Core::EnumerateType<PluginHost::IShell::reason> textReason(why);
                    const string webUI(PluginHost::Service::Configuration().WebUI.Value());
                    if ((PluginHost::Service::Configuration().WebUI.IsSet()) || (webUI.empty() == false)) {
                        EnableWebServer(webUI, EMPTY_STRING);
                    }

                    if (_jsonrpc != nullptr) {
                        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
                        _jsonrpc->Attach(sink, this);
                        if (sink != nullptr) {
                            Register(sink);
                            sink->Release();
                        }
                    }

                    if (_external.Connector().empty() == false) {
                        uint32_t result = _external.Open(0);
                        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS)) {
                            TRACE(Trace::Error, (_T("Could not open the external connector for %s"), Callsign().c_str()));
                        }
                    }

                    SYSLOG(Logging::Startup, (_T("Activated plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));
                    Lock();
                    State(ACTIVATED);
                    _administrator.Activated(callSign, this);

                    _stateControl = _handler->QueryInterface<PluginHost::IStateControl>();
                    if (_stateControl != nullptr) {
                        _stateControl->Register(&_composit);

                        if (Resumed() == true) {
                            _stateControl->Request(PluginHost::IStateControl::RESUME);
                        }
                    }

                    Unlock();

                    #ifdef THUNDER_RESTFULL_API
                    Notify(EMPTY_STRING, string(_T("{\"state\":\"activated\",\"reason\":\"")) + textReason.Data() + _T("\"}"));
                    #endif
                    Notify(_T("statechange"), string(_T("{\"state\":\"activated\",\"reason\":\"")) + textReason.Data() + _T("\"}"));
                }
            }
        } else {
            Unlock();
        }

        return (result);
    }

    uint32_t Server::Service::Resume(const reason why) /* override */ {
        uint32_t result = Core::ERROR_NONE;
        Lock();

        IShell::state currentState(State());

        if (currentState == IShell::state::ACTIVATION) {
            result = Core::ERROR_INPROGRESS;
        } else if ((currentState == IShell::state::DEACTIVATION) || (currentState == IShell::state::DESTROYED) || (currentState == IShell::state::HIBERNATED)) {
            result = Core::ERROR_ILLEGAL_STATE;
        } else if (currentState == IShell::state::DEACTIVATED) {
            result = Activate(why);
            currentState = State();
        }

        if (currentState == IShell::ACTIVATED) {
            // See if we need can and should RESUME.
            if (_stateControl == nullptr) {
                result = Core::ERROR_BAD_REQUEST;
            }
            else {
                // We have a StateControl interface, so at least start resuming, if not already resumed :-)
                if (_stateControl->State() == PluginHost::IStateControl::SUSPENDED) {
                    result = _stateControl->Request(PluginHost::IStateControl::RESUME);
                }
            }
        }

        Unlock();

        return (result);
    }

    Core::hresult Server::Service::Deactivate(const reason why) /* override */
    {
        Core::hresult result = Core::ERROR_NONE;

        Lock();

        IShell::state currentState(State());

        if (currentState == IShell::state::DEACTIVATION) {
            result = Core::ERROR_INPROGRESS;
            Unlock();
        }
        else if ( ((currentState == IShell::state::ACTIVATION) && (why != IShell::reason::INITIALIZATION_FAILED)) || (currentState == IShell::state::DESTROYED)) {
            result = Core::ERROR_ILLEGAL_STATE;
            Unlock();
        }
        else if ( ((currentState == IShell::state::ACTIVATION) && (why == IShell::reason::INITIALIZATION_FAILED)) || (currentState == IShell::state::UNAVAILABLE) || (currentState == IShell::state::ACTIVATED) || (currentState == IShell::state::PRECONDITION) || (currentState == IShell::state::HIBERNATED) ) {
            const Core::EnumerateType<PluginHost::IShell::reason> textReason(why);

            const string className(PluginHost::Service::Configuration().ClassName.Value());
            const string callSign(PluginHost::Service::Configuration().Callsign.Value());

            _reason = why;

            if(currentState == IShell::state::HIBERNATED)
            {
                uint32_t wakeupResult = Wakeup(3000);
                if(wakeupResult != Core::ERROR_NONE)
                {
                    //Force Activated state
                    State(ACTIVATED);
                }
                currentState = ACTIVATED;
            }

            if ( (currentState == IShell::ACTIVATION) || (currentState == IShell::ACTIVATED)) {
                ASSERT(_handler != nullptr);

                State(DEACTIVATION);

                SystemInfo& systeminfo = _administrator.SubSystemInfo();

                // On behalf of the plugin stop all subsystems it was controlling.
                for (const PluginHost::ISubSystem::subsystem sys : SubSystemControl()) {
                    systeminfo.Unset(sys);
                }

                Unlock();

                // Reevaluate status. In case some plugins were dependant on the subsystems
                // that have been just disabled, deactivate them too, recursively.
                _administrator.Evaluate();

                // And finally start tearing down this plugin...

                if (_stateControl != nullptr) {
                    _stateControl->Unregister(&_composit);
                    _stateControl->Release();
                    _stateControl = nullptr;
                }

                if (currentState == IShell::ACTIVATED) {
                    TRACE(Activity, (_T("Deactivating plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));
                    _administrator.Deactivated(callSign, this);
                }

                // We might require PostMortem analyses if the reason is not really clear. Call the PostMortum installed so it can generate
                // required logs/OS information before we start to kill it.
                Server::PostMortem(*this, why, _connection);

                // If we enabled the webserver, we should also disable it.
                if ((PluginHost::Service::Configuration().WebUI.IsSet()) || (PluginHost::Service::Configuration().WebUI.Value().empty() == false)) {
                    DisableWebServer();
                }

                REPORT_DURATION_WARNING( { _handler->Deinitialize(this); }, WarningReporting::TooLongPluginState, WarningReporting::TooLongPluginState::StateChange::DEACTIVATION, callSign.c_str());

                Lock();

                if (currentState != IShell::state::ACTIVATION) {
                    SYSLOG(Logging::Shutdown, (_T("Deactivated plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));

#ifdef THUNDER_RESTFULL_API
                    Notify(EMPTY_STRING, string(_T("{\"state\":\"deactivated\",\"reason\":\"")) + textReason.Data() + _T("\"}"));
#endif
                    Notify(_T("statechange"), string(_T("{\"state\":\"deactivated\",\"reason\":\"")) + textReason.Data() + _T("\"}"));
                }

                if (_external.Connector().empty() == false) {
                    _external.Close(0);
                }

                if (_jsonrpc != nullptr) {
                    PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;

                    _jsonrpc->Detach(sink);

                    if (sink != nullptr) {
                        Unregister(sink);
                        sink->Release();
                    }
                }
            }

            State(why == CONDITIONS ? PRECONDITION : DEACTIVATED);

            // We have no need for his module anymore..
            ReleaseInterfaces();
            Unlock();

            _administrator.Deinitialized(callSign, this);
        } else {
            Unlock();
        }

        return (result);
    }

    uint32_t Server::Service::Suspend(const reason why) {

        uint32_t result = Core::ERROR_NONE;

        if (StartMode() == PluginHost::IShell::startmode::DEACTIVATED) {
            // We need to shutdown completely
            result = Deactivate(why);
        }
        else {
            Lock();

            IShell::state currentState(State());

            if (currentState == IShell::state::DEACTIVATION) {
                result = Core::ERROR_INPROGRESS;
            } else if ((currentState == IShell::state::ACTIVATION) || (currentState == IShell::state::DESTROYED) || (currentState == IShell::state::HIBERNATED)) {
                result = Core::ERROR_ILLEGAL_STATE;
            } else if ((currentState == IShell::state::ACTIVATED) || (currentState == IShell::state::PRECONDITION)) {
                // See if we need can and should SUSPEND.
                if (_stateControl == nullptr) {
                    result = Core::ERROR_BAD_REQUEST;
                }
                else {
                    // We have a StateControl interface, so at least start suspending, if not already suspended :-)
                    if (_stateControl->State() == PluginHost::IStateControl::RESUMED) {
                        result = _stateControl->Request(PluginHost::IStateControl::SUSPEND);
                    }
                }
            }

            Unlock();
        }

        return (result);
    }

    Core::hresult Server::Service::Unavailable(const reason why) /* override */ {
        Core::hresult result = Core::ERROR_NONE;

        Lock();

        IShell::state currentState(State());

        if ((currentState == IShell::state::DEACTIVATION) ||
            (currentState == IShell::state::ACTIVATION)   ||
            (currentState == IShell::state::DESTROYED)    ||
            (currentState == IShell::state::ACTIVATED)    ||
            (currentState == IShell::state::PRECONDITION) ||
            (currentState == IShell::state::HIBERNATED)   ) {
            result = Core::ERROR_ILLEGAL_STATE;
        }
        else if (currentState == IShell::state::DEACTIVATED) {

            const Core::EnumerateType<PluginHost::IShell::reason> textReason(why);

            const string className(PluginHost::Service::Configuration().ClassName.Value());
            const string callSign(PluginHost::Service::Configuration().Callsign.Value());

            _reason = why;

            SYSLOG(Logging::Shutdown, (_T("Unavailable plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));

            TRACE(Activity, (Core::Format(_T("Unavailable plugin [%s]:[%s]"), className.c_str(), callSign.c_str())));

            State(UNAVAILABLE);
            _administrator.Unavailable(callSign, this);

            Unlock();

            #ifdef THUNDER_RESTFULL_API
            Notify(EMPTY_STRING, string(_T("{\"state\":\"unavailable\",\"reason\":\"")) + textReason.Data() + _T("\"}"));
            #endif
            Notify(_T("statechange"), string(_T("{\"state\":\"unavailable\",\"reason\":\"")) + textReason.Data() + _T("\"}"));
        }

        Unlock();

        return (result);

    }

    Core::hresult Server::Service::Hibernate(const uint32_t timeout VARIABLE_IS_NOT_USED) /* override */ {
        Core::hresult result = Core::ERROR_NONE;

        Lock();

        IShell::state currentState(State());

        if (currentState != IShell::state::ACTIVATED) {
            result = Core::ERROR_ILLEGAL_STATE;
        }
        else if (_connection == nullptr) {
            result = Core::ERROR_INPROC;
        }
        else {
            // Oke we have an Connection so there is something to Hibernate..
            RPC::IMonitorableProcess* local = _connection->QueryInterface< RPC::IMonitorableProcess>();

            if (local == nullptr) {
                result = Core::ERROR_BAD_REQUEST;
            }
            else {
                State(IShell::HIBERNATED);
#ifdef HIBERNATE_SUPPORT_ENABLED
                pid_t parentPID = local->ParentPID();
                local->Release();
                Unlock();

                TRACE(Activity, (_T("Hibernation of plugin [%s] process [%u]"), Callsign().c_str(), parentPID));
                result = HibernateProcess(timeout, parentPID, _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
                Lock();
                if (State() != IShell::HIBERNATED) {
                    SYSLOG(Logging::Startup, (_T("Hibernation aborted of plugin [%s] process [%u]"), Callsign().c_str(), parentPID));
                    result = Core::ERROR_ABORTED;
                }
                Unlock();

                if (result == HIBERNATE_ERROR_NONE) {
                    result = HibernateChildren(parentPID, timeout);
                }

                if (result != Core::ERROR_NONE && result != Core::ERROR_ABORTED) {
                    // try to wakeup Parent process to revert Hibernation and recover
                    TRACE(Activity, (_T("Wakeup plugin [%s] process [%u] on Hibernate error [%d]"), Callsign().c_str(), parentPID, result));
                    WakeupProcess(timeout, parentPID, _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
                }

                Lock();
#else
                local->Release();
                result = Core::ERROR_NONE;
#endif
                if (result == Core::ERROR_NONE) {
                    if (State() == IShell::state::HIBERNATED) {
                        SYSLOG(Logging::Startup, ("Hibernated plugin [%s]:[%s]", ClassName().c_str(), Callsign().c_str()));
                    } else {
                        // wakeup occured right after hibernation finished
                        SYSLOG(Logging::Startup, ("Hibernation aborted of plugin [%s]:[%s]", ClassName().c_str(), Callsign().c_str()));
                        result = Core::ERROR_ABORTED;
                    }
                }
                else if (State() == IShell::state::HIBERNATED) {
                    State(IShell::ACTIVATED);
                    SYSLOG(Logging::Startup, (_T("Hibernation error [%d] of [%s]:[%s]"), result, ClassName().c_str(), Callsign().c_str()));
                }
            }
        }
        Unlock();

        return (result);

    }

    uint32_t Server::Service::Wakeup(const uint32_t timeout VARIABLE_IS_NOT_USED) {
        Core::hresult result = Core::ERROR_NONE;

        IShell::state currentState(State());

        if (currentState != IShell::state::HIBERNATED) {
            result = Core::ERROR_ILLEGAL_STATE;
        }
        else {
            ASSERT(_connection != nullptr);

            // Oke we have an Connection so there is something to Wakeup..
            RPC::IMonitorableProcess* local = _connection->QueryInterface< RPC::IMonitorableProcess>();

            if (local == nullptr) {
                result = Core::ERROR_BAD_REQUEST;
            }
            else {
#ifdef HIBERNATE_SUPPORT_ENABLED
                pid_t parentPID = local->ParentPID();

                // There is no recovery path while doing Wakeup, don't care about errors
                WakeupChildren(parentPID, timeout);

                TRACE(Activity, (_T("Wakeup of plugin [%s] process [%u]"), Callsign().c_str(), parentPID));
                result = WakeupProcess(timeout, parentPID, _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
#else
                result = Core::ERROR_NONE;
#endif
                if (result == Core::ERROR_NONE) {
                    State(ACTIVATED);
                    SYSLOG(Logging::Startup, (_T("Activated plugin from hibernation [%s]:[%s]"), ClassName().c_str(), Callsign().c_str()));
                }
                local->Release();
            }
        }

        return (result);
    }

#ifdef HIBERNATE_SUPPORT_ENABLED
    uint32_t Server::Service::HibernateChildren(const pid_t parentPID, const uint32_t timeout)
    {
        Core::hresult result = Core::ERROR_NONE;
        Core::ProcessInfo::Iterator children(parentPID);
        std::vector<pid_t> childrenPIDs;

        if (children.Count() > 0) {

            while (children.Next()) {
                childrenPIDs.push_back(children.Current().Id());
            }

            for (auto iter = childrenPIDs.begin(); iter != childrenPIDs.end(); ++iter) {
                TRACE(Activity, (_T("Hibernation of plugin [%s] child process [%u]"), Callsign().c_str(), *iter));
                Lock();
                if (State() != IShell::HIBERNATED) {
                    SYSLOG(Logging::Startup, (_T("Hibernation aborted of plugin [%s] child process [%u]"), Callsign().c_str(), *iter));
                    result = Core::ERROR_ABORTED;
                    Unlock();
                    break;
                }
                Unlock();
                result = HibernateProcess(timeout, *iter, _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
                if (result == HIBERNATE_ERROR_NONE) {
                    // Hibernate Children of this process
                    result = HibernateChildren(*iter, timeout);
                    if (result == Core::ERROR_ABORTED) {
                        break;
                    }
                }

                if (result != HIBERNATE_ERROR_NONE) {
                    // try to recover by reverting current Hibernations
                    TRACE(Activity, (_T("Wakeup plugin [%s] process [%u] on Hibernate error [%d]"), Callsign().c_str(), *iter, result));
                    WakeupProcess(timeout, *iter, _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
                    // revert previous Hibernations and break
                    while (iter != childrenPIDs.begin()) {
                        --iter;
                        WakeupChildren(*iter, timeout);
                        TRACE(Activity, (_T("Wakeup plugin [%s] process [%u] on Hibernate error [%d]"), Callsign().c_str(), *iter, result));
                        WakeupProcess(timeout, *iter, _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
                    }
                    break;
                }
            }
        }

        return result;
    }

    uint32_t Server::Service::WakeupChildren(const pid_t parentPID, const uint32_t timeout)
    {
        Core::hresult result = Core::ERROR_NONE;
        Core::ProcessInfo::Iterator children(parentPID);

        if (children.Count() > 0) {
            // make sure to wakeup PIDs in opposite order to hibernation
            // to quickly go over not hibernated PIDs and abort currently processed
            children.Reset(false);
            while (children.Previous()) {
                // Wakeup children of this process
                // There is no recovery path while doing Wakeup, don't care about errors
                WakeupChildren(children.Current().Id(), timeout);
                TRACE(Activity, (_T("Wakeup of plugin [%s] child process [%u]"), Callsign().c_str(), children.Current().Id()));
                result = WakeupProcess(timeout, children.Current().Id(), _administrator.Configuration().HibernateLocator().c_str(), _T(""), &_hibernateStorage);
            }
        }

        return result;
    }
#endif

    uint32_t Server::Service::Submit(const uint32_t id, const Core::ProxyType<Core::JSON::IElement>& response) /* override */
    {
        return (_administrator.Submit(id, response));
    }

    ISubSystem* Server::Service::SubSystems() /* override */
    {
        return (_administrator.SubSystemsInterface(this));
    }

    void Server::Service::Notify(const string& jsonrpcEvent, const string& message) /* override */
    {
        // JSONRPC has been send by now, lets send it to the "notification"
        // observers..
        if (jsonrpcEvent.empty() == true) {
            BaseClass::Notification(message);
        }

        _administrator.Notification(PluginHost::Service::Callsign(), jsonrpcEvent, message);
    }

    //
    // class Server::ServiceMap
    // -----------------------------------------------------------------------------------------------------------------------------------
    void Server::ServiceMap::Open(std::vector<PluginHost::ISubSystem::subsystem>& externallyControlled) {
        // Load the metadata for the subsystem information..
        for (auto service : _services)
        {
            service.second->LoadMetadata();
            for (const PluginHost::ISubSystem::subsystem& entry : service.second->SubSystemControl()) {
                Core::EnumerateType<PluginHost::ISubSystem::subsystem> name(entry);
                if (std::find(externallyControlled.begin(), externallyControlled.end(), entry) != externallyControlled.end()) {
                    SYSLOG(Logging::Startup, (Core::Format(_T("Subsystem [%s] controlled by multiple plugins. Second: [%s]. Configuration error!!!"), name.Data(), service.second->Callsign().c_str())));
                }
                else if (entry >= PluginHost::ISubSystem::END_LIST) {
                    SYSLOG(Logging::Startup, (Core::Format(_T("Subsystem [%s] can not be used as a control value in [%s]!!!"), name.Data(), service.second->Callsign().c_str())));
                }
                else {
                    SYSLOG(Logging::Startup, (Core::Format(_T("Subsytem [%s] controlled by plugin [%s]"), name.Data(), service.second->Callsign().c_str())));
                    externallyControlled.emplace_back(entry);
                }
            }
        }
    }

    void Server::ServiceMap::Close()
    {
        _adminLock.Lock();

        // First, move them all to deactivated except Controller
        Core::ProxyType<Service> controller(_server.Controller());

        TRACE_L1("Destructing %d plugins", static_cast<uint32_t>(_services.size()));

        while (_services.empty() == false) {

            auto index = _services.begin();

            Core::ProxyType<Service> service(index->second);

            ASSERT(service.IsValid());

            if (index->first.c_str() != controller->Callsign()) {
                _adminLock.Unlock();

                index->second->Deactivate(PluginHost::IShell::SHUTDOWN);

                _adminLock.Lock();
            }

            _services.erase(index);
        }

        _adminLock.Unlock();

        // Now deactivate controller plugin, once other plugins are deactivated
        controller->Deactivate(PluginHost::IShell::SHUTDOWN);

        TRACE_L1("Pending notifiers are %zu", _notifiers.size());
        for (VARIABLE_IS_NOT_USED auto notifier : _notifiers) {
            TRACE_L1("   -->  %s", Core::ClassNameOnly(typeid(*notifier).name()).Text().c_str());
        }

        _processAdministrator.Close(Core::infinite);

        _processAdministrator.Destroy();
    }

    uint32_t Server::ServiceMap::FromLocator(const string& identifier, Core::ProxyType<Service>& service, PluginHost::Request::mode& callType)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;
        const string& serviceHeader(Configuration().WebPrefix());
        const string& JSONRPCHeader(Configuration().JSONRPCPrefix());


        // Check the header (prefix part)
        if (identifier.compare(0, serviceHeader.length(), serviceHeader.c_str()) == 0) {

            callType = PluginHost::Request::mode::RESTFULL;

            if (identifier.length() <= (serviceHeader.length() + 1)) {
                service = _server.Controller();
                result = Core::ERROR_NONE;
            } else {
                Core::ProxyType<IShell> actualService;
                size_t length;
                uint32_t offset = static_cast<uint32_t>(serviceHeader.length()) + 1; /* skip the slash after */

                const string callSign(identifier.substr(offset, ((length = identifier.find_first_of('/', offset)) == string::npos ? string::npos : length - offset)));

                if ( (result = FromIdentifier(callSign, actualService)) == Core::ERROR_NONE) {
                    service = Core::ProxyType<Service>(actualService);
                    if (service.IsValid() == false) {
                        result = Core::ERROR_BAD_REQUEST;
                    }
                }
            }
        } else if (identifier.compare(0, JSONRPCHeader.length(), JSONRPCHeader.c_str()) == 0) {

            callType = PluginHost::Request::mode::JSONRPC;

            if (identifier.length() <= (JSONRPCHeader.length() + 1)) {
                service = _server.Controller();
                result = Core::ERROR_NONE;
            } else {
                Core::ProxyType<IShell> actualService;
                size_t length;
                uint32_t offset = static_cast<uint32_t>(JSONRPCHeader.length()) + 1; /* skip the slash after */

                const string callSign(identifier.substr(offset, ((length = identifier.find_first_of('/', offset)) == string::npos ? string::npos : length - offset)));

                if ((result = FromIdentifier(callSign, actualService)) == Core::ERROR_NONE) {
                    service = Core::ProxyType<Service>(actualService);
                    if (service.IsValid() == false) {
                        result = Core::ERROR_BAD_REQUEST;
                    }
                }
            }
        }
        else {
            callType = PluginHost::Request::mode::PROPRIETARY;
        }

        return (result);
    }

    void Server::ServiceMap::Startup() {

        // sort plugins based on StartupOrder from configuration
        std::vector<Core::ProxyType<Service>> configured_services;

        for (auto service : _services) {
            configured_services.emplace_back(service.second);
        }

        std::sort(configured_services.begin(), configured_services.end(),
            [](const Core::ProxyType<Service>& lhs, const Core::ProxyType<Service>& rhs)
            {
                return lhs->StartupOrder() < rhs->StartupOrder();
            });

        for (auto service : configured_services)
        {
            if (service->State() != PluginHost::Service::state::UNAVAILABLE) {
                if (service->StartMode() == PluginHost::IShell::startmode::ACTIVATED) {
                    SYSLOG(Logging::Startup, (_T("Activating plugin [%s]:[%s]"),
                        service->ClassName().c_str(), service->Callsign().c_str()));
                    service->Activate(PluginHost::IShell::STARTUP);
                }
                else {
                    SYSLOG(Logging::Startup, (_T("Activation of plugin [%s]:[%s] delayed, start mode is %s"),
                        service->ClassName().c_str(), service->Callsign().c_str(),
                        Core::EnumerateType<PluginHost::IShell::startmode>(service->StartMode()).Data()));
                }
            }
        }
    }

    //
    // class Server::Channel
    // -----------------------------------------------------------------------------------------------------------------------------------
    Server::Channel::Channel(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Channel>* parent)
        : PluginHost::Channel(connector, remoteId)
        , _parent(static_cast<ChannelMap&>(*parent).Parent())
        , _security(_parent.Officer())
        , _service()
        , _requestClose(false)
        , _jobs()
    {
        TRACE(Activity, (_T("Construct a link with ID: [%d] to [%s]"), Id(), remoteId.QualifiedName().c_str()));

        _jobs.Slots(static_cast<ChannelMap&>(*parent).MaxRequests());
    }

    /* virtual */ Server::Channel::~Channel()
    {
        TRACE(Activity, (_T("Destruct a link with ID [%d] to [%s]"), Id(), RemoteId().c_str()));

        // If we are still atatched to a service, detach, we are out of scope...
        if (_service.IsValid() == true) {
            _service->Unsubscribe(*this);

            _service.Release();
        }
        if (_security != nullptr) {
            _security->Release();
            _security = nullptr;
        }

        Close(Core::infinite);
    }

    //
    // class Server
    // -----------------------------------------------------------------------------------------------------------------------------------
    PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    Server::Server(Config& configuration, const bool background)
        : _dispatcher(configuration.StackSize())
        , _config(configuration)
        , _connections(*this, configuration.Binder())
        , _services(*this)
        , _controller()
        , _factoriesImplementation()
    {
        IFactories::Assign(&_factoriesImplementation);

        // See if the persitent path for our-selves exist, if not we will create it :-)
        Core::File persistentPath(_config.PersistentPath() + _T("PluginHost"));

        if (persistentPath.IsDirectory() == false) {
            Core::Directory(persistentPath.Name().c_str()).Create();
        }

        // Lets assign a workerpool, we created it...
        Core::WorkerPool::Assign(&_dispatcher);

        Core::JSON::ArrayType<Plugin::Config>::Iterator index = configuration.Plugins();

        // First register all services, than if we got them, start "activating what is required.
        // Whatever plugin is needed, we at least have our Metadata plugin available (as the first entry :-).
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
                    #ifndef __WINDOWS__
                    if (background == true) {
                        syslog(LOG_NOTICE, "Configuration error. Controller is defined mutiple times [%s].\n", entry.Callsign.Value().c_str());
                    } else
                    #endif
                    {
                        fprintf(stdout, "Configuration error. Controller is defined mutiple times [%s].\n", entry.Callsign.Value().c_str());
                    }
                }
            } else {
                _services.Insert(entry, Service::mode::CONFIGURED);
            }
        }

        if (metaDataConfig.Callsign.Value().empty() == true) {
            // Oke, this is the first time we "initialize" it.
            metaDataConfig.Callsign = string(_defaultControllerCallsign);
        }

        // Get the configuration from the persistent location.
        Load();

        // Create input handle
        _inputHandler.Initialize(
            configuration.Input().Type(),
            configuration.Input().Locator(),
            configuration.Input().Enabled());

        // Initialize static message.
        Service::Initialize();
        Channel::Initialize(_config.WebPrefix());

        // Add the controller as a service to the services.
        _controller = _services.Insert(metaDataConfig, Service::mode::CONFIGURED);

#ifdef PROCESSCONTAINERS_ENABLED
        // turn on ProcessContainer logging
        ProcessContainers::ContainerAdministrator::Instance().Initialize(_config.ProcessContainersConfig());
#endif
    }
    POP_WARNING()

    Server::~Server()
    {
        // The workerpool is about to dissapear!!!!
        Core::WorkerPool::Assign(nullptr);
        IFactories::Assign(nullptr);
    }

    void Server::Notification(const string& callsign, const string& event, const string& parameters)
    {
        ASSERT((_controller.IsValid() == true) && (_controller->ClassType<Plugin::Controller>() != nullptr));

        Plugin::Controller* controller = _controller->ClassType<Plugin::Controller>();

        // Break a recursive loop, if it tries to arise ;-)
        if ( (controller != nullptr) && (callsign != controller->Callsign()) ) {

            ASSERT(callsign.empty() == false);

            if (event.empty() == false) {

                Core::OptionalType<string> eventParams;
                if (parameters.empty() == false) {
                    eventParams = parameters;
                }

                Core::OptionalType<string> eventCallsign;
                if (callsign.empty() == false) {
                    eventCallsign = callsign;
                }

                Exchange::Controller::JEvents::Event::ForwardMessage(*controller, event, eventCallsign, eventParams);
            }
            else {
                string messageString = string(_T("{\"callsign\":\"")) + callsign + _T("\", {\"data\":") + parameters + _T("}}");

                _controller->Notify(EMPTY_STRING, messageString);
            }
        }
    }

    void Server::StateControlStateChange(const string& callsign, const IStateControl::state state)
    {
        ASSERT((_controller.IsValid() == true) && (_controller->ClassType<Plugin::Controller>() != nullptr));
        Plugin::Controller* controller = _controller->ClassType<Plugin::Controller>();
        ASSERT(controller != nullptr);

        switch (state) {
            case IStateControl::state::SUSPENDED:
                controller->NotifyStateControlStateChange(callsign, Exchange::Controller::ILifeTime::state::SUSPENDED);
                break;
            case IStateControl::state::RESUMED:
                controller->NotifyStateControlStateChange(callsign, Exchange::Controller::ILifeTime::state::RESUMED);
                break;
            default:
                controller->NotifyStateControlStateChange(callsign, Exchange::Controller::ILifeTime::state::UNKNOWN);
                break;
        }
    }

    void Server::Open()
    {
        // Before we do anything with the subsystems (notifications)
        // Lets see if security is already set..
        DefaultSecurity* securityProvider = Core::ServiceType<DefaultSecurity>::Create<DefaultSecurity>(
            _config.WebPrefix(),
            _config.JSONRPCPrefix(),
            _controller->Callsign());

        _config.Security(securityProvider);

        std::vector<PluginHost::ISubSystem::subsystem> externallyControlled;
        _services.Open(externallyControlled);

        _controller->Activate(PluginHost::IShell::STARTUP);

        Plugin::Controller* controller = _controller->ClassType<Plugin::Controller>();

        ASSERT(controller != nullptr);

        controller->SetServer(this, std::move(externallyControlled));

        if ((_services.SubSystemInfo().Value() & (1 << ISubSystem::SECURITY)) != 0) {
            // The controller is in control of the security, so I guess all systems green
            // as the controller does not know anything about security :-)
            securityProvider->Security(false);
        } else {
            SYSLOG(Logging::Startup, (_T("Security ENABLED, incoming requests need to be authorized!!!")));
        }

        securityProvider->Release();

        _dispatcher.Run();
        _connections.Open(MAX_EXTERNAL_WAITS, _config.IdleTime());

        _services.Startup();
    }

    void Server::Close()
    {
        Plugin::Controller* destructor(_controller->ClassType<Plugin::Controller>());
        destructor->AddRef();
        _connections.Close(100);
        destructor->Stopped();
        _services.Close();
        _dispatcher.Stop();
        destructor->Release();
        _inputHandler.Deinitialize();
        _connections.Close(Core::infinite);

    }
}
}
