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

#include <gtest/gtest.h>

#include "core/WorkerPool.h"
#include "core/Sync.h"
#include <atomic>
#include <chrono>

using namespace Thunder;
using namespace Thunder::Core;

/**
 * Test to verify that a WorkerPool job can safely submit itself again
 * from within its own Dispatch() method.
 */
class WorkerPoolSelfResubmitTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Create a WorkerPool with multiple threads
        _dispatcher = new WorkerPoolDispatcher();
        _workerPool = new WorkerPool(
            1, // threadCount
            Core::Thread::DefaultStackSize(), // stackSize (0 = default)
            2, // queueSize
            _dispatcher,
            nullptr
        );

        IWorkerPool::Assign(_workerPool);
        _workerPool->Run();
    }

    void TearDown() override
    {
        if (_workerPool != nullptr) {
            _workerPool->Stop();
            IWorkerPool::Assign(nullptr);
            delete _workerPool;
            _workerPool = nullptr;
        }

        if (_dispatcher != nullptr) {
            delete _dispatcher;
            _dispatcher = nullptr;
        }
    }

    // Simple dispatcher for testing
    class WorkerPoolDispatcher : public ThreadPool::IDispatcher {
    public:
        WorkerPoolDispatcher() = default;
        ~WorkerPoolDispatcher() override = default;

        void Initialize() override {}
        void Deinitialize() override {}
        void Dispatch(IDispatch* job) override
        {
            job->Dispatch();
        }
    };

    WorkerPoolDispatcher* _dispatcher;
    WorkerPool* _workerPool;
};

/**
 * Test Case 1: Basic self-resubmission
 * Verify that a job can call Submit() on itself from within Dispatch()
 */
TEST_F(WorkerPoolSelfResubmitTest, BasicSelfResubmit)
{
    class SelfResubmittingJob {
    private:
        Core::WorkerPool::JobType<SelfResubmittingJob&> _job;
        std::atomic<uint32_t> _executionCount;
        std::atomic<uint32_t> _maxExecutions;
        Core::Event _completed;

    public:
        SelfResubmittingJob(uint32_t maxExecutions)
            : _job(*this)
            , _executionCount(0)
            , _maxExecutions(maxExecutions)
            , _completed(false, true)
        {
        }

        ~SelfResubmittingJob()
        {
            _job.Revoke();
        }

        void Start()
        {
            _job.Submit();
        }

        bool WaitForCompletion(uint32_t timeoutMs)
        {
            return (_completed.Lock(timeoutMs) == ERROR_NONE);
        }

        uint32_t GetExecutionCount() const
        {
            return _executionCount.load();
        }

        void Dispatch()
        {
            uint32_t currentCount = ++_executionCount;

            // Self-resubmit if we haven't reached max executions
            if (currentCount < _maxExecutions) {
                ASSERT_TRUE(_job.Submit());
            } else {
                _completed.SetEvent();
            }
        }
    };

    // Create a job that will resubmit itself 5 times
    SelfResubmittingJob job(5);
    job.Start();

    // Wait for completion with timeout
    ASSERT_TRUE(job.WaitForCompletion(Core::infinite)) << "Job did not complete within timeout";
    EXPECT_EQ(job.GetExecutionCount(), 5u) << "Job should have executed exactly 5 times";
}

/**
 * Test Case 2: Rapid self-resubmission stress test
 * Verify stability under rapid resubmissions
 */
TEST_F(WorkerPoolSelfResubmitTest, RapidSelfResubmitStressTest)
{
    class RapidResubmittingJob {
    private:
        Core::WorkerPool::JobType<RapidResubmittingJob&> _job;
        std::atomic<uint32_t> _executionCount;
        std::atomic<uint32_t> _maxExecutions;
        Core::Event _completed;

    public:
        RapidResubmittingJob(uint32_t maxExecutions)
            : _job(*this)
            , _executionCount(0)
            , _maxExecutions(maxExecutions)
            , _completed(false, true)
        {
        }

        ~RapidResubmittingJob()
        {
            _job.Revoke();
        }

        void Start()
        {
            _job.Submit();
        }

        bool WaitForCompletion(uint32_t timeoutMs)
        {
            return (_completed.Lock(timeoutMs) == ERROR_NONE);
        }

        uint32_t GetExecutionCount() const
        {
            return _executionCount.load();
        }

        void Dispatch()
        {
            uint32_t currentCount = ++_executionCount;

            if (currentCount < _maxExecutions) {
                // Immediately resubmit without any delay
                _job.Submit();
            } else {
                _completed.SetEvent();
            }
        }
    };

    // Stress test with 100 rapid resubmissions
    RapidResubmittingJob job(100);
    job.Start();

    ASSERT_TRUE(job.WaitForCompletion(10000)) << "Job did not complete within timeout";
    EXPECT_EQ(job.GetExecutionCount(), 100u) << "Job should have executed exactly 100 times";
}

