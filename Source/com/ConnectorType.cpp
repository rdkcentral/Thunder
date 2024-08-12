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
#include "ConnectorType.h"

namespace Thunder {

namespace RPC {

Core::ProxyType<RPC::IIPCServer> DefaultInvokeServer()
{
    class Engine : public RPC::InvokeServerType<1, 0, 8> {
    public:
        Engine() = default;
        ~Engine() override = default;

    protected:
        void Acquire(Core::ProxyType<Engine>& source VARIABLE_IS_NOT_USED) {
            RPC::InvokeServerType<1, 0, 8>::Run();
        }
        void Relinquish(Core::ProxyType<Engine>& source VARIABLE_IS_NOT_USED) {
            RPC::InvokeServerType<1, 0, 8>::Stop();
        }
    };
    static Core::ProxyType<Engine> engine = Core::ProxyType<Engine>::Create();
    return Core::ProxyType<RPC::IIPCServer>(engine);
}

Core::ProxyType<RPC::IIPCServer> WorkerPoolInvokeServer()
{
    return (Core::ProxyType<RPC::IIPCServer>(Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance())));
}

ConnectorController::ConnectorController() : _controller(nullptr) {
}

ConnectorController::~ConnectorController() {
    ASSERT(_controller == nullptr);
}

/* static */ ConnectorController ConnectorController::_instance;

} } 
