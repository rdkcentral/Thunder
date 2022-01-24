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
#include "Module.h"
#include "Connector.h"
#include "plugins/IShell.h"

namespace WPEFramework {
namespace Connector {

    /* static */ Connector& Connector::Instance()
    {
        static Connector* instance = new Connector;
        ASSERT(instance!=nullptr);
        return(*instance);
    }
    void Connector::Announce(PluginHost::IShell* controller)
    {
        ASSERT(_controller==nullptr);
        if(controller != nullptr){
            controller->AddRef();
            _controller = controller;
        }
    }
    void Connector::Revoke(PluginHost::IShell* controller)
    {
        ASSERT(_controller!=nullptr);
        ASSERT(_controller==controller);

        if(controller != nullptr){
            controller->Release();
            _controller = nullptr;
        }
    }
    PluginHost::IShell* Connector::Controller()
    {
        if(_controller != nullptr){
            _controller->AddRef();
        }
        return _controller;
    }
    Connector::~Connector()
    {
        ASSERT(_controller==nullptr);
    }

} // namespace Connector
} // namespace WPEFramework

extern "C" {
    void connector_announce(WPEFramework::PluginHost::IShell* controller)
    {
        WPEFramework::Connector::Connector::Instance().Announce(controller);
    }
    void connector_revoke(WPEFramework::PluginHost::IShell* controller)
    {
        WPEFramework::Connector::Connector::Instance().Revoke(controller);
    }
    WPEFramework::PluginHost::IShell* connector_controller()
    {
        return(WPEFramework::Connector::Connector::Instance().Controller());
    }
}
