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

using namespace WPEFramework;
using namespace WPEFramework::Core;

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
    TestJob(IMPLEMENTATION& parent, const Status status, const uint32_t waitTime = 0, const bool checkParentState = false)
        : _parent(parent)
        , _status(status)
        , _waitTime(waitTime)
        , _checkParentState(checkParentState)
    {
    }

public:
    Status GetStatus()
    {
        return _status;
    }
    void Cancelled()
    {
        _status = (_status != COMPLETED) ? CANCELED : _status;
    }
    void Dispatch() override
    {
        _status = COMPLETED;
        usleep(_waitTime);
        Notify();
        if (_checkParentState) {
            _parent.WaitForReady(this, _waitTime * 10);
        }
    }

private:
    IMPLEMENTATION& _parent;
    Status _status;
    uint32_t _waitTime;
    bool _checkParentState;
};


template<typename IMPLEMENTATION>
class JobControl {
private:
    typedef std::map<Core::IDispatch*, EventControl*> JobMap;
private:
    template<typename PARENTIMPL = IMPLEMENTATION>
    class ExternalWorker : public Thread {
    public:
        ExternalWorker() = delete;
        ExternalWorker& operator=(const ExternalWorker&) = delete;
        ExternalWorker(const ExternalWorker& copy)
            : _job(copy._job)
            , _parent(copy._parent)
            , _waitTime(copy._waitTime)
        {
        }

        ExternalWorker(PARENTIMPL &parent)
            : _job(nullptr)
            , _parent(parent)
            , _waitTime(0)
        {
        }
        ~ExternalWorker()
        {
            Stop();
        }

    public:
        void Stop()
        {
            Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
        }
        virtual uint32_t Worker() override
        {
            if (IsRunning()) {
                _parent.Submit(*_job, _waitTime);
            }
            Core::Thread::Block();
            return (Core::infinite);
        }

        void Submit(Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
        {
            _job = &job;
            _waitTime = waitTime;
            Core::Thread::Run();
        }
    private:
        Core::ProxyType<IDispatch>* _job;
        IMPLEMENTATION& _parent;
        uint32_t _waitTime;
    };

public:
    JobControl() = delete;
    JobControl(const JobControl&) = delete;
    JobControl& operator=(const JobControl&) = delete;
    JobControl(IMPLEMENTATION &parent)
        : _index(0)
        , _parent(parent)
        , _external()
    {
        for (uint8_t index = 0; index < MaxAdditionalWorker; ++index) {
            _external.push_back(*(new ExternalWorker<IMPLEMENTATION>(parent)));
        }
    }
    ~JobControl()
    {
        for (auto& job: _jobs) {
            delete job.second;
        }
        _jobs.clear();
        Singleton::Dispose();
    }
public:
    uint32_t WaitForReady(IDispatch* job, const uint32_t waitTime = 0)
    {
        uint32_t result = Core::ERROR_NONE;
        JobMap::iterator index = _jobs.find(job);
        if (index != _jobs.end()) {
            result = index->second->WaitForEvent(waitTime);
        }
        return result;
    }
    void NotifyReady(const Core::ProxyType<IDispatch>& job)
    {
        JobMap::iterator index = _jobs.find(job.operator->());
        if (index != _jobs.end()) {
            index->second->Notify();
        }
    }
    void SubmitUsingSelfWorker(Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        _jobs.emplace(std::piecewise_construct, std::forward_as_tuple(job.operator->()), std::forward_as_tuple(new EventControl()));

        _parent.Submit(job, waitTime);
    }
    void SubmitUsingExternalWorker(Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        if (_index < MaxAdditionalWorker) {
            _jobs.emplace(std::piecewise_construct, std::forward_as_tuple(job.operator->()), std::forward_as_tuple(new EventControl()));

            _external[_index++].Submit(job, waitTime);
        }
    }

private:
    uint8_t _index;
    JobMap _jobs;
    IMPLEMENTATION& _parent;
    std::vector<ExternalWorker<IMPLEMENTATION>> _external;
};

class Dispatcher : public Core::ThreadPool::IDispatcher {
public:
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    Dispatcher() = default;
    ~Dispatcher() override = default;

private:
    void Initialize() override {
    }
    void Deinitialize() override {
    }
    void Dispatch(Core::IDispatch* job) override {
        job->Dispatch();
    }
};

class QueueControl {
public:
    typedef Core::QueueType<Core::ProxyType<IDispatch>> MessageQueue;

