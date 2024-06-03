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

using namespace Thunder;
using namespace Thunder::Core;

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

        ExternalWorker(PARENTIMPL &parent, Core::ProxyType<IDispatch> job, const uint32_t waitTime = 0)
            : _job(job)
            , _parent(parent)
            , _waitTime(waitTime)
        {

            Core::Thread::Run();
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
                _parent.Submit(_job, _waitTime);
            }
            Core::Thread::Block();
            return (Core::infinite);
        }
    private:
        Core::ProxyType<IDispatch> _job;
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
    }
    ~JobControl() = default;

public:
    void Stop()
    {
        for (auto& job: _jobs) {
            delete job.second;
        }
        _jobs.clear();
        for (auto& external: _external) {
            delete external;
        }
        _external.clear();
        Singleton::Dispose();
    }
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
        if (_external.size() < MaxAdditionalWorker) {
            _jobs.emplace(std::piecewise_construct, std::forward_as_tuple(job.operator->()), std::forward_as_tuple(new EventControl()));

            _external.push_back(new ExternalWorker<IMPLEMENTATION>(_parent, job, waitTime));
        }
    }

private:
    uint8_t _index;
    JobMap _jobs;
    IMPLEMENTATION& _parent;
    std::vector<ExternalWorker<IMPLEMENTATION>*> _external;
};

class Scheduler : public ThreadPool::IScheduler {
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
        Timer(ThreadPool* pool, const ProxyType<IDispatch>& job)
            : _job(job)
            , _pool(pool)
        {
        }
        ~Timer()
        {
        }

    public:
        uint64_t Timed(const uint64_t /* scheduledTime */)
        {
            ASSERT(_pool != nullptr);
            _pool->Submit(_job, Core::infinite);
            //_job.Release();
            // No need to reschedule, just drop it..
            return (0);
        }

     private:
        ProxyType<IDispatch> _job;
        ThreadPool* _pool;
    };
public:
    Scheduler() = delete;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    Scheduler(ThreadPool& pool)
        : _pool(pool)
        , _timer(1024 * 1024, _T("ThreadPool::Timer"))
    {
    }
    ~Scheduler() override = default;

public:
    void Schedule(const Time& time, const ProxyType<IDispatch>& job) override
    {
        _timer.Schedule(time, Timer(&_pool, job));
    }

private:
    ThreadPool& _pool;
    Core::TimerType<Timer> _timer;
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

class ThreadPoolTester : public EventControl, public JobControl<ThreadPoolTester>, public ThreadPool {
private:
public:
    ThreadPoolTester() = delete;
    ThreadPoolTester(const ThreadPoolTester&) = delete;
    ThreadPoolTester& operator=(const ThreadPoolTester&) = delete;
    ThreadPoolTester(const uint8_t count, const uint32_t stackSize, const uint32_t queueSize)
        : JobControl(*this)
        , ThreadPool(count, stackSize, queueSize, &_dispatcher, &_scheduler, nullptr, nullptr)
        , _queueSize(queueSize)
        , _dispatcher()
        , _scheduler(*this)
    {
    }
    ~ThreadPoolTester()
    {
    }

public:
    uint32_t WaitForJobEvent(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        return static_cast<TestJob<ThreadPoolTester>&>(*job).WaitForEvent(waitTime);
    }
    bool QueueIsFull()
    {
        return (ThreadPool::Pending() == _queueSize);
    }
    bool QueueIsEmpty()
    {
         return (ThreadPool::Pending() == 0);
    }
    void Stop()
    {
        ThreadPool::Stop();
        JobControl<ThreadPoolTester>::Stop();
    }

private:
    uint32_t _queueSize;
    Dispatcher _dispatcher;
    Scheduler _scheduler;
};

class MinionTester : public Thread, public JobControl<MinionTester>, public ThreadPool::Minion {
public:

