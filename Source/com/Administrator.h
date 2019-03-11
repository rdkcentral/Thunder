#ifndef __COM_ADMINISTRATOR_H
#define __COM_ADMINISTRATOR_H

#include "Messages.h"
#include "Module.h"

namespace WPEFramework {

namespace ProxyStub {

    class UnknownStub;
    class UnknownProxy;
}

namespace RPC {

#ifdef __DEBUG__
    enum { CommunicationTimeOut = Core::infinite }; // Time in ms. Forever
#else
    enum { CommunicationTimeOut = 10000 }; // Time in ms. 10 Seconden
#endif
    enum { CommunicationBufferSize = 8120 }; // 8K :-)

    class EXTERNAL Administrator {
    private:
        Administrator();
        Administrator(const Administrator&) = delete;
        Administrator& operator=(const Administrator&) = delete;

        typedef std::list<ProxyStub::UnknownProxy*> ProxyList;
        typedef std::map<const Core::IPCChannel*, ProxyList> ChannelMap;

        struct EXTERNAL IMetadata {
            virtual ~IMetadata(){};

            virtual ProxyStub::UnknownProxy* CreateProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool remoteRefCounted) = 0;
        };

        template <typename PROXY>
        class ProxyType : public IMetadata {
        private:
            ProxyType(const ProxyType<PROXY>& copy) = delete;
            ProxyType<PROXY>& operator=(const ProxyType<PROXY>& copy) = delete;

        public:
            ProxyType()
            {
            }
            virtual ~ProxyType()
            {
            }

        private:
            virtual ProxyStub::UnknownProxy* CreateProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool remoteRefCounted)
            {
                return (new PROXY(channel, implementation, remoteRefCounted))->Administration();
            }
        };

    public:
        virtual ~Administrator();

        static Administrator& Instance();

    public:
        template <typename ACTUALINTERFACE, typename PROXY, typename STUB>
        void Announce()
        {
            _adminLock.Lock();

            _stubs.insert(std::pair<uint32_t, ProxyStub::UnknownStub*>(ACTUALINTERFACE::ID, new STUB()));
            _proxy.insert(std::pair<uint32_t, IMetadata*>(ACTUALINTERFACE::ID, new ProxyType<PROXY>()));

            _adminLock.Unlock();
        }

        Core::ProxyType<InvokeMessage> Message()
        {
            return (_factory.Element());
        }

        void DeleteChannel(const Core::IPCChannel* channel, std::list<ProxyStub::UnknownProxy*>& pendingProxies)
        {
            _adminLock.Lock();

            ChannelMap::iterator index(_channelProxyMap.find(channel));

            if (index != _channelProxyMap.end()) {
                ProxyList::iterator loop(index->second.begin());
                while (loop != index->second.end()) {
                    pendingProxies.push_back(*loop);
                    loop++;
                }
                _channelProxyMap.erase(index);
            }

            _adminLock.Unlock();
        }