    QueueControl() = delete;
    QueueControl(const QueueControl&) = delete;
    QueueControl& operator=(const QueueControl&) = delete;
    QueueControl(const uint32_t queueSize)
        : _queue(queueSize)
    {
        _queue.Enable();
    }
    ~QueueControl() = default;
    MessageQueue& Queue()
    {
        return _queue;
    }
    void Submit(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        _queue.Enable();
        if (waitTime) {
            _queue.Insert(job, waitTime);
        } else {
            _queue.Post(job);
        }
    }
    void Revoke(const Core::ProxyType<IDispatch>& job)
    {
        _queue.Remove(job);
    }
    void Shutdown()
    {
        _queue.Disable();
    }

private:
    MessageQueue _queue;
};

class MinionTester : public Thread, public QueueControl, public JobControl<MinionTester>, public ThreadPool::Minion {
public:

    MinionTester() = delete;
    MinionTester(const MinionTester&) = delete;
    MinionTester& operator=(const MinionTester&) = delete;
    MinionTester(const uint32_t queueSize)
        : QueueControl(queueSize)
        , JobControl(*this)
        , ThreadPool::Minion(QueueControl::Queue(), &_dispatcher)
        , _dispatcher()
    {
    }
    ~MinionTester()
    {
        Stop();
    }

public:
    void Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        QueueControl::Revoke(job);
        Completed(job, waitTime);
    }

    uint32_t Completed(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime)
    {
        uint32_t result = ThreadPool::Minion::Completed(job, waitTime);

        TestJob<MinionTester>& testJob = static_cast<TestJob<MinionTester>&>(*job);
        const_cast<TestJob<MinionTester>&>(testJob).Cancelled();
        return result;
    }
    virtual uint32_t Worker() override
    {
        if (IsRunning()) {
            ThreadPool::Minion::Process();
        }
        Core::Thread::Block();
        return (Core::infinite);
    }
    void Stop()
    {
        Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
    }
    uint32_t WaitForJobEvent(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        return static_cast<TestJob<MinionTester>&>(*job).WaitForEvent(waitTime);
    }

private:
    Dispatcher _dispatcher;
};

class ThreadPoolTester : public EventControl, public JobControl<ThreadPoolTester>, public ThreadPool {
private:
public:
    ThreadPoolTester() = delete;
    ThreadPoolTester(const ThreadPoolTester&) = delete;
    ThreadPoolTester& operator=(const ThreadPoolTester&) = delete;
    ThreadPoolTester(const uint8_t count, const uint32_t stackSize, const uint32_t queueSize)
        : JobControl(*this)
        , ThreadPool(count, stackSize, queueSize, &_dispatcher)
        , _dispatcher()
    {
    }
    ~ThreadPoolTester() = default;

public:
    uint32_t WaitForJobEvent(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        return static_cast<TestJob<ThreadPoolTester>&>(*job).WaitForEvent(waitTime);
    }

private:
    Dispatcher _dispatcher;
};

