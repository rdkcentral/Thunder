/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

    typedef std::pair<const Core::IUnknown*, const uint32_t> ExposedInterface;

    class EXTERNAL Administrator {
    private:
        Administrator();
        Administrator(const Administrator&) = delete;
        Administrator& operator=(const Administrator&) = delete;

        class ExternalReference {
        public:
            ExternalReference() = delete;
            ExternalReference(const ExternalReference&) = delete;
            ExternalReference& operator=(const ExternalReference&) = delete;

            ExternalReference(Core::IUnknown* baseInterface, void* implementation, const uint32_t id)
                : _baseInterface(baseInterface)
                , _implementation(implementation)
                , _id(id)
                , _refCount(1)
            {
            }
            ~ExternalReference()
            {
            }

        public:
            bool operator==(const void* source) const
            {
                return (source == _implementation);
            }
            bool operator!=(const void* source) const
            {
                return (!operator==(source));
            }
            void Increment()
            {
                _refCount++;
            }
            bool Decrement(const uint32_t dropCount)
            {
                return (_refCount.fetch_sub(dropCount) == dropCount);
            }
            uint32_t Id() const
            {
                return (_id);
            }
            const Core::IUnknown* Source() const
            {
                return (_baseInterface);
            }
            uint32_t RefCount() const
            {
                return (_refCount.load());
            }

        private:
            Core::IUnknown* _baseInterface;
            void* _implementation;
            const uint32_t _id;
            std::atomic<uint32_t> _refCount;
        };

        typedef std::list<ProxyStub::UnknownProxy*> ProxyList;
        typedef std::map<const Core::IPCChannel*, ProxyList> ChannelMap;
        typedef std::map<const Core::IPCChannel*, std::list<ExternalReference>> ReferenceMap;

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

        void DeleteChannel(const Core::ProxyType<Core::IPCChannel>& channel, std::list<ProxyStub::UnknownProxy*>& pendingProxies, std::list<ExposedInterface>& usedInterfaces);

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

        void RegisterInterface(Core::ProxyType<Core::IPCChannel>& channel, void* reference, const uint32_t id)
        {
            RegisterInterface(channel, Convert(reference, id), reference, id);
        }
        template <typename ACTUALINTERFACE>
        void RegisterInterface(Core::ProxyType<Core::IPCChannel>& channel, ACTUALINTERFACE* reference)
        {
            RegisterInterface(channel, static_cast<Core::IUnknown*>(reference), reinterpret_cast<void*>(reference), ACTUALINTERFACE::ID);
        }
        void UnregisterInterface(Core::ProxyType<Core::IPCChannel>& channel, void* reference, const uint32_t interfaceId, const uint32_t dropCount)
        {
            ReferenceMap::iterator index(_channelReferenceMap.find(channel.operator->()));

            if (index != _channelReferenceMap.end()) {
                std::list<ExternalReference>::iterator element(std::find(index->second.begin(), index->second.end(), reference));
                ASSERT(element != index->second.end());

                if (element != index->second.end()) {
                    if (element->Decrement(dropCount) == true) {
                        index->second.erase(element);
                        if (index->second.size() == 0) {
                            _channelReferenceMap.erase(index);
                        }
                    }
                } else {
                    printf("Unregistering an interface [0x%x, %d] which has not been registered!!!\n", interfaceId, Core::ProcessInfo().Id());
                }
            } else {
                printf("Unregistering an interface [0x%x, %d] from a non-existing channel!!!\n", interfaceId, Core::ProcessInfo().Id());
            }
        }

    private:
        Core::IUnknown* Convert(void* rawImplementation, const uint32_t id);
        void* ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const uint32_t interfaceId);
        void* ProxyInstanceQuery(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted, const uint32_t interfaceId, const bool piggyBack);
        void RegisterInterface(Core::ProxyType<Core::IPCChannel>& channel, Core::IUnknown* reference, void* rawImplementation, const uint32_t id);

    private:
        // Seems like we have enough information, open up the Process communcication Channel.
        Core::CriticalSection _adminLock;
        std::map<uint32_t, ProxyStub::UnknownStub*> _stubs;
        std::map<uint32_t, IMetadata*> _proxy;
        Core::ProxyPoolType<InvokeMessage> _factory;
        ChannelMap _channelProxyMap;
        ReferenceMap _channelReferenceMap;
    };

    class EXTERNAL Job : public Core::IDispatch {
    public:
        Job()
            : _message()
            , _channel()
            , _handler(nullptr)
        {
        }
        Job(Core::IPCChannel& channel, const Core::ProxyType<Core::IIPC>& message, Core::IIPCServer* handler)
            : _message(message)
            , _channel(channel)
            , _handler(handler)
        {
        }
        Job(const Job& copy)
            : _message(copy._message)
            , _channel(copy._channel)
            , _handler(copy._handler)
        {
        }
        virtual ~Job()
        {
        }

        Job& operator=(const Job& rhs) {
            _message = rhs._message;
            _channel = rhs._channel;
            _handler = rhs._handler;

            return (*this);
        }

    public:
        static Core::ProxyType<Job> Instance()
        {
            return (_factory.Element());
        }
        void Clear()
        {
            _message.Release();
            _channel.Release();
            _handler = nullptr;
        }
        void Set(Core::IPCChannel& channel, const Core::ProxyType<Core::IIPC>& message, Core::IIPCServer* handler)
        {
            _message = message;
            _channel = Core::ProxyType<Core::IPCChannel>(channel);
            _handler = handler;
        }
        virtual void Dispatch() override
        {
            if (_message->Label() == InvokeMessage::Id()) {
                Invoke(_channel, _message);
            } else {
                ASSERT(_message->Label() == AnnounceMessage::Id());
                ASSERT(_handler != nullptr);

                _handler->Procedure(*_channel, _message);
            }
        }

		static void Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<Core::IIPC>& data)
		{
            Core::ProxyType<InvokeMessage> message(data);
            ASSERT(message.IsValid() == true);
            _administrator.Invoke(channel, message);
            channel->ReportResponse(data);

		}

    private:
        Core::ProxyType<Core::IIPC> _message;
        Core::ProxyType<Core::IPCChannel> _channel;
        Core::IIPCServer* _handler;

        static Core::ProxyPoolType<Job> _factory;
        static Administrator& _administrator;
    };

    class EXTERNAL InvokeServer : public Core::IIPCServer {
    public:
        InvokeServer(const InvokeServer&) = delete;
        InvokeServer& operator=(const InvokeServer&) = delete;

        InvokeServer(Core::WorkerPool* workers)
            : _threadPoolEngine(*workers)
            , _handler(nullptr)
        {
            ASSERT(workers != nullptr);
        }
        ~InvokeServer()
        {
        }

        void Announcements(Core::IIPCServer* announces)
        {
            ASSERT((announces != nullptr) ^ (_handler != nullptr));
            _handler = announces;
        }

    private:
        virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message)
        {
            Core::ProxyType<Job> job(Job::Instance());

            job->Set(source, message, _handler);
            _threadPoolEngine.Submit(Core::ProxyType<Core::IDispatch>(job));
        }

    private:
        Core::WorkerPool& _threadPoolEngine;
        Core::IIPCServer* _handler;
    };

    template <const uint32_t MESSAGESLOTS, const uint16_t THREADPOOLCOUNT>
    class InvokeServerType : public Core::IIPCServer {
    public:
        InvokeServerType() = delete;
        InvokeServerType(const InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>&) = delete;
        InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>& operator = (const InvokeServerType<MESSAGESLOTS, THREADPOOLCOUNT>&) = delete;

        InvokeServerType(const uint32_t stackSize = Core::Thread::DefaultStackSize())
            : _threadPoolEngine(stackSize, _T("IPCInterfaceMessageHandler"))
            , _handler(nullptr)
        {
        }
        ~InvokeServerType()
        {
        }

        void Announcements(Core::IIPCServer* announces)
        {
            ASSERT((announces != nullptr) ^ (_handler != nullptr));
            _handler = announces;
        }

    private:
        virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message)
        {
            if (_threadPoolEngine.Pending() >= ((MESSAGESLOTS * 80) / 100)) {
                TRACE_L1("_threadPoolEngine.Pending() == %d", _threadPoolEngine.Pending());
            }

            if (message->Label() == AnnounceMessage::Id()) {
	            _handler->Procedure(source, message);
	        } else {
                _threadPoolEngine.Submit(Job(source, message, _handler), Core::infinite);
            }        
        }

    private:
        Core::ThreadPoolType<Job, THREADPOOLCOUNT, MESSAGESLOTS> _threadPoolEngine;
        Core::IIPCServer* _handler;
    };
}

} // namespace RPC

#endif // __COM_ADMINISTRATOR_H