    MinionTester() = delete;
    MinionTester(const MinionTester&) = delete;
    MinionTester& operator=(const MinionTester&) = delete;
    MinionTester(ThreadPoolTester& threadPool, const uint32_t queueSize)
        : JobControl(*this)
        , ThreadPool::Minion(threadPool, &_dispatcher)
        , _queueSize(queueSize)
        , _threadPool(threadPool)
        , _dispatcher()
    {
    }
    ~MinionTester()
    {
        Stop();
    }

public:
    void Submit(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        _threadPool.Submit(job, waitTime);
    }
    void RunThreadPool()
    {
        _threadPool.Run();
    }
    void Shutdown()
    {
        _threadPool.Stop();
    }
    void Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        _threadPool.Revoke(job, waitTime);
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
        Core::Thread::Stop();
        Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
        JobControl<MinionTester>::Stop();
    }
    uint32_t WaitForJobEvent(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = 0)
    {
        return static_cast<TestJob<MinionTester>&>(*job).WaitForEvent(waitTime);
    }
    bool QueueIsFull()
    {
        return (_threadPool.Pending() == _queueSize);
    }
    bool QueueIsEmpty()
    {
         return (_threadPool.Pending() == 0);
    }

private:
    uint32_t _queueSize;
    ThreadPoolTester& _threadPool;
    Dispatcher _dispatcher;
};
TEST(Core_ThreadPool, CheckMinion_ProcessJob)
{
    uint8_t queueSize = 5;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    minion.Submit(job);
    minion.Run();

    EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime * 2), Core::ERROR_NONE);
    minion.NotifyReady(job);
    minion.Shutdown();
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, 1u);

    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_ProcessJob_CheckActiveStateInBetweenDispatch)
{
    uint8_t queueSize = 5;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);

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

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, 1u);

    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_CancelJob_BeforeProcessing)
{
    uint8_t queueSize = 5;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Submit(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Revoke(job);
    EXPECT_EQ(minion.IsActive(), false);
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::CANCELED);

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, 0u);

    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_CancelJob_WhileProcessing)
{
    uint8_t queueSize = 5;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);

    Core::ProxyType<Core::IDispatch> job = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500));
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::INITIATED);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Submit(job);
    EXPECT_EQ(minion.IsActive(), false);
    minion.Run();

    volatile bool queueIsEmpty = false;
    while((queueIsEmpty = minion.QueueIsEmpty()) != true) {
        __asm__ volatile("nop");
    }
    minion.Revoke(job);
    EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    minion.Shutdown();
    EXPECT_EQ(minion.IsActive(), false);
    EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, 1u);

    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_CancelJob_WhileProcessing_ByAddingWaitOnTheDispatcher)
{
    uint8_t queueSize = 5;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);
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

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, 1u);

    minion.Shutdown();
    usleep(100);
    EXPECT_EQ(minion.IsActive(), false);
    job.Release();
}
TEST(Core_ThreadPool, CheckMinion_ProcessMultipleJobs)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 1;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);
    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500)));
    }
    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (minion.QueueIsFull() == false) {
            minion.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(minion.QueueIsFull(), true);
            minion.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(minion.QueueIsFull(), true);
    EXPECT_EQ(minion.QueueIsEmpty(), false);

    minion.Run();
    for (auto& job: jobs) {
        EXPECT_EQ(minion.WaitForJobEvent(job, MaxJobWaitTime), Core::ERROR_NONE);
    }

    minion.Shutdown();

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<MinionTester>&>(*job).GetStatus(), TestJob<MinionTester>::COMPLETED);
    }

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, 6u);

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, CheckMinion_ProcessMultipleJobs_CancelInBetween)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 2;
    ThreadPoolTester threadPool(0, 0, queueSize);
    MinionTester minion(threadPool, queueSize);
    std::vector<Core::ProxyType<Core::IDispatch>> jobs;
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob<MinionTester>>::Create(minion, TestJob<MinionTester>::INITIATED, 500)));
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::INITIATED);
        if (minion.QueueIsFull() == false) {
            minion.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(minion.QueueIsFull(), true);
            minion.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(minion.QueueIsFull(), true);
    EXPECT_EQ(minion.QueueIsEmpty(), false);

    minion.Revoke(jobs[3]);
    minion.Revoke(jobs[4]);

    minion.Run();
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

    Thunder::Core::ThreadPool::Metadata info;
    minion.Info(info);
    EXPECT_EQ(info.Runs, queueSize);

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
    threadPool.Submit(job, 0);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.QueueIsEmpty(), false);
    threadPool.Run();
    while(threadPool.QueueIsEmpty() != true) {
        __asm__ volatile("nop");
    }
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
    threadPool.Submit(job, 0);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    threadPool.Revoke(job, 0);
    EXPECT_EQ(threadPool.Pending(), 0u);
    EXPECT_EQ(threadPool.QueueIsEmpty(), true);
    threadPool.Run();
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
    threadPool.Submit(job, 0);
    EXPECT_EQ(threadPool.Pending(), queueSize);
    EXPECT_EQ(threadPool.QueueIsEmpty(), false);
    threadPool.Run();
    EXPECT_EQ(threadPool.QueueIsEmpty(), false);
    while(threadPool.QueueIsEmpty() != true) {
        __asm__ volatile("nop");
    }
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
        if (threadPool.QueueIsFull() == false) {
            threadPool.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(threadPool.QueueIsFull(), true);
            threadPool.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(threadPool.QueueIsFull(), true);
    EXPECT_EQ(threadPool.QueueIsEmpty(), false);
    EXPECT_EQ(threadPool.Pending(), queueSize);

    threadPool.Run();
    for (auto& job: jobs) {
        EXPECT_EQ(threadPool.WaitForJobEvent(job, MaxJobWaitTime * 3), Core::ERROR_NONE);
    }

    for (auto& job: jobs) {
        EXPECT_EQ(static_cast<TestJob<ThreadPoolTester>&>(*job).GetStatus(), TestJob<ThreadPoolTester>::COMPLETED);
    }

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
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
    //CheckThreadPool_ProcessMultipleJobs(5, 1, 1);
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
        if (threadPool.QueueIsFull() == false) {
            threadPool.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(threadPool.QueueIsFull(), true);
            threadPool.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(threadPool.QueueIsFull(), true);
    EXPECT_EQ(threadPool.QueueIsEmpty(), false);
    EXPECT_EQ(threadPool.Pending(), queueSize);

    threadPool.Run();
    threadPool.Revoke(jobs[3], 0);
    threadPool.Revoke(jobs[4], 0);

    EXPECT_EQ(threadPool.QueueIsFull(), false);
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

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
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
        if (threadPool.QueueIsFull() == false) {
            threadPool.SubmitUsingSelfWorker(job);
        } else {
            EXPECT_EQ(threadPool.QueueIsFull(), true);
            threadPool.SubmitUsingExternalWorker(job, Core::infinite);
        }
    }
    EXPECT_EQ(threadPool.QueueIsEmpty(), false);
    EXPECT_EQ(threadPool.Pending(), queueSize);

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

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
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

    Core::ProxyType<Core::IDispatch> Submit()
    {
        return _job.Submit();
    }
    Core::ProxyType<Core::IDispatch> Reschedule(const Time& time)
    {
        return _job.Reschedule(time);
    }
    Core::ProxyType<Core::IDispatch> Revoke()
    {
        Core::ProxyType<Core::IDispatch> job = _job.Revoke();
        _job.Revoked();
        return job;
    }
    Core::ProxyType<Core::IDispatch> Idle()
    {
        return _job.Idle();
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

void CheckThreadPool_JobType_Submit_Using_Idle(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId)
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
            job = (jobs[i]->Idle());
            EXPECT_EQ(jobs[i]->IsIdle(), false);
            job = (jobs[i]->Revoke());
            EXPECT_EQ(jobs[i]->IsIdle(), true);

        } else {
            EXPECT_EQ(jobs[i]->IsIdle(), true);
            job = (jobs[i]->Idle());
            EXPECT_EQ(jobs[i]->IsIdle(), false);

            // Try to get same job again
            Core::ProxyType<Core::IDispatch> jobRetry = (jobs[i]->Idle());
            EXPECT_EQ(jobRetry.IsValid(), false);
        }
        if (job.IsValid() && (jobs[i]->IsIdle() != true)) {
            if (threadPool.QueueIsFull() == false) {
                threadPool.SubmitUsingSelfWorker(job);
            } else {
                EXPECT_EQ(threadPool.QueueIsFull(), true);
                threadPool.SubmitUsingExternalWorker(job, Core::infinite);
            }
        }
    }
    EXPECT_EQ(threadPool.Pending(), static_cast<uint32_t>(queueSize - cancelJobsCount));

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
            usleep(500);
        }
        EXPECT_EQ(jobs[i]->IsIdle(), true);
    }

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
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
    CheckThreadPool_JobType_Submit_Using_Idle(1, 1, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_SingleJob_CancelJobs)
{
    const uint8_t job = 0;
    CheckThreadPool_JobType_Submit_Using_Idle(1, 1, 0, 1, &job);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_MultipleJobs)
{
    CheckThreadPool_JobType_Submit_Using_Idle(1, 5, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_SinglePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    CheckThreadPool_JobType_Submit_Using_Idle(1, 5, 0, sizeof(jobs), jobs);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_MultiplePool_MultipleJobs)
{
    CheckThreadPool_JobType_Submit_Using_Idle(5, 5, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_MultiplePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    CheckThreadPool_JobType_Submit_Using_Idle(5, 5, 0, sizeof(jobs), jobs);
}

void CheckThreadPool_JobType_Submit_Using_Submit(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId)
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
            job = (jobs[i]->Idle());
            EXPECT_EQ(jobs[i]->IsIdle(), false);
            job = (jobs[i]->Revoke());
            EXPECT_EQ(jobs[i]->IsIdle(), true);
        } else {
            EXPECT_EQ(jobs[i]->IsIdle(), true);
            job = (jobs[i]->Submit());
            EXPECT_EQ(jobs[i]->IsIdle(), false);

            // Try to get same job again
            Core::ProxyType<Core::IDispatch> jobRetry = (jobs[i]->Submit());
            EXPECT_EQ(jobRetry.IsValid(), false);
        }
        if (job.IsValid() && (jobs[i]->IsIdle() != true)) {
            if (threadPool.QueueIsFull() == false) {
                threadPool.SubmitUsingSelfWorker(job);
            } else {
                EXPECT_EQ(threadPool.QueueIsFull(), true);
                threadPool.SubmitUsingExternalWorker(job, Core::infinite);
            }
        }
    }
    EXPECT_EQ(threadPool.Pending(), static_cast<uint32_t>(queueSize - cancelJobsCount));

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
            usleep(100);
        }
        EXPECT_EQ(jobs[i]->IsIdle(), true);
    }

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
    }
    EXPECT_EQ(totalRuns, queueSize + additionalJobs - cancelJobsCount);

    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Submit_SinglePool_SingleJob)
{
    CheckThreadPool_JobType_Submit_Using_Submit(1, 1, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Submit_SinglePool_SingleJob_CancelJobs)
{
    const uint8_t job = 0;
    CheckThreadPool_JobType_Submit_Using_Submit(1, 1, 0, 1, &job);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Submit_SinglePool_MultipleJobs)
{
    CheckThreadPool_JobType_Submit_Using_Submit(1, 5, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Submit_SinglePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    CheckThreadPool_JobType_Submit_Using_Submit(1, 5, 0, sizeof(jobs), jobs);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Submit_MultiplePool_MultipleJobs)
{
    CheckThreadPool_JobType_Submit_Using_Submit(5, 5, 0, 0, nullptr);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Submit_MultiplePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    CheckThreadPool_JobType_Submit_Using_Submit(5, 5, 0, sizeof(jobs), jobs);
}

void CheckThreadPool_JobType_Submit_Using_Reschedule(const uint8_t threadCount, const uint8_t queueSize, const uint8_t additionalJobs, const uint8_t cancelJobsCount, const uint8_t* cancelJobsId, const uint16_t scheduledTimes[])
{
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    EXPECT_EQ(threadPool.Count(), threadCount);
    threadPool.Run();

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
            job = (jobs[i]->Idle());
            EXPECT_EQ(jobs[i]->IsIdle(), false);
            job = (jobs[i]->Revoke());
            EXPECT_EQ(jobs[i]->IsIdle(), true);
        } else {
            EXPECT_EQ(jobs[i]->IsIdle(), true);
            job = (jobs[i]->Reschedule(Core::Time::Now().Add(scheduledTimes[i])));
            EXPECT_EQ(jobs[i]->IsIdle(), false);

            // Try to get same job again
            Core::ProxyType<Core::IDispatch> jobRetry = (jobs[i]->Reschedule(Core::Time::Now().Add(scheduledTimes[i])));
            EXPECT_EQ(jobRetry.IsValid(), false);
        }
        if (job.IsValid() && (jobs[i]->IsIdle() != true)) {
            if (threadPool.QueueIsFull() == false) {
                threadPool.SubmitUsingSelfWorker(job);
            } else {
                EXPECT_EQ(threadPool.QueueIsFull(), true);
                threadPool.SubmitUsingExternalWorker(job, Core::infinite);
            }
        }
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
        if (isCanceledJob == true) {
            EXPECT_EQ(jobs[i]->WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_TIMEDOUT);
        } else {
            EXPECT_EQ(jobs[i]->WaitForEvent(MaxJobWaitTime * 4), Core::ERROR_NONE);
            usleep(100);
        }
        EXPECT_EQ(jobs[i]->IsIdle(), true);
    }

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
    }
    EXPECT_EQ(totalRuns, (queueSize + additionalJobs - cancelJobsCount) * 2);

    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Reschedule_SinglePool_SingleJob)
{
    const uint16_t scheduledTimes[] = {1000, 1000};
    CheckThreadPool_JobType_Submit_Using_Reschedule(1, 1, 1, 0, nullptr, scheduledTimes);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Reschedule_SinglePool_SingleJob_CancelJobs)
{
    const uint8_t job = 0;
    const uint16_t scheduledTimes[] = {2000};
    CheckThreadPool_JobType_Submit_Using_Reschedule(1, 1, 0, 1, &job, scheduledTimes);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Reschedule_SinglePool_MultipleJobs)
{
    const uint16_t scheduledTimes[] = {1000, 2000, 4000, 1000, 2000, 1000};
    CheckThreadPool_JobType_Submit_Using_Reschedule(1, 5, 1, 0, nullptr, scheduledTimes);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Reschedule_SinglePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    const uint16_t scheduledTimes[] = {2000, 2000, 2000, 1000, 2000};
    CheckThreadPool_JobType_Submit_Using_Reschedule(1, 5, 0, sizeof(jobs), jobs, scheduledTimes);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Reschedule_MultiplePool_MultipleJobs)
{
    const uint16_t scheduledTimes[] = {3000, 2000, 1000, 1000, 3000, 1000};
    CheckThreadPool_JobType_Submit_Using_Reschedule(5, 5, 1, 0, nullptr, scheduledTimes);
}
TEST(Core_ThreadPool, Check_ThreadPool_JobType_Submit_Using_Reschedule_MultiplePool_MultipleJobs_CancelJobs)
{
    const uint8_t jobs[] = {0, 2};
    const uint16_t scheduledTimes[] = {3000, 2000, 2000, 1000, 2000};
    CheckThreadPool_JobType_Submit_Using_Reschedule(5, 5, 0, sizeof(jobs), jobs, scheduledTimes);
}

