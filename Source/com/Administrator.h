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

        typedef std::pair<Core::IUnknown*, const uint32_t> ProxyEntry;
        typedef std::map<void*, ProxyEntry> ProxyMap;
        typedef std::map<const Core::IPCChannel*, ProxyMap> ChannelMap;

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
            Core::IUnknown* newProxy = nullptr;

            if (implementation != nullptr) {
 
                _adminLock.Lock ();

                std::map<uint32_t, IMetadata*>::iterator index(_proxy.find(interfaceNumber));

                if (index != _proxy.end()) {

					newProxy = index->second->CreateProxy(channel, implementation, otherSideInformed);

                    ASSERT (newProxy != nullptr);

                    if (record == true) {
                        _channelProxyMap[channel.operator->()].insert(std::pair<void*, ProxyEntry>(implementation, ProxyEntry(newProxy, interfaceNumber)));
                    }
                }
                else {
                    TRACE_L1("Failed to find a Proxy for %d.", interfaceNumber);
                }

                _adminLock.Lock ();
            }

            return (newProxy);
        }
		template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* FindProxy(const Core::IPCChannel* channel, void* implementation)
        {
            ACTUALINTERFACE* result = nullptr;

            _adminLock.Lock();

            ChannelMap::iterator index(_channelProxyMap.find(channel));

            if (index != _channelProxyMap.end()) {
                ProxyMap::iterator entry(index->second.find(implementation));
                if (entry != index->second.end()) {
                    result = entry->second.first->QueryInterface<ACTUALINTERFACE>();
                }
            }

            _adminLock.Unlock();

            return (result);
        }
		Core::IUnknown* FindProxy(const Core::IPCChannel* channel, void* implementation)
		{
			Core::IUnknown* result = nullptr;

			_adminLock.Lock();

			ChannelMap::iterator index(_channelProxyMap.find(channel));

			if (index != _channelProxyMap.end()) {
				ProxyMap::iterator entry(index->second.find(implementation));
				if (entry != index->second.end()) {
					result = entry->second.first;
					if (result != nullptr) {
						result->AddRef();
					}
				}
			}

			_adminLock.Unlock();

			return (result);
		}
		void Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message)
        {
            uint32_t interfaceId(message->Parameters().InterfaceId());

            _adminLock.Lock();

            std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

            if (index != _stubs.end()) {
                uint32_t methodId(message->Parameters().MethodId());
                index->second->Handle(methodId, channel, message);
            }
            else {
                // Oops this is an unknown interface, Do not think this could happen.
                TRACE_L1("Unknown interface. %d", interfaceId);
            }

            _adminLock.Unlock();
        }
        void DeleteProxy(const Core::IPCChannel* channel, void* implementation)
        {
            _adminLock.Lock();

            ChannelMap::iterator index(_channelProxyMap.find(channel));

            if (index != _channelProxyMap.end()) {
                ProxyMap::iterator entry(index->second.find(implementation));
                if (entry != index->second.end()) {
                    index->second.erase(entry);
                }
            }
            
            _adminLock.Unlock();
        }
        void DeleteChannel(const Core::IPCChannel* channel, std::list< std::pair<const uint32_t, const Core::IUnknown* > >& pendingProxies)
        {
            _adminLock.Lock();

            ChannelMap::iterator index(_channelProxyMap.find(channel));

            if (index != _channelProxyMap.end()) {
                ProxyMap::iterator loop (index->second.begin());
                while (loop != index->second.end()) {
					std::pair<const uint32_t, const Core::IUnknown*> entry (loop->second.second, loop->second.first);				
                    pendingProxies.push_back(entry);
                    loop++;
                }
                _channelProxyMap.erase(index);
            }

            _adminLock.Unlock();
        }

    private:
        // Seems like we have enough information, open up the Process communcication Channel.
        Core::CriticalSection _adminLock;
        std::map<uint32_t, ProxyStub::UnknownStub*> _stubs;
        std::map<uint32_t, IMetadata*> _proxy;
        Core::ProxyPoolType<InvokeMessage> _factory;
        ChannelMap _channelProxyMap;
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
