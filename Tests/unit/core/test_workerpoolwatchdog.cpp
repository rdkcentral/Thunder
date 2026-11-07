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

#include <thread>
#include <gtest/gtest.h>

#include "DeathTestStyle.h"

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/WorkerPool.h>

#ifdef __POSIX__
#include <csignal>
#include <unistd.h>
#endif

#ifdef THUNDER_CORE_WORKERPOOL_WATCHDOG

namespace Thunder {
namespace Tests {
namespace Core {

#ifdef __POSIX__
    class TestDispatcher : public ::Thunder::Core::ThreadPool::IDispatcher {
    private:
        void Initialize() override
        {
        }
        void Deinitialize() override
        {
        }
        void Dispatch(::Thunder::Core::IDispatch* job) override
        {
            job->Dispatch();
        }
    };

    class BlockingJob : public ::Thunder::Core::IDispatch {
    public:
        BlockingJob(
            ::Thunder::Core::Event& started,
            ::Thunder::Core::Event& release,
            ::Thunder::Core::Event& completed)
            : _started(started)
            , _release(release)
            , _completed(completed)
        {
        }

        void Dispatch() override
        {
            _started.SetEvent();
            _release.Lock(::Thunder::Core::infinite);
            _completed.SetEvent();
        }

    private:
        ::Thunder::Core::Event& _started;
        ::Thunder::Core::Event& _release;
        ::Thunder::Core::Event& _completed;
    };

    class CompletingJob : public ::Thunder::Core::IDispatch {
    public:
        explicit CompletingJob(::Thunder::Core::Event& completed)
            : _completed(completed)
        {
        }

        void Dispatch() override
        {
            _completed.SetEvent();
        }

    private:
        ::Thunder::Core::Event& _completed;
    };

    // Runs in a death test child, a fresh process, as the WatchDog thread does not survive a fork.
    void WorkerPoolWatchDogScenario(const bool releaseStalledJob)
    {
        TestDispatcher dispatcher;
        ::Thunder::Core::WorkerPool pool(1, 0, 2, &dispatcher, nullptr, 1, 1);
        ::Thunder::Core::WorkerPoolWatchable watchable(pool, 150);
        pool.Run();

        ::Thunder::Core::Event started(false, true);
        ::Thunder::Core::Event release(false, true);
        ::Thunder::Core::Event stalledJobCompleted(false, true);
        ::Thunder::Core::Event queuedJobCompleted(false, true);

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> stalledJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<BlockingJob>::Create(started, release, stalledJobCompleted));
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> queuedJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<CompletingJob>::Create(queuedJobCompleted));

        pool.Submit(stalledJob);
        if (started.Lock(1000) != ::Thunder::Core::ERROR_NONE) {
            ::_exit(2);
        }

        ::Thunder::Core::WatchDog::Instance().Register(watchable, true);
        std::thread submitter([&pool, &queuedJob]() {
            pool.Submit(queuedJob);
        });

        if (releaseStalledJob == true) {
            ::SleepMs(20);
            release.SetEvent();
            submitter.join();
            if ((stalledJobCompleted.Lock(1000) != ::Thunder::Core::ERROR_NONE)
                || (queuedJobCompleted.Lock(1000) != ::Thunder::Core::ERROR_NONE)) {
                ::_exit(3);
            }
            ::SleepMs(300);
        } else {
            ::SleepMs(1000);
        }
        ::_exit(0);
    }

    TEST(Core_WorkerPoolWatchDog, AbortsWhenQueuedWorkMakesNoProgress)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(WorkerPoolWatchDogScenario(false), ::testing::KilledBySignal(SIGABRT), "");
    }

    TEST(Core_WorkerPoolWatchDog, AllowsProgressAfterATemporaryStall)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(WorkerPoolWatchDogScenario(true), ::testing::ExitedWithCode(0), "");
    }

