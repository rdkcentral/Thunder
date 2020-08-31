/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "IShell.h"

namespace WPEFramework {
namespace PluginHost {

    template <typename INTERFACE>
    class PluginInterfaceType {
    private:
        class Notification
            : public PluginHost::IPlugin::INotification {

        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(PluginInterfaceType<INTERFACE>& parent)
                : _parent(parent)
                , _queriedInterface(nullptr)
                , _plugins()
                , _callsign()
                , _scanned(false)
                , _lock()
            {
            }

            ~Notification()
            {
            }

        public:
            void Set(const string& name)
            {
                _callsign = name;
            }

            void Process(PluginHost::IShell* plugin)
            {
                if (plugin->State() == PluginHost::IShell::ACTIVATED) {
                    ASSERT(_queriedInterface == nullptr);

                    _queriedInterface = plugin->QueryInterface<INTERFACE>();

                    if (_queriedInterface) {
                        _parent.Activated(_queriedInterface);
                    }

                } else if (_queriedInterface != nullptr && plugin->State() == PluginHost::IShell::DEACTIVATION) {
                    _parent.Deactivated(_queriedInterface);
                    _queriedInterface->Release();
                    _queriedInterface = nullptr;
                }
            }

            void StateChange(PluginHost::IShell* plugin) override
            {
                _lock.Lock();

                if (_scanned == false) {

                    plugin->AddRef();
                    _plugins.push_back(plugin);

                } else if (plugin->Callsign() == _callsign) {
                    Process(plugin);
                }

                _lock.Unlock();
            }

            void Process()
            {
                _lock.Lock();

                _scanned = true;

                for (auto& plugin : _plugins) {

                    if ((_queriedInterface == nullptr) && (plugin->Callsign() == _callsign)) {
                        Process(plugin);
                    }

                    plugin->Release();
                }

                _plugins.clear();

                _lock.Unlock();
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            PluginInterfaceType<INTERFACE>& _parent;
            INTERFACE* _queriedInterface;
            std::list<PluginHost::IShell*> _plugins;
            std::string _callsign;
            bool _scanned;
            Core::CriticalSection _lock;
        };

        class Job {
        public:
            Job(Core::Sink<Notification>& sink)
                : _shell(nullptr)
                , _sink(sink)
            {
            }

            void Set(PluginHost::IShell* shell)
            {
                if (_shell != nullptr) {
                    _shell->Unregister(&_sink);
                }

                ASSERT((_shell == nullptr) ^ (shell == nullptr));

                shell->AddRef();
                _shell = shell;
            }

            ~Job()
            {
                if (_shell != nullptr) {
                    _shell->Unregister(&_sink);
                    _shell->Release();
                }
            }

        public:
            void Dispatch()
            {
                _shell->Register(&_sink);
                _sink.Process();
            }

        private:
            PluginHost::IShell* _shell;
            Core::Sink<Notification>& _sink;
        };

    public:
        PluginInterfaceType(const PluginInterfaceType<INTERFACE>&) = delete;
        PluginInterfaceType<INTERFACE>& operator=(const PluginInterfaceType<INTERFACE>&) = delete;

        PluginInterfaceType()
            : _actualInterface(nullptr)
            , _sink(*this)
            , _job(_sink)
        {
        }

        void Activate(PluginHost::IShell* shell, const string& name)
        {
            _sink.Set(name);
            static_cast<Job&>(_job).Set(shell);
            _job.Submit();
        }

        void Deactivate()
        {
            static_cast<Job&>(_job).Set(nullptr);
            _job.Revoke();
        }

        ~PluginInterfaceType()
        {
            Deactivate();
        }

        virtual void Activated(INTERFACE*) = 0;

        virtual void Deactivated(INTERFACE*) = 0;

    private:
        INTERFACE* _actualInterface;
        Core::Sink<Notification> _sink;
        Core::WorkerPool::JobType<Job> _job;
    };
}
}
