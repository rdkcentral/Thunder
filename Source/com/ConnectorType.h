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
#include "Communicator.h"

namespace WPEFramework {
namespace RPC {

    EXTERNAL Core::ProxyType<RPC::IIPCServer> DefaultInvokeServer();
    EXTERNAL Core::ProxyType<RPC::IIPCServer> WorkerPoolInvokeServer();

    // This class is not thread safe. It is assumed that the Controller IUnknown
    // is always set prior to any retrieval (WPEFramework/WPEProcess startup) and
    // the interface is only revoked at process hsutdown, after shutting down all
    // the other class (shutdown of WPEFrmaework or WPEProcess. So no need to
    // lock the access to the _controller member variable.
    class EXTERNAL ConnectorController {
    private:
        ConnectorController();

    public:
        ConnectorController(const ConnectorController&) = delete;
        ConnectorController& operator=(const ConnectorController&) = delete;

        ~ConnectorController();

        static ConnectorController& Instance()
        {
            return(_instance);
        }

    public:
        void Announce(Core::IUnknown* controller)
        {
            ASSERT(_controller == nullptr);

            if (controller != nullptr){
                _controller = controller;
                _controller->AddRef();
            }
        }
        void Revoke(Core::IUnknown* controller VARIABLE_IS_NOT_USED)
        {
            ASSERT(_controller == controller);

            if(_controller != nullptr){
                _controller->Release();
                _controller = nullptr;
            }
        }

        Core::IUnknown* Controller()
        {
            if(_controller != nullptr){
                _controller->AddRef();
            }

            return (_controller);
        }

    private:
        Core::IUnknown* _controller;
        static ConnectorController _instance;
    };

    template <Core::ProxyType<RPC::IIPCServer> ENGINE() = DefaultInvokeServer>
    class ConnectorType {
    private:
        class Channel : public CommunicatorClient {
        public:
            Channel() = delete;
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;

            Channel(const Core::NodeId& remoteNode, const Core::ProxyType<RPC::IIPCServer>& handler)
                : CommunicatorClient(remoteNode, Core::ProxyType<Core::IIPCServer>(handler))
            {
            }
            ~Channel() override = default;

        public:
            uint32_t Initialize()
            {
                return (CommunicatorClient::Open(Core::infinite));
            }
            void Deintialize()
            {
                CommunicatorClient::Close(Core::infinite);
            }
        };
    public:
        ConnectorType(const ConnectorType<ENGINE>&) = delete;
        ConnectorType<ENGINE>& operator=(const ConnectorType<ENGINE>&) = delete;

        ConnectorType()
            : _comChannels()
        {
        }
        ~ConnectorType() = default;

    public:
        template <typename INTERFACE>
        INTERFACE* Aquire(const uint32_t waitTime, const Core::NodeId& nodeId, const string className, const uint32_t version)
        {
            INTERFACE* result = nullptr;

            Core::ProxyType<Channel> channel = _comChannels.template Instance<Channel>(nodeId, nodeId, ENGINE());

            if (channel.IsValid() == true) {
                result = channel->template Aquire<INTERFACE>(waitTime, className, version);
            }

            return (result);
        }
        Core::ProxyType<CommunicatorClient> Communicator(const Core::NodeId& nodeId){
            return _comChannels.Find(nodeId);
        }
        RPC::IIPCServer& Engine()
        {
            return *ENGINE();
        }

    private:
        Core::ProxyMapType<Core::NodeId, Channel> _comChannels;
    };

} // namespace RPC
} // namespace WPEFramework
