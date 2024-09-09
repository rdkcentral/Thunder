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

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    static int g_shared = 2; // Change it back to 1 after fixing and enabling simple_criticalsection test

    class ThreadClass : public ::Thunder::Core::Thread {
    public:
        ThreadClass() = delete;
        ThreadClass(const ThreadClass&) = delete;
        ThreadClass& operator=(const ThreadClass&) = delete;

        ThreadClass(::Thunder::Core::CriticalSection& lock,std::thread::id parentId)
            : ::Thunder::Core::Thread(::Thunder::Core::Thread::DefaultStackSize(), _T("Test"))
            , _lock(lock)
            , _parentId(parentId)
            , _done(false)
        {
        }

        virtual ~ThreadClass()
        {
        }

        virtual uint32_t Worker() override
        {
            while (IsRunning() && (!_done)) {
                EXPECT_TRUE(_parentId != std::this_thread::get_id());
                _done = true;
                _lock.Lock();
                g_shared++;
                _lock.Unlock();
                ::SleepMs(50);
            }
            return (::Thunder::Core::infinite);
        }

    private:
        ::Thunder::Core::CriticalSection& _lock;
        std::thread::id _parentId;
        volatile bool _done;
    };

    TEST(test_criticalsection, DISABLED_simple_criticalsection)
    {
        ::Thunder::Core::CriticalSection lock;

        ThreadClass object(lock, std::this_thread::get_id());
        object.Run();
        lock.Lock();
        g_shared++;
        lock.Unlock();
        object.Stop();
        object.Wait(::Thunder::Core::Thread::STOPPED, ::Thunder::Core::infinite);
        EXPECT_EQ(g_shared,2);
    }

    TEST(test_binairysemaphore, simple_binairysemaphore_timeout)
    {
        ::Thunder::Core::BinairySemaphore bsem(true);
        uint64_t timeOut(::Thunder::Core::Time::Now().Add(3).Ticks());
        uint64_t now(::Thunder::Core::Time::Now().Ticks());

        if (now < timeOut) {
            bsem.Lock(static_cast<uint32_t>((timeOut - now) / ::Thunder::Core::Time::TicksPerMillisecond));
            g_shared++;
        }
        EXPECT_EQ(g_shared,3);
    }

    TEST(test_binairysemaphore, simple_binairysemaphore)
    {
        ::Thunder::Core::BinairySemaphore bsem(1,5);
        bsem.Lock();
        g_shared++;
        bsem.Unlock();
        EXPECT_EQ(g_shared,4);
    }

    TEST(test_countingsemaphore, simple_countingsemaphore_timeout)
    {
        ::Thunder::Core::CountingSemaphore csem(1,5);
        uint64_t timeOut(::Thunder::Core::Time::Now().Add(3).Ticks());
        uint64_t now(::Thunder::Core::Time::Now().Ticks());
        do
        {
            if (now < timeOut) {
                csem.Lock(static_cast<uint32_t>((timeOut - now) / ::Thunder::Core::Time::TicksPerMillisecond));
                g_shared++;
            }
        } while (timeOut < ::Thunder::Core::Time::Now().Ticks());
        EXPECT_EQ(g_shared,5);
       
        timeOut = ::Thunder::Core::Time::Now().Add(3).Ticks();
        now = ::Thunder::Core::Time::Now().Ticks();
        do
        {
            if (now < timeOut) {
                csem.TryUnlock(static_cast<uint32_t>((timeOut - now) / ::Thunder::Core::Time::TicksPerMillisecond));
                g_shared++;
            }
        } while (timeOut < ::Thunder::Core::Time::Now().Ticks());
        EXPECT_EQ(g_shared,6);
    }

    TEST(test_countingsemaphore, simple_countingsemaphore)
    {
        ::Thunder::Core::CountingSemaphore csem(1,5);
        csem.Lock();
        g_shared++;
        csem.Unlock(1);
        EXPECT_EQ(g_shared,7);
    }

} // Core
} // Tests
} // Thunder
