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

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class StateController
        : public PluginHost::IPlugin {

    private:
        class Entry {
        private:
            Entry() = delete;
            Entry(const Entry& copy) = delete;
            Entry& operator=(const Entry&) = delete;

        public:
            Entry(PluginHost::IStateControl* entry)
                : _stateController(entry)
            {
                ASSERT(_stateController != nullptr);
                _stateController->AddRef();
            }
            ~Entry()
            {
                _stateController->Release();
            }

            PluginHost::IStateControl* _stateController;
        };

    private:
        class Notification
            : public PluginHost::IPlugin::INotification {

        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(StateController* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            void StateChange(PluginHost::IShell* plugin) override
            {
                _parent.StateChange(plugin);
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            StateController& _parent;
        };

    public:
        StateController(const StateController&) = delete;
        StateController& operator=(const StateController&) = delete;

        #ifdef __WINDOWS__
        #pragma warning(disable : 4355)
        #endif
        StateController()
            : _adminLock()
            , _skipURL(0)
            , _service(nullptr)
            , _clients()
            , _sink(this)
        {
        }
        #ifdef __WINDOWS__
        #pragma warning(default : 4355)
        #endif

        ~StateController() override
        {
        }

    public:
        BEGIN_INTERFACE_MAP(StateController)
        INTERFACE_ENTRY(PluginHost::IPlugin)
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
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

    private:
        void StateChange(PluginHost::IShell* plugin);

        Core::CriticalSection _adminLock;
        uint32_t _skipURL;
        PluginHost::IShell* _service;
        std::map<const string, Entry> _clients;
        Core::Sink<Notification> _sink;
    };
} //namespace Plugin
} //namespace WPEFramework
