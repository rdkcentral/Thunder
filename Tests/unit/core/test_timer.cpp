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
#include <condition_variable>
#include <mutex>

namespace Thunder {
namespace Tests {

    class TimeHandler {
    public:
        TimeHandler()
        {
        }

        ~TimeHandler()
        {
        }

    public:
        uint64_t Timed(const uint64_t scheduledTime)
        {
            constexpr uint32_t time = 100; // 0.1 second

            std::unique_lock<std::mutex> lock(_mutex);

            _timerDone++;

            lock.unlock();

            _cv.notify_one();

            Core::Time nextTick = Core::Time::Now() + time;

            return nextTick.Ticks();
        }

        static int GetCount()
        {
            std::unique_lock<std::mutex> lk(_mutex);

            _cv.wait(lk);

            return _timerDone;
        }

    private:
        static int _timerDone;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    int TimeHandler::_timerDone = 0;
    std::mutex TimeHandler::_mutex;
    std::condition_variable TimeHandler::_cv;

    class WatchDogHandler : Core::WatchDogType<WatchDogHandler&> {
    private:
        typedef Core::WatchDogType<WatchDogHandler&> BaseClass;

    public:
        WatchDogHandler& operator=(const WatchDogHandler&) = delete;
        WatchDogHandler()
            : BaseClass(Core::Thread::DefaultStackSize(), _T("WatchDogTimer"), *this)
            , _event(false, true)
        {
        }
        ~WatchDogHandler()
        {
        }

        void Start(uint32_t delay)
        {
            BaseClass::Arm(delay);
        }

        uint32_t Expired()
        {
            _event.SetEvent();
            return Core::infinite;
        }

        int Wait(unsigned int milliseconds) const
        {
            return _event.Lock(milliseconds);
        }

    private:
        uint32_t _delay;
        mutable Core::Event _event;
    };

    TEST(Core_Timer, LoopedTimer)
    {
        constexpr uint32_t time = 100;

        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T("LoopedTimer"));

        Core::Time nextTick = Core::Time::Now() + time;

        timer.Schedule(nextTick.Ticks(), TimeHandler());

        while (TimeHandler::GetCount() <= 2) {
        }

        timer.Flush();
    }

    TEST(Core_Timer, QueuedTimer)
    {
        constexpr uint32_t time = 100;

        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T("QueuedTimer"));

        Core::Time nextTick = Core::Time::Now();

        nextTick.Add(time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());

        nextTick.Add(2 * time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());

        nextTick.Add(3 * time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());

        while (TimeHandler::GetCount() <= 5) {
        }

        timer.Flush();
    }

    TEST(Core_Timer, PastTime)
    {
        constexpr uint32_t time = 100;

        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T("PastTime"));

        Core::Time pastTime = Core::Time::Now();

        ASSERT_GT(pastTime.Ticks() / 1000, 0);

        pastTime.Sub(time);

        timer.Schedule(pastTime.Ticks(), TimeHandler());

        while (TimeHandler::GetCount() <= 6) {
        }

        timer.Flush();
    }

    TEST(Core_Timer, WatchDogType)
    {
        WatchDogHandler timer;
        timer.Start(100); // 100 milliseconds delay
        int ret = timer.Wait(200); // Wait for 200 milliseconds
        EXPECT_EQ(ret, Core::ERROR_NONE);
    }
} // Tests
} // Thunder
