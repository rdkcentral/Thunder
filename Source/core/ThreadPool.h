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
#include "ResourceMonitor.h"
#include "Number.h"

namespace WPEFramework {

namespace Core {

    class EXTERNAL ThreadPool {
    public:
        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;
            virtual void Idle() = 0;
        };
        struct EXTERNAL IJob : public IDispatch {
            virtual ~IJob() override = default;

            virtual ProxyType<IDispatch> Resubmit(Time& time) = 0;
        };
        struct EXTERNAL IScheduler {
            virtual ~IScheduler() = default;

            virtual void Schedule(const Time& time, const ProxyType<IDispatch>& job) = 0;
        };
        struct EXTERNAL IDispatcher {
            virtual ~IDispatcher() = default;

            virtual void Initialize() = 0;
            virtual void Deinitialize() = 0;
            virtual void Dispatch(IDispatch*) = 0;
        };
        struct EXTERNAL Metadata {
            ::ThreadId                  WorkerId;
            uint32_t                    Runs;
            Core::OptionalType<string>  Job;
        };

        #ifdef __CORE_WARNING_REPORTING__
        struct EXTERNAL DispatchedJobMetaData {
            ::ThreadId WorkerId;
            string     CallSign;
            uint64_t   DispatchedTime;
            uint32_t   ReportRunCount;

            bool operator==(const DispatchedJobMetaData& other) const
            {
                //return std::tie(Id, DispatchedTime) == std::tie(other.Id, other.DispatchedTime);
                return (WorkerId == other.WorkerId && DispatchedTime == other.DispatchedTime);
            }
        };

        struct EXTERNAL IDispatchedJobMonitor {
            virtual ~IDispatchedJobMonitor() = default;

            virtual void InsertDispatchedJobMetaData(const DispatchedJobMetaData& data) = 0;
            virtual void RemoveDispatchedJobMetaData(const DispatchedJobMetaData& data) = 0;
        };
        #endif

    private:
        #ifdef __CORE_WARNING_REPORTING__
        class MeasurableJob {
        public:
            /**
             * @brief Measurable job is used with warning reporting to measure 
             *        time job was in queue and it's execution time.
             *        
             *        NOTE: Constructor is not marked as explicit to allow implicit
             *        conversion from ProxyType<IDispatch> to MeasurableJob in
             *        QueueType methods such as Post or Insert.
             */
            MeasurableJob(MeasurableJob&&) = delete;

            MeasurableJob()
                : _job()
                , _time(NumberType<uint64_t>::Max())
            {
            }
            MeasurableJob(const ProxyType<IDispatch>& job)
                : _job(job)
                , _time(Time::Now().Ticks())
            {
            }
            MeasurableJob(const MeasurableJob&) = default;
            ~MeasurableJob() {
                if (_job.IsValid() == true) {
                    _job.Release();
                }
            }

            MeasurableJob& operator=(const MeasurableJob&) = default;

        public:
            bool operator==(const MeasurableJob& other) const
            {
                return _job == other._job;
            }
            bool operator!=(const MeasurableJob& other) const
            {
                return _job != other._job;
            }
            IJob* Process(IDispatcher* dispatcher)
            {
                ASSERT(dispatcher != nullptr);
                ASSERT(_job.IsValid());
                ASSERT(_time != NumberType<uint64_t>::Max());

                IDispatch* request = &(*_job);

                REPORT_OUTOFBOUNDS_WARNING(WarningReporting::JobTooLongWaitingInQueue, static_cast<uint32_t>((Time::Now().Ticks() - _time) / Time::TicksPerMillisecond));
                REPORT_DURATION_WARNING({ dispatcher->Dispatch(request); }, WarningReporting::JobTooLongToFinish);

                return (dynamic_cast<IJob*>(request));
            }
            bool IsValid() const
            {
                return _job.IsValid();
            }

            uint32_t Release() const 
            {
                return _job.Release();
            }

            IDispatch* operator->() const
            {
                ASSERT( IsValid() == true );

                return (_job.operator->());
            }

            IDispatch& operator*() const
            {
                ASSERT( IsValid() == true );

                return (_job.operator*());
            }