TEST(Core_ThreadPool, CheckThreadPool_JobType_Reschedule_AfterSubmit)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 1;
    uint8_t threadCount = 5;
    const uint16_t scheduledTimes[] = {2000, 2000, 3000, 1000, 2000, 1000};
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    EXPECT_EQ(threadPool.Count(), threadCount);
    threadPool.Run();

    std::vector<Core::ProxyType<ThreadJobTester>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<ThreadJobTester>(Core::ProxyType<ThreadJobTester>::Create()));
    }

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        Core::ProxyType<Core::IDispatch> job;
        EXPECT_EQ(jobs[i]->IsIdle(), true);
        job = (jobs[i]->Submit());
        EXPECT_EQ(jobs[i]->IsIdle(), false);

        // Try to get same job again
        Core::ProxyType<Core::IDispatch> jobRetry = (jobs[i]->Reschedule(Core::Time::Now().Add(scheduledTimes[i])));
        EXPECT_EQ(jobs[i]->IsIdle(), false);
        EXPECT_EQ(job.IsValid(), true);

        if (job.IsValid() && (jobs[i]->IsIdle() != true)) {
            if (threadPool.QueueIsFull() == false) {
                threadPool.SubmitUsingSelfWorker(job);
            } else {
                EXPECT_EQ(threadPool.QueueIsFull(), true);
                threadPool.SubmitUsingExternalWorker(job, Core::infinite);
            }
        }
    }

    sleep(2);
    for (uint8_t i = 0; i < jobs.size(); ++i) {
        EXPECT_EQ(jobs[i]->WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_NONE);
        usleep(500);
        EXPECT_EQ(jobs[i]->IsIdle(), true);
    }

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
    }
    EXPECT_EQ(totalRuns, (queueSize + additionalJobs) * 2);

    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}
