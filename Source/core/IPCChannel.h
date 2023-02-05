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

#ifndef __PROCESS_CHANNEL_H__
#define __PROCESS_CHANNEL_H__

#include "IPCConnector.h"
#include "Module.h"
#include "Number.h"
#include "Portability.h"
#include "ProcessInfo.h"
#include "Singleton.h"
#include "SystemInfo.h"
#include "Trace.h"
#include "TypeTraits.h"

namespace WPEFramework {
namespace Core {

    template <typename EXTENSION, const bool LISTENING, const bool INTERNALFACTORY>
    class IPCChannelClientType : public IPCChannelType<SocketPort, EXTENSION> {
    private:
        using BaseClass = IPCChannelType<SocketPort, EXTENSION>;

    public:
        IPCChannelClientType() = delete;
        IPCChannelClientType(IPCChannelClientType<EXTENSION, LISTENING, INTERNALFACTORY>&&) = delete;
        IPCChannelClientType(const IPCChannelClientType<EXTENSION, LISTENING, INTERNALFACTORY>&) = delete;
        IPCChannelClientType<EXTENSION, LISTENING, INTERNALFACTORY>& operator=(const IPCChannelClientType<EXTENSION, LISTENING, INTERNALFACTORY>&) = delete;

        template <const bool FACTORY = INTERNALFACTORY, EnableIfParameter<FACTORY == true> = 0>
        IPCChannelClientType(const NodeId& node, const uint32_t bufferSize)
            : IPCChannelType<SocketPort, EXTENSION>((LISTENING ? SocketPort::LISTEN : SocketPort::STREAM), (LISTENING ? node : node.AnyInterface()), (LISTENING ? node.AnyInterface() : node), bufferSize, bufferSize)
            , _factory(ProxyType<FactoryType<IIPC, uint32_t>>::Create())
        {
            static_assert(INTERNALFACTORY == true, "This constructor can only be called if you specify an INTERNAL factory");
            TRACE_L1("Created an internal factory communication channel, on %s as %s", node.QualifiedName().c_str(), (LISTENING == true ? _T("Server") : _T("Client")));

            ASSERT(_factory.IsValid() == true);

            IPCChannelType<SocketPort, EXTENSION>::Factory(_factory);
        }
        template <const bool FACTORY = INTERNALFACTORY, EnableIfParameter<FACTORY == false> = 0>
        IPCChannelClientType(const NodeId& node, const uint32_t bufferSize, ProxyType<FactoryType<IIPC, uint32_t>>& factory)
            : IPCChannelType<SocketPort, EXTENSION>((LISTENING ? SocketPort::LISTEN : SocketPort::STREAM), (LISTENING ? node : node.AnyInterface()), (LISTENING ? node.AnyInterface() : node), bufferSize, bufferSize)
            , _factory(factory)
        {
            static_assert(INTERNALFACTORY == false, "This constructor can only be called if you specify an EXTERNAL factory");
            TRACE_L1("Created an external factory communication channel, on %s as %s", node.QualifiedName().c_str(), (LISTENING == true ? _T("Server") : _T("Client")));

            ASSERT(_factory.IsValid() == true);

            IPCChannelType<SocketPort, EXTENSION>::Factory(_factory);
        }
        template <const bool FACTORY = INTERNALFACTORY, EnableIfParameter<FACTORY == false> = 0>
        IPCChannelClientType(const NodeId& node, const uint32_t bufferSize, ProxyType<FactoryType<IIPC, uint32_t>>& factory, SOCKET newSocket)
            : IPCChannelType<SocketPort, EXTENSION>(factory, SocketPort::STREAM, newSocket, node, bufferSize, bufferSize)
            , _factory(factory)
        {
            static_assert(INTERNALFACTORY == false, "This constructor can only be called if you specify an EXTERNAL factory");

            TRACE_L1(" A remote socket hooked up to an external factory communication channels as %s", (LISTENING == true ? _T("Server") : _T("Client")));

            ASSERT(_factory.IsValid() == true);

            // We are ready to communicate over this socket.
            BaseClass::Source().Open(0);
        }
        ~IPCChannelClientType() override
        {
            IPCChannelType<SocketPort, EXTENSION>::Source().Close(Core::infinite);

            if (INTERNALFACTORY == true) {
                _factory->DestroyFactories();
            }
        }