        private:
            ProxyType<IDispatch> _job;
            uint64_t _time;
        };
        using QueueElement = MeasurableJob;
        #else
        using QueueElement = ProxyType<IDispatch>;
        #endif

        using MessageQueue = QueueType< QueueElement >;

    public:   
        template<typename IMPLEMENTATION>
        class JobType {
        private:
            enum state : uint8_t {
                IDLE,
                SUBMITTED,
                EXECUTING,
                RESUBMIT,
                SCHEDULE,
                REVOKING
            };

            class Worker : public IJob {
            public:
                Worker() = delete;
                Worker(const Worker&) = delete;
                Worker& operator=(const Worker&) = delete;

                Worker(JobType<IMPLEMENTATION>& parent) : _parent(parent) {
                }
                ~Worker() override = default;

            public:
                ProxyType<IDispatch> Resubmit(Time& time) override {
                    return (_parent.Resubmit(time));
                }
                void Dispatch() override {
                    _parent.Dispatch();
                }

            private:
                 JobType<IMPLEMENTATION>& _parent;
            };

        public:
            JobType(const JobType<IMPLEMENTATION>& copy) = delete;
            JobType<IMPLEMENTATION>& operator=(const JobType<IMPLEMENTATION>& RHS) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            template <typename... Args>
            JobType(Args&&... args)
                : _implementation(args...)
                , _state(IDLE)
                , _job(*this)
                , _time()
            {
                _job.AddRef();
            }
POP_WARNING()
            ~JobType()
            {
                ASSERT (_state == IDLE);
                _job.CompositRelease();
            }

        public:
            bool IsIdle() const {
                return (_state == IDLE);
            }
            ProxyType<IDispatch> Idle() {

                state idle = IDLE;

                ProxyType<IDispatch> result;

                if (_state.compare_exchange_strong(idle, SUBMITTED) == true) {
                    result = ProxyType<IDispatch>(ProxyType<Worker>(_job));
                }

                return (result);
            }
            ProxyType<IDispatch> Submit() {

                state executing = EXECUTING;
                state schedule = SCHEDULE;
                state idle = IDLE;

                ProxyType<IDispatch> result;

                if ( (_state.compare_exchange_strong(executing, RESUBMIT)  == false) &&
                     (_state.compare_exchange_strong(schedule,  RESUBMIT)  == false) &&
                     (_state.compare_exchange_strong(idle,      SUBMITTED) == true ) ) {
                    result = ProxyType<IDispatch>(ProxyType<Worker>(_job));
                }

                return (result);
            }
            ProxyType<IDispatch> Reschedule(const Time& time) {

                state executing = EXECUTING;
                state submitted = SUBMITTED;
                state resubmit = RESUBMIT;
                state idle = IDLE;

                ProxyType<IDispatch> result;

                if ( (_state.compare_exchange_strong(executing,   SCHEDULE) == false) &&
                     (_state.compare_exchange_strong(resubmit,    SCHEDULE) == false) &&
                     ( (_state.compare_exchange_strong(submitted, SCHEDULE) == true)  || 
                       (_state.compare_exchange_strong(idle,      SCHEDULE) == true) ) ) {
                    result = ProxyType<IDispatch>(ProxyType<Worker>(_job));
                }
                else {
                    _time = time;
                }

                return (result);
            }
            ProxyType<IDispatch> Revoke() {
                ProxyType<IDispatch> result;
                if (RevokeRequired() == true) {
                    result = ProxyType<IDispatch>(ProxyType<Worker>(_job));
                }
                return (result);
            }
            void Revoked() {
                state expected = REVOKING;
                VARIABLE_IS_NOT_USED bool result = _state.compare_exchange_strong(expected, IDLE);
                ASSERT(result == true);
            }
            operator IMPLEMENTATION& () {
                return (_implementation);
            }
            operator const IMPLEMENTATION& () const {
                return (_implementation);
            }

        private:
            friend class ThreadPool;