TEST(Core_ThreadPool, CheckThreadPool_JobType_Reschedule_AfterIdle)
{
    uint8_t queueSize = 5;
    uint8_t additionalJobs = 1;
    uint8_t threadCount = 5;
    const uint16_t scheduledTimes[] = {1000, 2000, 3000, 1000, 4000, 1000};
    ThreadPoolTester threadPool(threadCount, 0, queueSize);
    EXPECT_EQ(threadPool.Count(), threadCount);
    threadPool.Run();

    std::vector<Core::ProxyType<ThreadJobTester>> jobs;
    // Create Jobs with more than Queue size. i.e, queueSize + additionalJobs
    for (uint8_t i = 0; i < queueSize + additionalJobs; ++i) {
        jobs.push_back(Core::ProxyType<ThreadJobTester>(Core::ProxyType<ThreadJobTester>::Create()));
    }

    for (uint8_t i = 0; i < jobs.size(); ++i) {
        Core::ProxyType<Core::IDispatch> job;
        EXPECT_EQ(jobs[i]->IsIdle(), true);
        job = (jobs[i]->Submit());
        EXPECT_EQ(jobs[i]->IsIdle(), false);

        // Try to get same job again
        Core::ProxyType<Core::IDispatch> jobRetry = (jobs[i]->Reschedule(Core::Time::Now().Add(scheduledTimes[i])));
        EXPECT_EQ(jobs[i]->IsIdle(), false);
        EXPECT_EQ(job.IsValid(), true);

        if (job.IsValid() && (jobs[i]->IsIdle() != true)) {
            if (threadPool.QueueIsFull() == false) {
                threadPool.SubmitUsingSelfWorker(job);
            } else {
                EXPECT_EQ(threadPool.QueueIsFull(), true);
                threadPool.SubmitUsingExternalWorker(job, Core::infinite);
            }
        }
    }

    sleep(2);
    for (uint8_t i = 0; i < jobs.size(); ++i) {
        EXPECT_EQ(jobs[i]->WaitForEvent(MaxJobWaitTime * 3), Core::ERROR_NONE);
        usleep(500);
        EXPECT_EQ(jobs[i]->IsIdle(), true);
    }

    uint8_t totalRuns = 0;

    Thunder::Core::ThreadPool::Metadata info[threadCount];
    std::vector<string> jobsStrings;

    threadPool.Snapshot(threadCount, info, jobsStrings);

    for (uint8_t i = 0; i < threadCount; ++i) {
        totalRuns += info[i].Runs;
    }
    EXPECT_EQ(totalRuns, (queueSize + additionalJobs) * 2);

    threadPool.Stop();

    for (auto& job: jobs) {
        job.Release();
    }
    jobs.clear();
}

