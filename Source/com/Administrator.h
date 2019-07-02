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

        void DeleteChannel(const Core::ProxyType<Core::IPCChannel>& channel, std::list<ProxyStub::UnknownProxy*>& pendingProxies, std::list<ExposedInterface>& usedInterfaces)
        {
            _adminLock.Lock();

            ChannelMap::iterator index(_channelProxyMap.find(channel.operator->()));

            if (index != _channelProxyMap.end()) {
                ProxyList::iterator loop(index->second.begin());
                while (loop != index->second.end()) {
                    pendingProxies.push_back(*loop);
                    loop++;
                }
                _channelProxyMap.erase(index);
            }
            ReferenceMap::iterator remotes(_channelReferenceMap.find(channel.operator->()));

            if (remotes != _channelReferenceMap.end()) {
                std::list<ExternalReference>::iterator loop(remotes->second.begin());
                while (loop != remotes->second.end()) {
                    usedInterfaces.emplace_back(loop->Source(), loop->RefCount());
                    loop++;
                }
                _channelReferenceMap.erase(remotes);
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

    class EXTERNAL Job {
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
        ~Job()
        {
        }

        Job& operator=(const Job& rhs)
        {
            _message = rhs._message;
            _channel = rhs._channel;
            _handler = rhs._handler;

            return (*this);
		}

    public:
        void Dispatch()
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

    protected:
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

    private:
        Core::ProxyType<Core::IIPC> _message;
        Core::ProxyType<Core::IPCChannel> _channel;
        Core::IIPCServer* _handler;
        static Administrator& _administrator;
    };

    struct EXTERNAL WorkerPool {

	    struct Metadata {
		    uint32_t Pending;
            uint32_t Occupation;
            uint32_t Slots;
            uint32_t* Slot;
        };

        virtual ~WorkerPool() = default;

		static WorkerPool& Instance();
        static void Instance(WorkerPool& instance);

        virtual void Submit(const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) = 0;
        virtual const Metadata& Snapshot() const = 0;
        virtual bool IsAvailable() = 0;
    };

	template <const uint8_t THREAD_COUNT>
	class WorkerPoolType : public WorkerPool {
    private:
        class TimedJob {
        public:
            TimedJob()
                : _job()
            {
            }
            TimedJob(const Core::ProxyType<Core::IDispatch>& job)
                : _job(job)
            {
            }
            TimedJob(const TimedJob& copy)
                : _job(copy._job)
            {
            }
            ~TimedJob()
            {
            }

            TimedJob& operator=(const TimedJob& RHS)
            {
                _job = RHS._job;
                return (*this);
            }
            bool operator==(const TimedJob& RHS) const
            {
                return (_job == RHS._job);
            }
            bool operator!=(const TimedJob& RHS) const
            {
                return (_job != RHS._job);
            }

        public:
            uint64_t Timed(const uint64_t /* scheduledTime */)
            {
                WorkerPoolType <THREAD_COUNT>::Instance().Submit(_job);
                _job.Release();

                // No need to reschedule, just drop it..
                return (0);
            }

        private:
            Core::ProxyType<Core::IDispatchType<void>> _job;
        };

        typedef Core::ThreadPoolType<Core::Job, THREAD_COUNT> ThreadPool;

    public:
        WorkerPoolType() = delete;
        WorkerPoolType(const WorkerPoolType<THREAD_COUNT>&) = delete;
        WorkerPoolType<THREAD_COUNT>& operator=(const WorkerPoolType<THREAD_COUNT>&) = delete;

        WorkerPoolType(const uint32_t stackSize)
            : _workers(stackSize, _T("WorkerPool::Implementation"))
            , _timer(stackSize, _T("WorkerPool::Timer"))
        {
        }
        virtual ~WorkerPoolType()
        {
        }

    public:
        // A-synchronous calls. If the method returns, the workers are accepting and handling work.
        inline void Run()
        {
            _workers.Run();
        }
        // A-synchronous calls. If the method returns, the workers are all blocked, no new work will
        // be accepted. Work in progress will be completed. Use the WaitState to wait for the actual block.
        inline void Block()
        {
            _workers.Block();
        }
        inline void Wait(const uint32_t waitState, const uint32_t time)
        {
            _workers.Wait(waitState, time);
        }
        virtual void Submit(const Core::ProxyType<Core::IDispatch>& job) override
        {
            _workers.Submit(Core::Job(job), Core::infinite);
        }
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) override
        {
            _timer.Schedule(time, TimedJob(job));
        }
        virtual uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) override
        {
            // First check the timer if it can be removed from there.
            _timer.Revoke(TimedJob(job));

            // Also make sure it is taken of the WorkerPoolImplementation, if applicable.
            return (_workers.Revoke(Core::Job(job), waitTime));
        }
        virtual const Metadata& Snapshot() const override
        {
            _snapshot.Pending = _workers.Pending();
            _snapshot.Occupation = _workers.Active();
            _snapshot.Slots = THREAD_COUNT;
            _snapshot.Slot = _slots;

            for (uint8_t teller = 0; (teller < THREAD_COUNT); teller++) {
                // Example of why copy-constructor and assignment constructor should be equal...
                _slots[teller] = _workers[teller].Runs();
            }

			return (_snapshot);
        }
        inline ::ThreadId ThreadId(const uint8_t index) const
        {
            return (index == 0 ? _timer.ThreadId() : _workers.ThreadId(index - 1));
        }
        virtual bool IsAvailable() override
        {
            return true;
        }

    private:
        ThreadPool _workers;
        Core::TimerType<TimedJob> _timer;
        mutable Metadata _snapshot;
        mutable uint32_t _slots[THREAD_COUNT];
    };

    class EXTERNAL InvokeServer : public Core::IIPCServer {
    private:
        class DispatchJob : public Core::IDispatch, public RPC::Job {
        public:
            DispatchJob(const DispatchJob&) = delete;
            DispatchJob& operator=(const DispatchJob&) = delete;

            DispatchJob()
                : RPC::Job()
            {
            }
            virtual ~DispatchJob()
            {
            }

        public:
            inline void Clear()
            {
                RPC::Job::Clear();
            }
            inline void Set(Core::IPCChannel& channel, const Core::ProxyType<Core::IIPC>& message, Core::IIPCServer* handler)
            {
                RPC::Job::Set(channel, message, handler);
            }
            virtual void Dispatch() override
            {
                RPC::Job::Dispatch();
            }
        };

    public:
        InvokeServer(const InvokeServer&) = delete;
        InvokeServer& operator=(const InvokeServer&) = delete;

        InvokeServer()
            : _threadPoolEngine(RPC::WorkerPool::Instance())
            , _handler(nullptr)
        {
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
            Core::ProxyType<DispatchJob> job(_factory.Element());

            job->Set(source, message, _handler);
            _threadPoolEngine.Submit(Core::ProxyType<Core::IDispatch>(job));
        }

    private:
        RPC::WorkerPool& _threadPoolEngine;
        Core::IIPCServer* _handler;
        static Core::ProxyPoolType<DispatchJob> _factory;
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

            _threadPoolEngine.Submit(Job(source, message, _handler), Core::infinite);
        }

    private:
        Core::ThreadPoolType<Job, THREADPOOLCOUNT, MESSAGESLOTS> _threadPoolEngine;
        Core::IIPCServer* _handler;
    };
}

} // namespace RPC

#endif // __COM_ADMINISTRATOR_H
