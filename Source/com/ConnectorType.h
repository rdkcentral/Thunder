#pragma once

#include "Module.h"
#include "Communicator.h"

namespace WPEFramework {
namespace RPC {

    EXTERNAL Core::ProxyType<RPC::IIPCServer> DefaultInvokeServer();
    EXTERNAL Core::ProxyType<RPC::IIPCServer> WorkerPoolInvokeServer();

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

            Core::ProxyType<Channel> channel = _comChannels.Instance(nodeId, ENGINE());

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