    public:
        uint32_t Open(const uint32_t waitTime)
        {
            return (BaseClass::Source().Open(waitTime));
        }
        uint32_t Close(const uint32_t waitTime)
        {
            return (BaseClass::Source().Close(waitTime));
        }
        bool IsOpen() const
        {
            return (BaseClass::Source().IsOpen());
        }
        bool IsClosed() const
        {
            return (BaseClass::Source().IsClosed());
        }
        template <typename ACTUALELEMENT>
        void CreateFactory(const uint32_t initialSize)
        {
            ASSERT(_factory.IsValid());
            ASSERT(BaseClass::Source().IsOpen() == false);

            static_assert(INTERNALFACTORY == true, "This method can only be called if you specify an INTERNAL factory");

            _factory->CreateFactory<ACTUALELEMENT>(initialSize);
        }
        template <typename ACTUALELEMENT>
        void DestroyFactory()
        {

            ASSERT(_factory.IsValid());
            ASSERT(BaseClass::Source().IsOpen() == false);

            static_assert(INTERNALFACTORY == true, "This method can only be called if you specify an INTERNAL factory");

            _factory->DestroyFactory<ACTUALELEMENT>();
        }

    protected:
        void StateChange() override
        {
            // Do not forget to call the base...
            BaseClass::StateChange();

            StateChange(TemplateIntToType<LISTENING>());
        }

    private:
        void StateChange(const TemplateIntToType<true>&)
        {
            if ((BaseClass::Source().HasError() == true) && (BaseClass::Source().IsListening() == false)) {

                TRACE_L1("Error on socket. Not much we can do except for closing up, Try to recover. (%d)", BaseClass::Source().State());
                // In case on an error, not much more we can do then close up..
                BaseClass::Source().Close(0);
            } else if ((BaseClass::Source().Type() == SocketPort::LISTEN) && (BaseClass::Source().IsForcedClosing() == false) && (BaseClass::Source().IsSuspended() == false)) {
                if (BaseClass::Source().IsListening() == true) {
                    // This potentially means, we have a new connection coming in, swap..
                    NodeId remoteHost = BaseClass::Source().Accept();

                    if (remoteHost.IsValid() == true) {
                        TRACE_L1("Moving into a connected mode. (%d)", 0);
                    }
                } else {
                    // If we did not request the close, we can move back to Listening on this socket..
                    TRACE_L1("Moving into listening mode. (%d)", 0);

                    // Oops this means we are closed. Try to get a new connection.
                    BaseClass::Source().Listen();
                }
            }
        }
        void StateChange(const TemplateIntToType<false>&)
        {
            if (BaseClass::Source().HasError() == true) {
                TRACE_L1("Error on socket. Not much we can do except for closing up, Try to recover. (%d)", BaseClass::Source().State());
                // In case on an error, not much more we can do then close up..
                BaseClass::Source().Close(0);
            }
        }

    private:
        ProxyType<FactoryType<IIPC, uint32_t>> _factory;
    };

    // Server connection for the IPC
    template <typename EXTENSION, const bool INTERNALFACTORY>
    class IPCChannelServerType : public SocketListner {
    public:
        using Client = IPCChannelClientType<EXTENSION, false, false>;

    private:
        using ThisClass = IPCChannelServerType<EXTENSION, INTERNALFACTORY>;
        using Clients = std::unordered_map<EXTENSION*, ProxyType<Client>>;