/**
 * Test Case 3: Multiple concurrent self-resubmitting jobs
 * Verify that multiple jobs can self-resubmit without interfering with each other
 */
TEST_F(WorkerPoolSelfResubmitTest, MultipleConcurrentSelfResubmittingJobs)
{
    class ConcurrentResubmittingJob {
    private:
        Core::WorkerPool::JobType<ConcurrentResubmittingJob&> _job;
        std::atomic<uint32_t> _executionCount;
        std::atomic<uint32_t> _maxExecutions;
        Core::Event _completed;
        uint32_t _jobId;

    public:
        ConcurrentResubmittingJob(uint32_t jobId, uint32_t maxExecutions)
            : _job(*this)
            , _executionCount(0)
            , _maxExecutions(maxExecutions)
            , _completed(false, true)
            , _jobId(jobId)
        {
        }

        ~ConcurrentResubmittingJob()
        {
            _job.Revoke();
        }

        void Start()
        {
            _job.Submit();
        }

        bool WaitForCompletion(uint32_t timeoutMs)
        {
            return (_completed.Lock(timeoutMs) == ERROR_NONE);
        }

        uint32_t GetExecutionCount() const
        {
            return _executionCount.load();
        }

        void Dispatch()
        {
            uint32_t currentCount = ++_executionCount;

            if (currentCount < _maxExecutions) {
                _job.Submit();
            } else {
                _completed.SetEvent();
            }
        }
    };

    // Create multiple jobs that will run concurrently
    constexpr uint32_t NUM_JOBS = 5;
    constexpr uint32_t EXECUTIONS_PER_JOB = 20;

    std::vector<std::unique_ptr<ConcurrentResubmittingJob>> jobs;

    for (uint32_t i = 0; i < NUM_JOBS; ++i) {
        jobs.push_back(std::make_unique<ConcurrentResubmittingJob>(i, EXECUTIONS_PER_JOB));
    }

    // Start all jobs
    for (auto& job : jobs) {
        job->Start();
    }

    // Wait for all jobs to complete
    for (auto& job : jobs) {
        ASSERT_TRUE(job->WaitForCompletion(10000)) << "Job did not complete within timeout";
        EXPECT_EQ(job->GetExecutionCount(), EXECUTIONS_PER_JOB)
            << "Job should have executed exactly " << EXECUTIONS_PER_JOB << " times";
    }
}

/**
 * Test Case 4: Self-resubmission with conditional logic
 * Verify that jobs can make decisions about resubmission
 */
TEST_F(WorkerPoolSelfResubmitTest, ConditionalSelfResubmit)
{
    class ConditionalResubmittingJob {
    private:
        Core::WorkerPool::JobType<ConditionalResubmittingJob&> _job;
        std::atomic<uint32_t> _executionCount;
        std::atomic<uint32_t> _successCount;
        Core::Event _completed;

    public:
        ConditionalResubmittingJob()
            : _job(*this)
            , _executionCount(0)
            , _successCount(0)
            , _completed(false, true)
        {
        }

        ~ConditionalResubmittingJob()
        {
            _job.Revoke();
        }

        void Start()
        {
            _job.Submit();
        }

        bool WaitForCompletion(uint32_t timeoutMs)
        {
            return (_completed.Lock(timeoutMs) == ERROR_NONE);
        }

        uint32_t GetExecutionCount() const
        {
            return _executionCount.load();
        }

        uint32_t GetSuccessCount() const
        {
            return _successCount.load();
        }

        void Dispatch()
        {
            uint32_t currentCount = ++_executionCount;

            // Simulate some work that might succeed or fail
            bool success = (currentCount % 3) != 0; // Fails every 3rd execution

            if (success) {
                _successCount++;
            }

            // Keep retrying until we have 5 successes
            if (_successCount < 5) {
                _job.Submit();
            } else {
                _completed.SetEvent();
            }
        }
    };

    ConditionalResubmittingJob job;
    job.Start();

    ASSERT_TRUE(job.WaitForCompletion(5000)) << "Job did not complete within timeout";
    EXPECT_EQ(job.GetSuccessCount(), 5u) << "Job should have achieved 5 successes";
    EXPECT_GE(job.GetExecutionCount(), 5u) << "Job should have executed at least 5 times";
}

