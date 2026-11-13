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
#include <gtest/gtest.h>

#include "DeathTestStyle.h"

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/WatchDog.h>

#ifdef __POSIX__
#include <csignal>
#include <unistd.h>
#endif

namespace Thunder {
namespace Tests {
namespace Core {

    class TestWatchable : public ::Thunder::Core::IWatchable {
    public:
        TestWatchable(const uint32_t timeout, const bool expired)
            : _timeout(timeout)
            , _expired(expired)
            , _evaluations(0)
        {
        }

        const TCHAR* Name() const override
        {
            return (_T("TestWatchable"));
        }
        uint32_t Timeout() const override
        {
            return (_timeout);
        }
        bool Expired(const uint64_t VARIABLE_IS_NOT_USED now) override
        {
            _evaluations++;
            return (_expired.load());
        }
        uint32_t Evaluations() const
        {
            return (_evaluations.load());
        }

    private:
        const uint32_t _timeout;
        std::atomic<bool> _expired;
        std::atomic<uint32_t> _evaluations;
    };

    TEST(Core_WatchDog, EvaluatesAllRegisteredWatchablesOnOneThread)
    {
        ::Thunder::Core::WatchDog& watchdog = ::Thunder::Core::WatchDog::Instance();
        TestWatchable first(20, false);
        TestWatchable second(40, false);

        watchdog.Register(first, false);
        watchdog.Register(second, false);
        ::SleepMs(200);
        watchdog.Unregister(first);
        watchdog.Unregister(second);

        EXPECT_GT(first.Evaluations(), 0u);
        EXPECT_GT(second.Evaluations(), 0u);
    }

    TEST(Core_WatchDog, StopsEvaluatingAfterUnregister)
    {
        ::Thunder::Core::WatchDog& watchdog = ::Thunder::Core::WatchDog::Instance();
        TestWatchable watchable(20, false);

        watchdog.Register(watchable, false);
        ::SleepMs(100);
        watchdog.Unregister(watchable);

        const uint32_t evaluations = watchable.Evaluations();
        ::SleepMs(100);

        EXPECT_GT(evaluations, 0u);
        EXPECT_EQ(watchable.Evaluations(), evaluations);
    }

    TEST(Core_WatchDog, MonotonicTicksNeverGoBackwards)
    {
        const uint64_t before = ::Thunder::Core::WatchDog::Ticks();
        const uint64_t after = ::Thunder::Core::WatchDog::Ticks();

        EXPECT_GE(after, before);
    }

#ifdef __POSIX__
    // Runs in a death test child, a fresh process, as the WatchDog thread does not survive a fork.
    void ExpiredWatchableScenario(const bool abortOnExpiry)
    {
        ::Thunder::Core::WatchDog& watchdog = ::Thunder::Core::WatchDog::Instance();
        TestWatchable healthy(20, false);
        TestWatchable stalled(20, true);

        watchdog.Register(healthy, abortOnExpiry);
        watchdog.Register(stalled, abortOnExpiry);
        ::SleepMs(500);
        ::_exit(0);
    }

    TEST(Core_WatchDog, AbortsWhenAWatchableExpiresAndAbortIsConfigured)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(ExpiredWatchableScenario(true), ::testing::KilledBySignal(SIGABRT), "");
    }

    TEST(Core_WatchDog, OnlyReportsWhenAWatchableExpiresAndAbortIsNotConfigured)
    {
        THREADSAFE_DEATH_TESTS();
        EXPECT_EXIT(ExpiredWatchableScenario(false), ::testing::ExitedWithCode(0), "");
    }
#endif
}
}
}