TEST(Core_ThreadPool, CheckMinion_ProcessJob)
{
    MinionTester minion(5);
    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Submit(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Run();

    EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime * 2), Core::ERROR_NONE);
    minion.NotifyReady(job);
    minion.Shutdown();
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);
    EXPECT_EQ(minion.Runs(), 1u);
    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_ProcessJob_CheckActiveStateInBetweenDispatch)
{
    MinionTester minion(5);
    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500, true));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.SubmitUsingSelfWorker(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Run();
    EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime * 2), Core::ERROR_NONE);
    EXPECT_EQ(minion.IsActive(), true);
    minion.NotifyReady(job);
    minion.Shutdown();

    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);
    EXPECT_EQ(minion.Runs(), 1u);
    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_CancelJob_BeforeProcessing)
{
    MinionTester minion(5);
    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Submit(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Revoke(job);
    EXPECT_EQ(minion.IsActive(), false);
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::CANCELED);
    EXPECT_EQ(minion.Runs(), 0u);
    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_CancelJob_WhileProcessing)
{
    MinionTester minion(5);
    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Submit(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Run();

    while(minion.Queue().IsEmpty() != true);
    minion.Revoke(job);
    EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    minion.Shutdown();
    EXPECT_EQ(minion.IsActive(), false);
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);
    EXPECT_EQ(minion.Runs(), 1u);
    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_CancelJob_WhileProcessing_ByAddingWaitOnTheDispatcher)
{
    MinionTester minion(5);
    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500, true));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.SubmitUsingSelfWorker(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Run();
    EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    minion.Revoke(job);
    EXPECT_EQ(minion.IsActive(), true);
    minion.NotifyReady(job);
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);
    EXPECT_EQ(minion.Runs(), 1u);
    minion.Shutdown();
    usleep(100);
    EXPECT_EQ(minion.IsActive(), false);
    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_ProcessMultipleJobs)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 1;
    MinionTester minion(queueSize);
    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500)));
    }
    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (minion.Queue().IsFull() == false) {
            minion.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(minion.Queue().IsFull(), true);
            minion.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(minion.Queue().IsFull(), true);
    EXPECT_EQ(minion.Queue().IsEmpty(), false);
    EXPECT_EQ(minion.Queue().Length(), queueSize);

    minion.Run();
    for (auto& job: jobs) {
        EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime), Core::ERROR_NONE);
    }

    minion.Shutdown();

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);
    }

    EXPECT_EQ(minion.Runs(), 6u);

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, CheckMinion_ProcessMultipleJobs_CancelInBetween)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 2;
    MinionTester minion(queueSize);
    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500)));
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (minion.Queue().IsFull() == false) {
            minion.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(minion.Queue().IsFull(), true);
            minion.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(minion.Queue().IsFull(), true);
    EXPECT_EQ(minion.Queue().IsEmpty(), false);
    EXPECT_EQ(minion.Queue().Length(), queueSize);

    minion.Run();

    minion.Revoke(jobs[3]);
    minion.Revoke(jobs[4]);

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        if ((i == 3) || (i == 4)) {
            EXPECT_EQ(minion.WaitForJobEvent(jobs[i], MaxJobWaitTime), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(minion.WaitForJobEvent(jobs[i], MaxJobWaitTime), Core::ERROR_NONE);
        }
    }

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        if ((i == 3) || (i == 4)) {
            EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*jobs[i]).GetStatus(), TestJob<MinionTester>::CANCELED);
        } else {
            EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*jobs[i]).GetStatus(), TestJob<MinionTester>::COMPLETED);
        }
    }

    minion.Shutdown();

    EXPECT_EQ(minion.Runs(), queueSize);

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessJob)
{
    uint32_t queueSize = 1;
    uint32_t threadCount = 1;
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED, 500));
    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Submit(job, 0);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.Queue().IsEmpty(), false);
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Run();
    while(threadPool.Queue().IsEmpty() != true);
    EXPECT_EQ(threadPool.Active(), true);
    EXPECT_EQ(threadPool.WaitForJobEvent(job, MaxJobWaitTime), Core::ERROR_NONE);
    EXPECT_EQ(threadPool.Pending(), 0u);
    threadPool.Stop();

    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::COMPLETED);
    job.Release();
}
TEST(Core_ThreadPool, CheckThreadPool_RevokeJob)
{
    uint32_t queueSize = 1;
    uint32_t threadCount = 1;
    ThreadPoolTester threadPool(threadCount, 0, queueSize);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED));
    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Submit(job, 0);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    threadPool.Revoke(job, 0);
    EXPECT_EQ(threadPool.Pending(), 0u);
    EXPECT_EQ(threadPool.Queue().IsEmpty(), true);
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Run();
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Stop();

    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
    job.Release();
}
TEST(Core_ThreadPool, CheckThreadPool_CancelJob_WhileProcessing)
{
    uint32_t queueSize = 1;
    uint32_t threadCount = 1;
    ThreadPoolTester threadPool(threadCount, 0, queueSize);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED, 1000));
    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Submit(job, 0);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.Queue().IsEmpty(), false);
    EXPECT_EQ(threadPool.Active(), false);
    threadPool.Run();
    EXPECT_EQ(threadPool.Queue().IsEmpty(), false);
    while(threadPool.Queue().IsEmpty() != true);
    EXPECT_EQ(threadPool.Active(), true);
    threadPool.Revoke(job, 0);
    EXPECT_EQ(threadPool.Pending(), 0u);
    EXPECT_EQ(threadPool.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    threadPool.Stop();

    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::COMPLETED);
    job.Release();
}
void CheckThreadPool_ProcessMultipleJobs(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs)
{
    ThreadPoolTester threadPool(threadCount, 0, queueSize);

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED, 500)));
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (threadPool.Queue().IsFull() == false) {
            threadPool.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(threadPool.Queue().IsFull(), true);
            threadPool.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(threadPool.Queue().IsFull(), true);
    EXPECT_EQ(threadPool.Queue().IsEmpty(), false);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.Queue().Length(), queueSize);

    threadPool.Run();
    for (auto& job: jobs) {
        EXPECT_EQ(threadPool.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::COMPLETED);
    }

    uint32_t counters[threadCount] = {0};
    uint8_t totalRuns = 0;
    threadPool.Runs(threadCount, counters);
    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += counters[i];
    }
    EXPECT_EQ(totalRuns, static_cast<uint32_t>(queueSize + additionalJobs));

    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs)
{
    CheckThreadPool_ProcessMultipleJobs(5, 1, 0);
    CheckThreadPool_ProcessMultipleJobs(5, 5, 0);
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs_AdditionalJobs)
{
    CheckThreadPool_ProcessMultipleJobs(5, 1, 1);
    CheckThreadPool_ProcessMultipleJobs(5, 5, 5);
}

TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs_CancelInBetween)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 2;
    uint8_t threadCount = 1;
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    EXPECT_EQ(threadPool.Count(), threadCount);

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED, 500)));
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (threadPool.Queue().IsFull() == false) {
            threadPool.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(threadPool.Queue().IsFull(), true);
            threadPool.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(threadPool.Queue().IsFull(), true);
    EXPECT_EQ(threadPool.Queue().IsEmpty(), false);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.Queue().Length(), queueSize);

    threadPool.Run();
    threadPool.Revoke(jobs[3], 0);
    threadPool.Revoke(jobs[4], 0);

    EXPECT_EQ(threadPool.Queue().IsFull(), false);
    Core::ProxyType<Core::IDispatch> newJob = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED));
    EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*newJob).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
    // Try to push additional job to queue from external worker
    threadPool.SubmitUsingSelfWorker(newJob, MaxJobWaitTime);
    jobs.push_back(newJob);

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        if ((i == 3) || (i == 4)) {
            EXPECT_EQ(threadPool.WaitForJobEvent(jobs[i], MaxJobWaitTime), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(threadPool.WaitForJobEvent(jobs[i], MaxJobWaitTime * 2), Core::ERROR_NONE);
        }
    }
    for (uint8_t i = 0; i < jobs.size(); ++i) {
        if ((i == 3) || (i == 4)) {
            EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*jobs[i]).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        } else {
            EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*jobs[i]).GetStatus(), TestJob<ThreadPoolTester>::COMPLETED);
        }
    }

    uint32_t counters[threadCount] = {0};
    threadPool.Runs(threadCount, counters);
    uint8_t totalRuns = 0;
    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += counters[i];
    }

    EXPECT_EQ(totalRuns, queueSize + additionalJobs - 1);
    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
void CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs)
{
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    EXPECT_EQ(threadPool.Count(), threadCount);

    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<ThreadPoolTester>>::Create(threadPool, TestJob<ThreadPoolTester>::INITIATED, 100, true)));
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (threadPool.Queue().IsFull() == false) {
            threadPool.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(threadPool.Queue().IsFull(), true);
            threadPool.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(threadPool.Queue().IsEmpty(), false);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.Queue().Length(), queueSize);

    threadPool.Run();
    usleep(100);

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        EXPECT_EQ(threadPool.WaitForJobEvent(jobs[i], MaxJobWaitTime * 5), Core::ERROR_NONE);
        if ((i == 3) || (i == 4)) {
            threadPool.Revoke(jobs[i], 0);
        }
        threadPool.NotifyReady(jobs[i]);
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::COMPLETED);
    }

    uint8_t totalRuns = 0;
    uint32_t counters[threadCount] = {0};
    threadPool.Runs(threadCount, counters);
    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += counters[i];
    }

    EXPECT_EQ(totalRuns, queueSize + additionalJobs);
    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithSinglePool)
{
    CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(1, 5, 0);
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithSinglePool_AdditionalJob)
{
    CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(1, 5, 1);
    CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(1, 5, 5);
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool)
{
    CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(5, 5, 0);
}
TEST(Core_ThreadPool, CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool_AdditionalJob)
{
    CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(5, 5, 1);
    CheckThreadPool_ProcessMultipleJobs_CancelInBetween_WithMultiplePool(5, 5, 5);
}