    public:
        IPCChannelServerType() = delete;
        IPCChannelServerType(IPCChannelServerType<EXTENSION, INTERNALFACTORY>&&) = delete;
        IPCChannelServerType(const IPCChannelServerType<EXTENSION, INTERNALFACTORY>&) = delete;
        IPCChannelServerType<EXTENSION, INTERNALFACTORY>& operator=(IPCChannelServerType<EXTENSION, INTERNALFACTORY>&) = delete;

        template <const bool FACTORY = INTERNALFACTORY, EnableIfParameter<FACTORY == true> = 0>
        IPCChannelServerType(const NodeId& node, const uint32_t bufferSize)
            : SocketListner(node)
            , _adminLock()
            , _factory(ProxyType<FactoryType<IIPC, uint32_t>>::Create())
            , _clients()
            , _connector()
            , _bufferSize(bufferSize)
        {
            ASSERT(node.IsValid() == true);

            static_assert(INTERNALFACTORY == true, "This constructor can only be called if you specify an INTERNAL factory");

            if (node.IsValid() == true) {
                if (node.Type() == NodeId::TYPE_DOMAIN) {
                    _connector = node.QualifiedName();
                } else if (node.IsAnyInterface() == true) {
                    _connector = SystemInfo::Instance().GetHostName() + ':' + NumberType<uint16_t>(node.PortNumber()).Text();
                } else {
                    _connector = node.HostAddress() + ':' + NumberType<uint16_t>(node.PortNumber()).Text();
                }
            }
        }
        template <const bool FACTORY = INTERNALFACTORY, EnableIfParameter<FACTORY == false> = 0>
        IPCChannelServerType(const NodeId& node, const uint32_t bufferSize, ProxyType<FactoryType<IIPC, uint32_t>>& factory)
            : SocketListner(node)
            , _adminLock()
            , _factory(factory)
            , _clients()
            , _connector()
            , _bufferSize(bufferSize)
        {
            ASSERT(node.IsValid() == true);

            static_assert(INTERNALFACTORY == false, "This constructor can only be called if you specify an EXTERNAL factory");

            if (node.IsValid() == true) {
                if (node.Type() == NodeId::TYPE_DOMAIN) {
                    _connector = node.QualifiedName();
                } else if (node.IsAnyInterface() == true) {
                    _connector = SystemInfo::Instance().GetHostName() + ':' + NumberType<uint16_t>(node.PortNumber()).Text();
                } else {
                    _connector = node.HostAddress() + ':' + NumberType<uint16_t>(node.PortNumber()).Text();
                }
            }
        }
        ~IPCChannelServerType() override {
            TRACE_L1("Closing server in Process. %d", ProcessInfo().Id());

            Close(infinite);

            ASSERT(_clients.empty() == true);

            if (_clients.empty() == false) {

                TRACE_L1("Closing clients that should have been closed before destruction [%d].", static_cast<uint32_t>(_clients.size()));
                CloseClients();
            }

            _factory->DestroyFactories();
        }

