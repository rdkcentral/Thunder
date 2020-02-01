#pragma once

#include "Thread.h"
#include "Timer.h"
#include <atomic>
#include <functional>

namespace WPEFramework {

namespace Core {

    struct EXTERNAL IWorkerPool {
        virtual ~IWorkerPool() {};

        template <typename IMPLEMENTATION>
        class JobType {
        private:
            class Worker : public Core::IDispatch {
            public:
                Worker() = delete;
                Worker(const Worker&) = delete;
                Worker& operator=(const Worker&) = delete;

                Worker(JobType<IMPLEMENTATION>* parent) : _parent(*parent) {
                }
                ~Worker() override {
                }

            private:
                void Dispatch() override {
                    _parent.Dispatch();
                }

            private:
                 JobType<IMPLEMENTATION>& _parent;
            };

        public:
            JobType(const JobType<IMPLEMENTATION>&) = delete;
            JobType<IMPLEMENTATION>& operator=(const JobType<IMPLEMENTATION>&) = delete;
            template <typename... Args>
            JobType(Args... args)
                : _implementation(args...)
                , _submitted(false)
                , _job(this)
            {
                _job.AddRef();                
            }
            ~JobType()
            {
                _submitted.store(false, std::memory_order_relaxed);
                Core::IWorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(&_job, &_job));
                _job.CompositRelease();
            }

         public:
            void Submit()
            {
                bool expected = false;
                if (_submitted.compare_exchange_strong(expected, true) == true) {
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(&_job, &_job));
                }
            }

        private:
            virtual void Dispatch()
            {
                bool expected = true;
                if (_submitted.compare_exchange_strong(expected, false) == true) {
                    _implementation.Dispatch();
                }
            }

        private:
            IMPLEMENTATION _implementation;
            std::atomic<bool> _submitted;
            ProxyObject<Worker> _job;
        };

        struct Metadata {
            uint32_t Pending;
            uint32_t Occupation;
            uint8_t Slots;
            uint32_t* Slot;
        };

	static void Assign(IWorkerPool* instance);
	static IWorkerPool& Instance();
	static bool IsAvailable();

        virtual ThreadId Id(const uint8_t index) const = 0;
        virtual void Submit(const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) = 0;
        virtual void Join() = 0;
        virtual const Metadata& Snapshot() const = 0;
    };

    class WorkerPool : public IWorkerPool {
    private:
        class Timer {
        public:
            Timer& operator=(const Timer& RHS) = delete;
            Timer()
                : _job()
                , _pool(nullptr)
            {
            }
            Timer(const Timer& copy)
                : _job(copy._job)
                , _pool(copy._pool)
            {
            }
            Timer(IWorkerPool* pool, const Core::ProxyType<Core::IDispatch>& job)
                : _job(job)
                , _pool(pool)
            {
            }
            ~Timer()
            {
            }

        public:
            bool operator==(const Timer& RHS) const
            {
                return (_job == RHS._job);
            }
            bool operator!=(const Timer& RHS) const
            {
                return (!operator==(RHS));
            }
            uint64_t Timed(const uint64_t /* scheduledTime */)
            {
                ASSERT (_pool != nullptr);
                _pool->Submit(_job);
                _job.Release();

                // No need to reschedule, just drop it..
                return (0);
            }

        private:
            Core::ProxyType<Core::IDispatch> _job;
            IWorkerPool* _pool;
        };

    public:
        WorkerPool(const WorkerPool&) = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

        WorkerPool(const uint8_t threadCount, const uint32_t stackSize, const uint32_t queueSize) 
            : _threadPool(threadCount, stackSize, queueSize)
            , _external(_threadPool.Queue())
            , _timer(1024 * 1024, _T("WorkerPoolType::Timer"))
            , _metadata() 
            , _joined(0)
        {
            _metadata.Slots = threadCount + 1;
            _metadata.Slot = new uint32_t [threadCount + 1];

            _threadPool.Run();
        }
        ~WorkerPool() {
            _threadPool.Stop();
            delete [] _metadata.Slot;
        }

    public:
        void Submit(const Core::ProxyType<Core::IDispatch>& job) override
        {
            _threadPool.Submit(job, Core::infinite);
        }
        void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) override
        {
            _timer.Schedule(time, Timer(this, job));
        }
        uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) override
        {
            _timer.Revoke(Timer(this, job));

            uint32_t result = _threadPool.Revoke (job, waitTime);

            uint32_t outcome = _external.Completed(job, waitTime);

            return (outcome != Core::ERROR_NONE ? outcome : result);
        }
	void Join() override {
            _joined = Thread::ThreadId();
            _external.Process();
            _joined = 0;
	}
        ::ThreadId Id(const uint8_t index) const override {
            ThreadId result = ~0;

            if (index == 0) {
                result = _timer.ThreadId();
            }
            else if (index == 1) {
                result = _joined;
            }
            else if ((index-2) < _threadPool.Count()) {
                result = _threadPool.Id(index-2);
            }

            return (result);
        }
        virtual const Metadata& Snapshot() const {
            
            _metadata.Pending = _threadPool.Pending();
            _metadata.Occupation = _threadPool.Active();
            _metadata.Slot[0] = _external.Runs();

            _threadPool.Runs(_threadPool.Count(), &(_metadata.Slot[1]));

            return (_metadata);
        }
        void Run() {
            _threadPool.Run();
        }
        void Stop () {
            _threadPool.Stop();
        }

    private:
        ThreadPool _threadPool;
        ThreadPool::Minion _external;
        Core::TimerType<Timer> _timer;
        mutable Metadata _metadata;
        ::ThreadId _joined;
    };
}
}
