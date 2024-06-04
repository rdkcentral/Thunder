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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace Thunder; using namespace Thunder::Core;

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
    Event _event;
};

template<typename IMPLEMENTATION>
class TestJob : public Core::IDispatch, public EventControl {
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
        Core::Time invokedTime = Core::Time::Now();
        _status = COMPLETED;
        usleep(_waitTime);
        Notify();
        if (_checkParentState) {
            _parent.WaitForReady(this, _waitTime * 10);
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
    Core::Time CurrentTime;
    uint16_t ScheduledTime;
};

template<typename IMPLEMENTATION>
class JobControl {
private:
    typedef std::map<Core::IDispatch*, JobData> JobMap;
private:
    template<typename PARENTIMPL = IMPLEMENTATION>
    class ExternalWorker : public Thread {
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
            Core::Thread::Stop();
            Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
        }
        virtual uint32_t Worker() override
        {
            if (IsRunning()) {
                _parent.SubmitJob(*_job);
            }
            Core::Thread::Block();
            return (Core::infinite);
        }

        void Submit(Core::ProxyType<IDispatch>& job)
        {
            _job = &job;
            Core::Thread::Run();
        }
    private:
        Core::ProxyType<IDispatch>* _job;
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

        Singleton::Dispose();
    }

public:
    static void CheckScheduledTime(Core::Time current, Core::Time invokedTime,  uint16_t scheduledTime)
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
    uint32_t WaitForReady(IDispatch* job, const uint32_t waitTime = 0)
    {
        uint32_t result = Core::ERROR_NONE;
        JobMap::iterator index = _jobs.find(job);
        if (index != _jobs.end()) {
            result = index->second.Event->WaitForEvent(waitTime);
        }
        return result;
    }
    void NotifyReady(const Core::ProxyType<IDispatch>& job)
    {
        JobMap::iterator index = _jobs.find(job.operator->());
        if (index != _jobs.end()) {
            index->second.Event->Notify();
        }
    }
    void SubmitUsingSelfWorker(Core::ProxyType<IDispatch>& job)
    {
        InsertJobData(job, 0);
        _parent.Submit(job);
    }
    void SubmitUsingExternalWorker(Core::ProxyType<IDispatch>& job)
    {
        if (_index < MaxAdditionalWorker) {
            InsertJobData(job, 0);
            _external[_index++]->Submit(job);
        }
    }
    void ScheduleJobs(Core::ProxyType<IDispatch>& job, const uint16_t scheduledTime)
    {
        InsertJobData(job, scheduledTime);
        _parent.Schedule(Core::Time::Now().Add(scheduledTime), job);
    }
    void RescheduleJobs(Core::ProxyType<IDispatch>& job, const uint16_t scheduledTime)
    {
        InsertJobData(job, scheduledTime);
        _parent.Reschedule(Core::Time::Now().Add(scheduledTime), job);
    }
    void ValidateId()
    {
        const uint8_t MaxSize = 15;
        bool isPoolId = false;
        char id[MaxSize];
        sprintf(id, "%x", static_cast<uint32_t>(pthread_self()));
        for (uint8_t index = 0; index < _threadsCount + 2; index++) {
        char workerId[MaxSize];
        sprintf(workerId, "%x", static_cast<uint32_t>(IWorkerPool::Instance().Id(index)));

            if (strcpy(workerId, id)) {
                isPoolId = true;
                break;
            }
        }
        EXPECT_EQ(isPoolId, true);
    }
    void InvokedTime(IDispatch* job, const Core::Time& invokedTime)
    {
        JobMap::iterator index = _jobs.find(job);
        if (index != _jobs.end()) {
            CheckScheduledTime(index->second.CurrentTime, invokedTime, index->second.ScheduledTime);
        }
    }
    void RemoveJobData(Core::ProxyType<IDispatch>& job)
    {
        JobMap::iterator index = _jobs.find(job.operator->());
        if (index != _jobs.end()) {
            delete index->second.Event;
            _jobs.erase(index);
        }
    }

private:
    void InsertJobData(Core::ProxyType<IDispatch>& job, uint16_t scheduledTime)
    {
        JobData jobData;
        jobData.Event = new EventControl();
        jobData.ScheduledTime = scheduledTime;
        jobData.CurrentTime = Core::Time::Now();
        JobMap::iterator index = _jobs.find(job.operator->());
        if (index == _jobs.end()) {
            _jobs.emplace(std::piecewise_construct, std::forward_as_tuple(job.operator->()), std::forward_as_tuple(jobData));
        } else {
            index->second.CurrentTime = Core::Time::Now();
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

class WorkerPoolTester : public Core::WorkerPool, public JobControl<WorkerPoolTester>, public Core::Thread {
public:
    WorkerPoolTester() = delete;
    WorkerPoolTester(const WorkerPoolTester&) = delete;
    WorkerPoolTester& operator=(const WorkerPoolTester&) = delete;

    WorkerPoolTester(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize)
        : WorkerPool(threads, stackSize, queueSize, &_dispatcher)
        , JobControl(*this, threads)
    {
    }

    ~WorkerPoolTester()
    {
        // Diable the queue so the minions can stop, even if they are processing and waiting for work..
        Stop();
        Core::Singleton::Dispose();
    }

public:
    void Stop()
    {
        Core::WorkerPool::Stop();
        Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
    }

    uint32_t WaitForJobEvent(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        return static_cast<TestJob<WorkerPoolTester>&>(*job).WaitForEvent(waitTime);
    }
    void SubmitJob(const Core::ProxyType<IDispatch>& job)
    {
        Submit(job);
    }
    virtual uint32_t Worker() override
    {
        if (IsRunning()) {
            Join();
        }
        Core::Thread::Block();
        return (Core::infinite);
    }
    void RunExternal()
    {
        Core::Thread::Run();
    }
    void RunThreadPool()
    {
        static_cast<WorkerPool*>(this)->Run();
    }

private:
    class Dispatcher : public Core::ThreadPool::IDispatcher {
    public:
      Dispatcher(const Dispatcher&) = delete;
      Dispatcher& operator=(const Dispatcher&) = delete;

      Dispatcher() = default;
      ~Dispatcher() override = default;

    private:
      void Initialize() override { }
      void Deinitialize() override { }
      void Dispatch(Core::IDispatch* job) override
        { job->Dispatch(); }
    };

    Dispatcher _dispatcher;
};
TEST(Core_WorkerPool, CheckWorkerStaticMethods)
{
    uint8_t queueSize = 5;
    uint8_t threadCount = 1;
    WorkerPoolTester workerPool(threadCount, 0, queueSize);

    EXPECT_EQ(Core::WorkerPool::IsAvailable(), false);

    Core::WorkerPool::Assign(&workerPool);
    EXPECT_EQ(&Core::WorkerPool::Instance(), &workerPool);
    EXPECT_EQ(Core::WorkerPool::IsAvailable(), true);

    Core::WorkerPool::Assign(nullptr);
    EXPECT_EQ(Core::WorkerPool::IsAvailable(), false);
}
TEST(Core_WorkerPool, Check_WithSingleJob)
{
    uint8_t queueSize = 5;
    uint8_t threadCount = 1;
    WorkerPoolTester workerPool(threadCount, 0, queueSize);
    Core::WorkerPool::Assign(&workerPool);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 500));
    EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
    workerPool.Submit(job);
    workerPool.RunThreadPool();
    EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), Core::ERROR_NONE);
    workerPool.Stop();

    EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
    Core::WorkerPool::Assign(nullptr);
    job.Release();
}
TEST(Core_WorkerPool, Check_WithSingleJob_CancelJob_BeforeProcessing)
{
    uint8_t queueSize = 5;
    uint8_t threadCount = 1;
    WorkerPoolTester workerPool(threadCount, 0, queueSize);
    Core::WorkerPool::Assign(&workerPool);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED));
    EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
    workerPool.Submit(job);
    workerPool.Revoke(job);
    workerPool.RunThreadPool();
    EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), Core::ERROR_TIMEDOUT);
    workerPool.Stop();

    EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
    Core::WorkerPool::Assign(nullptr);
    job.Release();
}
TEST(Core_WorkerPool, Check_WithSingleJob_CancelJob_WhileProcessing)
{
    uint8_t queueSize = 5;
    uint8_t threadCount = 1;
    WorkerPoolTester workerPool(threadCount, 0, queueSize);
    Core::WorkerPool::Assign(&workerPool);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 1000));
    EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
    workerPool.Submit(job);
    workerPool.RunThreadPool();
    usleep(100);
    workerPool.Revoke(job);
    EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime), Core::ERROR_NONE);
    workerPool.Stop();

    EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
    Core::WorkerPool::Assign(nullptr);
    job.Release();
}
void CheckWorkerPool_MultipleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const bool runExternal = false)
{
    WorkerPoolTester workerPool(threadCount, 0, queueSize);
    Core::WorkerPool::Assign(&workerPool);

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100)));
    }

    if (runExternal == true) {
        workerPool.RunExternal();
    }

    for (uint8_t i = 0; i < queueSize; ++i) {
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.SubmitUsingSelfWorker(jobs[i]);
    }
    for (uint8_t i = queueSize; i < jobs.size(); ++i) {
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.SubmitUsingExternalWorker(jobs[i]);
    }

    workerPool.RunThreadPool();
    usleep(MaxJobWaitTime);
    for (auto& job: jobs) {
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
    }

    workerPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
    Core::WorkerPool::Assign(nullptr);
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
    Core::WorkerPool::Assign(&workerPool);

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 1000)));
    }

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
         workerPool.Revoke(jobs[cancelJobsId[index]], 0);
    }
    workerPool.RunThreadPool();

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
            EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), Core::ERROR_NONE);
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
    Core::WorkerPool::Assign(nullptr);
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
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 500, false, true)));
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
        workerPool.Revoke(jobs[cancelJobsId[index]], 0);
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
            EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 15), Core::ERROR_NONE);
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
    Core::WorkerPool::Assign(nullptr);
}
TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_SingleJob)
{
    const uint16_t scheduledTimes[] = {2000}; //In milliseconds
    CheckWorkerPool_ScheduleJobs(1, 1, 0, 0, nullptr, scheduledTimes);
}
TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_SingleJob_ZeroTime)
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
TEST(Core_WorkerPool, Check_ScheduleJobs_SinglePool_MultipleJobs_ZeroTime)
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
TEST(Core_WorkerPool, Check_ScheduleJobs_MultiplePool_MultipleJobs_ZeroTime)
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
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 500, false, true)));
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
        workerPool.Revoke(jobs[cancelJobsId[index]], 0);
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
            EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(workerPool.WaitForJobEvent(jobs[index], MaxJobWaitTime * 15), Core::ERROR_NONE);
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
    Core::WorkerPool::Assign(nullptr);
}
TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_SingleJob)
{
    const uint16_t scheduledTimes[] = {1000};
    const uint16_t rescheduledTimes[] = {2000};
    CheckWorkerPool_RescheduleJobs(1, 1, 0, 0, nullptr, scheduledTimes, rescheduledTimes);
}
TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_SingleJob_ZeroTime)
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
TEST(Core_WorkerPool, Check_RescheduleJobs_SinglePool_MultipleJobs_ZeroTime)
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
TEST(Core_WorkerPool, Check_RescheduleJobs_MultiplePool_MultipleJobs_ZeroTime)
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
    const IWorkerPool::Metadata& metaData = IWorkerPool::Instance().Snapshot();

    EXPECT_EQ(metaData.Pending.size(), pending); // Whatever is in the ThreadPool::_queue is considered pending

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
    Core::WorkerPool::Assign(&workerPool);

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100)));
    }

    for (uint8_t i = 0; i < queueSize; ++i) {
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.SubmitUsingSelfWorker(jobs[i]);
    }
    for (uint8_t i = queueSize; i < jobs.size(); ++i) {
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*jobs[i]).GetStatus(), TestJob<WorkerPoolTester>::INITIATED);
        workerPool.SubmitUsingExternalWorker(jobs[i]);
    }

    CheckMetaData(queueSize, 1, 0);
    workerPool.RunThreadPool();

    usleep(MaxJobWaitTime);
    for (auto& job: jobs) {
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
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
    Core::WorkerPool::Assign(nullptr);
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
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<WorkerPoolTester>>::Create(workerPool, TestJob<WorkerPoolTester>::INITIATED, 100, false, false, true)));
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
        EXPECT_EQ(workerPool.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<WorkerPoolTester>&>(*job).GetStatus(), TestJob<WorkerPoolTester>::COMPLETED);
    }

    workerPool.Stop();
    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
    Core::WorkerPool::Assign(nullptr);
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
        _expectedTime = Core::Time::Now();
        return _job.Submit();
    }
    bool Schedule(const Core::Time& time)
    {
        _expectedTime = time;
        return _job.Reschedule(time);
    }
    bool Reschedule(const Core::Time& time)
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
        Core::Time invokedTime = Core::Time::Now();
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
    Core::Time _expectedTime;
    bool _waitForCaller;
    Core::WorkerPool::JobType<WorkerJobTester&> _job;
};