    public:
        // void action(const Client& client)
        template<typename ACTION>
        void Visit(ACTION&& action) {
            _adminLock.Lock();
            for (const std::pair< EXTENSION*, ProxyType<Client>>& entry : _clients) {
                action(*(entry.second));
            }
            _adminLock.Unlock();
        }
        // void action(const Client& client)
        template<typename ACTION>
        void Visit(ACTION&& action) const {
            _adminLock.Lock();
            for (const std::pair< EXTENSION*, ProxyType<Client>>& entry : _clients) {
                action(*(entry.second));
            }
            _adminLock.Unlock();
        }
        // bool action(const Client& client) const
        template<typename ACTION>
        ProxyType<Client> Find(ACTION&& action) {
            ProxyType<Client> result;
            _adminLock.Lock();
            typename Clients::iterator index = _clients.begin();
            while ((result.IsValid() == false) && (index != _clients.end())) {
                if (action(*(index->second)) == true) {
                    result = index->second;
                }
                index++;
            }
            _adminLock.Unlock();
            return (result);
        }
        // bool action(const Client& client) const
        template<typename ACTION>
        ProxyType<const Client> Find(ACTION&& action) const {
            ProxyType<const Client> result;
            _adminLock.Lock();
            typename Clients::const_iterator index = _clients.cbegin();
            while ((result.IsValid() == false) && (index != _clients.cend())) {
                if (action(*(index->second)) == true) {
                    result = index->second;
                }
                index++;
            }
            _adminLock.Unlock();
            return (result);
        }
        const string& Connector() const
        {
            return (_connector);
        }
        void Register(const uint32_t id, const ProxyType<IIPCServer>& handler)
        {
            _adminLock.Lock();

			ASSERT(handler.IsValid() == true);
            ASSERT(_handlers.find(id) == _handlers.end());

            _handlers.emplace(id, handler);

            _adminLock.Unlock();
        }
        void Unregister(const uint32_t id)
        {
            _adminLock.Lock();

            std::map<uint32_t, ProxyType<IIPCServer>>::iterator index(_handlers.find(id));

            ASSERT(index != _handlers.end());

            if (index != _handlers.end()) {
                _handlers.erase(index);
            }

            _adminLock.Unlock();
        }

        template <typename ACTUALELEMENT>
        void CreateFactory(const uint32_t initialSize)
        {
            ASSERT(_factory.IsValid());
            ASSERT(SocketListner::IsListening() == false);
            ASSERT(_clients.size() == 0);
#ifdef __WINDOWS__
            ASSERT(INTERNALFACTORY == true && "This method can only be called if you specify an INTERNAL factory");
#else
            static_assert(INTERNALFACTORY == true, "This method can only be called if you specify an INTERNAL factory");
#endif

            _factory->CreateFactory<ACTUALELEMENT>(initialSize);
        }
        template <typename ACTUALELEMENT>
        void DestroyFactory()
        {

            ASSERT(_factory.IsValid());
            ASSERT(SocketListner::IsListening() == false);
            ASSERT(_clients.size() == 0);

            static_assert(INTERNALFACTORY == true, "This method can only be called if you specify an INTERNAL factory");

            _factory->DestroyFactory<ACTUALELEMENT>();
        }

        uint32_t Invoke(ProxyType<IIPC>& command, const uint32_t waitTime)
        {
            // Lock the list, unlock if we reach the end...
            _adminLock.Lock();

            typename Clients::iterator index(_clients.begin());

            return (CallRecursive(index, command, waitTime));
        }
        void Cleanup()
        {
            _adminLock.Lock();

            CloseClients();

            _adminLock.Unlock();
        }

    protected:
        virtual void Added(ProxyType<Client>& client VARIABLE_IS_NOT_USED)
        {
        }
        virtual void Removed(ProxyType<Client>& client VARIABLE_IS_NOT_USED)
        {
        }

    private:
        ProxyType<IIPC> InvokeAllowed(const Client& client, const ProxyType<IIPC>& command) const
        {
            return (__InvokeAllowed(client, command));
        }

        IS_MEMBER_AVAILABLE(InvokeAllowed, hasInvokeAllowed);

        template <typename CLASSNAME = EXTENSION>
        typename Core::TypeTraits::enable_if<hasInvokeAllowed<const CLASSNAME, ProxyType<IIPC>, const ProxyType<IIPC>&>::value, ProxyType<IIPC>>::type
        __InvokeAllowed(const Client& client, const ProxyType<IIPC>& command) const
        {
            return (client.Extension().InvokeAllowed(command));
        }

        template <typename CLASSNAME = EXTENSION>
        typename Core::TypeTraits::enable_if<!hasInvokeAllowed<const CLASSNAME, ProxyType<IIPC>, const ProxyType<IIPC>&>::value, ProxyType<IIPC>>::type
        __InvokeAllowed(const Client&, const ProxyType<IIPC>& command) const
        {
            return (command);
        }