            ProxyType<IDispatch> Resubmit(Time& time) {
                ProxyType<IDispatch> result;
                state executing = EXECUTING;

                if (_state.compare_exchange_strong(executing, IDLE) == false) {
                    state resubmit = RESUBMIT;
                    state schedule = SCHEDULE;
                    if (_state.compare_exchange_strong(resubmit, SUBMITTED) == true) {
                        result = ProxyType<IDispatch>(ProxyType<Worker>(_job));
                    }
                    else if (_state.compare_exchange_strong(schedule, SUBMITTED) == true) {
                        time = _time;
                        result = ProxyType<IDispatch>(ProxyType<Worker>(_job));
                    }
                }
                return (result);
            }
            bool RevokeRequired() {

                bool result = (_state == REVOKING);

                if (result == false) {
                    state submitted = SUBMITTED;
                    state executing = EXECUTING;
                    state resubmit = RESUBMIT;
                    state schedule = SCHEDULE;

                    if ((_state.compare_exchange_strong(submitted, REVOKING) == true) ||
                        (_state.compare_exchange_strong(executing, REVOKING) == true) ||
                        (_state.compare_exchange_strong(resubmit,  REVOKING) == true) ||
                        (_state.compare_exchange_strong(schedule,  REVOKING) == true) ) {
                        result = true;
                    }
                }

                return (result);
            }
            void Dispatch() {
                state expected = SUBMITTED;
                if (_state.compare_exchange_strong(expected, EXECUTING) == true) {
                    _implementation.Dispatch();
                }
            }
            // -----------------------------------------------------
            // Check for Clear method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(JobIdentifier, hasJobIdentifier);

            template <typename TYPE = IMPLEMENTATION>
            inline typename Core::TypeTraits::enable_if<hasJobIdentifier<const TYPE, string>::value, string>::type
                Identifier() const
            {
                return (_implementation.JobIdentifier());
            }
            template <typename TYPE = IMPLEMENTATION>
            inline typename Core::TypeTraits::enable_if<!hasJobIdentifier<const TYPE, string>::value, string>::type
                Identifier() const
            {
                return("UnknownJob");
            }

        private:
            IMPLEMENTATION _implementation;
            std::atomic<state> _state;
            ProxyObject<Worker> _job;
            Time _time;
        };

        class EXTERNAL Minion {
        public:
            Minion(const Minion&) = delete;
            Minion& operator=(const Minion&) = delete;

            Minion(ThreadPool& parent, IDispatcher* dispatcher)
                : _parent(parent)
                , _dispatcher(dispatcher)
                , _adminLock()
                , _signal(false, true)
                , _interestCount(0)
                , _currentRequest()
                , _runs(0)
            {
                ASSERT(dispatcher != nullptr);
            }
            ~Minion() = default;

        public:
            bool IsActive() const {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                return (_currentRequest.IsValid());
            }
            void Info(Metadata& info) const {
                info.Runs = _runs;

		        _adminLock.Lock();
                if (_currentRequest.IsValid() == false) {
		            _adminLock.Unlock();
                    info.Job.Clear();
                }
                else {
                    info.Job = _currentRequest->Identifier();
		            _adminLock.Unlock();
                }
            }
            uint32_t Completed (const ProxyType<IDispatch>& job, const uint32_t waitTime) {
                uint32_t result = ERROR_UNKNOWN_KEY;

                _adminLock.Lock();

                if (_currentRequest != job) {
                    _adminLock.Unlock();
                }
                else {
                    _interestCount++;
                    _adminLock.Unlock();
                    result = _signal.Lock(waitTime);
                    _interestCount--;
                }

                return(result);
            }
            void Process()
            {
                _dispatcher->Initialize();

                while (_parent._queue.Extract(_currentRequest, infinite) == true) {

                    ASSERT(_currentRequest.IsValid() == true);

                    _runs++;

                    #ifdef __CORE_WARNING_REPORTING__
                    // Add an entry into the JobMonitor list
                    DispatchedJobMetaData data{Thread::ThreadId(),
                        string(WPEFramework::Core::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign()),
                        Time::Now().Ticks(), 0};

                    _parent.SaveDispatchedJobContext(data);

                    IJob* job = _currentRequest.Process(_dispatcher);

                    _parent.RemoveDispatchedJobContext(data);

                    if (job != nullptr) {
                        // Maybe we need to reschedule this request....
                        _parent.Closure(*job);
                    }
                    #else
                    IDispatch* request = &(*_currentRequest);

                    _dispatcher->Dispatch(request); 

                    IJob* job = dynamic_cast<IJob*>(request);

                    if (job != nullptr) {
                        // Maybe we need to reschedule this request....
                        _parent.Closure(*job);
                    }

                    #endif

                    // if someone is observing this run, (WaitForCompletion) make sure that
                    // thread, sees that his object was running and is now completed.
                    _adminLock.Lock();

                    _currentRequest.Release();

                    if (_interestCount > 0) {

                        _signal.SetEvent();

                        while (_interestCount > 0) {
                            std::this_thread::yield();
                        }

                        _signal.ResetEvent();
                    }
                    _adminLock.Unlock();

                    _parent.Idle();
                }

                _dispatcher->Deinitialize();
            }

