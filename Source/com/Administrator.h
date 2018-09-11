#ifndef __COM_ADMINISTRATOR_H
#define __COM_ADMINISTRATOR_H

#include "Module.h"
#include "Messages.h"

namespace WPEFramework {
namespace RPC {

#ifdef __DEBUG__
    enum { CommunicationTimeOut = Core::infinite }; // Time in ms. Forever
#else
	enum { CommunicationTimeOut = 10000 }; // Time in ms. 10 Seconden
#endif
	enum { CommunicationBufferSize = 8120 }; // 8K :-)
}

namespace ProxyStub {
    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    class EXTERNAL UnknownStub {
    private:
        UnknownStub(const UnknownStub&) = delete;
        UnknownStub& operator=(const UnknownStub&) = delete;

    public:
        UnknownStub();
        virtual ~UnknownStub();

    public:
        inline uint16_t Length() const
        {
            return (3);
        }
        virtual Core::IUnknown* Convert(void* incomingData) const;
        virtual void Handle(const uint16_t index, Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message);
    };
}

namespace RPC {
    class EXTERNAL Administrator {
    private:
        Administrator();
        Administrator(const Administrator&) = delete;
        Administrator& operator=(const Administrator&) = delete;

        struct EXTERNAL IMetadata {
            virtual ~IMetadata(){};

            virtual Core::IUnknown* CreateProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed) = 0;
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
            virtual Core::IUnknown* CreateProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            {
                return (new PROXY(channel, implementation, otherSideInformed));
            }
		};

    public:
        virtual ~Administrator();

        static Administrator& Instance();

    public:
        template <typename ACTUALINTERFACE, typename PROXY, typename STUB>
        void Announce()
        {
            _stubs.insert(std::pair<uint32_t, ProxyStub::UnknownStub*>(ACTUALINTERFACE::ID, new STUB()));
            _proxy.insert(std::pair<uint32_t, IMetadata*>(ACTUALINTERFACE::ID, new ProxyType<PROXY>()));
        }

        Core::ProxyType<InvokeMessage> Message()
        {
            return (_factory.Element());
        }
        template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* CreateProxy(Core::ProxyType<Core::IPCChannel>& channel, ACTUALINTERFACE* implementation, const bool record, const bool otherSideInformed)
        {
			ACTUALINTERFACE* response = nullptr;
			Core::IUnknown* result = CreateProxy(ACTUALINTERFACE::ID, channel, implementation, record, otherSideInformed);

			if (result != nullptr) {
				response = result->QueryInterface<ACTUALINTERFACE>();
				result->Release();
			}
            return (response);
        }
		Core::IUnknown* CreateProxy(const uint32_t interfaceNumber, Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool record, const bool otherSideInformed)
        {
            if (implementation == nullptr) {
                return (nullptr);
            }
            else {
                std::map<uint32_t, IMetadata*>::iterator index(_proxy.find(interfaceNumber));

                if (index != _proxy.end()) {

					Core::IUnknown* newProxy = index->second->CreateProxy(channel, implementation, otherSideInformed);

                    ASSERT (newProxy != nullptr);

                    if (record == true) {
                        _proxyMap.insert(std::pair<void*, Core::IUnknown*>(implementation, newProxy));
                    }

                    return (newProxy);
                }

                TRACE_L1("Failed to find a Proxy for %d.", interfaceNumber);
            }

            return (nullptr);
        }
		template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* FindProxy(void* implementation)
        {
            std::map<void*, Core::IUnknown*>::iterator index(_proxyMap.find(implementation));

            return (index != _proxyMap.end() ? index->second->QueryInterface< ACTUALINTERFACE>() : nullptr);
        }
        void Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message)
        {

            uint32_t interfaceId(message->Parameters().InterfaceId());
            std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

            if (index != _stubs.end()) {
                uint32_t methodId(message->Parameters().MethodId());
                index->second->Handle(methodId, channel, message);
            }
            else {
                // Oops this is an unknown interface, Do not think this could happen.
                TRACE_L1("Unknown interface. %d", interfaceId);
            }
        }
        void DeleteProxy(void* implementation)
        {
            std::map<void*, Core::IUnknown*>::iterator index(_proxyMap.find(implementation));

            if (index != _proxyMap.end()) {
                _proxyMap.erase(index);
            }
        }

    private:
        // Seems like we have enough information, open up the Process communcication Channel.
        std::map<uint32_t, ProxyStub::UnknownStub*> _stubs;
        std::map<uint32_t, IMetadata*> _proxy;
        Core::ProxyPoolType<InvokeMessage> _factory;
        std::map<void*, Core::IUnknown*> _proxyMap;
    };

    template <const uint32_t MESSAGESLOTS, const uint16_t THREADPOOLCOUNT>
    class InvokeServerType : public Core::IPCServerType<InvokeMessage> {
    private:
        class Info {
        public:
            Info()
                : _message()
                , _channel()
            {
            }
            Info(Core::ProxyType<InvokeMessage> message, Core::ProxyType<Core::IPCChannel> channel)
                : _message(message)
                , _channel(channel)
            {
            }
            Info(const Info& copy)
                : _message(copy._message)
                , _channel(copy._channel)
            {
            }
            ~Info()
            {
            }
            Info& operator=(const Info& rhs)
            {
                _message = rhs._message;
                _channel = rhs._channel;

                return (*this);
            }

        public:
            inline void Dispatch()
            {

                Administrator::Instance().Invoke(_channel, _message);
                Core::ProxyType<Core::IIPC> response(Core::proxy_cast<Core::IIPC>(_message));
                _channel->ReportResponse(response);
            }

        private:
            Core::ProxyType<InvokeMessage> _message;
            Core::ProxyType<Core::IPCChannel> _channel;
        };

        InvokeServerType(const InvokeServerType<THREADPOOLCOUNT, MESSAGESLOTS>&) = delete;
        InvokeServerType<THREADPOOLCOUNT, MESSAGESLOTS>& operator=(const InvokeServerType<THREADPOOLCOUNT, MESSAGESLOTS>&) = delete;

    public:
        InvokeServerType(const uint32_t stackSize = Core::Thread::DefaultStackSize())
            : _threadPoolEngine(stackSize, _T("IPCInterfaceMessageHandler"))
        {
        }
        ~InvokeServerType()
        {
        }

    public:
        virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<InvokeMessage>& data)
        {
            // Oke, see if we can reference count the IPCChannel
            Core::ProxyType<Core::IPCChannel> refChannel(channel);

            ASSERT(refChannel.IsValid());

            if (refChannel.IsValid() == true) {
                if (_threadPoolEngine.Pending() >= ((MESSAGESLOTS * 80) / 100)) {
                    TRACE_L1("_threadPoolEngine.Pending() == %d", _threadPoolEngine.Pending());
                }
                _threadPoolEngine.Submit(Info(data, refChannel), Core::infinite);
            }
        }

    private:
        Core::ThreadPoolType<Info, THREADPOOLCOUNT, MESSAGESLOTS> _threadPoolEngine;
    };
}

} // namespace RPC

#endif // __COM_ADMINISTRATOR_H
