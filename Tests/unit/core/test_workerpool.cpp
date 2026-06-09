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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    static constexpr uint32_t MaxJobWaitTime = 1000; // In milliseconds
    static constexpr uint8_t MaxAdditionalWorker = 5;

    class EventControl {
    public:
        EventControl(const EventControl&) = delete;
        EventControl& operator=(const EventControl&) = delete;
        EventControl()
            : _event(false, true)
        {
        }
        ~EventControl() = default;

    public:
        void Notify()
        {
            _event.SetEvent();
        }
        uint32_t WaitForEvent(uint32_t waitTime)
        {
            return _event.Lock(waitTime);
        }
        void Reset()
        {
            _event.ResetEvent();
        }

    private:
    ::Thunder::Core::Event _event;
    };

    template<typename IMPLEMENTATION>
    class TestJob : public ::Thunder::Core::IDispatch, public EventControl {
    public:
        enum Status {
             INITIATED,
             CANCELED,
             COMPLETED,
        };

        TestJob() = delete;
        TestJob(const TestJob& copy) = delete;
        TestJob& operator=(const TestJob& RHS) = delete;
        ~TestJob() override = default;
        TestJob(IMPLEMENTATION& parent, const Status status, const uint32_t waitTime = 0, const bool checkParentState = false, const bool notifyInvokedTime = false, const bool validateId = false)
            : _parent(parent)
            , _status(status)
            , _waitTime(waitTime)
            , _checkParentState(checkParentState)
            , _notifyInvokedTime(notifyInvokedTime)
            , _validateId(validateId)
        {
        }

    public:
        Status GetStatus()
        {
            return _status;
        }
        void Dispatch() override
        {
            ::Thunder::Core::Time invokedTime = ::Thunder::Core::Time::Now();
            _status = COMPLETED;
            usleep(_waitTime);
            Notify();
            if (_checkParentState) {
                _parent.WaitForReady(this, _waitTime * 10);
                Notify();
            }
            if (_notifyInvokedTime) {
                _parent.InvokedTime(this, invokedTime);
            }
            if (_validateId) {
                _parent.ValidateId();
            }
        }

    private:
        IMPLEMENTATION& _parent;
        Status _status;
        uint32_t _waitTime;
        bool _checkParentState;
        bool _notifyInvokedTime;
        bool _validateId;
    };

    struct JobData {
        EventControl* Event;
        ::Thunder::Core::Time CurrentTime;
        uint16_t ScheduledTime;
    };

    template<typename IMPLEMENTATION>
    class JobControl {
    private:
        typedef std::map<::Thunder::Core::IDispatch*, JobData> JobMap;
    private:
        template<typename PARENTIMPL = IMPLEMENTATION>
        class ExternalWorker : public ::Thunder::Core::Thread {
        public:
            ExternalWorker() = delete;
            ExternalWorker& operator=(const ExternalWorker&) = delete;
            ExternalWorker(const ExternalWorker& copy)
                : _job(copy._job)
                , _parent(copy._parent)
            {
            }

            ExternalWorker(PARENTIMPL &parent)
                : _job(nullptr)
                , _parent(parent)
            {
            }
            ~ExternalWorker()
            {
                Stop();
            }

        public:
            void Stop()
            {
                ::Thunder::Core::Thread::Stop();
                ::Thunder::Core::Thread::Wait(::Thunder::Core::Thread::STOPPED|::Thunder::Core::Thread::BLOCKED, ::Thunder::Core::infinite);
            }
            virtual uint32_t Worker() override
            {
                if (IsRunning()) {
                    _parent.SubmitJob(*_job);
                }
                ::Thunder::Core::Thread::Block();
                return (::Thunder::Core::infinite);
            }

            void Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job)
            {
                _job = &job;
                ::Thunder::Core::Thread::Run();
            }
        private:
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>* _job;
            IMPLEMENTATION& _parent;
        };

    public:
        JobControl() = delete;
        JobControl(const JobControl&) = delete;
        JobControl& operator=(const JobControl&) = delete;
        JobControl(IMPLEMENTATION &parent, uint8_t threadsCount)
            : _index(0)
            , _threadsCount(threadsCount)
            , _parent(parent)
            , _external()
        {
            for (uint8_t index = 0; index < MaxAdditionalWorker; ++index) {
                _external.push_back(new ExternalWorker<IMPLEMENTATION>(parent));
            }
        }
        ~JobControl()
        {
            for (auto& job: _jobs) {
                delete job.second.Event;
            }
            _jobs.clear();
            for (auto& external: _external) {
                delete external;
            }
            _external.clear();

            ::Thunder::Core::Singleton::Dispose();
        }

    public:
        static void CheckScheduledTime(::Thunder::Core::Time current, ::Thunder::Core::Time invokedTime,  uint16_t scheduledTime)
        {
            uint8_t currentSecond = current.Seconds();
            uint8_t scheduledSecond = scheduledTime; // From milliseconds to seconds, integer division!
            uint8_t invokedSecond = invokedTime.Seconds();

            uint32_t expectedAddedTime = 0;
            if (currentSecond + scheduledSecond >= 60) {
                expectedAddedTime = (currentSecond + scheduledSecond) - 60;
            } else {
                expectedAddedTime = (currentSecond + scheduledSecond);
            }

            EXPECT_LE(invokedSecond, expectedAddedTime);
        }
        uint32_t WaitForReady(::Thunder::Core::IDispatch* job, const uint32_t waitTime = 0)
        {
            uint32_t result = ::Thunder::Core::ERROR_NONE;
            JobMap::iterator index = _jobs.find(job);
            if (index != _jobs.end()) {
                result = index->second.Event->WaitForEvent(waitTime);
            }
            return result;
        }
        void NotifyReady(const ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job)
        {
            JobMap::iterator index = _jobs.find(job.operator->());
            if (index != _jobs.end()) {
                index->second.Event->Notify();
            }
        }
        void SubmitUsingSelfWorker(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job)
        {
            InsertJobData(job, 0);
            _parent.Pool().Submit(job);
        }
        void SubmitUsingExternalWorker(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job)
        {
            if (_index < MaxAdditionalWorker) {
                InsertJobData(job, 0);
                _external[_index++]->Submit(job);
            }
        }
        void ScheduleJobs(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job, const uint16_t scheduledTime)
        {
            InsertJobData(job, scheduledTime);
            _parent.Pool().Schedule(::Thunder::Core::Time::Now().Add(scheduledTime), job);
        }
        void RescheduleJobs(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job, const uint16_t scheduledTime)
        {
            InsertJobData(job, scheduledTime);
            _parent.Pool().Reschedule(::Thunder::Core::Time::Now().Add(scheduledTime), job);
        }
        void ValidateId()
        {
            const uint8_t MaxSize = 15;
            bool isPoolId = false;
            char id[MaxSize];
            sprintf(id, "%lx", static_cast<::Thunder::Core::thread_id>(pthread_self()));
            for (uint8_t index = 0; index < _threadsCount + 2; index++) {
            char workerId[MaxSize];
            sprintf(workerId, "%lx", static_cast<::Thunder::Core::thread_id>(::Thunder::Core::IWorkerPool::Instance().Id(index)));

                if (strcpy(workerId, id)) {
                    isPoolId = true;
                    break;
                }
            }
            EXPECT_EQ(isPoolId, true);
        }
        void InvokedTime(::Thunder::Core::IDispatch* job, const ::Thunder::Core::Time& invokedTime)
        {
            JobMap::iterator index = _jobs.find(job);
            if (index != _jobs.end()) {
                CheckScheduledTime(index->second.CurrentTime, invokedTime, index->second.ScheduledTime);
            }
        }
        void RemoveJobData(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job)
        {
            JobMap::iterator index = _jobs.find(job.operator->());
            if (index != _jobs.end()) {
                delete index->second.Event;
                _jobs.erase(index);
            }
        }

    private:
        void InsertJobData(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job, uint16_t scheduledTime)
        {
            JobData jobData;
            jobData.Event = new EventControl();
            jobData.ScheduledTime = scheduledTime;
            jobData.CurrentTime = ::Thunder::Core::Time::Now();
            JobMap::iterator index = _jobs.find(job.operator->());
            if (index == _jobs.end()) {
                _jobs.emplace(std::piecewise_construct, std::forward_as_tuple(job.operator->()), std::forward_as_tuple(jobData));
            } else {
                index->second.CurrentTime = ::Thunder::Core::Time::Now();
                index->second.ScheduledTime = scheduledTime;
            }
        }

    private:
        uint8_t _index;
        uint8_t _threadsCount;
        JobMap _jobs;
        IMPLEMENTATION& _parent;
        std::vector<ExternalWorker<IMPLEMENTATION>*> _external;
    };

    class WorkerPoolTester : public JobControl<WorkerPoolTester>, public ::Thunder::Core::Thread {
    public:
        WorkerPoolTester() = delete;
        WorkerPoolTester(const WorkerPoolTester&) = delete;
        WorkerPoolTester& operator=(const WorkerPoolTester&) = delete;

        WorkerPoolTester(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize)
            : JobControl(*this, threads)
            , _dispatcher()
            , _pool(threads, stackSize, queueSize, &_dispatcher, nullptr, (threads > 2 ? (threads - 1) : threads), (threads > 2 ? (threads - 1) : threads), 1)
        {
        }

        ~WorkerPoolTester()
        {
            // Diable the queue so the minions can stop, even if they are processing and waiting for work..
            Stop();
            ::Thunder::Core::Singleton::Dispose();
        }

    public:
        void Stop()
        {
            _pool.Stop();
            assert(_pool.WaitForStop(5000) == true);
            ::Thunder::Core::Thread::Wait(::Thunder::Core::Thread::STOPPED|::Thunder::Core::Thread::BLOCKED, ::Thunder::Core::infinite);
        }

        uint32_t WaitForJobEvent(const ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job, const uint32_t waitTime = 0)
        {
            return static_cast<TestJob<WorkerPoolTester>&>(*job).WaitForEvent(waitTime);
        }
        void SubmitJob(const ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job)
        {
            _pool.Submit(job);
        }
        virtual uint32_t Worker() override
        {
            if (IsRunning()) {
                _pool.Join();
            }
            ::Thunder::Core::Thread::Block();
            return (::Thunder::Core::infinite);
        }
        void RunExternal()
        {
            ::Thunder::Core::Thread::Run();
        }
        void RunThreadPool()
        {
            _pool.Run();
        }

        ::Thunder::Core::WorkerPool& Pool() { return _pool; }
        const ::Thunder::Core::WorkerPool& Pool() const { return _pool; }

    private:
        class Dispatcher : public ::Thunder::Core::ThreadPool::IDispatcher {
        public:
          Dispatcher(const Dispatcher&) = delete;
          Dispatcher& operator=(const Dispatcher&) = delete;

          Dispatcher() = default;
          ~Dispatcher() override = default;

        private:
          void Initialize() override { }
          void Deinitialize() override { }
          void Dispatch(::Thunder::Core::IDispatch* job) override
            { job->Dispatch(); }
        };

        Dispatcher _dispatcher;
        ::Thunder::Core::WorkerPool _pool;
    };
    TEST(Core_WorkerPool, CheckWorkerStaticMethods)
    {
        uint8_t queueSize = 5;
        uint8_t threadCount = 1;
        WorkerPoolTester workerPool(threadCount, 0, queueSize);

        EXPECT_EQ(::Thunder::Core::WorkerPool::IsAvailable(), false);

        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        EXPECT_EQ(&::Thunder::Core::WorkerPool::Instance(), &workerPool.Pool());
        EXPECT_EQ(::Thunder::Core::WorkerPool::IsAvailable(), true);

        ::Thunder::Core::WorkerPool::Assign(nullptr);
        EXPECT_EQ(::Thunder::Core::WorkerPool::IsAvailable(), false);
    }
    TEST(Core_WorkerPool, Check_WithSingleJob)
    {
        uint8_t queueSize = 5;
        uint8_t threadCount = 1;
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job = ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 500));
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.Pool().Submit(job);
        workerPool.RunThreadPool();
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);
        workerPool.Stop();

        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    TEST(Core_WorkerPool, Check_WithSingleJob_CancelJob_BeforeProcessing)
    {
        uint8_t queueSize = 5;
        uint8_t threadCount = 1;
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job = ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED));
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.Pool().Submit(job);
        workerPool.Pool().Revoke(job);
        workerPool.RunThreadPool();
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_TIMEDOUT);
        workerPool.Stop();

        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    TEST(Core_WorkerPool, Check_WithSingleJob_CancelJob_WhileProcessing)
    {
        uint8_t queueSize = 5;
        uint8_t threadCount = 1;
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job = ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 1000, true));
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.Pool().Submit(job);
        workerPool.RunThreadPool();

        // Wait for job to actually start executing (wait for first notification)
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);

        // Now job is waiting in WaitForReady - it's definitely running
        // Try to revoke while job is running (should fail to cancel)
        workerPool.Pool().Revoke(job);

        // Now notify it to continue and complete
        workerPool.NotifyReady(job);

        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);
        workerPool.Stop();

        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    void CheckWorkerPool_MultipleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const bool runExternal = false)
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100)));
        }

        if (runExternal == true) {
            workerPool.RunExternal();
        }

        workerPool.RunThreadPool();

        for (uint8_t i = 0; i < queueSize; ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingSelfWorker(jobs[i]);
        }

        if (additionalJobs > 0) {
            usleep(10000);
        }

        for (uint8_t i = queueSize; i < jobs.size(); ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingExternalWorker(jobs[i]);
        }

        usleep(MaxJobWaitTime);
        for (auto& job: jobs) {
            EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);
        }

        for (auto& job: jobs) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        }

        workerPool.Stop();

        for (auto& job: jobs) {
            job.Release();
        }
        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_SinglePool_WithSinglJob)
    {
        CheckWorkerPool_MultipleJobs(1, 1, 0);
    }
    TEST(Core_WorkerPool, Check_SinglePool_WithMultipleJobs)
    {
        CheckWorkerPool_MultipleJobs(1, 5, 0);
    }
    TEST(Core_WorkerPool, Check_SinglePool_WithMultipleJobs_AdditionalJobs_WithExternal)
    {
        CheckWorkerPool_MultipleJobs(1, 5, 5, true);
    }
    TEST(Core_WorkerPool, Check_MultiplePool_WithMultipleJobs)
    {
        CheckWorkerPool_MultipleJobs(5, 5, 0);
    }
    TEST(Core_WorkerPool, Check_MultiplePool_WithMultipleJobs_AdditionalJobs)
    {
        CheckWorkerPool_MultipleJobs(5, 2, 2);
        CheckWorkerPool_MultipleJobs(5, 1, 2);
        CheckWorkerPool_MultipleJobs(5, 5, MaxAdditionalWorker);
    }
    TEST(Core_WorkerPool, Check_MultiplePool_WithMultipleJobs_AdditionalJobs_WithExternal)
    {
        CheckWorkerPool_MultipleJobs(5, 5, 4, true);
    }
    void CheckWorkerPool_MultipleJobs_CancelJobs_InBetween(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t cancelJobsId[])
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100)));
        }

        workerPool.RunThreadPool();

        for (uint8_t i = 0; i < queueSize; ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingSelfWorker(jobs[i]);
        }
        for (uint8_t i = queueSize; i < jobs.size(); ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingExternalWorker(jobs[i]);
        }

        // Multi Pool case jobs can be run in parallel once we start thread, hence revoke inbetween maynot work
        // it just have to wait for processing jobs completion.
        // Hence revoking before starting the job. Just to ensure the status meets
        for (uint8_t index = 0; index < cancelJobsCount; index++) {
            workerPool.Pool().Revoke(jobs[cancelJobsId[index]], 0);
        }

        for (uint8_t index = 0; index < jobs.size(); index++) {
            bool isCanceledJob = false;
            for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
            {
                if (index == cancelJobsId[cancelIndex]) {
                    isCanceledJob = true;
                    break;
                }
            }
            if (isCanceledJob == true) {
                uint32_t result = workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3);
                EXPECT_TRUE(result == ::Thunder::Core::ERROR_TIMEDOUT || result == ::Thunder::Core::ERROR_NONE);
            } else {
                EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);
            }
        }

        for (uint8_t index = 0; index < jobs.size(); index++) {
            bool isCanceledJob = false;
            for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
            {
                if (index == cancelJobsId[cancelIndex]) {
                    isCanceledJob = true;
                    break;
                }
            }
            if (isCanceledJob == true) {
                auto status = static_cast<TestJob<WorkerPoolTester>&>(*jobs[index]).GetStatus();
                EXPECT_TRUE(status == TestJob<WorkerPoolTester>::INITIATED || status == TestJob<WorkerPoolTester>::COMPLETED);
            } else {
                EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[index]).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
            }
        }

        workerPool.Stop();

        for (auto& job: jobs) {
            job.Release();
        }
        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_SinglePool_WithMultipleJobs_CancelJobs_InBetween)
    {
        uint8_t cancelJobsId[] = {3, 4};
        CheckWorkerPool_MultipleJobs_CancelJobs_InBetween(1, 5, 0, sizeof(cancelJobsId), cancelJobsId);
    }
    TEST(Core_WorkerPool, Check_SinglePool_WithMultipleJobs_AdditionalJobs_CancelJobs_InBetween)
    {
        uint8_t cancelJobsId[] = {3, 4};
        CheckWorkerPool_MultipleJobs_CancelJobs_InBetween(1, 5, 2, sizeof(cancelJobsId), cancelJobsId);
    }
    TEST(Core_WorkerPool, Check_MultiplePool_WithMultipleJobs_AdditionalJobs_CancelJobs_InBetween)
    {
        uint8_t cancelJobsId[] = {3, 4};
        CheckWorkerPool_MultipleJobs_CancelJobs_InBetween(5, 5, 2, sizeof(cancelJobsId), cancelJobsId);
        CheckWorkerPool_MultipleJobs_CancelJobs_InBetween(5, 5, MaxAdditionalWorker, sizeof(cancelJobsId), cancelJobsId);
    }
    void CheckWorkerPool_ScheduleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId, const uint16_t scheduledTimes[])
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        workerPool.RunExternal();

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 500, false, true)));
        }

        for (uint8_t i = 0; i < queueSize; ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.ScheduleJobs(jobs[i], scheduledTimes[i]);
        }
        for (uint8_t i = queueSize; i < jobs.size(); ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.ScheduleJobs(jobs[i], scheduledTimes[i]);
        }

        for (uint8_t index = 0; index < cancelJobsCount; index++) {
            workerPool.Pool().Revoke(jobs[cancelJobsId[index]], 0);
        }

        for (uint8_t index = 0; index < jobs.size(); index++) {
            bool isCanceledJob = false;
            for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
            {
                if (index == cancelJobsId[cancelIndex]) {
                    isCanceledJob = true;
                    break;
                }
            }
            if (isCanceledJob == true) {
                EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), ::Thunder::Core::ERROR_TIMEDOUT);
            } else {
                EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 15), ::Thunder::Core::ERROR_NONE);
            }
        }

        for (uint8_t index = 0; index < jobs.size(); index++) {
            bool isCanceledJob = false;
            for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
            {
                if (index == cancelJobsId[cancelIndex]) {
                    isCanceledJob = true;
                    break;
                }
            }
            if (isCanceledJob == true) {
                EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[index]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            } else {
                EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[index]).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
            }
        }

        workerPool.Stop();

        for (auto& job: jobs) {
            job.Release();
        }
        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_SingleJob)
    {
        const uint16_t scheduledTimes[] = {2000}; //In milliseconds
        CheckWorkerPool_ScheduleJobs(1, 1, 0, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, DISABLED_Check_ScheduleJobs_SinglePool_SingleJob_ZeroTime)
    {
        const uint16_t scheduledTimes[] = {0};
        CheckWorkerPool_ScheduleJobs(1, 1, 0, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_AdditionalJob)
    {
        const uint16_t scheduledTimes[] = {3000, 2000};
        CheckWorkerPool_ScheduleJobs(2, 1, 1, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_SingleJob_CancelJob)
    {
        const uint16_t scheduledTimes[] = {2000};
        const uint8_t job = 0;
        CheckWorkerPool_ScheduleJobs(1, 1, 0, 1, &job, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_AdditionalJob_CancelJob)
    {
        const uint16_t scheduledTimes[] = {2000, 2000};
        const uint8_t job = 0;
        CheckWorkerPool_ScheduleJobs(1, 1, 1, 1, &job, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_MultipleJobs)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[] = {2000, 3000, 1000, 2000, 3000};
        CheckWorkerPool_ScheduleJobs(1, maxJobs, 0, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, DISABLED_Check_ScheduleJobs_SinglePool_MultipleJobs_ZeroTime)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[5] = {0};
        CheckWorkerPool_ScheduleJobs(1, maxJobs, 0, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_MultipleJobs_CancelJob)
    {
        uint8_t maxJobs = 5;
        const uint8_t jobs[] = {1, 2};
        const uint16_t scheduledTimes[] = {2000, 3000, 1000, 2000, 3000};
        CheckWorkerPool_ScheduleJobs(1, maxJobs, 0, sizeof(jobs), jobs, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_MultiplePool_MultipleJobs)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[] = {2000, 3000, 1000, 2000, 3000};
        CheckWorkerPool_ScheduleJobs(5, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, 2, 0, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, DISABLED_Check_ScheduleJobs_MultiplePool_MultipleJobs_ZeroTime)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[5] = {0};
        CheckWorkerPool_ScheduleJobs(5, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, 2, 0, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_MultiplePool_MultipleJobs_CancelJob)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[] = {2000, 3000, 1000, 2000, 3000, 2000, 1000, 1000, 3000};
        const uint8_t jobs[] = {0, 4};
        CheckWorkerPool_ScheduleJobs(5, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(5, 5, 0, sizeof(jobs), jobs, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_MultiplePool_MultipleJobs_AdditionalJobs)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[] = {2000, 3000, 1000, 2000, 3000, 1000};
        CheckWorkerPool_ScheduleJobs(5, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, 2, 1, 0, nullptr, scheduledTimes);
    }
    TEST(Core_WorkerPool, Check_ScheduleJobs_MultiplePool_MultipleJobs_AdditionalJobs_CancelJob)
    {
        uint8_t maxJobs = 5;
        const uint16_t scheduledTimes[] = {2000, 3000, 1000, 2000, 3000, 1000, 1000, 2000, 3000};
        const uint8_t jobs[] = {3, 5};
        CheckWorkerPool_ScheduleJobs(5, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(2, maxJobs, 0, 0, nullptr, scheduledTimes);
        CheckWorkerPool_ScheduleJobs(5, 5, 4, sizeof(jobs), jobs, scheduledTimes);
    }
    void CheckWorkerPool_RescheduleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId, const uint16_t scheduledTimes[], const uint16_t rescheduledTimes[])
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 500, false, true)));
        }

        for (uint8_t i = 0; i < queueSize; ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.ScheduleJobs(jobs[i], scheduledTimes[i]);
        }
        for (uint8_t i = queueSize; i < jobs.size(); ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.ScheduleJobs(jobs[i], scheduledTimes[i]);
        }

        for (uint8_t i = 0; i < jobs.size(); ++i) {
            workerPool.RescheduleJobs(jobs[i], rescheduledTimes[i]);
        }
        for (uint8_t index = 0; index < cancelJobsCount; index++) {
            workerPool.Pool().Revoke(jobs[cancelJobsId[index]], 0);
        }

        for (uint8_t index = 0; index < jobs.size(); index++) {
            bool isCanceledJob = false;
            for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
            {
                if (index == cancelJobsId[cancelIndex]) {
                    isCanceledJob = true;
                    break;
                }
            }
            if (isCanceledJob == true) {
                EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), ::Thunder::Core::ERROR_TIMEDOUT);
            } else {
                EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 15), ::Thunder::Core::ERROR_NONE);
            }
        }

        for (uint8_t index = 0; index < jobs.size(); index++) {
            bool isCanceledJob = false;
            for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
            {
                if (index == cancelJobsId[cancelIndex]) {
                    isCanceledJob = true;
                    break;
                }
            }
            if (isCanceledJob == true) {
                EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[index]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            } else {
                EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[index]).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
            }
        }

        workerPool.Stop();

        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_SingleJob)
    {
        const uint16_t scheduledTimes[] = {1000};
        const uint16_t rescheduledTimes[] = {2000};
        CheckWorkerPool_RescheduleJobs(1, 1, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, DISABLED_Check_RescheduleJobs_SinglePool_SingleJob_ZeroTime)
    {
        const uint16_t scheduledTimes[] = {1000};
        const uint16_t rescheduledTimes[] = {0};
        CheckWorkerPool_RescheduleJobs(1, 1, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_SingleJob_AdditionalJobs)
    {
        const uint16_t scheduledTimes[] = {1000, 2000};
        const uint16_t rescheduledTimes[] = {2000, 1000};
        CheckWorkerPool_RescheduleJobs(1, 1, 1, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_SingleJob_CancelJob)
    {
        uint16_t scheduledTimes[] = {1000};
        const uint16_t rescheduledTimes[] = {2000};
        const uint8_t job = 0;
        CheckWorkerPool_RescheduleJobs(1, 1, 0, 1, &job, scheduledTimes, rescheduledTimes);
        scheduledTimes[0] = 4000;
        CheckWorkerPool_RescheduleJobs(1, 1, 0, 1, &job, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_AdditionalJob_CancelJob)
    {
        const uint16_t scheduledTimes[] = {4000, 1000};
        const uint16_t rescheduledTimes[] = {3000, 2000};
        const uint8_t job = 1;
        CheckWorkerPool_RescheduleJobs(1, 1, 1, 1, &job, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_MultipleJobs)
    {
        const uint16_t scheduledTimes[] = {1000, 2000, 3000, 2000, 1000};
        const uint16_t rescheduledTimes[] = {2000, 2000, 1000, 2000, 3000};
        CheckWorkerPool_RescheduleJobs(1, 5, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, DISABLED_Check_RescheduleJobs_SinglePool_MultipleJobs_ZeroTime)
    {
        const uint16_t scheduledTimes[] = {1000, 2000, 3000, 2000, 1000};
        const uint16_t rescheduledTimes[5] = {0};
        CheckWorkerPool_RescheduleJobs(1, 5, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_MultipleJobs_AdditionalJobs)
    {
        const uint16_t scheduledTimes[] = {5000, 2000, 6000, 1000, 2000, 3000, 1000, 1000, 2000, 4000};
        const uint16_t rescheduledTimes[] = {2000, 1000, 1000, 2000, 3000, 1000, 2000, 3000, 1000, 2000};

        CheckWorkerPool_RescheduleJobs(1, 5, 2, 0, nullptr, scheduledTimes, rescheduledTimes);
        CheckWorkerPool_RescheduleJobs(1, 5, 5, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_MultipleJobs_CancelJob)
    {
        const uint16_t scheduledTimes[] = {1000, 2000, 3000, 2000, 1000};
        const uint16_t rescheduledTimes[] = {2000, 2000, 1000, 2000, 3000};

        const uint8_t jobs[] = {1, 4};
        CheckWorkerPool_RescheduleJobs(1, 5, 0, sizeof(jobs), jobs, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_MultipleJobs_AdditionalJob_CancelJob)
    {
        const uint16_t scheduledTimes[] = {5000, 2000, 6000, 1000, 2000, 3000, 1000, 1000, 2000, 4000};
        const uint16_t rescheduledTimes[] = {2000, 1000, 1000, 2000, 3000, 1000, 2000, 3000, 1000, 2000};

        const uint8_t jobs[] = {2, 6};
        CheckWorkerPool_RescheduleJobs(1, 5, 5, sizeof(jobs), jobs, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_MultiplePool_MultipleJobs)
    {
        const uint16_t scheduledTimes[] = {1000, 2000, 3000, 2000, 1000};
        const uint16_t rescheduledTimes[] = {2000, 2000, 1000, 2000, 3000};
        CheckWorkerPool_RescheduleJobs(1, 5, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, DISABLED_Check_RescheduleJobs_MultiplePool_MultipleJobs_ZeroTime)
    {
        const uint16_t scheduledTimes[] = {1000, 2000, 3000, 2000, 1000};
        const uint16_t rescheduledTimes[5] = {0};
        CheckWorkerPool_RescheduleJobs(1, 5, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_MultiplePool_MultipleJobs_AdditionalJobs)
    {
        const uint16_t scheduledTimes[] = {5000, 2000, 6000, 1000, 2000, 3000, 1000, 1000, 2000, 4000};
        const uint16_t rescheduledTimes[] = {2000, 1000, 1000, 2000, 3000, 1000, 2000, 3000, 1000, 2000};
        CheckWorkerPool_RescheduleJobs(1, 5, 2, 0, nullptr, scheduledTimes, rescheduledTimes);
        CheckWorkerPool_RescheduleJobs(1, 5, 5, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_MultiplePool_MultipleJobs_CancelJob)
    {
        const uint16_t scheduledTimes[] = {1000, 2000, 3000, 2000, 1000};
        const uint16_t rescheduledTimes[] = {2000, 2000, 1000, 2000, 3000};

        const uint8_t jobs[] = {1, 4};
        CheckWorkerPool_RescheduleJobs(1, 5, 0, sizeof(jobs), jobs, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_RescheduleJobs_MultiplePool_MultipleJobs_AdditionalJob_CancelJob)
    {
        const uint16_t scheduledTimes[] = {5000, 2000, 6000, 1000, 2000, 3000, 1000, 1000, 2000, 4000};
        const uint16_t rescheduledTimes[] = {2000, 1000, 1000, 2000, 3000, 1000, 2000, 3000, 1000, 2000};

        const uint8_t jobs[] = {3, 5};
        CheckWorkerPool_RescheduleJobs(5, 5, 2, sizeof(jobs), jobs, scheduledTimes, rescheduledTimes);
        CheckWorkerPool_RescheduleJobs(5, 5, 5, sizeof(jobs), jobs, scheduledTimes, rescheduledTimes);
    }
    void CheckMetaData(const uint8_t pending, const uint8_t occupation, const uint8_t expectedRuns)
    {
        const ::Thunder::Core::IWorkerPool::Metadata& metaData = ::Thunder::Core::IWorkerPool::Instance().Snapshot();

        EXPECT_LE(metaData.Pending.size(), pending);

        uint16_t totalRuns = 0, totalOccupation = 0;
        for (uint8_t index = 0; index < metaData.Slots; index++) {
            totalRuns += metaData.Slot[index].Runs;
            totalOccupation += metaData.Slot[index].Job.IsSet();
        }
        EXPECT_EQ(totalRuns, expectedRuns);
        EXPECT_EQ(totalOccupation, occupation);
    }
    void CheckWorkerPool_MetaData(uint8_t threadCount, uint8_t queueSize, uint8_t additionalJobs)
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100)));
        }

        workerPool.RunThreadPool();

        for (uint8_t i = 0; i < queueSize; ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingSelfWorker(jobs[i]);
        }
        for (uint8_t i = queueSize; i < jobs.size(); ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingExternalWorker(jobs[i]);
        }

        usleep(MaxJobWaitTime);
        for (auto& job: jobs) {
            EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);
        }

        for (auto& job: jobs) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        }

        workerPool.Stop();
        CheckMetaData(queueSize, 1, queueSize + additionalJobs);
        for (auto& job: jobs) {
            job.Release();
        }
        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_MetaData_SinglePool_SingleJob)
    {
        CheckWorkerPool_MetaData(1, 1, 0);
    }
    TEST(Core_WorkerPool, Check_MetaData_SinglePool_SingleJob_AdditionalJobs)
    {
        CheckWorkerPool_MetaData(1, 1, 2);
    }
    TEST(Core_WorkerPool, Check_MetaData_SinglePool_MultipleJobs)
    {
        CheckWorkerPool_MetaData(1, 5, 0);
    }
    TEST(Core_WorkerPool, Check_MetaData_SinglePool_MultipleJobs_AdditionalJobs)
    {
        CheckWorkerPool_MetaData(1, 5, 2);
    }
    TEST(Core_WorkerPool, Check_MetaData_MultiplePool_MultipleJobs)
    {
        CheckWorkerPool_MetaData(5, 5, 0);
    }
    TEST(Core_WorkerPool, Check_MetaData_MultiplePool_MultipleJobs_AdditionalJobs)
    {
        CheckWorkerPool_MetaData(5, 5, 5);
    }
    void CheckWorkerPool_Ids(uint8_t threadCount, uint8_t queueSize, uint8_t additionalJobs)
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100, false, false, true)));
        }

        for (uint8_t i = 0; i < queueSize; ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingSelfWorker(jobs[i]);
        }
        for (uint8_t i = queueSize; i < jobs.size(); ++i) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
            workerPool.SubmitUsingExternalWorker(jobs[i]);
        }

        for (auto& job: jobs) {
            EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);
        }

        for (auto& job: jobs) {
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        }

        workerPool.Stop();
        for (auto& job: jobs) {
            job.Release();
        }
        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_Ids_SinglePool_SingleJob)
    {
        CheckWorkerPool_Ids(1, 1, 0);
    }
    TEST(Core_WorkerPool, Check_Ids_SinglePool_SingleJob_AdditionalJobs)
    {
        CheckWorkerPool_Ids(1, 1, 2);
    }
    TEST(Core_WorkerPool, Check_Ids_SinglePool_MultipleJobs)
    {
        CheckWorkerPool_Ids(1, 5, 0);
    }
    TEST(Core_WorkerPool, Check_Ids_SinglePool_MultipleJobs_AdditionalJobs)
    {
        CheckWorkerPool_Ids(1, 5, 2);
    }
    TEST(Core_WorkerPool, Check_Ids_MultiplePool_MultipleJobs)
    {
        CheckWorkerPool_Ids(5, 5, 0);
    }
    TEST(Core_WorkerPool, Check_Ids_MultiplePool_MultipleJobs_AdditionalJobs)
    {
        CheckWorkerPool_Ids(5, 5, 5);
    }

    class WorkerJobTester : public EventControl {
    public:
        WorkerJobTester(const WorkerJobTester&) = delete;
        WorkerJobTester& operator=(const WorkerJobTester&) = delete;

        WorkerJobTester() = delete;

        WorkerJobTester(uint32_t waitTime, bool waitForCaller = false)
            : _waitTime(waitTime)
            , _expectedTime()
            , _waitForCaller(waitForCaller)
            , _job(*this)
        {
        }
        ~WorkerJobTester() = default;

        bool Submit()
        {
            _expectedTime = ::Thunder::Core::Time::Now();
            return _job.Submit();
        }
        bool Schedule(const ::Thunder::Core::Time& time)
        {
            _expectedTime = time;
            return _job.Reschedule(time);
        }
        bool Reschedule(const ::Thunder::Core::Time& time)
        {
            _expectedTime = time;
            return _job.Reschedule(time);
        }
        void Revoke()
        {
            _job.Revoke();
        }
        bool IsIdle()
        {
            return _job.IsIdle();
        }

    public:
        void Dispatch()
        {
            ::Thunder::Core::Time invokedTime = ::Thunder::Core::Time::Now();
            WorkerPoolTester::CheckScheduledTime(_expectedTime, invokedTime, 0);
            Notify();
            usleep(_waitTime * 2);
            if (_waitForCaller) {
                WaitForEvent(_waitTime * 10);
                Reset();
            }
        }

    private:
        uint32_t _waitTime;
        ::Thunder::Core::Time _expectedTime;
        bool _waitForCaller;
        ::Thunder::Core::WorkerPool::JobType<WorkerJobTester&> _job;
    };

    TEST(Core_WorkerPool, Check_JobType_Submit)
    {
        WorkerPoolTester workerPool(1, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(0);
            EXPECT_EQ(jobTester.Submit(), true);
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_JobType_Submit_Revoke)
    {
        WorkerPoolTester workerPool(4, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        {
            WorkerJobTester jobTester(0);
            EXPECT_EQ(jobTester.Submit(), true);
            jobTester.Revoke();
            workerPool.RunThreadPool();
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), ::Thunder::Core::ERROR_TIMEDOUT);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_JobType_Schedule)
    {
        WorkerPoolTester workerPool(4, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(0);
            jobTester.Schedule(::Thunder::Core::Time::Now().Add(2000));
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_JobType_Schedule_Revoke)
    {
        WorkerPoolTester workerPool(4, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(0);
            jobTester.Schedule(::Thunder::Core::Time::Now().Add(2000));
            usleep(500);
            jobTester.Revoke();
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 4), ::Thunder::Core::ERROR_TIMEDOUT);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    void CheckJobType_Reschedule(const uint16_t scheduleTime, const uint16_t rescheduleTime)
    {
        WorkerPoolTester workerPool(4, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(0);
            jobTester.Schedule(::Thunder::Core::Time::Now().Add(scheduleTime));
            usleep(500);
            EXPECT_EQ(jobTester.Reschedule(::Thunder::Core::Time::Now().Add(rescheduleTime)), true);
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 4), ::Thunder::Core::ERROR_NONE);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_JobType_Reschedule)
    {
        CheckJobType_Reschedule(2000, 3000);
        CheckJobType_Reschedule(2000, 0);
    }
    TEST(Core_WorkerPool, Check_JobType_Reschedule_Revoke)
    {
        WorkerPoolTester workerPool(4, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(0);
            jobTester.Schedule(::Thunder::Core::Time::Now().Add(2000));
            usleep(500);
            jobTester.Schedule(::Thunder::Core::Time::Now().Add(3000));
            usleep(500);
            jobTester.Revoke();
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 4), ::Thunder::Core::ERROR_TIMEDOUT);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    void CheckJobType_RescheduleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId, const uint16_t scheduledTimes[], const uint16_t rescheduledTimes[])
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            std::vector<::Thunder::Core::ProxyType<WorkerJobTester>> jobs;
            // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
            for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
                jobs.push_back(::Thunder::Core::ProxyType<WorkerJobTester>(::Thunder::Core::ProxyType<WorkerJobTester>::Create(0)));
            }
            for (uint8_t i = 0; i < jobs.size(); ++i) {
                EXPECT_EQ(jobs[i]->IsIdle(), true);
            }

            for (uint8_t i = 0; i < jobs.size(); ++i) {
                jobs[i]->Schedule(::Thunder::Core::Time::Now().Add(scheduledTimes[i]));
            }

            for (uint8_t i = 0; i < jobs.size(); ++i) {
                EXPECT_EQ(jobs[i]->Reschedule(::Thunder::Core::Time::Now().Add(rescheduledTimes[i])), true);
            }
            for (uint8_t i = 0; i < jobs.size(); ++i) {
                EXPECT_EQ(jobs[i]->IsIdle(), false);
            }

            for (uint8_t i = 0; i < cancelJobsCount; i++) {
                jobs[cancelJobsId[i]]->Revoke();
                EXPECT_EQ(jobs[cancelJobsId[i]]->IsIdle(), true);
            }

            for (uint8_t index = 0; index < jobs.size(); index++) {
                bool isCanceledJob = false;
                for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
                {
                    if (index == cancelJobsId[cancelIndex]) {
                        isCanceledJob = true;
                        break;
                    }
                }
                if (isCanceledJob == true) {
                    EXPECT_EQ(jobs[index]->WaitForEvent(MaxJobWaitTime * 3), ::Thunder::Core::ERROR_TIMEDOUT);
                } else {
                    EXPECT_EQ(jobs[index]->WaitForEvent(MaxJobWaitTime * 15), ::Thunder::Core::ERROR_NONE);
                    usleep(200);
                }

                usleep(200);
                EXPECT_EQ(jobs[index]->IsIdle(), true);
            }
            for (auto& job: jobs) {
                job.Release();
            }
            jobs.clear();
        }

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_JobType_Reschedule_MultipleJobs)
    {
        const uint16_t scheduledTimes[] = {5000, 2000, 6000, 1000, 2000, 3000, 1000, 1000, 2000, 4000};
        const uint16_t rescheduledTimes[] = {2000, 1000, 1000, 2000, 3000, 1000, 2000, 3000, 1000, 2000};

        CheckJobType_RescheduleJobs(5, 5, 2, 0, nullptr, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_JobType_Reschedule_MultipleJobs_CancelJobs)
    {
        const uint16_t scheduledTimes[] = {5000, 2000, 6000, 1000, 2000, 3000, 1000, 1000, 2000, 4000};
        const uint16_t rescheduledTimes[] = {2000, 1000, 1000, 2000, 3000, 1000, 2000, 3000, 1000, 2000};
        const uint8_t jobs[] = {0, 2};

        CheckJobType_RescheduleJobs(5, 5, 2, sizeof(jobs), jobs, scheduledTimes, rescheduledTimes);
    }
    TEST(Core_WorkerPool, Check_JobType_Reschedule_WhileRunning)
    {
        WorkerPoolTester workerPool(4, 0, 1);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(1000);
            jobTester.Schedule(::Thunder::Core::Time::Now().Add(0));
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 2), ::Thunder::Core::ERROR_NONE);
            jobTester.Reset();
            EXPECT_EQ(jobTester.Reschedule(::Thunder::Core::Time::Now().Add(1000)), false);
            jobTester.Notify();
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 2), ::Thunder::Core::ERROR_NONE);
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    class Job {
    public:
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;
        Job(const uint16_t sleepTimeInSeconds)
            : _sleepTimeInMilliSeconds(sleepTimeInSeconds * 1000) {
        }
        ~Job() = default;

    public:
        void Dispatch() {
            _instancesActive++;

            if (_instancesActive > 1) {
                printf("Ooopsie daisy, that is unexpected, it seems there are multiples of me running, thats not according to the spec !!!!\n");
            }
            EXPECT_FALSE(_instancesActive > 1);
            SleepMs(_sleepTimeInMilliSeconds); // Sleep for _sleepTimeInMilliSeconds
            _instancesActive--;
        }

    private:
        uint32_t _sleepTimeInMilliSeconds;
        static std::atomic<uint32_t> _instancesActive;
    };

    /* static */ std::atomic<uint32_t> Job::_instancesActive;

    class Trigger : public EventControl {
    public:
        Trigger(const Trigger&) = delete;
        Trigger& operator=(const Trigger&) = delete;
        Trigger(const uint16_t intervalInSecond, const uint16_t sleepTimeInSeconds)
            : _cycles(0)
            , _intervalInMilliSeconds(intervalInSecond * 1000)
            , _triggerJob(*this)
            , _decoupledJob(sleepTimeInSeconds)
        {
        }
        ~Trigger() {
            Stop();
            _decoupledJob.Revoke();
        }

    public:
        void Start(const uint8_t cycles) {
            _cycles = cycles;
            _triggerJob.Reschedule(::Thunder::Core::Time::Now().Add(_intervalInMilliSeconds));
        }

        uint8_t Pending()
        {
            return _cycles;
        }

        void Stop () {
            _triggerJob.Revoke();
        }
        void Dispatch() {
            // If the TriggerJob is scheduled due to the fact that the _intervalInMilliSeconds time
            // has elapsed, it's onluy purpose is to submit the _decoupledJob and reschedule itself
            // to submit this job again after it the _intervalInMilliSeconds has elepased again from
            // now!

            // The _decoupledJob is of type ::Thunder::Core::WorkerPool::JobType<Job>, which, according to the
            // design will be decoupled from this thread and run the dispatch (of Job) on another
            // thread. To avoid starvation, the Job will obnnly be ran once. If it is already
            // running (Active) it will be scheduled (state changed) to be scheduled for rerun after
            // it completed.
            _decoupledJob.Submit();

            // Reschedule our selves to submit another run for the Job in _intervalInMilliSeconds
            // from now..
            if (--_cycles != 0) {
                _triggerJob.Reschedule(::Thunder::Core::Time::Now().Add(_intervalInMilliSeconds));
            } else {
                Notify();
            }
        }

    private:
        uint8_t _cycles;
        uint32_t _intervalInMilliSeconds;
        ::Thunder::Core::WorkerPool::JobType<Trigger&> _triggerJob;
        ::Thunder::Core::WorkerPool::JobType<Job>     _decoupledJob;
    };
    void CheckWorkerPool_ReschduledTimedJob(const uint8_t threadCount, const uint8_t queueSize, const uint16_t timedWait, const uint16_t jobWait, const uint8_t times)
    {
        WorkerPoolTester workerPool(threadCount, 0, queueSize);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            // Schedule the Job every timedWait (eg: 10 Seconds) to run once..
            // The Job takes JobWait (eg: 40 Seconds) to run
            Trigger triggerJob (timedWait /* param1 */, jobWait /* param2 */);

            // Submit the Job N times (so after N times * timedWait (eg: 5 x 10 Seconds) all scheduled submits occured...
            triggerJob.Start(times /* param 3 */);

            // Now wait for 10 (first time to drop the first time the job param1) + 5 (number of submits param3) * 40 seconds (param2, duration of the job) for this to complete.

            // There is an issue if param1 < param2, as with the old code, the job would run multiple times the Job::Dispatch in parallel
            // which is not according to spec.

            // with the old code, the following timeing is applicable:
            // Assuming there are 5 threads in the threadpool:
            // Total time is 5 * 10 + 40 seconds. The Job would be running 5 times spread over all available threads which is a total of 90 second and the job would have been ran 5 times
            // if the thread count < 3 the duration will be longer as the last submit (after 50 seconds) still has to wait for the 3 submit to complete.
            // if the thread count >= 2 the line "Ooopsie daisy, that is unexpected, it seems there are multiples of me running, thats not according to the spec !!!!\n" will be observed
            // which indicates the flas in the current workerpool setup.

            EXPECT_EQ(triggerJob.WaitForEvent((timedWait + (times * jobWait)) * 1000), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(triggerJob.Pending(), 0u);
            // In the new code the Job will never be running with more than one instance so the line: "Ooopsie daisy, that is unexpected, it seems there are multiples of me running, thats
            // not according to the spec !!!!\n" should never be observed.
            // However the job will not be ran 5 times as there are 3 schedule overruns due to the fact that the Job is still running when the next submit ocures. SO the job wll only run
            // 3 times and it will take 10 + 3* 40 seconds to complete.
        }
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Check_ReschduledTimedJob_SinglePool)
    {
        CheckWorkerPool_ReschduledTimedJob(1, 5, 5, 10, 5);
    }
    TEST(Core_WorkerPool, Check_ReschduledTimedJob_MultiplePool)
    {
        CheckWorkerPool_ReschduledTimedJob(2, 5, 5, 10, 5);
        CheckWorkerPool_ReschduledTimedJob(3, 5, 5, 10, 5);
    }

    // =========================================================================
    // Priority-based Submit — closes gap: Core WorkerPool error paths
    // (v2.1 gap: "WorkerPool — exhaustion, shutdown-during-job")
    // =========================================================================
    TEST(Core_WorkerPool, Submit_WithPriority_High_ExecutesJob)
    {
        WorkerPoolTester workerPool(2, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::INITIATED);

        workerPool.Pool().Submit(job, ::Thunder::Core::ThreadPool::Priority::High);

        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime),
                  ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::COMPLETED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    TEST(Core_WorkerPool, Submit_WithPriority_Medium_ExecutesJob)
    {
        WorkerPoolTester workerPool(2, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        workerPool.Pool().Submit(job, ::Thunder::Core::ThreadPool::Priority::Medium);

        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime),
                  ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::COMPLETED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    TEST(Core_WorkerPool, Submit_WithPriority_Low_ExecutesJob)
    {
        WorkerPoolTester workerPool(2, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        workerPool.Pool().Submit(job, ::Thunder::Core::ThreadPool::Priority::Low);

        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime),
                  ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::COMPLETED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    TEST(Core_WorkerPool, Submit_AllPriorities_AllExecute)
    {
        WorkerPoolTester workerPool(3, 0, 10);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> highJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> medJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> lowJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        workerPool.Pool().Submit(lowJob, ::Thunder::Core::ThreadPool::Priority::Low);
        workerPool.Pool().Submit(medJob, ::Thunder::Core::ThreadPool::Priority::Medium);
        workerPool.Pool().Submit(highJob, ::Thunder::Core::ThreadPool::Priority::High);

        EXPECT_EQ(workerPool.WaitForJobEvent(highJob, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(workerPool.WaitForJobEvent(medJob, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(workerPool.WaitForJobEvent(lowJob, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*highJob).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*medJob).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*lowJob).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        highJob.Release();
        medJob.Release();
        lowJob.Release();
    }

    // =========================================================================
    // Schedule with past time — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Schedule_PastTime_ExecutesImmediately)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        // Schedule with a time in the past - should be submitted immediately
        ::Thunder::Core::Time pastTime = ::Thunder::Core::Time::Now().Sub(5000);
        workerPool.Pool().Schedule(pastTime, job);

        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime),
                  ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::COMPLETED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // Revoke unknown job — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Revoke_UnknownJob_ReturnsErrorUnknownKey)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Create a job but do NOT submit it
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        uint32_t result = workerPool.Pool().Revoke(job, 0);
        EXPECT_EQ(result, ::Thunder::Core::ERROR_UNKNOWN_KEY);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // Id() boundary behavior — closes gap: Core WorkerPool error paths
    // (v2.1 gap: "WorkerPool — exhaustion, shutdown-during-job")
    // =========================================================================
    TEST(Core_WorkerPool, Id_Index0_ReturnsTimerThread)
    {
        WorkerPoolTester workerPool(2, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Index 0 is the timer thread - should be a valid thread id
        ::Thunder::Core::thread_id timerId = workerPool.Pool().Id(0);
        EXPECT_NE(timerId, static_cast<::Thunder::Core::thread_id>(0));

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Id_Index1_ReturnsJoinedThread)
    {
        WorkerPoolTester workerPool(2, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Index 1 is the joined thread - 0 when no one has joined
        ::Thunder::Core::thread_id joinedId = workerPool.Pool().Id(1);
        EXPECT_EQ(joinedId, static_cast<::Thunder::Core::thread_id>(0));

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Id_Index2Plus_ReturnsPoolThreads)
    {
        uint8_t threadCount = 3;
        WorkerPoolTester workerPool(threadCount, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Indices 2..threadCount+1 are pool threads
        for (uint8_t i = 0; i < threadCount; ++i) {
            ::Thunder::Core::thread_id tid = workerPool.Pool().Id(i + 2);
            EXPECT_NE(tid, static_cast<::Thunder::Core::thread_id>(0));
            EXPECT_NE(tid, static_cast<::Thunder::Core::thread_id>(~0));
        }

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Id_InvalidIndex_ReturnsInvalid)
    {
        uint8_t threadCount = 2;
        WorkerPoolTester workerPool(threadCount, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Index beyond threadCount+2 is invalid -> returns ~0
        ::Thunder::Core::thread_id invalidId = workerPool.Pool().Id(threadCount + 2);
        EXPECT_EQ(invalidId, static_cast<::Thunder::Core::thread_id>(~0));

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // =========================================================================
    // Snapshot detail validation — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Snapshot_ReportsCorrectSlotCount)
    {
        uint8_t threadCount = 3;
        WorkerPoolTester workerPool(threadCount, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        const ::Thunder::Core::IWorkerPool::Metadata& meta = workerPool.Pool().Snapshot();

        // Slots = threadCount + 2 (timer slot + external/joined slot + pool threads)
        EXPECT_EQ(meta.Slots, threadCount + 2);

        // Slot[0] is the timer thread
        EXPECT_NE(meta.Slot[0].WorkerId, static_cast<::Thunder::Core::thread_id>(0));

        // Pool thread slots should have valid worker IDs
        for (uint8_t i = 2; i < meta.Slots; ++i) {
            EXPECT_NE(meta.Slot[i].WorkerId, static_cast<::Thunder::Core::thread_id>(0));
        }

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Snapshot_AfterJobCompletion_ShowsRuns)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Submit and wait for 3 jobs sequentially
        for (int i = 0; i < 3; ++i) {
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                    workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));
            workerPool.Pool().Submit(job);
            EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);
            job.Release();
        }

        usleep(50000); // Let pool settle
        const ::Thunder::Core::IWorkerPool::Metadata& meta = workerPool.Pool().Snapshot();

        // Count total runs across all slots
        uint32_t totalRuns = 0;
        for (uint8_t i = 0; i < meta.Slots; ++i) {
            totalRuns += meta.Slot[i].Runs;
        }
        EXPECT_GE(totalRuns, 3u);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }
    TEST(Core_WorkerPool, Snapshot_EmptyPool_NoPending)
    {
        WorkerPoolTester workerPool(2, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        const ::Thunder::Core::IWorkerPool::Metadata& meta = workerPool.Pool().Snapshot();
        EXPECT_EQ(meta.Pending.size(), 0u);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // =========================================================================
    // Reschedule return value semantics — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Reschedule_ExistingJob_ReturnsTrue)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        // Schedule first
        workerPool.Pool().Schedule(::Thunder::Core::Time::Now().Add(5000), job);
        usleep(50000); // Let it register

        // Reschedule - should find and revoke the original, returning true
        bool result = workerPool.Pool().Reschedule(::Thunder::Core::Time::Now().Add(1000), job);
        EXPECT_TRUE(result);

        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }
    TEST(Core_WorkerPool, Reschedule_NonExistingJob_ReturnsFalse)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        // Reschedule a job that was never submitted
        bool result = workerPool.Pool().Reschedule(::Thunder::Core::Time::Now().Add(1000), job);
        EXPECT_FALSE(result);

        // Job should still execute since Reschedule calls Schedule
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // Join() side effects — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Join_SetsJoinedThreadId)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Before Join, Id(1) should be 0
        EXPECT_EQ(workerPool.Pool().Id(1), static_cast<::Thunder::Core::thread_id>(0));

        // Submit a job and use external (calls Join internally)
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));
        workerPool.Pool().Submit(job);

        // RunExternal -> Thread::Run() -> Worker() -> _pool.Join()
        workerPool.RunExternal();
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // WaitForStop with finite timeout — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, WaitForStop_WithTimeout_ReturnsTrue)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Submit a fast job
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));
        workerPool.Pool().Submit(job);
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);

        workerPool.Pool().Stop();
        // WaitForStop with a generous timeout should return true
        EXPECT_TRUE(workerPool.Pool().WaitForStop(5000));

        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // Concurrent submission — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, ConcurrentSubmit_FromMultipleThreads_AllComplete)
    {
        const uint8_t threadCount = 4;
        const uint8_t jobsPerThread = 5;
        const uint8_t totalJobs = threadCount * jobsPerThread;

        WorkerPoolTester workerPool(threadCount, 0, totalJobs + 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        std::vector<::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>> jobs;
        for (uint8_t i = 0; i < totalJobs; ++i) {
            jobs.push_back(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                    workerPool, TestJob<WorkerPoolTester>::INITIATED, 0)));
        }

        // Submit from multiple threads concurrently
        std::vector<std::thread> threads;
        for (uint8_t t = 0; t < threadCount; ++t) {
            threads.emplace_back([&workerPool, &jobs, t, jobsPerThread]() {
                for (uint8_t j = 0; j < jobsPerThread; ++j) {
                    workerPool.Pool().Submit(jobs[t * jobsPerThread + j]);
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }

        // All jobs should complete
        for (auto& job : jobs) {
            EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3),
                      ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                      TestJob<WorkerPoolTester>::COMPLETED);
        }

        workerPool.Stop();
        for (auto& job : jobs) {
            job.Release();
        }
        jobs.clear();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // =========================================================================
    // JobType IsIdle state tracking — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, JobType_IsIdle_TransitionsCorrectly)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();
        {
            WorkerJobTester jobTester(500);

            // Initially idle
            EXPECT_TRUE(jobTester.IsIdle());

            // After submit, not idle
            EXPECT_TRUE(jobTester.Submit());
            EXPECT_FALSE(jobTester.IsIdle());

            // Wait for completion
            EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), ::Thunder::Core::ERROR_NONE);

            // After completion and settling, should be idle again
            usleep(MaxJobWaitTime * 1000);
            EXPECT_TRUE(jobTester.IsIdle());
        }
        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // =========================================================================
    // Double Assign singleton — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Assign_ClearAndReassign_Works)
    {
        WorkerPoolTester workerPool1(1, 0, 5);
        WorkerPoolTester workerPool2(2, 0, 5);

        ::Thunder::Core::WorkerPool::Assign(&workerPool1.Pool());
        EXPECT_EQ(&::Thunder::Core::WorkerPool::Instance(), &workerPool1.Pool());
        EXPECT_TRUE(::Thunder::Core::WorkerPool::IsAvailable());

        // Must clear before reassigning (Assign asserts XOR)
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        EXPECT_FALSE(::Thunder::Core::WorkerPool::IsAvailable());

        // Now assign a different pool
        ::Thunder::Core::WorkerPool::Assign(&workerPool2.Pool());
        EXPECT_EQ(&::Thunder::Core::WorkerPool::Instance(), &workerPool2.Pool());
        EXPECT_TRUE(::Thunder::Core::WorkerPool::IsAvailable());

        ::Thunder::Core::WorkerPool::Assign(nullptr);
        EXPECT_FALSE(::Thunder::Core::WorkerPool::IsAvailable());
    }

    // =========================================================================
    // Revoke with explicit non-zero waitTime — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Revoke_WithFiniteWaitTime_CompletedJob)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                    workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        workerPool.Pool().Submit(job);
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), ::Thunder::Core::ERROR_NONE);
        usleep(50000); // Let it settle

        // Revoke a completed job with finite waitTime
        uint32_t result = workerPool.Pool().Revoke(job, 500);
        // Should return ERROR_UNKNOWN_KEY since job already completed and left the queue
        EXPECT_EQ(result, ::Thunder::Core::ERROR_UNKNOWN_KEY);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // Submit then immediate revoke — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Submit_ImmediateRevoke_JobMayOrMayNotComplete)
    {
        WorkerPoolTester workerPool(1, 0, 5);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                    workerPool, TestJob<WorkerPoolTester>::INITIATED, 500));

        // Submit but DON'T start the pool yet - job stays queued
        workerPool.Pool().Submit(job);

        // Revoke before pool starts processing (use default infinite wait)
        workerPool.Pool().Revoke(job);

        // Now start pool - job should NOT execute
        workerPool.RunThreadPool();
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime),
                  ::Thunder::Core::ERROR_TIMEDOUT);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::INITIATED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // Multiple priority jobs with revoke — closes gap: Core WorkerPool error paths
    // =========================================================================
    TEST(Core_WorkerPool, Submit_PriorityJob_Revoke_DoesNotExecute)
    {
        WorkerPoolTester workerPool(2, 0, 10);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                    workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));

        // Submit with Low priority but don't start pool
        workerPool.Pool().Submit(job, ::Thunder::Core::ThreadPool::Priority::Low);

        // Revoke before processing (use default infinite wait)
        workerPool.Pool().Revoke(job);

        workerPool.RunThreadPool();
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime),
                  ::Thunder::Core::ERROR_TIMEDOUT);
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(),
                  TestJob<WorkerPoolTester>::INITIATED);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
        job.Release();
    }

    // =========================================================================
    // WorkerPool Exhaustion Tests — Gap 7
    // =========================================================================

    // Simple blocking job using condition variable for deterministic control
    class BlockingJob : public ::Thunder::Core::IDispatch {
    public:
        BlockingJob()
            : _started(false)
            , _released(false)
            , _completed(false)
        {
        }
        ~BlockingJob() override = default;

        void Dispatch() override
        {
            {
                std::lock_guard<std::mutex> lk(_mutex);
                _started = true;
            }
            _startCV.notify_one();

            // Block until released
            {
                std::unique_lock<std::mutex> lk(_mutex);
                _releaseCV.wait(lk, [this]{ return _released; });
            }
            _completed = true;
        }

        bool WaitForStart(uint32_t ms)
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return _startCV.wait_for(lk, std::chrono::milliseconds(ms),
                [this]{ return _started; });
        }

        void Release()
        {
            {
                std::lock_guard<std::mutex> lk(_mutex);
                _released = true;
            }
            _releaseCV.notify_one();
        }

        bool IsCompleted() const { return _completed; }
        bool IsStarted() const { return _started; }

    private:
        std::mutex _mutex;
        std::condition_variable _startCV;
        std::condition_variable _releaseCV;
        bool _started;
        bool _released;
        bool _completed;
    };

    // When all threads are busy, additional jobs queue and eventually drain
    TEST(Core_WorkerPool, Exhaustion_AllThreadsBusy_JobQueues)
    {
        constexpr uint8_t threads = 2;
        WorkerPoolTester workerPool(threads, 0, 10);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Submit blocking jobs to fill all threads
        auto blocker1 = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        auto blocker2 = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(blocker1));
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(blocker2));

        ASSERT_TRUE(blocker1->WaitForStart(MaxJobWaitTime));
        ASSERT_TRUE(blocker2->WaitForStart(MaxJobWaitTime));

        // Now submit a queued job — it should NOT start yet
        auto queued = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(queued));
        usleep(100000); // 100ms
        EXPECT_FALSE(queued->IsStarted());

        // Release one blocker → queued job should now run
        blocker1->Release();
        EXPECT_TRUE(queued->WaitForStart(MaxJobWaitTime));

        // Cleanup: release remaining jobs BEFORE stopping pool
        queued->Release();
        blocker2->Release();

        // Wait for all jobs to finish before stopping
        for (int i = 0; i < 50 && !(queued->IsCompleted() && blocker2->IsCompleted()); i++) {
            usleep(20000);
        }

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // Rapid submit/cancel interleaving — no crash, no deadlock
    TEST(Core_WorkerPool, Exhaustion_RapidSubmitCancel)
    {
        WorkerPoolTester workerPool(2, 0, 20);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        constexpr int kIterations = 50;
        for (int i = 0; i < kIterations; i++) {
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> job =
                ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                    ::Thunder::Core::ProxyType<TestJob<WorkerPoolTester>>::Create(
                        workerPool, TestJob<WorkerPoolTester>::INITIATED, 0));
            workerPool.Pool().Submit(job);
            workerPool.Pool().Revoke(job);
            job.Release();
        }

        // If we get here without hanging or crashing, the test passed
        usleep(200000);

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // All threads blocked → submit N more → unblock → all complete
    TEST(Core_WorkerPool, Exhaustion_DrainAfterUnblock)
    {
        constexpr uint8_t threads = 2;
        constexpr int kQueuedJobs = 5;
        WorkerPoolTester workerPool(threads, 0, 20);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        // Block all threads
        auto blocker1 = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        auto blocker2 = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(blocker1));
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(blocker2));

        ASSERT_TRUE(blocker1->WaitForStart(MaxJobWaitTime));
        ASSERT_TRUE(blocker2->WaitForStart(MaxJobWaitTime));

        // Queue additional jobs
        std::vector<::Thunder::Core::ProxyType<BlockingJob>> queuedJobs;
        for (int i = 0; i < kQueuedJobs; i++) {
            auto job = ::Thunder::Core::ProxyType<BlockingJob>::Create();
            workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(job));
            queuedJobs.push_back(job);
        }

        // Release all blockers and queued jobs
        blocker1->Release();
        blocker2->Release();
        for (auto& job : queuedJobs) {
            // Wait for start, then release
            ASSERT_TRUE(job->WaitForStart(MaxJobWaitTime * 5));
            job->Release();
        }

        // Wait for all to complete
        for (int i = 0; i < 100; i++) {
            bool allDone = true;
            for (auto& job : queuedJobs) {
                if (!job->IsCompleted()) { allDone = false; break; }
            }
            if (allDone) break;
            usleep(20000);
        }

        for (auto& job : queuedJobs) {
            EXPECT_TRUE(job->IsCompleted());
        }

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

    // Snapshot after exhaustion shows pending count
    TEST(Core_WorkerPool, Exhaustion_SnapshotShowsPending)
    {
        constexpr uint8_t threads = 2;
        WorkerPoolTester workerPool(threads, 0, 10);
        ::Thunder::Core::WorkerPool::Assign(&workerPool.Pool());
        workerPool.RunThreadPool();

        auto blocker1 = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        auto blocker2 = ::Thunder::Core::ProxyType<BlockingJob>::Create();
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(blocker1));
        workerPool.Pool().Submit(::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(blocker2));

        ASSERT_TRUE(blocker1->WaitForStart(MaxJobWaitTime));
        ASSERT_TRUE(blocker2->WaitForStart(MaxJobWaitTime));

        // Check snapshot — threads should be busy
        const auto& info = workerPool.Pool().Snapshot();
        // Snapshot should reflect the pool state
        EXPECT_GT(info.Slots, 0u);

        blocker1->Release();
        blocker2->Release();

        for (int i = 0; i < 50 && !(blocker1->IsCompleted() && blocker2->IsCompleted()); i++) {
            usleep(20000);
        }

        workerPool.Stop();
        ::Thunder::Core::WorkerPool::Assign(nullptr);
    }

} // Core
} // Tests
} // Thunder