        private:
            ThreadPool& _parent;
            IDispatcher* _dispatcher;
            mutable CriticalSection _adminLock;
            Event _signal;
            std::atomic<uint32_t> _interestCount;
            #ifdef __CORE_WARNING_REPORTING__
            MeasurableJob _currentRequest;
            #else
            ProxyType<IDispatch> _currentRequest;
            #endif
            uint32_t _runs;
        };

    private:
        class EXTERNAL Executor : public Thread {
        public:
            Executor() = delete;
            Executor(const Executor&) = delete;
            Executor& operator=(const Executor&) = delete;

            Executor(ThreadPool& parent, IDispatcher* dispatcher, const uint32_t stackSize, const TCHAR* name)
                : Thread(stackSize == 0 ? Thread::DefaultStackSize() : stackSize, name)
                , _minion(parent, dispatcher)
            {
            }
            ~Executor() override
            {
                Thread::Stop();
                Wait(Thread::STOPPED, infinite);
            }

        public:
            bool IsActive() const {
                return (_minion.IsActive());
            }
            void Info(Metadata& info) const {
                _minion.Info(info);
                info.WorkerId = Id();
            }
            void Run () {
                Thread::Run();
            }
            void Stop () {
                Thread::Wait(Thread::STOPPED|Thread::BLOCKED, infinite);
            }
            Minion& Me() {
                return (_minion);
            }

        private:
            uint32_t Worker() override
            {
                _minion.Process();
                Thread::Block();
                return (infinite);
            }

        private:
            Minion _minion;
        };

    public:
        ThreadPool(const ThreadPool& a_Copy) = delete;
        ThreadPool& operator=(const ThreadPool& a_RHS) = delete;

        ThreadPool(const uint8_t count, const uint32_t stackSize, const uint32_t queueSize, IDispatcher* dispatcher, IScheduler* scheduler, Minion* external, ICallback* callback) 
            : _queue(queueSize)
            , _scheduler(scheduler)
            #ifdef __CORE_WARNING_REPORTING__
            , _dispatchedJobMonitor(nullptr)
            #endif
            , _external(external)
            , _callback(callback)
        {
            const TCHAR* name = _T("WorkerPool::Thread");
            for (uint8_t index = 0; index < count; index++) {
                _units.emplace_back(*this, dispatcher, stackSize, name);
            }
        }
        ~ThreadPool() {
            Stop();
            _units.clear();
        }

    public:
        uint8_t Count() const
        {
            return (static_cast<uint8_t>(_units.size()));
        }
        uint32_t Pending() const {
            return (_queue.Length());
        }
        void Snapshot(const uint8_t length, Metadata* entries, std::vector<string>& jobs) const
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();

            // Make sure jobs do not move while we are creating a snapshot !!
            _queue.Lock();

            while ((count < length) && (ptr != _units.cend())) { 
                ptr->Info(entries[count]);
                ptr++; 
                count++; 
            }
            _queue.Visit([&](const QueueElement& element) {
                jobs.emplace_back(element->Identifier());
                });

            _queue.Unlock();
        }
        ::ThreadId Id(const uint8_t index) const
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while ((index != count) && (ptr != _units.cend())) { ptr++; count++; }

            ASSERT (ptr != _units.cend());