        void UnregisterHandlers(const typename Clients::iterator& client)
        {
            // Make sure all handlers of the server are deattached from the client...
            std::map<uint32_t, ProxyType<IIPCServer>>::iterator index(_handlers.begin());

            while (index != _handlers.end()) {
                client->second->Unregister(index->first);
                index++;
            }

            Removed(client->second);
        }
        void InternalCleanup()
        {
            typename Clients::iterator cleaner(_clients.begin());

            while (cleaner != _clients.end()) {
                if (cleaner->second->IsClosed() == true) {
                    UnregisterHandlers(cleaner);
                    cleaner = _clients.erase(cleaner);
                } else {
                    cleaner++;
                }
            }
        }
        uint32_t CloseClients()
        {
            uint32_t result = Core::ERROR_NONE;

            _adminLock.Lock();
            typename Clients::iterator index(_clients.begin());

            while (index != _clients.end()) {
                Core::ProxyType<Client> item(index->second);

                if (item->Source().IsClosed() == false) {
                    if (item->Source().Close(0) != Core::ERROR_NONE) {
                        TRACE_L1("client close failed %d", __LINE__);
                        result = Core::ERROR_CLOSING_FAILED;
                    }
                }

                index++;
            }

            // Wait till all clients have signalled that they are closed.
            while (_clients.size() > 0) {
                Core::ProxyType<Client> item(_clients.begin()->second);

                if (item->Source().IsClosed() == false) {
                    _adminLock.Unlock();

                    // Give up our slice, for the StateChange to happen..
                    SleepMs(10);

                    _adminLock.Lock();
                } else {
                    // Ensure there is no pending handlers to cleanup after client force close.
                    UnregisterHandlers(_clients.begin());
                    _clients.erase(_clients.begin());
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        void Accept(SOCKET& newClient, const NodeId& remoteId) override
        {
            _adminLock.Lock();

            InternalCleanup();

            ProxyType<Client> newLink(ProxyType<Client>::Create(remoteId, _bufferSize, _factory, newClient));

            _clients.emplace(std::piecewise_construct, 
                                std::forward_as_tuple(&(newLink->Extension())),
                                std::forward_as_tuple(newLink));

            // Make sure all handlers form the server are attached to the client...
            std::map<uint32_t, ProxyType<IIPCServer>>::iterator index(_handlers.begin());

            while (index != _handlers.end()) {
                newLink->Register(index->first, index->second);
                index++;
            }

            Added(newLink);

            _adminLock.Unlock();
        }
        uint32_t CallRecursive(typename Clients::iterator& index, ProxyType<IIPC>& command, const uint32_t waitTime)
        {
            uint32_t result = Core::ERROR_NONE;

            if (index != _clients.end()) {
                ProxyType<Client> thisClient(index->second);
                ProxyType<IIPC> realCommand;
                index++;
                uint32_t status = CallRecursive(index, command, waitTime);
                if (status != Core::ERROR_NONE) {
                    result = status;
                }
                realCommand = InvokeAllowed(*thisClient, command);

                if (realCommand.IsValid() == true) {
                    //TRACE_L1("Invoked %d client.", 1);
                    status = thisClient->Invoke(realCommand, waitTime);
                    if (status != Core::ERROR_NONE) {
                        result = status;
                    }
                }
            } else {
                _adminLock.Unlock();
            }
            return (result);
        }

    private:
        mutable CriticalSection _adminLock;
        ProxyType<FactoryType<IIPC, uint32_t>> _factory;
        std::map<uint32_t, ProxyType<IIPCServer>> _handlers;
        Clients _clients;
        string _connector;
        const uint32_t _bufferSize;
    };
}
} // namespace Core

#endif // __IPC_CHANNEL_H__
