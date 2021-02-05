#pragma once
#include "Module.h"

namespace WPEFramework {
namespace RPC {
    static Core::ProxyType<RPC::IIPCServer> DefaultInvokeServer()
    {
        static Core::ProxyType<RPC::IIPCServer> instance = Core::ProxyType<RPC::IIPCServer>(Core::SingletonProxyType<RPC::InvokeServerType<1, 0, 8>>::Instance());
        return (instance);
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
                handler->Announcements(CommunicatorClient::Announcement());
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
            if (_engine.IsValid() == false) {
                _engine = ENGINE();
            }
        }
        ~ConnectorType() = default;

    public:
        template <typename INTERFACE>
        void Aquire(const uint32_t waitTime, const Core::NodeId& nodeId, const string& className, const uint32_t version, INTERFACE*& result)
        {
            result = nullptr;

            ASSERT(_engine.IsValid() == true);

            Core::ProxyType<Channel> channel = _comChannels.Instance(nodeId, _engine);

            if (channel.IsValid() == true) {
                result = channel->CommunicatorClient::Aquire<INTERFACE>(waitTime, className, version);
            }
        }
        RPC::IIPCServer& Engine()
        {
            // The engine has to be running :-)
            ASSERT(_engine.IsValid() == true);

            return (*_engine);
        }

    private:
        Core::ProxyMapType<Core::NodeId, Channel> _comChannels;

        static Core::ProxyType<RPC::IIPCServer> _engine;
    };

    template <Core::ProxyType<RPC::IIPCServer> ENGINE()>
    EXTERNAL_HIDDEN typename Core::ProxyType<RPC::IIPCServer> ConnectorType<ENGINE>::_engine;
} // namespace RPC
} // namespace WPEFramework