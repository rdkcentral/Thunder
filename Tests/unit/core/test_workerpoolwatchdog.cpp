/*
 * If not stated otherwise in this file or this component's license file the
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

#include "DeathTestStyle.h"

#include "../Module.h"
#include <core/WorkerPool.h>

#ifdef __POSIX__
#include <csignal>
#include <thread>
#include <unistd.h>
#endif

#ifdef THUNDER_CORE_WORKERPOOL_WATCHDOG

namespace WPEFramework {
namespace Tests {
namespace Core {

#ifdef __POSIX__
    class TestDispatcher : public ::WPEFramework::Core::ThreadPool::IDispatcher {
    private:
        void Initialize() override
        {
        }
        void Deinitialize() override
        {
        }
        void Dispatch(::WPEFramework::Core::IDispatch* job) override
        {
            job->Dispatch();
        }
    };

    class BlockingJob : public ::WPEFramework::Core::IDispatch {
    public:
        BlockingJob(::WPEFramework::Core::Event& started, ::WPEFramework::Core::Event& release, ::WPEFramework::Core::Event& completed)
            : _started(started)
            , _release(release)
            , _completed(completed)
        {
        }
        void Dispatch() override
        {
            _started.SetEvent();
            _release.Lock(::WPEFramework::Core::infinite);
            _completed.SetEvent();
        }

    private:
        ::WPEFramework::Core::Event& _started;
        ::WPEFramework::Core::Event& _release;
        ::WPEFramework::Core::Event& _completed;
    };

    class CompletingJob : public ::WPEFramework::Core::IDispatch {
    public:
        explicit CompletingJob(::WPEFramework::Core::Event& completed)
            : _completed(completed)
        {
        }
        void Dispatch() override
        {
            _completed.SetEvent();
        }

    private:
        ::WPEFramework::Core::Event& _completed;
    };

    void WorkerPoolWatchDogScenario(const bool releaseStalledJob)
    {
        TestDispatcher dispatcher;
        ::WPEFramework::Core::WorkerPool pool(1, 0, 2, &dispatcher);
        ::WPEFramework::Core::WorkerPoolWatchable watchable(pool, 150);
        pool.Run();

        ::WPEFramework::Core::Event started(false, true);
        ::WPEFramework::Core::Event release(false, true);
        ::WPEFramework::Core::Event stalledJobCompleted(false, true);
        ::WPEFramework::Core::Event queuedJobCompleted(false, true);

        ::WPEFramework::Core::ProxyType<::WPEFramework::Core::IDispatch> stalledJob =
            ::WPEFramework::Core::ProxyType<::WPEFramework::Core::IDispatch>(
                ::WPEFramework::Core::ProxyType<BlockingJob>::Create(started, release, stalledJobCompleted));
        ::WPEFramework::Core::ProxyType<::WPEFramework::Core::IDispatch> queuedJob =
            ::WPEFramework::Core::ProxyType<::WPEFramework::Core::IDispatch>(
                ::WPEFramework::Core::ProxyType<CompletingJob>::Create(queuedJobCompleted));

        pool.Submit(stalledJob);
        if (started.Lock(1000) != ::WPEFramework::Core::ERROR_NONE) {
            ::_exit(2);
        }

        ::WPEFramework::Core::WatchDog::Instance().Register(watchable, true);
        std::thread submitter([&pool, &queuedJob]() {
            pool.Submit(queuedJob);
        });

        if (releaseStalledJob == true) {
            ::SleepMs(20);
            release.SetEvent();
            submitter.join();
            if ((stalledJobCompleted.Lock(1000) != ::WPEFramework::Core::ERROR_NONE)
                || (queuedJobCompleted.Lock(1000) != ::WPEFramework::Core::ERROR_NONE)) {
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

    TEST(Core_WorkerPoolWatchDog, AllowsProgressAfterTemporaryStall)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(WorkerPoolWatchDogScenario(true), ::testing::ExitedWithCode(0), "");
    }

    TEST(Core_WorkerPoolWatchDog, DoesNotExpireAfterPoolStops)
    {
        TestDispatcher dispatcher;
        ::WPEFramework::Core::WorkerPool pool(1, 0, 2, &dispatcher);
        ::WPEFramework::Core::WorkerPoolWatchable watchable(pool, 150);
        pool.Run();
        pool.Stop();

        const uint64_t afterTimeout = ::WPEFramework::Core::WatchDog::Ticks()
            + (2 * 150 * ::WPEFramework::Core::Time::MicroSecondsPerMilliSecond);
        EXPECT_FALSE(watchable.Expired(afterTimeout));
    }
#endif
}
}
}

#endif