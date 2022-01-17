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
#include "RootShell.h"

namespace WPEFramework {

namespace RPC {

template <Core::ProxyType<RPC::IIPCServer> ENGINE() = DefaultInvokeServer>
    class Connector {
    
    public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        Connector()
            : _controller(nullptr)
            , _administrator()
        {
            _controller = PluginHost::RootShell::Instance().Get();
        }

#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
        ~Connector() {
        }

    public:
        PluginHost::IShell* ControllerInterface(const uint32_t waitTime, const Core::NodeId& node)
        {
            if (_controller == nullptr) {
                printf("We are not in-process with Thunder\n");
                return(_administrator.template Aquire<PluginHost::IShell>(waitTime, node, _T(""), ~0));
            }
            else
            {
                printf("We got the Ishell in-process\n");
            }
            return _controller;
        }
  
    private:
        PluginHost::IShell* _controller;
        ConnectorType<ENGINE> _administrator;
    };

}//namespace RPC
}//namespace WPEFramework