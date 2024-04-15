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

class ThreadClass : public Core::Thread {
public:
    ThreadClass() = delete;
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

    ThreadClass(Event& event, std::thread::id parentTid, bool& threadDone, volatile bool& lock, volatile bool& setEvent)
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
        , _threadDone(threadDone)
        , _lock(lock)
        , _setEvent(setEvent)
        , _event(event)
        , _parentTid(parentTid)
    {
    }

    virtual ~ThreadClass()
    {
    }

    virtual uint32_t Worker() override
    {
        while (IsRunning() && (!_threadDone)) {
            EXPECT_TRUE(_parentTid != std::this_thread::get_id());
            ::SleepMs(50);
            if (_lock) {
                _threadDone = true;
                _lock = false;
                _event.Unlock();
            }else if (_setEvent) {
                _threadDone = true;
                _setEvent = false;
                _event.SetEvent();
            }
        }
        return (Core::infinite);
    }
private:
    volatile bool&  _threadDone;
    volatile bool&  _lock;
    volatile bool&  _setEvent;
    Event& _event;
    std::thread::id _parentTid;
};

TEST(test_event, simple_event)
{
    Event event(false,true);
    uint64_t timeOut(Core::Time::Now().Add(3).Ticks());
    uint64_t now(Core::Time::Now().Ticks());

    if (now < timeOut) {
        event.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
        EXPECT_FALSE(event.IsSet());
    }
}

TEST(test_event, unlock_event)
{    
    Event event(false,true);
    std::thread::id parentTid;
    bool threadDone = false;
    volatile bool lock = true;
    volatile bool setEvent = false;

    ThreadClass object(event, parentTid, threadDone, lock, setEvent);
    object.Run();
    event.Lock();
    EXPECT_FALSE(lock);
    object.Stop();
}

TEST(DISABLED_test_event, set_event)
{
    Event event(false,true);
    std::thread::id parentTid;
    bool threadDone = false;
    volatile bool lock = false;
    volatile bool setEvent = true;

    event.ResetEvent();
    ThreadClass object(event, parentTid, threadDone, lock, setEvent);
    object.Run();
    event.Lock();
    EXPECT_FALSE(setEvent);
    object.Stop();     
}
