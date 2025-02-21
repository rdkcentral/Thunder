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

#include "IPlugin.h"
#include "IShell.h"

namespace WPEFramework {
namespace PluginHost {

    template <typename INTERFACE, typename HANDLER>
    class PluginMonitorType {
    private:
        class Sink : public PluginHost::IPlugin::INotification {
        private:
            enum state : uint8_t {
                RUNNING,
                REGISTRING,
                LOADED
            };

        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;
            Sink(Sink&&) = delete;
            Sink& operator=(Sink&&) = delete;

            Sink(PluginMonitorType<INTERFACE, HANDLER>& parent)
                : _adminLock()
                , _parent(parent)
                , _state(state::RUNNING)
                , _callsign()
                , _designated(nullptr)
            {
            }
            ~Sink() override = default;

        public:
            bool IsOperational() const
            {
                return (_designated != nullptr);
            }
            // set callsign (must be called before Register, cannot be used to change callsign when monitoring active)
            void Callsign(const string& callsign) 
            {
                _callsign = callsign;
            }
            void Register(IShell* shell)
            {
                ASSERT(shell != nullptr);
                _adminLock.Lock();
                _state = state::REGISTRING;
                _adminLock.Unlock();

                shell->Register(this);

                _adminLock.Lock();
                if (_state == state::LOADED) {

                    INTERFACE* entry = _designated->QueryInterface<INTERFACE>();
                    _designated->Release();
                    _designated = entry;
                    _state = state::RUNNING;
                    if (entry != nullptr) {
                        _parent.Activated(entry);
                    }
                } else {
                    _state = state::RUNNING;
                }
                _adminLock.Unlock();
            }
            void Unregister(IShell* shell)
            {
                ASSERT(shell != nullptr);

                if (shell != nullptr) {

                    _adminLock.Lock();
                    shell->Unregister(this);
                    _callsign.clear();

                    if (_designated != nullptr) {

                        _designated->Release();
                        _designated = nullptr;

                        _parent.Deactivated();
                    }

                    _adminLock.Unlock();
                }            
            }
            INTERFACE* Interface()
            {
                INTERFACE* result = nullptr;

                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                if ((_state == state::RUNNING) && (_designated != nullptr)) {
                    result = _designated->QueryInterface<INTERFACE>();
                }

                return (result);
            }
            const INTERFACE* Interface() const
            {
                const INTERFACE* result = nullptr;

                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                if ((_state == state::RUNNING) && (_designated != nullptr)) {
                    result = _designated->QueryInterface<INTERFACE>();
                }

                return (result);
            }

            BEGIN_INTERFACE_MAP(Sink)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            void Activated(const string& name, PluginHost::IShell* plugin) override
            {
                if (_callsign == name) {

                    ASSERT(_designated == nullptr);

                    _adminLock.Lock();
                    if (_state == state::REGISTRING) {

                        _state = state::LOADED;
                        _designated = plugin;
                        _designated->AddRef();
                    } else {
                        INTERFACE* entry = plugin->QueryInterface<INTERFACE>();
                        _designated = entry;

                        ASSERT(_state == state::RUNNING);

                        if (entry != nullptr) {
                            _parent.Activated(entry);
                        }
                    }
                    _adminLock.Unlock();
                }
            }
            void Deactivated(const string& name, PluginHost::IShell* plugin) override
            {
                if (_callsign == name) {

                    _adminLock.Lock();
                    if (_designated != nullptr) {

                        _designated->Release();
                        _designated = nullptr;

                        if (_state != state::RUNNING) {
                            _state = state::REGISTRING;
                        } else {
                            _parent.Deactivated();
                        }
                    }
                    _adminLock.Unlock();
                }
            }
            void Unavailable(const string& name, PluginHost::IShell* plugin) override
            {
            }

        private:
            mutable Core::CriticalSection _adminLock;
            PluginMonitorType<INTERFACE, HANDLER>& _parent;
            state _state;
            string _callsign;
            Core::IUnknown* _designated;
        };

