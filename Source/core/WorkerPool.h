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
        virtual ~IWorkerPool() = default;

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
                Revoke();
            }

        public:
            bool Submit()
            {
                ProxyType<IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Submit());

                if (job.IsValid()) {
                    IWorkerPool::Instance().Submit(job);
                }
             
                return (ThreadPool::JobType<IMPLEMENTATION>::IsIdle() == false);
            }
            bool Reschedule(const Core::Time& time)
            {
                bool rescheduled = false;

                Core::ProxyType<IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Reschedule(time));

                if (job.IsValid() == true) {

                    job = ThreadPool::JobType<IMPLEMENTATION>::Revoke();

                    if (job.IsValid() == true) {
                        rescheduled = (IWorkerPool::Instance().Revoke(job) == Core::ERROR_NONE);
                        ThreadPool::JobType<IMPLEMENTATION>::Revoked();
                    }

                    job = (ThreadPool::JobType<IMPLEMENTATION>::Idle());

                    if (job.IsValid() == true) {
                        IWorkerPool::Instance().Schedule(time, job);
                    }
                }

                return (rescheduled && (ThreadPool::JobType<IMPLEMENTATION>::IsIdle() == false));
            }
            void Revoke()
            {
                Core::ProxyType<IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Revoke());

                if (job.IsValid() == true) {
                    Core::IWorkerPool::Instance().Revoke(job);
                    ThreadPool::JobType<IMPLEMENTATION>::Revoked();
                }
            }
        };

        struct Metadata {
            std::vector<string> Pending;
            uint8_t Slots;
            ThreadPool::Metadata* Slot;
        };

        static void Assign(IWorkerPool* instance);
        static IWorkerPool& Instance();
        static bool IsAvailable();

        virtual ::ThreadId Id(const uint8_t index) const = 0;
        virtual void Submit(const Core::ProxyType<IDispatch>& job) = 0;
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<IDispatch>& job) = 0;
        virtual bool Reschedule(const Core::Time& time, const Core::ProxyType<IDispatch>& job) = 0;
        virtual uint32_t Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = Core::infinite) = 0;
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
            Timer(IWorkerPool* pool, const ProxyType<IDispatch>& job)
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
            ProxyType<IDispatch> _job;
            IWorkerPool* _pool;
        };
        class Scheduler : public ThreadPool::IScheduler {
        public:
            Scheduler() = delete;
            Scheduler(const Scheduler&) = delete;
            Scheduler& operator=(const Scheduler&) = delete;

            Scheduler(IWorkerPool* pool, Core::TimerType<Timer>& timer) : _pool(pool), _timer(timer) {
            }
            ~Scheduler() override = default;

        public:
            // Inherited via IScheduler
            void Schedule(const Time& time, const ProxyType<IDispatch>& job) override {
                _timer.Schedule(time, Timer(_pool, job));
            }

        private:
            IWorkerPool* _pool;
            Core::TimerType<Timer>& _timer;
        };

        #ifdef __CORE_WARNING_REPORTING__
        class DispatchedJobMonitor : public ThreadPool::IDispatchedJobMonitor {
        /**
        * @brief DispatchedJobMonitor monitor/maintains a list of Dispatched Jobs for analysis.
        *        This object periodically schedule an internal worker job to walk-thru the
        *        active/in-progress dispatched jobs, analyse and report warning messages
        *        based on the configured bounds values.
        *
        */
        private:
            class EXTERNAL MontiorJob : public IDispatch {
            public:
                MontiorJob() = delete;
                MontiorJob(const MontiorJob&) = delete;
                MontiorJob& operator=(const MontiorJob&) = delete;

                MontiorJob(DispatchedJobMonitor& parent)
                    : _parent(parent)
                {
                }
                ~MontiorJob() override = default;

                void Dispatch() override
                {
                    _parent.AnalyseAndReportDispatchedJobs();
                    _parent.ScheduleMonitorJob();
                }

            private:
                DispatchedJobMonitor& _parent;
            };
        public:
            static constexpr uint32_t DefaultScheduleIntervalInMilliSeconds = 10000;
            DispatchedJobMonitor() = delete;
            DispatchedJobMonitor(const DispatchedJobMonitor&) = delete;
            DispatchedJobMonitor& operator=(const DispatchedJobMonitor&) = delete;

            DispatchedJobMonitor(WorkerPool& parent, uint32_t intervalInMilliSeconds)
                : _parent(parent)
                , _lock()
                , _dispatchedJobList()
                , _monitorJob()
                , _isActive(false)
                , _intervalInMilliSeconds(intervalInMilliSeconds)
            {
            }
            ~DispatchedJobMonitor() override = default;

            void Start()
            {
                _lock.Lock();

                if (!IsActive()) {
                    _monitorJob = Core::ProxyType<IDispatch>(ProxyType<MontiorJob>::Create(*this));
                    _isActive = true;
                    _parent.Schedule(Core::Time::Now().Add(_intervalInMilliSeconds), _monitorJob);
                }

                _lock.Unlock();
            }
            void Stop()
            {
                _lock.Lock();

                if (IsActive()) {
                    _isActive = false;
                    _parent.Revoke(_monitorJob);
                    _dispatchedJobList.clear();
                    _monitorJob.Release();
                }

                _lock.Unlock();
            }
            void InsertDispatchedJobMetaData(const ThreadPool::DispatchedJobMetaData& data) override
            {
                _lock.Lock();
                _dispatchedJobList.emplace_back(data);
                _lock.Unlock();
            }
            void RemoveDispatchedJobMetaData(const ThreadPool::DispatchedJobMetaData& data) override
            {
                _lock.Lock();
                _dispatchedJobList.remove(data);
                _lock.Unlock();
            }
            void AnalyseAndReportDispatchedJobs()
            {
                _lock.Lock();

                if (_dispatchedJobList.size() > 0 && IsActive()) {
                    for (auto &job : _dispatchedJobList) {
                        ++job.ReportRunCount;
                        REPORT_OUTOFBOUNDS_WARNING_EX(WarningReporting::JobActiveForTooLong, job.CallSign.c_str(),
                        static_cast<uint32_t>((Time::Now().Ticks() - job.DispatchedTime) / Time::TicksPerMillisecond));
                    }
                }

                _lock.Unlock();

            }
            void ScheduleMonitorJob()
            {
                // Submit the monitor job to the scheduler.
                _lock.Lock();
                if (IsActive()){
                    _parent.Schedule(Core::Time::Now().Add(_intervalInMilliSeconds), _monitorJob);
                }
                _lock.Unlock();
            }
        private:
            bool IsActive()
            {
                //DO NOT use Lock here..
                return _isActive;
            }
        private:
            WorkerPool& _parent;
            CriticalSection _lock;
            std::list<ThreadPool::DispatchedJobMetaData> _dispatchedJobList;
            Core::ProxyType<IDispatch> _monitorJob;
            bool _isActive;
            uint32_t _intervalInMilliSeconds;
    };
    #endif
    public:
        WorkerPool(const WorkerPool&) = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        WorkerPool(const uint8_t threadCount, const uint32_t stackSize, const uint32_t queueSize, ThreadPool::IDispatcher* dispatcher, ThreadPool::ICallback* callback = nullptr)
            : _scheduler(this, _timer)
            , _threadPool(threadCount, stackSize, queueSize, dispatcher, &_scheduler, &_external, callback)
            , _external(_threadPool, dispatcher)
            , _timer(1024 * 1024, _T("WorkerPoolType::Timer"))
            , _metadata()
            , _joined(0)
            #ifdef __CORE_WARNING_REPORTING__
            , _dispatchedJobMonitor(*this, static_cast<uint32_t>(DispatchedJobMonitor::DefaultScheduleIntervalInMilliSeconds))
            #endif 
        {
            _metadata.Slots = threadCount + 2;
            _metadata.Slot = new Core::ThreadPool::Metadata[threadCount + 2];
        }