TEST(Core_WorkerPool, Check_JobType_Submit)
{
    WorkerPoolTester workerPool(1, 0, 1);
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        WorkerJobTester jobTester(0);
        EXPECT_EQ(jobTester.Submit(), true);
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_NONE);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
}
TEST(Core_WorkerPool, Check_JobType_Submit_Revoke)
{
    WorkerPoolTester workerPool(4, 0, 1);
    Core::WorkerPool::Assign(&workerPool);
    {
        WorkerJobTester jobTester(0);
        EXPECT_EQ(jobTester.Submit(), true);
        jobTester.Revoke();
        workerPool.RunThreadPool();
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
}
TEST(Core_WorkerPool, Check_JobType_Schedule)
{
    WorkerPoolTester workerPool(4, 0, 1);
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        WorkerJobTester jobTester(0);
        jobTester.Schedule(Core::Time::Now().Add(2000));
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_NONE);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
}
TEST(Core_WorkerPool, Check_JobType_Schedule_Revoke)
{
    WorkerPoolTester workerPool(4, 0, 1);
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        WorkerJobTester jobTester(0);
        jobTester.Schedule(Core::Time::Now().Add(2000));
        usleep(500);
        jobTester.Revoke();
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 4), Core::ERROR_TIMEDOUT);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
}
void CheckJobType_Reschedule(const uint16_t scheduleTime, const uint16_t rescheduleTime)
{
    WorkerPoolTester workerPool(4, 0, 1);
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        WorkerJobTester jobTester(0);
        jobTester.Schedule(Core::Time::Now().Add(scheduleTime));
        usleep(500);
        EXPECT_EQ(jobTester.Reschedule(Core::Time::Now().Add(rescheduleTime)), true);
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 4), Core::ERROR_NONE);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
}
TEST(Core_WorkerPool, Check_JobType_Reschedule)
{
    CheckJobType_Reschedule(2000, 3000);
    CheckJobType_Reschedule(2000, 0);
}
TEST(Core_WorkerPool, Check_JobType_Reschedule_Revoke)
{
    WorkerPoolTester workerPool(4, 0, 1);
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        WorkerJobTester jobTester(0);
        jobTester.Schedule(Core::Time::Now().Add(2000));
        usleep(500);
        jobTester.Schedule(Core::Time::Now().Add(3000));
        usleep(500);
        jobTester.Revoke();
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 4), Core::ERROR_TIMEDOUT);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
}
void CheckJobType_RescheduleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId, const uint16_t scheduledTimes[], const uint16_t rescheduledTimes[])
{
    WorkerPoolTester workerPool(threadCount, 0, queueSize);
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        std::vector<Core::ProxyType<WorkerJobTester>> jobs;
        // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
        for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
            jobs.push_back(Core::ProxyType<WorkerJobTester>(Core::ProxyType<WorkerJobTester>::Create(0)));
        }
        for (uint8_t i = 0; i < jobs.size(); ++i) {
            EXPECT_EQ(jobs[i]->IsIdle(), true);
        }

        for (uint8_t i = 0; i < jobs.size(); ++i) {
            jobs[i]->Schedule(Core::Time::Now().Add(scheduledTimes[i]));
        }

        for (uint8_t i = 0; i < jobs.size(); ++i) {
            EXPECT_EQ(jobs[i]->Reschedule(Core::Time::Now().Add(rescheduledTimes[i])), true);
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
                EXPECT_EQ(jobs[index]->WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
            } else {
                EXPECT_EQ(jobs[index]->WaitForEvent(MaxJobWaitTime * 15), Core::ERROR_NONE);
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
    Core::WorkerPool::Assign(nullptr);
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
    Core::WorkerPool::Assign(&workerPool);
    workerPool.RunThreadPool();
    {
        WorkerJobTester jobTester(1000);
        jobTester.Schedule(Core::Time::Now().Add(0));
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 2), Core::ERROR_NONE);
        jobTester.Reset();
        EXPECT_EQ(jobTester.Reschedule(Core::Time::Now().Add(1000)), false);
        jobTester.Notify();
        EXPECT_EQ(jobTester.WaitForEvent(MaxJobWaitTime * 2), Core::ERROR_NONE);
    }
    workerPool.Stop();
    Core::WorkerPool::Assign(nullptr);
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
        _triggerJob.Reschedule(Core::Time::Now().Add(_intervalInMilliSeconds));
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

        // The _decoupledJob is of type Core::WorkerPool::JobType<Job>, which, according to the
        // design will be decoupled from this thread and run the dispatch (of Job) on another
        // thread. To avoid starvation, the Job will obnnly be ran once. If it is already
        // running (Active) it will be scheduled (state changed) to be scheduled for rerun after
        // it completed.
        _decoupledJob.Submit();

        // Reschedule our selves to submit another run for the Job in _intervalInMilliSeconds
        // from now..
        if (--_cycles != 0) {
            _triggerJob.Reschedule(Core::Time::Now().Add(_intervalInMilliSeconds));
        } else {
            Notify();
        }
    }

private:
    uint8_t _cycles;
    uint32_t _intervalInMilliSeconds;
    Core::WorkerPool::JobType<Trigger&> _triggerJob;
    Core::WorkerPool::JobType<Job>     _decoupledJob;
};
void CheckWorkerPool_ReschduledTimedJob(const uint8_t threadCount, const uint8_t queueSize, const uint16_t timedWait, const uint16_t jobWait, const uint8_t times)
{
    WorkerPoolTester workerPool(threadCount, 0, queueSize);
    Core::WorkerPool::Assign(&workerPool);
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

        EXPECT_EQ(triggerJob.WaitForEvent((timedWait + (times * jobWait)) * 1000), Core::ERROR_NONE);
        EXPECT_EQ(triggerJob.Pending(), 0u);
        // In the new code the Job will never be running with more than one instance so the line: "Ooopsie daisy, that is unexpected, it seems there are multiples of me running, thats
        // not according to the spec !!!!\n" should never be observed.
        // However the job will not be ran 5 times as there are 3 schedule overruns due to the fact that the Job is still running when the next submit ocures. SO the job wll only run
        // 3 times and it will take 10 + 3* 40 seconds to complete.
    }
    Core::WorkerPool::Assign(nullptr);
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