    public:
        PluginMonitorType() = delete;
        PluginMonitorType(const PluginMonitorType<INTERFACE, HANDLER>&) = delete;
        PluginMonitorType<INTERFACE, HANDLER>& operator=(const PluginMonitorType<INTERFACE, HANDLER>&) = delete;
        PluginMonitorType(PluginMonitorType<INTERFACE, HANDLER>&&) = delete;
        PluginMonitorType<INTERFACE, HANDLER>& operator=(PluginMonitorType<INTERFACE, HANDLER>&&) = delete;

        template <typename... Args>
        PluginMonitorType(Args&&... args)
            : _reporter(std::forward<Args>(args)...)
            , _sink(*this)
        {
        }
        ~PluginMonitorType() = default;

    public:
        inline bool IsOperational() const
        {
            return (_sink.IsOperational());
        }
        // set callsign (must be called before Register, cannot be used to change callsign when monitoring active)
        void Callsign(const string& callsign) 
        {
            _sink.Callsign(callsign);
        }
        void Register(PluginHost::IShell* shell)
        {
            _sink.Register(shell);
        }
        void Register(PluginHost::IShell* shell, const string& callsign)
        {
            _sink.Callsign(callsign);
            _sink.Register(shell);
        }
        void Unregister(PluginHost::IShell* shell)
        {
            _sink.Unregister(shell);
        }
        INTERFACE* Interface()
        {
            return (_sink.Interface());
        }
        const INTERFACE* Interface() const
        {
            return (_sink.Interface());
        }

    private:
        void Activated(INTERFACE* element)
        {
            _reporter.Activated(element);
        }
        void Deactivated()
        {
            _reporter.Deactivated();
        }

    private:
        HANDLER _reporter;
        Core::Sink<Sink> _sink;
    };
}


/*
*
* Use the SmartInterfaceType to interact with (a) COMRPC interface(s) exposed by a plugin (identified by its callsign).
* This class will handle the situation where that plugin could be activated and deactivated.
* This class is intended to be used from code which is NOT a plugin.
* If you require this functionality from inside a plugin please see the PluginSmartInterfaceType below.
*
*/

namespace RPC {

    template <typename INTERFACE, Core::ProxyType<RPC::IIPCServer> ENGINE() = DefaultInvokeServer>
    class SmartInterfaceType {
    private:
        using Monitor = PluginHost::PluginMonitorType<INTERFACE, SmartInterfaceType<INTERFACE, ENGINE>&>;

    public:
        SmartInterfaceType(const SmartInterfaceType<INTERFACE, ENGINE>&) = delete;
        SmartInterfaceType<INTERFACE, ENGINE>& operator=(const SmartInterfaceType<INTERFACE, ENGINE>&) = delete;
        SmartInterfaceType(SmartInterfaceType<INTERFACE, ENGINE>&&) = delete;
        SmartInterfaceType<INTERFACE, ENGINE>& operator=(SmartInterfaceType<INTERFACE, ENGINE>&&) = delete;

        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        SmartInterfaceType()
            : _controller(nullptr)
            , _administrator()
            , _monitor(*this)
            , _connectionId(~0)
        {
        }
POP_WARNING()
        virtual ~SmartInterfaceType()
        {
            ASSERT(_controller == nullptr);
        }