POP_WARNING()

        ~WorkerPool()
        {
            _threadPool.Stop();
            delete[] _metadata.Slot;
        }

    public:
        void Submit(const Core::ProxyType<IDispatch>& job) override
        {
            // A job should always be submitted only once, see if te offered job does not reside in the _timer...
            ASSERT(_timer.HasEntry(Timer(this, job)) == false);

            _threadPool.Submit(job, Core::infinite);
        }
        void Schedule(const Core::Time& time, const Core::ProxyType<IDispatch>& job) override
        {
            if (time > Core::Time::Now()) {
                ASSERT(job.IsValid() == true);
                ASSERT(_timer.HasEntry(Timer(this, job)) == false);

                _timer.Schedule(time, Timer(this, job));
            }
            else {
                _threadPool.Submit(job, Core::infinite);
            }
        }
        bool Reschedule(const Core::Time& time, const Core::ProxyType<IDispatch>& job) override
        {
            ASSERT(job.IsValid() == true);

            bool revoked = (Revoke(job) != Core::ERROR_UNKNOWN_KEY);
            Schedule(time, job);
            return (revoked);
        }
        uint32_t Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = Core::infinite) override
        {
            uint32_t result(_timer.Revoke(Timer(this, job)) ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY);

            uint32_t report = _threadPool.Revoke(job, waitTime);

            if (report == Core::ERROR_UNKNOWN_KEY) {
                report = _external.Completed(job, waitTime);
                
                if ( (report != Core::ERROR_UNKNOWN_KEY) && (result == Core::ERROR_UNKNOWN_KEY) ) {
                    result = report;
                }
            }

            return (result);
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
        const Metadata& Snapshot() const
        {
            _metadata.Slot[0].WorkerId = _timer.ThreadId();
            _metadata.Slot[0].Runs = _timer.Pending();
            _metadata.Slot[0].Job = string(_T("WorkerPool::Timer"));
            _external.Info(_metadata.Slot[1]);
            _threadPool.Snapshot(_threadPool.Count(), &(_metadata.Slot[2]), _metadata.Pending);
            _metadata.Slot[1].WorkerId = _joined;
            return (_metadata);
        }
        void Run()
        {
            _threadPool.Run();
            #ifdef __CORE_WARNING_REPORTING__
            _threadPool.SetDispatchedJobMonitor(&_dispatchedJobMonitor);
            _dispatchedJobMonitor.Start();
            #endif
        }
        void Stop()
        {
            #ifdef __CORE_WARNING_REPORTING__
            _dispatchedJobMonitor.Stop();
            _threadPool.ResetDispatchedJobMonitor();
            #endif
            _threadPool.Stop();
        }

    private:
        Scheduler _scheduler;
        ThreadPool _threadPool;
        ThreadPool::Minion _external;
        Core::TimerType<Timer> _timer;
        mutable Metadata _metadata;
        ::ThreadId _joined;
        #ifdef __CORE_WARNING_REPORTING__
        DispatchedJobMonitor _dispatchedJobMonitor;
        #endif
    };
}
}