        template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, void* impl)
        {
            return (reinterpret_cast<ACTUALINTERFACE*>(ProxyFind(channel, impl, ACTUALINTERFACE::ID, ACTUALINTERFACE::ID)));
        }
        template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id)
        {
            return (reinterpret_cast<ACTUALINTERFACE*>(ProxyFind(channel, impl, id, ACTUALINTERFACE::ID)));
        }
        template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted)
        {

            return (reinterpret_cast<ACTUALINTERFACE*>(ProxyInstanceQuery(channel, impl, id, refCounted, ACTUALINTERFACE::ID, false)));
        }
        ProxyStub::UnknownProxy* ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted, const uint32_t interfaceId, const bool piggyBack);

        void AddRef(void* impl, const uint32_t interfaceId);
        void Release(void* impl, const uint32_t interfaceId);
        void Release(ProxyStub::UnknownProxy* proxy, Data::Output& response);
        void Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message);
        void RegisterProxy(ProxyStub::UnknownProxy& proxy);
        void UnregisterProxy(ProxyStub::UnknownProxy& proxy);

    private:
        void* ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const uint32_t interfaceId);
        void* ProxyInstanceQuery(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted, const uint32_t interfaceId, const bool piggyBack);

    private:
        // Seems like we have enough information, open up the Process communcication Channel.
        Core::CriticalSection _adminLock;
        std::map<uint32_t, ProxyStub::UnknownStub*> _stubs;
        std::map<uint32_t, IMetadata*> _proxy;
        Core::ProxyPoolType<InvokeMessage> _factory;
        ChannelMap _channelProxyMap;
    };

    struct IHandler {
        virtual ~IHandler() {}

        virtual Core::ProxyType<Core::IIPCServer> InvokeHandler() = 0;
        virtual Core::ProxyType<Core::IIPCServer> AnnounceHandler() = 0;
        virtual void AnnounceHandler(Core::IPCServerType<AnnounceMessage>* handler) = 0;
    };

    template <const uint32_t MESSAGESLOTS, const uint16_t THREADPOOLCOUNT>
    class InvokeServerType : public IHandler {
    private:
        class Info {
        public:
            Info()
                : _message()
                , _channel()
                , _parent(nullptr)
            {
            }
            Info(Core::ProxyType<Core::IPCChannel> channel, Core::ProxyType<InvokeMessage> message)
                : _message(message)
                , _channel(channel)
                , _parent(nullptr)
            {
            }
            Info(Core::ProxyType<Core::IPCChannel> channel, Core::ProxyType<AnnounceMessage> message, InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>& parent)
                : _message(message)
                , _channel(channel)
                , _parent(&parent)
            {
            }
            Info(const Info& copy)
                : _message(copy._message)
                , _channel(copy._channel)
                , _parent(copy._parent)
            {
            }
            ~Info()
            {
            }
            Info& operator=(const Info& rhs)
            {
                _message = rhs._message;
                _channel = rhs._channel;
                _parent = rhs._parent;

                return (*this);
            }

        public:
            inline void Dispatch()
            {
                if (_message->Label() == InvokeMessage::Id()) {
                    Core::ProxyType<InvokeMessage> message(Core::proxy_cast<InvokeMessage>(_message));

                    Administrator::Instance().Invoke(_channel, message);
                    _channel->ReportResponse(_message);
                } else {
                    ASSERT(_message->Label() == AnnounceMessage::Id());
                    ASSERT(_parent != nullptr);

                    Core::ProxyType<AnnounceMessage> message(Core::proxy_cast<AnnounceMessage>(_message));

                    _parent->Dispatch(*_channel, message);
                }
            }

        private:
            Core::ProxyType<Core::IIPC> _message;
            Core::ProxyType<Core::IPCChannel> _channel;
            InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>* _parent;
        };
        class InvokeHandlerImplementation : public Core::IPCServerType<InvokeMessage> {
        private:
            InvokeHandlerImplementation() = delete;
            InvokeHandlerImplementation(const InvokeHandlerImplementation&) = delete;
            InvokeHandlerImplementation& operator=(const InvokeHandlerImplementation&) = delete;

        public:
            InvokeHandlerImplementation(InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>* parent)
                : _parent(*parent)
            {
            }
            virtual ~InvokeHandlerImplementation()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<InvokeMessage>& data)
            {
                // Oke, see if we can reference count the IPCChannel
                Core::ProxyType<Core::IPCChannel> refChannel(channel);

                ASSERT(refChannel.IsValid());

                if (refChannel.IsValid() == true) {
                    _parent.Submit(Info(refChannel, data));
                }
            }

        private:
            InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>& _parent;
        };
        class AnnounceHandlerImplementation : public Core::IPCServerType<AnnounceMessage> {
        private:
            AnnounceHandlerImplementation() = delete;
            AnnounceHandlerImplementation(const AnnounceHandlerImplementation&) = delete;
            AnnounceHandlerImplementation& operator=(const AnnounceHandlerImplementation&) = delete;

        public:
            AnnounceHandlerImplementation(InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>* parent)
                : _parent(*parent)
            {
            }
            virtual ~AnnounceHandlerImplementation()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<AnnounceMessage>& data)
            {
                // Oke, see if we can reference count the IPCChannel
                Core::ProxyType<Core::IPCChannel> refChannel(channel);

                ASSERT(refChannel.IsValid());

                if (refChannel.IsValid() == true) {
                    _parent.Submit(Info(refChannel, data, _parent));
                }
            }

        private:
            InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>& _parent;
        };

        InvokeServerType(const InvokeServerType<THREADPOOLCOUNT, MESSAGESLOTS>&) = delete;
        InvokeServerType<THREADPOOLCOUNT, MESSAGESLOTS>& operator=(const InvokeServerType<THREADPOOLCOUNT, MESSAGESLOTS>&) = delete;

    public:
        InvokeServerType(const uint32_t stackSize = Core::Thread::DefaultStackSize())
            : _threadPoolEngine(stackSize, _T("IPCInterfaceMessageHandler"))
            , _invokeHandler(Core::ProxyType<InvokeHandlerImplementation>::Create(this))
            , _announceHandler(Core::ProxyType<AnnounceHandlerImplementation>::Create(this))
            , _handler(nullptr)
        {
        }
        ~InvokeServerType()
        {
        }

    public:
        virtual Core::ProxyType<Core::IIPCServer> InvokeHandler() override
        {
            return (_invokeHandler);
        }
        virtual Core::ProxyType<Core::IIPCServer> AnnounceHandler() override
        {
            return (_announceHandler);
        }
        virtual void AnnounceHandler(Core::IPCServerType<AnnounceMessage>* handler) override
        {

            // Concurrency aspect is out of scope as the implementation of this interface is currently limited
            // to the RPC::COmmunicator and RPC::CommunicatorClient. Both of these implementations will first
            // set this callback before any communication is happeing (Open happens after this)
            // Also the announce handler will not be removed until the line is closed and the server (or client)
            // is destructed!!!
            ASSERT((handler == nullptr) ^ (_handler == nullptr));

            _handler = handler;
        }

    private:
        inline void Submit(const Info& data)
        {

            if (_threadPoolEngine.Pending() >= ((MESSAGESLOTS * 80) / 100)) {
                TRACE_L1("_threadPoolEngine.Pending() == %d", _threadPoolEngine.Pending());
            }
            _threadPoolEngine.Submit(data, Core::infinite);
        }
        inline void Dispatch(Core::IPCChannel& channel, Core::ProxyType<AnnounceMessage>& data)
        {

            ASSERT(_handler != nullptr);

            _handler->Procedure(channel, data);
        }

    private:
        Core::ThreadPoolType<Info, THREADPOOLCOUNT, MESSAGESLOTS> _threadPoolEngine;
        Core::ProxyType<Core::IPCServerType<InvokeMessage>> _invokeHandler;
        Core::ProxyType<Core::IPCServerType<AnnounceMessage>> _announceHandler;
        Core::IPCServerType<AnnounceMessage>* _handler;
    };
}

} // namespace RPC

#endif // __COM_ADMINISTRATOR_H