    public:
        uint32_t ConnectionId() const {
            return (_connectionId);
        }
        bool IsOperational() const
        {
            return (_monitor.IsOperational());
        }
        uint32_t Open(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        {
            Core::IUnknown* controller = RPC::ConnectorController::Instance().Controller();

            ASSERT(_controller == nullptr);

            if (controller != nullptr) {
                // Seems like we already have a connection to the IShell of the Controller plugin, reuse it.
                _controller = controller->QueryInterface<PluginHost::IShell>();
                controller->Release();
            }
            if (_controller == nullptr) {
                _controller = _administrator.template Acquire<PluginHost::IShell>(waitTime, node, _T(""), ~0);
            }
            if (_controller != nullptr) {
                _monitor.Register(_controller, callsign);
                Core::ProxyType<CommunicatorClient> channel(_administrator.Communicator(node));
                if (channel.IsValid() == true) {
                    _connectionId = channel->ConnectionId();
                }
            }

            return (_controller != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        uint32_t Close(const uint32_t waitTime)
        {
            if (_controller != nullptr) {
                _monitor.Unregister(_controller);
                _controller->Release();
                _controller = nullptr;
            }

            return (Core::ERROR_NONE);
        }
        template <typename EXPECTED_INTERFACE>
        EXPECTED_INTERFACE* Acquire(const uint32_t waitTime, const Core::NodeId& nodeId, const string className, const uint32_t version = ~0)
        {
            return (_administrator.template Acquire<EXPECTED_INTERFACE>(waitTime, nodeId, className, version));
        }

        INTERFACE* Interface()
        {
            return (_monitor.Interface());
        }
        const INTERFACE* Interface() const
        {
            return (_monitor.Interface());
        }
        PluginHost::IShell* ControllerInterface()
        {
            return _controller;
        }
        const PluginHost::IShell* ControllerInterface() const
        {
            return _controller;
        }

        // Allow a derived class to take action on a new interface, or almost dissapeared interface..
        virtual void Operational(const bool upAndRunning)
        {
        }

        static Core::NodeId Connector()
        {
            string comPath;

            if (Core::SystemInfo::GetEnvironment(_T("COMMUNICATOR_CONNECTOR"), comPath) == false) {
#ifdef __WINDOWS__
                comPath = _T("127.0.0.1:62000");
#else
                comPath = _T("/tmp/communicator");
#endif
            }

            return Core::NodeId(comPath.c_str());
        }

    private:
        friend Monitor;

        void Activated(INTERFACE* plugin)
        {
            Operational(true);
        }
        void Deactivated()
        {
            Operational(false);
        }

    private:
        PluginHost::IShell* _controller;
        ConnectorType<ENGINE> _administrator;
        Monitor _monitor;
        uint32_t _connectionId;
    };

/*
*
* Use the PluginSmartInterfaceType to interact with (a) COMRPC interface(s) exposed by a plugin (identified by its callsign).
* This class will handle the situation where that plugin could be activated and deactivated.
* This class is intended to be used from code which is a plugin. It can be used from both an in process as well as an 
* out of process plugin.
* WARNING: if you use this class in the out of process part of a plugin this plugin must have a minimum workerpool thread count
* of 2 otherwise deadlocks can occur!!!!
*
* If you require this functionality from code that is NOT a plugin please see the SmartInterfaceType above.
*
*/

    template <typename INTERFACE>
    class PluginSmartInterfaceType {
    private:
        using Monitor = PluginHost::PluginMonitorType<INTERFACE, PluginSmartInterfaceType<INTERFACE>&>;

    public:
        PluginSmartInterfaceType(const PluginSmartInterfaceType<INTERFACE>&) = delete;
        PluginSmartInterfaceType<INTERFACE>& operator=(const PluginSmartInterfaceType<INTERFACE>&) = delete;
        PluginSmartInterfaceType(PluginSmartInterfaceType<INTERFACE>&&) = delete;
        PluginSmartInterfaceType<INTERFACE>& operator=(PluginSmartInterfaceType<INTERFACE>&&) = delete;

        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        PluginSmartInterfaceType()
            : _shell(nullptr)
            , _monitor(*this)
            , _job(*this)
        {
        }
        POP_WARNING()
        virtual ~PluginSmartInterfaceType()
        {
            ASSERT(_shell == nullptr);
            ASSERT(_job.IsIdle() == true);
        }

        class RegisterJob { 
        public: 
            RegisterJob(RegisterJob&&) = delete;
            RegisterJob(const RegisterJob&) = delete;
            RegisterJob& operator=(const RegisterJob&) = delete;
            RegisterJob& operator=(RegisterJob&&) = delete;

            RegisterJob(PluginSmartInterfaceType& parent)
                : _parent(parent) 
            {
            }
            ~RegisterJob() = default;

        public:
            void Dispatch()
            {
                _parent.Register();
            }

        private:
            PluginSmartInterfaceType& _parent;
        };

         using Job = Core::IWorkerPool::JobType<RegisterJob>;

    public:
        bool IsOperational() const
        {
            return (_monitor.IsOperational());
        }
        uint32_t Open(PluginHost::IShell* shell, const string& callsign)
        {
            ASSERT(_shell == nullptr);
            ASSERT(_job.IsIdle() == true);

            if(shell != nullptr) {

                _shell = shell;
                _shell->AddRef();

                _monitor.Callsign(callsign);

                _job.Submit();

            }

            return (Core::ERROR_NONE);
        }
        uint32_t Close()
        {
            ASSERT(_shell != nullptr);

            if(_shell != nullptr) {
                _job.Revoke();
                _monitor.Unregister(_shell);
                _shell->Release();
                _shell = nullptr;
            }

            return (Core::ERROR_NONE);
        }

        INTERFACE* Interface()
        {
            return (_monitor.Interface());
        }
        const INTERFACE* Interface() const
        {
            return (_monitor.Interface());
        }
        PluginHost::IShell* PluginInterface()
        {
            _shell->AddRef();
            return _shell;
        }
        const PluginHost::IShell* PluginInterface() const
        {
            _shell->AddRef();
            return _shell;
        }

        // Allow a derived class to take action on a new interface, or almost dissapeared interface..
        virtual void Operational(const bool upAndRunning)
        {
        }

    private:
        friend Monitor;
        friend Job;

        void Activated(INTERFACE* plugin)
        {
            Operational(true);
        }
        void Deactivated()
        {
            Operational(false);
        }

        void Register() 
        {
            _monitor.Register(_shell);
        }

    private:
        PluginHost::IShell* _shell;
        Monitor _monitor;
        Job _job;
    };

    template <typename INTERFACE, Core::ProxyType<RPC::IIPCServer> ENGINE() = DefaultInvokeServer>
    class SmartControllerInterfaceType {

    public:
        SmartControllerInterfaceType(const SmartControllerInterfaceType<INTERFACE, ENGINE>&) = delete;
        SmartControllerInterfaceType<INTERFACE, ENGINE>& operator=(const SmartControllerInterfaceType<INTERFACE, ENGINE>&) = delete;

        SmartControllerInterfaceType()
            : _controller(nullptr)
            , _administrator()
            , _connectionId(~0)
        {
        }
        
        ~SmartControllerInterfaceType()
        {
            ASSERT(_controller == nullptr);
        }

    public:
        uint32_t ConnectionId() const {
            return (_connectionId);
        }
        bool IsOperational() const
        {
            return (_controller != nullptr);
        }
        uint32_t Open(const uint32_t waitTime, const Core::NodeId& node)
        {
            Core::IUnknown* controller = RPC::ConnectorController::Instance().Controller();

            ASSERT(_controller == nullptr);

            if (controller != nullptr) {
                // Seems like we already have a connection to the IShell of the Controller plugin, reuse it.
                _controller = controller->QueryInterface<PluginHost::IShell>();
                controller->Release();
            }
            if (_controller == nullptr) {
                _controller = _administrator.template Acquire<PluginHost::IShell>(waitTime, node, _T(""), ~0);
            }
            if (_controller != nullptr) {
                Core::ProxyType<CommunicatorClient> channel(_administrator.Communicator(node));
                if (channel.IsValid() == true) {
                    _connectionId = channel->ConnectionId();
                }
            }

            return (_controller != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        uint32_t Close(const uint32_t waitTime)
        {
            if (_controller != nullptr) {
                _controller->Release();
                _controller = nullptr;
            }

            return (Core::ERROR_NONE);
        }

        INTERFACE* Interface()
        {
            INTERFACE* result = nullptr;
            if(_controller != nullptr) {
                result = _controller->QueryInterface<INTERFACE>();
            }
            return result;
        }
        const INTERFACE* Interface() const
        {
            INTERFACE* result = nullptr;
            if(_controller != nullptr) {
                result = _controller->QueryInterface<INTERFACE>();
            }
            return result;
        }
        PluginHost::IShell* ControllerInterface()
        {
            _controller->AddRef();
            return _controller;
        }
        const PluginHost::IShell* ControllerInterface() const
        {
            _controller->AddRef();
            return _controller;
        }

        static Core::NodeId Connector()
        {
            return SmartInterfaceType<PluginHost::IShell>::Connector();
        }

    private:
        PluginHost::IShell* _controller;
        ConnectorType<ENGINE> _administrator;
        uint32_t _connectionId;
    };

}
}
