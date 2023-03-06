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

namespace WPEFramework {
namespace Tests {

    class ThreadClass : public Core::Thread {
    public:
        ThreadClass() = delete;
        ThreadClass(const ThreadClass&) = delete;
        ThreadClass& operator=(const ThreadClass&) = delete;

        ThreadClass(volatile bool& threadDone, std::mutex& mutex, std::condition_variable& cv, ::ThreadId parentTid)
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
            , _done(threadDone)
            , _threadMutex(mutex)
            , _threadCV(cv)
            , _parentTid(parentTid)
        {
        }

        virtual ~ThreadClass()
        {
        }

        virtual uint32_t Worker() override
        {
            while (IsRunning() && (!_done)) {
                EXPECT_NE(_parentTid, Core::Thread::ThreadId());
                std::unique_lock<std::mutex> lk(_threadMutex);
                _done = true;
                _threadCV.notify_one();
            }
            return (Core::infinite);
        }

    private :
        volatile bool& _done;
        std::mutex& _threadMutex;
        std::condition_variable& _threadCV;
        ::ThreadId _parentTid;
    };

    class Job : public Core::IDispatch {
    public:
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;

        Job()
        {
        }

        ~Job()
        {
        }

        virtual void Dispatch() override
        {
            EXPECT_NE(_parentTPid, Core::Thread::ThreadId());
            std::unique_lock<std::mutex> lk(_mutex);
            _threadDone = true;
            _cv.notify_one();
        }

        static bool GetState()
        {
            return _threadDone;
        }

    private:
        static bool _threadDone;
        static ::ThreadId _parentTPid;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    bool Job::_threadDone = false;
    std::mutex Job::_mutex;
    std::condition_variable Job::_cv;
    ::ThreadId Job::_parentTPid = Core::Thread::ThreadId();

    TEST(Core_Thread, DISABLED_SimpleThread)
    {
        ::ThreadId parentTid = Core::Thread::ThreadId();
        volatile bool threadDone = false;
        std::mutex mutex;
        std::condition_variable cv;

        ThreadClass object(threadDone, mutex, cv, parentTid);
        object.Run();
        EXPECT_EQ(object.State(), Core::Thread::RUNNING);
        std::unique_lock<std::mutex> lk(mutex);
        while (!threadDone) {
            cv.wait(lk);
        }
        object.Stop();
        EXPECT_EQ(object.State(), Core::Thread::STOPPING);
        object.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED | Core::Thread::STOPPING, Core::infinite);
    }
} // Tests
} // WPEFramework
