/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

using namespace WPEFramework;
using namespace WPEFramework::Core;

class WorkerPoolImplementation : public Core::WorkerPool {
public:
    WorkerPoolImplementation() = delete;
    WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
    WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

    WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize)
        : WorkerPool(threads, stackSize, queueSize)
    {
    }

    ~WorkerPoolImplementation()
    {
        // Diable the queue so the minions can stop, even if they are processing and waiting for work..
        Stop();
    }

public:
    void Run()
    {
        Core::WorkerPool::Run();
        Core::WorkerPool::Join();
    }

    void Stop()
    {
        Core::WorkerPool::Stop();
    }
};

Core::ProxyType<WorkerPoolImplementation> workerpool = Core::ProxyType<WorkerPoolImplementation>::Create(2, Core::Thread::DefaultStackSize(), 8);

class WorkerThreadClass : public Core::Thread {
public:
    WorkerThreadClass() = delete;
    WorkerThreadClass(const WorkerThreadClass&) = delete;
    WorkerThreadClass& operator=(const WorkerThreadClass&) = delete;

    WorkerThreadClass(std::thread::id parentworkerId)
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
        , _parentworkerId(parentworkerId)
        , _threadDone(false)
    {
    }

    virtual ~WorkerThreadClass()
    {
    }

    virtual uint32_t Worker() override
    {
        while (IsRunning() && (!_threadDone)) {
            EXPECT_TRUE(_parentworkerId != std::this_thread::get_id());
            ::SleepMs(250);
            _threadDone = true;
            workerpool->Stop();
        }
        return (Core::infinite);
    }

private:
    std::thread::id _parentworkerId;
    volatile bool _threadDone;
};

class WorkerJob : public Core::IDispatch {
public:
    WorkerJob(const WorkerJob&) = delete;
    WorkerJob& operator=(const WorkerJob&) = delete;

    WorkerJob()
    {
    }

    ~WorkerJob()
    {
    }

    virtual void Dispatch() override
    {
        EXPECT_NE(_parentJobId, std::this_thread::get_id());
    }

public:
    static std::thread::id _parentJobId;
};

std::thread::id WorkerJob::_parentJobId;

TEST(test_workerpool, simple_workerpool)
{
    WorkerThreadClass object(std::this_thread::get_id());
    object.Run();

    WorkerPoolImplementation workerpool_impl(2,Core::Thread::DefaultStackSize(),8);
    Core::WorkerPool::Assign(&workerpool_impl);
    workerpool->Run();
    workerpool->Id(0);
    EXPECT_EQ(workerpool->Id(1),0u);
    workerpool->Snapshot();
    Core::ProxyType<Core::IDispatch> job(Core::ProxyType<WorkerJob>::Create());
    workerpool->Submit(job);
    workerpool->Schedule(Core::infinite, job);
    workerpool->Revoke(job);
    workerpool->Instance();
    EXPECT_TRUE(workerpool->IsAvailable());
    workerpool->Stop();
    object.Stop();
}