/**
 * Test Case 5: Verify no deadlock when pool is under heavy load
 * This is critical - ensure self-resubmission doesn't cause deadlocks
 */
TEST_F(WorkerPoolSelfResubmitTest, NoDeadlockUnderLoad)
{
    class LoadTestJob {
    private:
        Core::WorkerPool::JobType<LoadTestJob&> _job;
        std::atomic<uint32_t> _executionCount;
        std::atomic<uint32_t> _maxExecutions;
        Core::Event _completed;

    public:
        LoadTestJob(uint32_t maxExecutions)
            : _job(*this)
            , _executionCount(0)
            , _maxExecutions(maxExecutions)
            , _completed(false, true)
        {
        }

        ~LoadTestJob()
        {
            _job.Revoke();
        }

        void Start()
        {
            _job.Submit();
        }

        bool WaitForCompletion(uint32_t timeoutMs)
        {
            return (_completed.Lock(timeoutMs) == ERROR_NONE);
        }

        uint32_t GetExecutionCount() const
        {
            return _executionCount.load();
        }

        void Dispatch()
        {
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            uint32_t currentCount = ++_executionCount;

            if (currentCount < _maxExecutions) {
                _job.Submit();
            } else {
                _completed.SetEvent();
            }
        }
    };

    // Create many jobs that will run concurrently and self-resubmit
    // This stresses the pool and tests for deadlock conditions
    constexpr uint32_t NUM_JOBS = 10;
    constexpr uint32_t EXECUTIONS_PER_JOB = 10;

    std::vector<std::unique_ptr<LoadTestJob>> jobs;

    for (uint32_t i = 0; i < NUM_JOBS; ++i) {
        jobs.push_back(std::make_unique<LoadTestJob>(EXECUTIONS_PER_JOB));
    }

    // Start all jobs simultaneously
    for (auto& job : jobs) {
        job->Start();
    }

    // All jobs should complete without deadlock
    for (auto& job : jobs) {
        ASSERT_TRUE(job->WaitForCompletion(30000))
            << "Job did not complete - possible deadlock detected";
        EXPECT_EQ(job->GetExecutionCount(), EXECUTIONS_PER_JOB);
    }
}

/**
 * Test Case 6: Self-resubmission after revoke should not execute
 * Verify that revoked jobs don't continue to resubmit
 */
TEST_F(WorkerPoolSelfResubmitTest, RevokeStopsSelfResubmission)
{
    class RevocableResubmittingJob {
    private:
        Core::WorkerPool::JobType<RevocableResubmittingJob&> _job;
        std::atomic<uint32_t> _executionCount;
        std::atomic<bool> _shouldStop;
        Core::Event _started;

    public:
        RevocableResubmittingJob()
            : _job(*this)
            , _executionCount(0)
            , _shouldStop(false)
            , _started(false, true)
        {
        }

        ~RevocableResubmittingJob()
        {
            _shouldStop = true;
            _job.Revoke();
        }

        void Start()
        {
            _job.Submit();
        }

        void Stop()
        {
            _shouldStop = true;
            _job.Revoke();
        }

        bool WaitForStart(uint32_t timeoutMs)
        {
            return (_started.Lock(timeoutMs) == ERROR_NONE);
        }

        uint32_t GetExecutionCount() const
        {
            return _executionCount.load();
        }

        void Dispatch()
        {
            _executionCount++;
            _started.SetEvent();

            // Keep resubmitting unless stopped
            if (!_shouldStop) {
                // Small delay to allow revoke to happen
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (!_shouldStop) {
                    _job.Submit();
                }
            }
        }
    };

    RevocableResubmittingJob job;
    job.Start();

    // Wait for job to start
    ASSERT_TRUE(job.WaitForStart(1000)) << "Job did not start";

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the job
    uint32_t countBeforeStop = job.GetExecutionCount();
    job.Stop();

    // Wait a bit and verify it stopped resubmitting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint32_t countAfterStop = job.GetExecutionCount();

    // The count should not increase significantly after stop
    EXPECT_LE(countAfterStop - countBeforeStop, 2u)
        << "Job continued to execute after revoke";
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