class ThreadJobTester : public EventControl {
public:
    ThreadJobTester(const ThreadJobTester&) = delete;
    ThreadJobTester& operator=(const ThreadJobTester&) = delete;

    ThreadJobTester()
        : _expectedTime()
        , _job(*this)
    {
    }
    ~ThreadJobTester() = default;

    Core::ProxyType<Core::IDispatch> Reset()
    {
        return _job.Reset();
    }
    Core::ProxyType<Core::IDispatch> Aquire()
    {
        return _job.Aquire();
    }
    bool IsIdle()
    {
        return _job.IsIdle();
    }

public:
    void Dispatch()
    {
        Notify();
    }

private:
    Core::Time _expectedTime;
    Core::ThreadPool::JobType<ThreadJobTester&> _job;
};


void CheckThreadPool_JobType_Submit(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId)
{
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    EXPECT_EQ(threadPool.Count(), threadCount);

    std::vector<Core::ProxyType<ThreadJobTester>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<ThreadJobTester>(Core::ProxyType<ThreadJobTester>::Create()));
    }

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        bool isCanceledJob = false;
        for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
        {
            if (i == cancelJobsId[cancelIndex]) {
                isCanceledJob = true;
                break;
            }
        }
        Core::ProxyType<Core::IDispatch> job;
        if (isCanceledJob == true) {
            EXPECT_EQ(jobs[i]->IsIdle(), true);
            EXPECT_EQ(jobs[i]->IsIdle(), true);
            job = (jobs[i]->Aquire());
            job = (jobs[i]->Reset());

        } else {
            EXPECT_EQ(jobs[i]->IsIdle(), true);
            job = (jobs[i]->Aquire());
            EXPECT_EQ(jobs[i]->IsIdle(), false);
        }
        if (job.IsValid() && (jobs[i]->IsIdle() != true)) {
            if (threadPool.Queue().IsFull() == false) {
                threadPool.SubmitUsingSelfWorker(job);
            } else {
                EXPECT_EQ(threadPool.Queue().IsFull(), true);
                threadPool.SubmitUsingExternalWorker(job, Core::infinite);
            }
        }
    }
    EXPECT_EQ(threadPool.Pending(), queueSize - cancelJobsCount);
    EXPECT_EQ(threadPool.Queue().Length(), queueSize - cancelJobsCount);

    threadPool.Run();
    usleep(100);

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        bool isCanceledJob = false;
        for (uint8_t cancelIndex = 0; cancelIndex < cancelJobsCount; cancelIndex++)
        {
            if (i == cancelJobsId[cancelIndex]) {
                isCanceledJob = true;
                break;
            }
        }
        if (isCanceledJob == true) {
            EXPECT_EQ(jobs[i]->WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(jobs[i]->WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_NONE);
        }
        EXPECT_EQ(jobs[i]->IsIdle(), true);
    }

    uint8_t totalRuns = 0;
    uint32_t counters[threadCount] = {0};
    threadPool.Runs(threadCount, counters);
    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += counters[i];
    }

    EXPECT_EQ(totalRuns, queueSize + additionalJobs - cancelJobsCount);
    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}

TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_SingleJob)
{
    CheckThreadPool_JobType_Submit(1, 1, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_SingleJob_CancelJobs)
{
    const uint8_t job = 0;
    CheckThreadPool_JobType_Submit(1, 1, 0, 1, &job);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_MultipleJobs)
{
    CheckThreadPool_JobType_Submit(1, 5, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    CheckThreadPool_JobType_Submit(1, 5, 0, sizeof(jobs), jobs);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_MultiplePool_MultipleJobs)
{
    CheckThreadPool_JobType_Submit(5, 5, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_MultiplePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    CheckThreadPool_JobType_Submit(5, 5, 0, sizeof(jobs), jobs);
}
