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

#include "Thread.h"
#include "ThreadPool.h"
#include "Timer.h"

namespace WPEFramework {

namespace Core {

    struct EXTERNAL IWorkerPool {
        virtual ~IWorkerPool(){};

        template <typename IMPLEMENTATION>
        class JobType : public ThreadPool::JobType<IMPLEMENTATION> {
        public:
            JobType(const JobType<IMPLEMENTATION>&) = delete;
            JobType<IMPLEMENTATION>& operator=(const JobType<IMPLEMENTATION>&) = delete;

            template <typename... Args>
            JobType(Args&&... args)
                : ThreadPool::JobType<IMPLEMENTATION>(std::forward<Args>(args)...)
            {
            }
            ~JobType()
            {
                Core::IWorkerPool::Instance().Revoke(ThreadPool::JobType<IMPLEMENTATION>::Reset());
            }

        public:
            bool Submit()
            {
                bool result = false;
                Core::ProxyType<Core::IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Aquire());

                if (job.IsValid()) {
                    Core::IWorkerPool::Instance().Submit(job);
                    result = true;
                }

                return result;
            }
            bool Schedule(const Core::Time& time)
            {
                bool result = false;
                Core::ProxyType<Core::IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Aquire());

                if (job.IsValid()) {
                    Core::IWorkerPool::Instance().Schedule(time, job);
                    result = true;
                }
                return (result);
            }
            bool Reschedule(const Core::Time& time)
            {
                Core::ProxyType<Core::IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Forced());

                ASSERT(job.IsValid() == true);

                return (Core::IWorkerPool::Instance().Reschedule(time, job));
            }
            void Revoke()
            {
                Core::IWorkerPool::Instance().Revoke(ThreadPool::JobType<IMPLEMENTATION>::Reset());
            }
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

        virtual ::ThreadId Id(const uint8_t index) const = 0;
        virtual void Submit(const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual bool Reschedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) = 0;
        virtual void Join() = 0;
        virtual const Metadata& Snapshot() const = 0;
    };

    class EXTERNAL WorkerPool : public IWorkerPool {
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
                ASSERT(_pool != nullptr);
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

        WorkerPool(const uint8_t threadCount, const uint32_t stackSize, const uint32_t queueSize, Core::ThreadPool::IDispatcher* dispatcher)
            : _threadPool(threadCount, stackSize, queueSize, dispatcher)
            , _external(_threadPool.Queue(), dispatcher)
            , _timer(1024 * 1024, _T("WorkerPoolType::Timer"))
            , _metadata()
            , _joined(0)
        {
            _metadata.Slots = threadCount + 1;
            _metadata.Slot = new uint32_t[threadCount + 1];
        }
        ~WorkerPool()
        {
            _threadPool.Stop();
            delete[] _metadata.Slot;
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
        bool Reschedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) override
        {
            bool rescheduled = false;
            if (_timer.Revoke(Timer(this, job)) == true) {
                rescheduled = true;
                if (time > Core::Time::Now()) {
                    _timer.Schedule(time, Timer(this, job));
                }
                else {
                    _threadPool.Submit(job, Core::infinite);
                }
            }
            return (rescheduled);
        }
        uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) override
        {
            _timer.Revoke(Timer(this, job));

            uint32_t result = _threadPool.Revoke(job, waitTime);

            uint32_t outcome = _external.Completed(job, waitTime);

            return (outcome != Core::ERROR_NONE ? outcome : result);
        }
        void Join() override
        {
            _joined = Thread::ThreadId();
            _external.Process();
            _joined = 0;
        }
        ::ThreadId Id(const uint8_t index) const override
        {
            ::ThreadId result = (::ThreadId)(~0);

            if (index == 0) {
                result = _timer.ThreadId();
            } else if (index == 1) {
                result = _joined;
            } else if ((index - 2) < _threadPool.Count()) {
                result = _threadPool.Id(index - 2);
            }

            return (result);
        }
        virtual const Metadata& Snapshot() const
        {

            _metadata.Pending = _threadPool.Pending();
            _metadata.Occupation = _threadPool.Active();
            _metadata.Slot[0] = _external.Runs();

            _threadPool.Runs(_threadPool.Count(), &(_metadata.Slot[1]));

            return (_metadata);
        }
        void Run()
        {
            _threadPool.Run();
        }
        void Stop()
        {
            _threadPool.Stop();
        }

    protected:
        inline void Shutdown()
        {
            _threadPool.Queue().Disable();
        }

    private:
        ThreadPool _threadPool;
        ThreadPool::Minion _external;
        Core::TimerType<Timer> _timer;
        mutable Metadata _metadata;
        ::ThreadId _joined;
    };

    class DecoupledJob : private Core::ThreadPool::JobType<DecoupledJob&> {
    public:
        using Job = std::function<void()>;

        DecoupledJob(const DecoupledJob&) = delete;
        DecoupledJob& operator=(const DecoupledJob&) = delete;

        DecoupledJob()
            : Core::ThreadPool::JobType<DecoupledJob&>(*this)
            , _lock()
            , _job(nullptr)
        {
        }

        ~DecoupledJob()
        {
            Revoke();
        }

    public:
        bool Submit(const Job& job, const uint32_t defer = 0)
        {
            bool submitted = false;
            Core::ProxyType<Core::IDispatch> handler(Aquire());

            _lock.Lock();
            if (handler.IsValid() == true) {

                _job = job;

                if (defer == 0) {
                    Core::WorkerPool::Instance().Submit(handler);
                } else {
                    Core::WorkerPool::Instance().Schedule(Core::Time::Now().Add(defer), handler);
                }

                submitted = true;
            }
            _lock.Unlock();

            return submitted;
        }

        void Revoke()
        {
            _lock.Lock();
            Core::WorkerPool::Instance().Revoke(Reset());
            _job = nullptr;
            _lock.Unlock();
        }

    private:
        friend class Core::ThreadPool::JobType<DecoupledJob&>;
        void Dispatch()
        {
            _lock.Lock();
            ASSERT(_job != nullptr);
            Job job = _job;
            _job = nullptr;
            _lock.Unlock();
            job();
        }

    private:
        Core::CriticalSection _lock;
        Job _job;
    };
}
}