#if defined(__JOB_QUEUE_STATIC_PRIORITY__) || defined(__JOB_QUEUE_DYNAMIC_PRIORITY__)
    // Two workers but a low priority quota of one: the second low priority job waits for the quota while one worker is idle.
    // Runs in a death test child, a fresh process, as the WatchDog thread does not survive a fork.
    void WorkerPoolQuotaScenario(const bool releaseStalledJob)
    {
        TestDispatcher dispatcher;
        ::Thunder::Core::WorkerPool pool(2, 0, 8, &dispatcher, nullptr, 1, 1);
        ::Thunder::Core::WorkerPoolWatchable watchable(pool, 150);
        pool.Run();

        ::Thunder::Core::Event started(false, true);
        ::Thunder::Core::Event release(false, true);
        ::Thunder::Core::Event stalledJobCompleted(false, true);
        ::Thunder::Core::Event queuedJobCompleted(false, true);

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> stalledJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<BlockingJob>::Create(started, release, stalledJobCompleted));
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> queuedJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<CompletingJob>::Create(queuedJobCompleted));

        pool.Submit(stalledJob, ::Thunder::Core::ThreadPool::Priority::Low);
        if (started.Lock(1000) != ::Thunder::Core::ERROR_NONE) {
            ::_exit(2);
        }

        ::Thunder::Core::WatchDog::Instance().Register(watchable, true);
        std::thread submitter([&pool, &queuedJob]() {
            pool.Submit(queuedJob, ::Thunder::Core::ThreadPool::Priority::Low);
        });

        if (releaseStalledJob == true) {
            ::SleepMs(20);
            release.SetEvent();
            submitter.join();
            if ((stalledJobCompleted.Lock(1000) != ::Thunder::Core::ERROR_NONE)
                || (queuedJobCompleted.Lock(1000) != ::Thunder::Core::ERROR_NONE)) {
                ::_exit(3);
            }
            ::SleepMs(300);
        } else {
            ::SleepMs(1000);
        }
        ::_exit(0);
    }

    TEST(Core_WorkerPoolWatchDog, AbortsWhenWorkIsPendingForACategoryQuotaWhileAWorkerIsIdle)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(WorkerPoolQuotaScenario(false), ::testing::KilledBySignal(SIGABRT), "");
    }

    TEST(Core_WorkerPoolWatchDog, AllowsCategoryQuotaWorkToProceedAfterATemporaryStall)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(WorkerPoolQuotaScenario(true), ::testing::ExitedWithCode(0), "");
    }
#endif

    // Work that is left over in a pool that has been stopped, like during the shutdown of the daemon, is not a stall.
    // Runs in a death test child, a fresh process, as the WatchDog thread does not survive a fork.
    void StoppedPoolWithPendingWorkScenario()
    {
        TestDispatcher dispatcher;
        ::Thunder::Core::WorkerPool pool(1, 0, 2, &dispatcher, nullptr, 1, 1);
        ::Thunder::Core::WorkerPoolWatchable watchable(pool, 150);
        pool.Run();

        ::Thunder::Core::Event started(false, true);
        ::Thunder::Core::Event release(false, true);
        ::Thunder::Core::Event stalledJobCompleted(false, true);
        ::Thunder::Core::Event queuedJobCompleted(false, true);

        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> stalledJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<BlockingJob>::Create(started, release, stalledJobCompleted));
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> queuedJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(
                ::Thunder::Core::ProxyType<CompletingJob>::Create(queuedJobCompleted));

        pool.Submit(stalledJob);
        if (started.Lock(1000) != ::Thunder::Core::ERROR_NONE) {
            ::_exit(2);
        }

        // The job waits for the category quota, the submitter is blocked till it is promoted.
        std::thread submitter([&pool, &queuedJob]() {
            pool.Submit(queuedJob);
        });
        ::SleepMs(50);

        ::Thunder::Core::WatchDog::Instance().Register(watchable, true);

        pool.Stop();
        ::SleepMs(500);

        release.SetEvent();
        submitter.join();
        ::Thunder::Core::WatchDog::Instance().Unregister(watchable);
        ::_exit(pool.WaitForStop(1000) == true ? 0 : 3);
    }

    TEST(Core_WorkerPoolWatchDog, DoesNotExpireForPendingWorkOfAStoppedPool)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(StoppedPoolWithPendingWorkScenario(), ::testing::ExitedWithCode(0), "");
    }

    TEST(Core_WorkerPoolWatchDog, StopsCleanlyWhenPoolIsIdle)
    {
        TestDispatcher dispatcher;
        ::Thunder::Core::WorkerPool pool(1, 0, 2, &dispatcher, nullptr, 1, 1, 0);
        ::Thunder::Core::WorkerPoolWatchable watchable(pool, 100);
        pool.Run();

        ::Thunder::Core::WatchDog::Instance().Register(watchable, false);
        ::SleepMs(20);
        ::Thunder::Core::WatchDog::Instance().Unregister(watchable);

        pool.Stop();
        EXPECT_TRUE(pool.WaitForStop(1000));
    }

#endif
}
}
}

#endif
