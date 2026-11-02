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

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/WorkerPool.h>

#ifdef __POSIX__
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

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

    int WorkerPoolWatchDogScenario(const bool releaseStalledJob)
    {
        const pid_t pid = ::fork();
        if (pid < 0) {
            return (-1);
        }
        if (pid == 0) {
            TestDispatcher dispatcher;
            ::Thunder::Core::WorkerPool pool(1, 0, 2, &dispatcher, nullptr, 1, 1);
            ::Thunder::Core::WorkerPoolWatchDog watchdog(pool, 150, true);
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

            watchdog.Start();
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

        int status = 0;
        if (::waitpid(pid, &status, 0) != pid) {
            return (-1);
        }
        return (status);
    }

    TEST(Core_WorkerPoolWatchDog, AbortsWhenQueuedWorkMakesNoProgress)
    {
        const int status = WorkerPoolWatchDogScenario(false);
        ASSERT_NE(status, -1);
        ASSERT_TRUE(WIFSIGNALED(status));
        EXPECT_EQ(WTERMSIG(status), SIGABRT);
    }

    TEST(Core_WorkerPoolWatchDog, AllowsProgressAfterATemporaryStall)
    {
        const int status = WorkerPoolWatchDogScenario(true);
        ASSERT_NE(status, -1);
        ASSERT_TRUE(WIFEXITED(status));
        EXPECT_EQ(WEXITSTATUS(status), 0);
    }

    TEST(Core_WorkerPoolWatchDog, StopsCleanlyWhenPoolIsIdle)
    {
        TestDispatcher dispatcher;
        ::Thunder::Core::WorkerPool pool(1, 0, 2, &dispatcher, nullptr, 1, 1, 0);
        ::Thunder::Core::WorkerPoolWatchDog watchdog(pool, 100, false);
        pool.Run();

        watchdog.Start();
        ::SleepMs(20);

        pool.Stop();
        EXPECT_TRUE(pool.WaitForStop(1000));
    }

#endif
}
}
}