            return (ptr != _units.cend() ? ptr->Id() : 0);
        }
        void Submit(const ProxyType<IDispatch>& job, const uint32_t waitTime)
        {
            ASSERT(job.IsValid() == true);
            ASSERT(_queue.HasEntry(job) == false);

            if (Thread::ThreadId() == ResourceMonitor::Instance().Id()) {
                _queue.Post(job);
            }
            else {
                _queue.Insert(job, waitTime);
            }

        }
        uint32_t Revoke(const ProxyType<IDispatch>& job, const uint32_t waitTime)
        {
            uint32_t result = ERROR_UNKNOWN_KEY;

            ASSERT(job.IsValid() == true);

            if (_queue.Remove(job) == true) {
                result = ERROR_NONE;
            }
            else {
                // Check if it is currently being executed and wait till it is done.
                std::list<Executor>::iterator index = _units.begin();

                while ((result == ERROR_UNKNOWN_KEY) && (index != _units.end())) {
                    // If we are the running job, no need to revoke ourselves, I guess we know what we are doing :-)
                    // and we would cause a deadlock if we are waiting for our selves to complete :-)
                    if (index->Id() == Thread::ThreadId()) {
                        result = ERROR_NONE;
                    }
                    else {
                        uint32_t outcome = index->Me().Completed(job, waitTime);
                        if ( (outcome == ERROR_NONE) || (outcome == ERROR_TIMEDOUT) ) {
                            result = outcome;
                        }
                    }
                    index++;
                }
            }

            return (result);
        }
        void Run()
        {
            _queue.Enable();
            std::list<Executor>::iterator index = _units.begin();
            while (index != _units.end()) {
                index->Run();
                index++;
            }
        }
        void Stop()
        {
            _queue.Disable();
            std::list<Executor>::iterator index = _units.begin();
            while (index != _units.end()) {
                index->Stop();
                index++;
            }
        }

        #ifdef __CORE_WARNING_REPORTING__
        void SetDispatchedJobMonitor(IDispatchedJobMonitor* dispatchedJobMonitor)
        {
            _dispatchedJobMonitor = dispatchedJobMonitor;
        }
        void ResetDispatchedJobMonitor()
        {
            _dispatchedJobMonitor = nullptr;
        }
        void SaveDispatchedJobContext(const DispatchedJobMetaData& data)
        {
            if (_dispatchedJobMonitor)
                _dispatchedJobMonitor->InsertDispatchedJobMetaData(data);
        }
        void RemoveDispatchedJobContext(const DispatchedJobMetaData& data)
        {
            if (_dispatchedJobMonitor)
                _dispatchedJobMonitor->RemoveDispatchedJobMetaData(data);
        }
        #endif

    private:
        void Idle() {
            if (_callback != nullptr) {
                _queue.Lock();
                if ((_queue.IsEmpty() == false) || ((_external != nullptr) && (_external->IsActive() == true))) {
                    _queue.Unlock();
                }
                else {
                    std::list<Executor>::const_iterator index(_units.begin());
                    while ((index != _units.end()) && (index->IsActive() == false)) { index++; }
                    bool idle = (index == _units.end());
                    _queue.Unlock();

                    if (idle == true) {
                        // There is nothing to process, we are idle, going into Idle mode...
                        _callback->Idle();
                    }
                }
            }
        }
        void Closure(IJob& job) {
            Time scheduleTime;
            _queue.Lock();
            ProxyType<IDispatch> resubmit = job.Resubmit(scheduleTime);
            if (resubmit.IsValid() == true) {
                if ((scheduleTime.IsValid() == false) || (_scheduler == nullptr) || (scheduleTime < Time::Now()) ) {
                    _queue.Post(resubmit);
                }
                else {
                    // See if we have a hook that can process scheduled entries :-)
                    _scheduler->Schedule(scheduleTime, resubmit);
                }
            }
            _queue.Unlock();
        }

    private:
        MessageQueue _queue;
        std::list<Executor> _units;
        IScheduler* _scheduler;
        #ifdef __CORE_WARNING_REPORTING__
        IDispatchedJobMonitor* _dispatchedJobMonitor;
        #endif
        Minion* _external;
        ICallback* _callback;
    };

}
} // namespace Core
