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

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#ifdef __POSIX__
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Thunder {
namespace Tests {
namespace Core {

    using TestResourceMonitor = ::Thunder::Core::ResourceMonitorType<
        ::Thunder::Core::IResource,
        ::Thunder::Core::ResourceMonitorWatchDog,
        0,
        32>;

    TEST(Core_ResourceMonitor, WatchDogIsDisabledUntilEnabled)
    {
        TestResourceMonitor monitor;

        EXPECT_FALSE(monitor.WatchDogEnabled());
        monitor.EnableWatchDog(1000, false);
        EXPECT_TRUE(monitor.WatchDogEnabled());
    }

    TEST(Core_ResourceMonitor, ExpiryDoesNotAbortWhenConfiguredOff)
    {
        ::Thunder::Core::ResourceMonitorHandler handler(1, false);

        EXPECT_EQ(handler.Expired(), ::Thunder::Core::infinite);
    }

#ifdef __POSIX__
    class TestDispatch : public ::Thunder::Core::IDispatch {
    public:
        void Dispatch() override
        {
        }
    };

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

    class QueueSubmissionResource : public ::Thunder::Core::IResource {
    public:
        QueueSubmissionResource(
            const handle descriptor,
            ::Thunder::Core::ThreadPool& pool,
            const ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>& job,
            ::Thunder::Core::Event& submitted)
            : _descriptor(descriptor)
            , _pool(pool)
            , _job(job)
            , _submitted(submitted)
        {
        }

        handle Descriptor() const override
        {
            return (_descriptor);
        }

        uint16_t Events() override
        {
            return (POLLIN);
        }

        void Handle(const uint16_t VARIABLE_IS_NOT_USED events) override
        {
            char event;
            ::read(_descriptor, &event, sizeof(event));
            _pool.Submit(_job, 0);
            _submitted.SetEvent();
        }

    private:
        const handle _descriptor;
        ::Thunder::Core::ThreadPool& _pool;
        const ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> _job;
        ::Thunder::Core::Event& _submitted;
    };

    uint32_t PendingAfterResourceMonitorSubmit(const bool enableWatchDog)
    {
        int descriptors[2];
        if (::pipe(descriptors) != 0) {
            return (::Thunder::Core::infinite);
        }

        ::Thunder::Core::SingletonType<::Thunder::Core::ResourceMonitor>::Dispose();
        ::Thunder::Core::ResourceMonitor& monitor = ::Thunder::Core::ResourceMonitor::Instance();
        if (enableWatchDog == true) {
            monitor.EnableWatchDog(5000, false);
        }

        TestDispatcher dispatcher;
        ::Thunder::Core::ThreadPool pool(0, 0, 1, &dispatcher, nullptr, nullptr, nullptr, 1, 1, 2);
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> queuedJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestDispatch>::Create());
        ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch> submittedJob =
            ::Thunder::Core::ProxyType<::Thunder::Core::IDispatch>(::Thunder::Core::ProxyType<TestDispatch>::Create());
        pool.Submit(queuedJob, 0);

        ::Thunder::Core::Event submitted(false, true);
        uint32_t pending = ::Thunder::Core::infinite;
        {
            QueueSubmissionResource resource(descriptors[0], pool, submittedJob, submitted);
            monitor.Register(resource);

            const char event = 'x';
            ::write(descriptors[1], &event, sizeof(event));
            if (submitted.Lock(1000) == ::Thunder::Core::ERROR_NONE) {
                monitor.Unregister(resource);
                pending = pool.Pending();
            } else {
                monitor.Unregister(resource);
            }
        }

        ::Thunder::Core::SingletonType<::Thunder::Core::ResourceMonitor>::Dispose();
        ::close(descriptors[0]);
        ::close(descriptors[1]);
        return (pending);
    }

    TEST(Core_ResourceMonitor, QueueGrowthBypassesLimitWhenWatchDogDisabled)
    {
        EXPECT_EQ(PendingAfterResourceMonitorSubmit(false), 2u);
    }

    TEST(Core_ResourceMonitor, QueueLimitAppliesWhenWatchDogEnabled)
    {
        EXPECT_EQ(PendingAfterResourceMonitorSubmit(true), 1u);
    }

    class BlockingResource : public ::Thunder::Core::IResource {
    public:
        BlockingResource(const handle descriptor, const bool blockInEvents)
            : _descriptor(descriptor)
            , _blockInEvents(blockInEvents)
        {
        }

        handle Descriptor() const override
        {
            return (_descriptor);
        }

        uint16_t Events() override
        {
            if (_blockInEvents == true) {
                ::SleepMs(500);
            }
            return (POLLIN);
        }

        void Handle(const uint16_t VARIABLE_IS_NOT_USED events) override
        {
            if (_blockInEvents == false) {
                ::SleepMs(500);
            }
        }

    private:
        const handle _descriptor;
        const bool _blockInEvents;
    };

    void ExpectResourceMonitorWatchDogAbort(const bool blockInEvents)
    {
        int descriptors[2];
        ASSERT_EQ(::pipe(descriptors), 0);

        const pid_t pid = ::fork();
        ASSERT_GE(pid, 0);

        if (pid == 0) {
            TestResourceMonitor monitor;
            monitor.EnableWatchDog(50, true);

            BlockingResource resource(descriptors[0], blockInEvents);
            monitor.Register(resource);

            const char event = 'x';
            ::write(descriptors[1], &event, sizeof(event));
            ::SleepMs(1000);
            ::_exit(0);
        }

        ::close(descriptors[0]);
        ::close(descriptors[1]);

        int status = 0;
        ASSERT_EQ(::waitpid(pid, &status, 0), pid);
        ASSERT_TRUE(WIFSIGNALED(status));
        EXPECT_EQ(WTERMSIG(status), SIGABRT);
    }

    TEST(Core_ResourceMonitor, ExpiryAbortsTheProcessWhenHandleBlocks)
    {
        ExpectResourceMonitorWatchDogAbort(false);
    }

    TEST(Core_ResourceMonitor, ExpiryAbortsTheProcessWhenEventsBlocks)
    {
        ExpectResourceMonitorWatchDogAbort(true);
    }
#endif
}
}
}
