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
            void Register(IShell* controller, const string& callsign)
            {
                _adminLock.Lock();
                _callsign = callsign;
                _state = state::REGISTRING;
                _adminLock.Unlock();

                controller->Register(this);

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
            void Unregister(IShell* controller)
            {
                _adminLock.Lock();

                controller->Unregister(this);
                _callsign.clear();

                if (_designated != nullptr) {

                    _designated->Release();
                    _designated = nullptr;

                    _parent.Deactivated();
                }

                _adminLock.Unlock();
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
        void Register(PluginHost::IShell* controller, const string& callsign)
        {
            _sink.Register(controller, callsign);
        }
        void Unregister(PluginHost::IShell* controller)
        {
            _sink.Unregister(controller);
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

namespace RPC {

    template <typename INTERFACE, Core::ProxyType<RPC::IIPCServer> ENGINE() = DefaultInvokeServer>
    class SmartInterfaceType {
    private:
        using Monitor = PluginHost::PluginMonitorType<INTERFACE, SmartInterfaceType<INTERFACE, ENGINE>&>;

    public:
        SmartInterfaceType(const SmartInterfaceType<INTERFACE, ENGINE>&) = delete;
        SmartInterfaceType<INTERFACE, ENGINE>& operator=(const SmartInterfaceType<INTERFACE, ENGINE>&) = delete;

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
}
}
