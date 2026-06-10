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
    /* static */ const TCHAR* Server::ExtensionsConfigDirectory = _T("extensions/");
    /* static */ const TCHAR* Server::PluginConfigDirectory = _T("plugins/");
    /* static */ const TCHAR* Server::PluginOverrideDirectory = _T(EXPAND_AND_QUOTE(NAMESPACE) "/services/");
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
        // coverity[ATOMICITY] - Lock is intentionally dropped around Deactivate() which can
        // block. The iterator is refreshed (begin()) after re-acquiring the lock each iteration.
        // This is correct lock-straddling, not a race.
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

#ifdef __DEBUG__
        TRACE_L1("Pending notifiers are %u", _notifiers.Size());
        _notifiers.Visit([](const PluginHost::IPlugin::INotification* notification) {
            ASSERT(notification != nullptr);
            TRACE_L1("   -->  %s", Core::ClassNameOnly(typeid(*notification).name()).Text().c_str());
        });
#endif
        _processAdministrator.Close(Core::infinite);

        _processAdministrator.Destroy();
    }

    void* Server::Service::QueryInterface(const uint32_t id, const bool asIUnknown) /* override */
    {
        void* result = nullptr;
        if (id == Core::IUnknown::ID) {
            AddRef();
            result = static_cast<IUnknown*>(this);
        }
        else if (id == PluginHost::IShell::ID) {
            AddRef();
            asIUnknown == false ? result = static_cast<PluginHost::IShell*>(this) : result = static_cast<Core::IUnknown*>(this);
        }
        else if (id == PluginHost::IShell::ICOMLink::ID) {
            AddRef();
            asIUnknown == false ? result = static_cast<PluginHost::IShell::ICOMLink*>(this) : result = static_cast<Core::IUnknown*>(this);
        }
        else if (id == PluginHost::IShell::IConnectionServer::ID) {
            AddRef();
            asIUnknown == false ? result = static_cast<PluginHost::IShell::IConnectionServer*>(this) : result = static_cast<Core::IUnknown*>(this);
        }
        else {
            // Route through the state machine — only ACTIVATED and states that
            // explicitly override QueryInterface will return a non-null result.
            result = _stateMachine.QueryInterface(id, asIUnknown);
        }

        return (result);
    }

    void* Server::Service::QueryInterfaceByCallsign(const uint32_t id, const string& name) /* override */
    {
        return (_administrator.QueryInterfaceByCallsign(id, name));
    }

    void Server::Service::Register(IPlugin::INotification * sink, const Core::OptionalType<string>& callsign) /* override */
    {
        _administrator.Register(sink, callsign);
    }

    void Server::Service::Unregister(IPlugin::INotification * sink, const Core::OptionalType<string>& callsign) /* override */
    {
        _administrator.Unregister(sink, callsign);
    }

    void Server::Service::Register(IPlugin::INotification * sink, const uint32_t interface_id)
    {
        _administrator.Register(sink, interface_id);
    }
    void Server::Service::Unregister(IPlugin::INotification * sink, const uint32_t interface_id)
    {
        _administrator.Unregister(sink, interface_id);
    }

    // Methods to stop/start/update the service.
    Core::hresult Server::Service::Activate(const PluginHost::IShell::reason why) /* override */
    {
        return _stateMachine.Activate(why);
    }

    uint32_t Server::Service::Resume(const reason why) /* override */
    {
        return _stateMachine.Resume(why);
    }

    Core::hresult Server::Service::Deactivate(const reason why) /* override */
    {
        return _stateMachine.Deactivate(why);
    }

    uint32_t Server::Service::Suspend(const reason why) /* override */
    {
        uint32_t result = Core::ERROR_NONE;

        if (StartMode() == PluginHost::IShell::startmode::DEACTIVATED) {
            result = _stateMachine.Deactivate(why);
        } else {
            result = _stateMachine.Suspend(why);
        }

        return (result);
    }

    Core::hresult Server::Service::Unavailable(const reason why) /* override */
    {
        return _stateMachine.Unavailable(why);
    }

    Core::hresult Server::Service::Hibernate(const uint32_t timeout VARIABLE_IS_NOT_USED) /* override */
    {
        return _stateMachine.Hibernate(timeout);
    }

    // -------------------------------------------------------------------------
    // StateMachine — static state instance definitions
    // -------------------------------------------------------------------------

    Server::Service::StateMachine::DeactivatedState  Server::Service::StateMachine::_stateDeactivated;
    Server::Service::StateMachine::PreconditionState Server::Service::StateMachine::_statePrecondition;
    Server::Service::StateMachine::ActivationState   Server::Service::StateMachine::_stateActivation;
    Server::Service::StateMachine::ActivatedState    Server::Service::StateMachine::_stateActivated;
    Server::Service::StateMachine::DeactivationState Server::Service::StateMachine::_stateDeactivation;
    Server::Service::StateMachine::HibernatedState   Server::Service::StateMachine::_stateHibernated;
    Server::Service::StateMachine::UnavailableState  Server::Service::StateMachine::_stateUnavailable;
    Server::Service::StateMachine::DestroyedState    Server::Service::StateMachine::_stateDestroyed;

    // -------------------------------------------------------------------------
    // DeactivatedState
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::StateMachine::DeactivatedState::Activate(StateMachine& sm, const reason why)
    {
        const string callSign(sm._parent.PluginHost::Service::Configuration().Callsign.Value());

        sm._parent.Lock();
        sm._parent._reason = why;

        Core::hresult result = sm._parent.LoadPlugin();

        if (result == Core::ERROR_UNAVAILABLE) {
            sm.SetState(_stateDeactivated);
            sm._parent.Unlock();
            return result;
        }
        if (result == Core::ERROR_PENDING_CONDITIONS) {
            sm.SetState(_statePrecondition);
            sm._parent.Unlock();
            return result;
        }

        sm._parent._administrator.Initialize(callSign, &sm._parent);
        sm.SetState(_stateActivation);
        sm._parent.Unlock();

        result = sm._parent.InitializePlugin();

        if (result != Core::ERROR_NONE) {
            sm._parent.Lock();
            sm._parent._reason = IShell::reason::INITIALIZATION_FAILED;
            sm._parent.UnloadPlugin();
            sm.SetState(_stateDeactivated);
            sm._parent.Unlock();
            sm._callback(IShell::DEACTIVATED);
            return result;
        }

        sm._parent.Attach();

        sm._parent.Lock();
        sm.SetState(_stateActivated);
        sm._parent.Unlock();

        sm._callback(IShell::ACTIVATED);
        return Core::ERROR_NONE;
    }

    Core::hresult Server::Service::StateMachine::DeactivatedState::Unavailable(StateMachine& sm, const reason why)
    {
        if (sm._parent.AllowedUnavailable() == false) {
            return Core::ERROR_NOT_SUPPORTED;
        }

        const string callSign(sm._parent.PluginHost::Service::Configuration().Callsign.Value());
        const string className(sm._parent.PluginHost::Service::Configuration().ClassName.Value());

        sm._parent.Lock();
        sm._parent._reason = why;
        SYSLOG(Logging::Shutdown, (_T("Unavailable plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));
        TRACE(Activity, (Core::Format(_T("Unavailable plugin [%s]:[%s]"), className.c_str(), callSign.c_str())));
        sm.SetState(_stateUnavailable);
        sm._parent.Unlock();

        sm._callback(IShell::UNAVAILABLE);
        return Core::ERROR_NONE;
    }

    uint32_t Server::Service::StateMachine::DeactivatedState::Resume(StateMachine & sm, const reason why)
    {
        Core::hresult result = sm._Activate(why);
        if (result != Core::ERROR_NONE) {
            return result;
        }

        return sm._current.load(std::memory_order_acquire)->Resume(sm, why);
    }

    // -------------------------------------------------------------------------
    // PreconditionState
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::StateMachine::PreconditionState::Deactivate(StateMachine& sm, const reason why)
    {
        sm._parent.Lock();
        sm._parent._reason = why;
        const IShell::state finalState(why == IShell::CONDITIONS ? IShell::PRECONDITION : IShell::DEACTIVATED);
        sm.SetState(why == IShell::CONDITIONS ? static_cast<StateBase&>(_statePrecondition) : static_cast<StateBase&>(_stateDeactivated));
        sm._parent.Unlock();

        sm._parent.UnloadPlugin();

        sm._callback(finalState);
        return Core::ERROR_NONE;
    }

    uint32_t Server::Service::StateMachine::PreconditionState::Resume(StateMachine & sm, const reason /* why */)
    {
        return sm._parent.RequestResume();
    }

    uint32_t Server::Service::StateMachine::PreconditionState::Suspend(StateMachine& sm, const reason /* why */)
    {
        return sm._parent.RequestSuspend();
    }

    void Server::Service::StateMachine::PreconditionState::Reevaluate(StateMachine& sm)
    {
        sm._parent.Lock();
        const uint32_t subsystems(sm._parent._administrator.SubSystemInfo().Value());

        if ((sm._parent._precondition.Evaluate(subsystems) == true) && (sm._parent._precondition.IsMet() == true)) {
            sm._parent.Unlock();
            sm._Activate(sm._parent._reason);
        } else {
            sm._parent.Unlock();
        }
    }

    // -------------------------------------------------------------------------
    // ActivationState — only INITIALIZATION_FAILED deactivation is legal
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::StateMachine::ActivationState::Deactivate(StateMachine& sm, const reason why)
    {
        if (why != IShell::reason::INITIALIZATION_FAILED) {
            return Core::ERROR_ILLEGAL_STATE;
        }

        const string callSign(sm._parent.PluginHost::Service::Configuration().Callsign.Value());

        sm._parent.Lock();
        sm._parent._reason = why;
        sm.SetState(_stateDeactivation);
        sm._parent.Unlock();

        sm.Evaluate();  // temporarily releases _transitionLock — safe, DEACTIVATION is set
        Server::PostMortem(sm._parent, why, sm._parent._connection);
        sm._parent.Detach();

        sm._parent.DeinitializePlugin();
        sm._parent.Lock();

        sm.SetState(_stateDeactivated);
        sm._parent.UnloadPlugin();
        sm._parent.Unlock();

        sm._callback(IShell::DEACTIVATED);
        return Core::ERROR_NONE;
    }

    // -------------------------------------------------------------------------
    // ActivatedState
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::StateMachine::ActivatedState::Deactivate(StateMachine& sm, const reason why)
    {
        const string callSign(sm._parent.PluginHost::Service::Configuration().Callsign.Value());

        sm._parent.Lock();
        sm._parent._reason = why;

        SystemInfo& systeminfo = sm._parent._administrator.SubSystemInfo();
        for (const PluginHost::ISubSystem::subsystem sys : sm._parent.SubSystemControl()) {
            systeminfo.Unset(sys);
        }

        // Set transient state before Evaluate() so RecursiveNotification sees
        // DEACTIVATION and short-circuits — no-op in DeactivationState::Reevaluate.
        sm.SetState(_stateDeactivation);
        sm._parent.Unlock();

        // Temporarily releases _transitionLock — safe because DEACTIVATION is set.
        sm.Evaluate();

        // Fire Deactivated() while _handler is still alive — subscribers may
        // safely call QueryInterface during this notification.
        sm._parent._administrator.Deactivated(callSign, &sm._parent);

        Server::PostMortem(sm._parent, why, sm._parent._connection);
        sm._parent.Detach();

        sm._parent.DeinitializePlugin();
        sm._parent.Lock();

        const IShell::state finalState(why == IShell::CONDITIONS ? IShell::PRECONDITION : IShell::DEACTIVATED);
        sm.SetState(why == IShell::CONDITIONS ? static_cast<StateBase&>(_statePrecondition) : static_cast<StateBase&>(_stateDeactivated));
        sm._parent.UnloadPlugin();
        sm._parent.Unlock();

        sm._callback(finalState);
        return Core::ERROR_NONE;
    }

    Core::hresult Server::Service::StateMachine::ActivatedState::Hibernate(StateMachine& sm, const uint32_t timeout VARIABLE_IS_NOT_USED)
    {
        if (sm._parent.AllowedHibernate() == false) {
            return Core::ERROR_NOT_SUPPORTED;
        }

        sm._parent.Lock();

        if (sm._parent._connection == nullptr) {
            sm._parent.Unlock();
            return Core::ERROR_INPROC;
        }

        RPC::IMonitorableProcess* local = sm._parent._connection->QueryInterface<RPC::IMonitorableProcess>();

        if (local == nullptr) {
            sm._parent.Unlock();
            return Core::ERROR_BAD_REQUEST;
        }

        // Set HIBERNATED under lock as the in-progress guard.
        // Note: this is set before the blocking HibernateProcess() call — the process
        // is still running at this point. A HIBERNATING transient state would be
        // more accurate but requires an IShell::state enum change (tracked as debt).
        sm.SetState(_stateHibernated);

#ifdef HIBERNATE_SUPPORT_ENABLED
        pid_t parentPID = local->ParentPID();
        local->Release();
        sm._parent.Unlock();

        TRACE(Activity, (_T("Hibernation of plugin [%s] process [%u]"), sm._parent.Callsign().c_str(), parentPID));
        Core::hresult result = HibernateProcess(timeout, parentPID, sm._parent._administrator.Configuration().HibernateLocator().c_str(), _T(""), &sm._parent._hibernateStorage);

        sm._parent.Lock();
        // Note: in the original design this checked whether a concurrent Wakeup() or
        // Deactivate() had changed state during HibernateProcess(). Under the new design
        // _transitionLock is held throughout — no concurrent transition can run — so this
        // check can never fire. Retained as a defensive guard only.
        if (sm._parent.State() != IShell::HIBERNATED) {
            SYSLOG(Logging::Startup, (_T("Hibernation aborted of plugin [%s] process [%u]"), sm._parent.Callsign().c_str(), parentPID));
            sm._parent.Unlock();
            return Core::ERROR_ABORTED;
        }
        sm._parent.Unlock();

        if (result == HIBERNATE_ERROR_NONE) {
            result = sm._parent.HibernateChildren(parentPID, timeout);
        }

        if (result != Core::ERROR_NONE && result != Core::ERROR_ABORTED) {
            // Rollback — try to wake the parent process to recover from failed hibernation.
            TRACE(Activity, (_T("Wakeup plugin [%s] process [%u] on Hibernate error [%d]"), sm._parent.Callsign().c_str(), parentPID, result));
            WakeupProcess(timeout, parentPID, sm._parent._administrator.Configuration().HibernateLocator().c_str(), _T(""), &sm._parent._hibernateStorage);
        }

        sm._parent.Lock();
#else
        local->Release();
        Core::hresult result = Core::ERROR_NONE;
#endif
        // coverity[DEADCODE] — on the non-HIBERNATE_ENABLED path result is always
        // ERROR_NONE here, making the else-if appear unreachable to Coverity.
        // Both branches are reachable when HIBERNATE_SUPPORT_ENABLED is defined.
        if (result == Core::ERROR_NONE) {
            if (sm._parent.State() == IShell::state::HIBERNATED) {
                SYSLOG(Logging::Startup, ("Hibernated plugin [%s]:[%s]", sm._parent.ClassName().c_str(), sm._parent.Callsign().c_str()));
                sm._parent.Unlock();
                sm._callback(IShell::HIBERNATED);
            } else {
                // Wakeup occurred right after hibernation finished.
                SYSLOG(Logging::Startup, ("Hibernation aborted of plugin [%s]:[%s]", sm._parent.ClassName().c_str(), sm._parent.Callsign().c_str()));
                sm._parent.Unlock();
                result = Core::ERROR_ABORTED;
            }
        } else if (sm._parent.State() == IShell::state::HIBERNATED) {
            // Hibernation failed — roll back state to ACTIVATED.
            sm.SetState(_stateActivated);
            SYSLOG(Logging::Startup, (_T("Hibernation error [%d] of [%s]:[%s]"), result, sm._parent.ClassName().c_str(), sm._parent.Callsign().c_str()));
            sm._parent.Unlock();
            sm._callback(IShell::ACTIVATED);
        } else {
            sm._parent.Unlock();
        }

        return result;
    }

    uint32_t Server::Service::StateMachine::ActivatedState::Resume(StateMachine & sm, const reason why VARIABLE_IS_NOT_USED)
    {
        return sm._parent.RequestResume();
    }

    uint32_t Server::Service::StateMachine::ActivatedState::Suspend(StateMachine & sm, const reason /* why */)
    {
        return sm._parent.RequestSuspend();
    }

    void Server::Service::StateMachine::ActivatedState::Reevaluate(StateMachine& sm)
    {
        sm._parent.Lock();
        const uint32_t subsystems(sm._parent._administrator.SubSystemInfo().Value());

        if ((sm._parent._termination.Evaluate(subsystems) == true) && (sm._parent._termination.IsMet() == false)) {
            sm._parent.Unlock();
            sm._Deactivate(IShell::CONDITIONS);
        } else {
            sm._parent.Unlock();
        }
    }

    void* Server::Service::StateMachine::ActivatedState::QueryInterface(StateMachine& sm, const uint32_t id, const bool asIUnknown)
    {
        // Take a ref-counted local copy of _handler under _pluginHandling.
        // This keeps _handler alive across the lock boundary even if
        // UnloadPlugin() runs concurrently on another thread.
        sm._parent._pluginHandling.Lock();

        if (id == PluginHost::IDispatcher::ID) {
            // Fast path: return the cached _jsonrpc pointer directly rather than
            // routing through _handler->QueryInterface(). This is intentional.
            // _jsonrpc is set once in AcquireInterfaces() via
            // _handler->QueryInterface<IDispatcher>() and is the same pointer the
            // plugin would return. Bypassing the plugin's own QueryInterface avoids
            // a potential lock inversion if the plugin's QI acquires internal locks.
            // Assumption: plugins do not conditionally expose IDispatcher at runtime.
            // If a plugin gates IDispatcher on runtime state, this fast path will
            // return the cached pointer regardless — revisit if that pattern emerges.
            IDispatcher* jsonrpc = sm._parent._jsonrpc;
            if (jsonrpc != nullptr) {
                jsonrpc->AddRef();
            }
            sm._parent._pluginHandling.Unlock();
            if (jsonrpc != nullptr) {
                return asIUnknown == false ? static_cast<void*>(jsonrpc) : static_cast<void*>(static_cast<Core::IUnknown*>(jsonrpc));
            }
            return nullptr;
        }

        IPlugin* handler = sm._parent._handler;
        if (handler != nullptr) {
            handler->AddRef();
        }
        sm._parent._pluginHandling.Unlock();

        if (handler == nullptr) {
            return nullptr;
        }

        void* result = handler->QueryInterface(id, asIUnknown);
        handler->Release();
        return result;
    }

    // -------------------------------------------------------------------------
    // HibernatedState
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::StateMachine::HibernatedState::Activate(StateMachine& sm, const reason why VARIABLE_IS_NOT_USED)
    {
        const Core::hresult result = sm._parent.Wakeup(3000);
        if (result == Core::ERROR_NONE) {
            sm.SetState(_stateActivated);
            sm._callback(IShell::ACTIVATED);
        }
        return result;
    }

    Core::hresult Server::Service::StateMachine::HibernatedState::Deactivate(StateMachine& sm, const reason why)
    {
        // Always update _current before dispatching regardless of Wakeup result.
        // If _current is not updated and Wakeup succeeds, _Deactivate dispatches
        // back to HibernatedState::Deactivate → infinite recursion → stack overflow.
        sm._parent.Wakeup(3000);       // best-effort, errors are non-fatal here
        sm.SetState(_stateActivated);  // _current must be updated before _Deactivate
        return sm._Deactivate(why);
    }

    // -------------------------------------------------------------------------
    // UnavailableState
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::StateMachine::UnavailableState::Deactivate(StateMachine& sm, const reason why)
    {
        sm._parent.Lock();
        sm._parent._reason = why;
        sm.SetState(_stateDeactivated);
        sm._parent.Unlock();
        sm._callback(IShell::DEACTIVATED);
        return Core::ERROR_NONE;
    }

    // -------------------------------------------------------------------------
    // Work methods — called by StateMachine, one concern each
    // -------------------------------------------------------------------------

    Core::hresult Server::Service::LoadPlugin()
    {
        ASSERT(_stateMachine.IsTransitionThread());
        // Pre: Lock() held by caller (state transition in progress)
        const string callSign(PluginHost::Service::Configuration().Callsign.Value());
        const string className(PluginHost::Service::Configuration().ClassName.Value());

        if (_handler == nullptr) {
            AcquireInterfaces();
        }

        if (_handler == nullptr) {
            SYSLOG(Logging::Startup, (_T("Loading of plugin [%s]:[%s], failed. Error [%s]"), className.c_str(), callSign.c_str(), ErrorMessage().c_str()));
            _reason = reason::INSTANTIATION_FAILED;
            return Core::ERROR_UNAVAILABLE;
        }

        if (_precondition.IsMet() == false) {
            SYSLOG(Logging::Startup, (_T("Activation of plugin [%s]:[%s], postponed, preconditions have not been met, yet."), className.c_str(), callSign.c_str()));

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

            return Core::ERROR_PENDING_CONDITIONS;
        }

        if (_lastId != 0) {
            _administrator.Destroy(_lastId);
            _lastId = 0;
        }

        TRACE(Activity, (_T("Activation plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));
        return Core::ERROR_NONE;
    }

    Core::hresult Server::Service::InitializePlugin()
    {
        ASSERT(_stateMachine.IsTransitionThread());
        // Pre: no service Lock() — blocking call
        const string callSign(PluginHost::Service::Configuration().Callsign.Value());
        const string className(PluginHost::Service::Configuration().ClassName.Value());

        REPORT_DURATION_WARNING({ ErrorMessage(_handler->Initialize(this)); }, WarningReporting::TooLongPluginState, WarningReporting::TooLongPluginState::StateChange::ACTIVATION, callSign.c_str());

        if (HasError() == true) {
            SYSLOG(Logging::Startup, (_T("Activation of plugin [%s]:[%s], failed. Error [%s]"), className.c_str(), callSign.c_str(), ErrorMessage().c_str()));

            if (_administrator.Configuration().LegacyInitialize() == false) {
                REPORT_DURATION_WARNING({ _handler->Deinitialize(this); }, WarningReporting::TooLongPluginState, WarningReporting::TooLongPluginState::StateChange::DEACTIVATION, callSign.c_str());
            }

            return Core::ERROR_GENERAL;
        }

        SYSLOG(Logging::Startup, (_T("Activated plugin [%s]:[%s]"), className.c_str(), callSign.c_str()));
        return Core::ERROR_NONE;
    }

    void Server::Service::DeinitializePlugin()
    {
        ASSERT(_stateMachine.IsTransitionThread());
        // Pre: no service Lock() — blocking call
        const string callSign(PluginHost::Service::Configuration().Callsign.Value());
        REPORT_DURATION_WARNING({ _handler->Deinitialize(this); }, WarningReporting::TooLongPluginState, WarningReporting::TooLongPluginState::StateChange::DEACTIVATION, callSign.c_str());
        SYSLOG(Logging::Shutdown, (_T("Deactivated plugin [%s]:[%s]"), ClassName().c_str(), callSign.c_str()));
    }

    void Server::Service::UnloadPlugin()
    {
        ASSERT(_stateMachine.IsTransitionThread());
        // Pre: Lock() held by caller
        ReleaseInterfaces();
    }

    void Server::Service::Attach()
    {
        ASSERT(_stateMachine.IsTransitionThread());
        // Pre: no locks held
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

        _stateControl = _handler->QueryInterface<PluginHost::IStateControl>();
        if (_stateControl != nullptr) {
            _stateControl->Register(&_composit);
            if (Resumed() == true) {
                _stateControl->Request(PluginHost::IStateControl::RESUME);
            }
        }
    }

    void Server::Service::Detach()
    {
        ASSERT(_stateMachine.IsTransitionThread());
        // Pre: no locks held — reverse of Attach()
        if (_stateControl != nullptr) {
            _stateControl->Unregister(&_composit);
            _stateControl->Release();
            _stateControl = nullptr;
        }

        if ((PluginHost::Service::Configuration().WebUI.IsSet()) || (PluginHost::Service::Configuration().WebUI.Value().empty() == false)) {
            DisableWebServer();
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

    uint32_t Server::Service::RequestResume()
    {
        ASSERT(_stateMachine.IsTransitionThread());

        uint32_t result = Core::ERROR_NONE;
        PluginHost::IStateControl* stateControl = nullptr;

        Lock();
        if (_stateControl == nullptr) {
            result = Core::ERROR_BAD_REQUEST;
        } else {
            stateControl = _stateControl;
        }
        Unlock();

        if (stateControl != nullptr) {
            if (stateControl->State() == PluginHost::IStateControl::SUSPENDED) {
                result = stateControl->Request(PluginHost::IStateControl::RESUME);
            }
        }

        return result;
    }

    uint32_t Server::Service::RequestSuspend()
    {
        ASSERT(_stateMachine.IsTransitionThread());

        uint32_t result = Core::ERROR_NONE;
        PluginHost::IStateControl* stateControl = nullptr;

        Lock();
        if (_stateControl == nullptr) {
            result = Core::ERROR_BAD_REQUEST;
        } else {
            stateControl = _stateControl;
        }
        Unlock();

        if (stateControl != nullptr) {
            if (stateControl->State() == PluginHost::IStateControl::RESUMED) {
                result = stateControl->Request(PluginHost::IStateControl::SUSPEND);
            }
        }

        return result;
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
                    // Updates base class _state only — _current in StateMachine is NOT updated.
                    // Callers (HibernatedState::Activate and HibernatedState::Deactivate) are
                    // responsible for calling SetState(_stateActivated) immediately after Wakeup()
                    // returns to close the window where State() == ACTIVATED but _current still
                    // points to HibernatedState (which returns nullptr from QueryInterface).
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
        _processAdministrator.Open();
        // Load the metadata for the subsystem information..
        if (Configuration().MetadataDiscovery() == false) {
            SYSLOG(Logging::Startup, (_T("Automatic metadata discovery and plugin versioning is DISABLED!!!")));
        }
        else {
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
    }

    void Server::ServiceMap::Close()
    {
        // coverity[ATOMICITY] - Lock is intentionally dropped around Deactivate() which can
        // block. The iterator is refreshed after re-acquiring the lock each iteration.
        // This is correct lock-straddling, not a race.
        _adminLock.Lock();

        Core::ProxyType<Service> controller(_server.Controller());

        TRACE_L1("Destructing %d plugins", static_cast<uint32_t>(_services.size()));

        // first we move all non priority plugins to deactivated, 
        auto index = _services.begin();
        while (index != _services.end()) {

            Core::ProxyType<Service> service(index->second);

            ASSERT(service.IsValid());

            if ((service->PriorityStart() == false) && (index->first != controller->Callsign())) {
                _adminLock.Unlock();

                index->second->Deactivate(PluginHost::IShell::SHUTDOWN);

                _adminLock.Lock();

                index = _services.erase(index);
            } else {
                ++index;
            }

        }

        // now we do the priority ones that have no specific order
        index = _services.begin();
        while (index != _services.end()) {

            Core::ProxyType<Service> service(index->second);

            ASSERT(service.IsValid());

            if ((std::find(_prioritystartorder.begin(), _prioritystartorder.end(), index->first) == _prioritystartorder.end()) && (index->first != controller->Callsign()))
            {
                _adminLock.Unlock();

                index->second->Deactivate(PluginHost::IShell::SHUTDOWN);

                _adminLock.Lock();

                index = _services.erase(index);
            } else {
                ++index;
            }

        }

        // and now the priority ones with order in reverse order
        for (auto it = _prioritystartorder.rbegin(); it != _prioritystartorder.rend(); ++it) {
            Plugins::iterator index = _services.find(*it);
            if (index != _services.end()) {
                _adminLock.Unlock();
                index->second->Deactivate(PluginHost::IShell::SHUTDOWN);
                _adminLock.Lock();
                _services.erase(index);
            } 
        }

        // and now only the controller is left...
        ASSERT((_services.size() == 1) && (_services.begin()->first == controller->Callsign()));
        _services.clear();

        _adminLock.Unlock();

        // Now deactivate controller plugin, once other plugins are deactivated
        controller->Deactivate(PluginHost::IShell::SHUTDOWN);

#ifdef __DEBUG__
        TRACE_L1("Pending notifiers are %u", _notifiers.Size());
        _notifiers.Visit([](const PluginHost::IPlugin::INotification* notification) {
            ASSERT(notification != nullptr);
            TRACE_L1("   -->  %s", Core::ClassNameOnly(typeid(*notification).name()).Text().c_str());
        });
#endif

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

    void Server::ServiceMap::ActivateService(Core::ProxyType<PluginHost::Server::Service>& service)
    {
        ASSERT(service.IsValid() == true);

        if ((service->State() != PluginHost::Service::state::UNAVAILABLE) && (service->State() != PluginHost::Service::state::ACTIVATED)) { // 2nd prevents the controller from tried to activate twice
            if (service->StartMode() == PluginHost::IShell::startmode::ACTIVATED) {
                SYSLOG(Logging::Startup, (_T("Activating plugin [%s]:[%s]"), service->ClassName().c_str(), service->Callsign().c_str()));
                service->Activate(PluginHost::IShell::STARTUP);
            } else {
                SYSLOG(Logging::Startup, (_T("Activation of plugin [%s]:[%s] delayed, start mode is %s"), service->ClassName().c_str(), service->Callsign().c_str(), Core::EnumerateType<PluginHost::IShell::startmode>(service->StartMode()).Data()));
            }
        }
    }

    bool Server::ServiceMap::AutoActivateAllowed(Core::ProxyType<PluginHost::Server::Service>& service) const
    {
        ASSERT(service.IsValid() == true);

        return (service->AutoActivationAlwaysEnabled() || (_disablePluginAutoActivation == false));
    }

    void Server::ServiceMap::Startup() {

        //first we start the priority start plugins in the requested order (if any)
        for (const string& prioservice : _prioritystartorder) {
            if (prioservice != PluginHost::Config::AllExtensionsAuthorized()) {
                Plugins::iterator index = _services.find(prioservice);
                if ((index != _services.end()) && (AutoActivateAllowed(index->second) == true)) {
                    ActivateService(index->second);
                } 
            }
        }

        // sort plugins based on StartupOrder from configuration
        std::vector<Core::ProxyType<Service>> configured_services;

        bool needssorting = false;

        for (auto& service : _services) {  
            if (service.second->PriorityStart() == true) {
                if ((AutoActivateAllowed(service.second) == true) && (std::find(_prioritystartorder.begin(), _prioritystartorder.end(), service.second->Callsign()) == _prioritystartorder.end())) {
                    ActivateService(service.second);
                }
            } else if (AutoActivateAllowed(service.second) == true) {
                configured_services.emplace_back(service.second);
                if (service.second->StartupOrderSet() == true) {
                    needssorting = true;
                }
            }
        }

        if ((needssorting == true) && (configured_services.size() != 0)) {
            std::sort(configured_services.begin(), configured_services.end(),
                [](const Core::ProxyType<Service>& lhs, const Core::ProxyType<Service>& rhs) {
                    return lhs->StartupOrder() < rhs->StartupOrder();
                });
        }

        for (auto& service : configured_services) {
            ActivateService(service);
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
        , _serviceCleanedUp(false)
    {
        TRACE(Activity, (_T("Construct a link with ID: [%d] to [%s]"), Id(), remoteId.QualifiedName().c_str()));

        _jobs.Slots(static_cast<ChannelMap&>(*parent).MaxRequests());
    }

    /* virtual */ Server::Channel::~Channel()
    {
        TRACE(Activity, (_T("Destruct a link with ID [%d] to [%s]"), Id(), RemoteId().c_str()));

        // If we are still attached to a service, detach, we are out of scope...
        CleanupService();

        if (_security != nullptr) {
            _security->Release();
            _security = nullptr;
        }

        Close(Core::infinite);
    }

    void Server::InsertLoadPluginConfig(Core::JSON::ArrayType<Plugin::Config>::Iterator index, Plugin::Config& metaDataConfig, const bool thunderextension, const bool background)
    {
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
                        fprintf(stdout, "Configuration error. Controller is defined multiple times [%s].\n", entry.Callsign.Value().c_str());
                    }
                }
            } else {
                _services.Insert(entry, Service::mode::CONFIGURED, thunderextension);
            }
        }
    }

    //
    // class Server
    // -----------------------------------------------------------------------------------------------------------------------------------
    PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    Server::Server(Config& configuration, const bool background)
        : _dispatcher(configuration.ThreadPoolCount(), configuration.StackSize(), configuration.QueueSize(), configuration.LowPriorityThreadCount(), configuration.MediumPriorityThreadCount())
        , _config(configuration)
        , _connections(*this, configuration.Binder())
        , _services(*this)
        , _controller()
        , _factoriesImplementation()
    {
        IFactories::Assign(&_factoriesImplementation);

        // See if the persistent path for our-selves exist, if not we will create it :-)
        Core::File persistentPath(_config.PersistentPath() + PluginOverrideDirectory);

        if (persistentPath.IsDirectory() == false) {
            Core::Directory(persistentPath.Name().c_str()).Create();
        }

        // Lets assign a workerpool, we created it...
        Core::WorkerPool::Assign(&_dispatcher);

        Plugin::Config metaDataConfig;

        metaDataConfig.ClassName = Core::ClassNameOnly(typeid(Plugin::Controller).name()).Text();

        Core::JSON::ArrayType<Plugin::Config>::Iterator index = configuration.Extensions();

        InsertLoadPluginConfig(index, metaDataConfig, true, background);

        index = configuration.Plugins();

        InsertLoadPluginConfig(index, metaDataConfig, false, background);

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
        _controller = _services.Insert(metaDataConfig, Service::mode::CONFIGURED, true);

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

        SYSLOG(Logging::Startup, (_T("Activating controller")));
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

        ASSERT(destructor != nullptr);

        if (destructor != nullptr) {
            destructor->AddRef();
            _connections.Close(100);
            destructor->Stopped();
            _services.Close();
            _dispatcher.Stop();
            destructor->Release();
        }
        _inputHandler.Deinitialize();
        _connections.Close(Core::infinite);

    }
}
}
