#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

namespace WPEFramework {
namespace Tests {

    bool g_threadDone = false;
    std::thread::id g_parentTid;

    class ThreadClass : public Core::Thread {
    private:
        ThreadClass(const ThreadClass&) = delete;
        ThreadClass& operator=(const ThreadClass&) = delete;

    public:
        ThreadClass()
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
        {
        }

        virtual ~ThreadClass()
        {
        }

        virtual uint32_t Worker() override
        {
            while (IsRunning() && (!g_threadDone)) {
                EXPECT_NE(g_parentTid, std::this_thread::get_id());
                g_threadDone = true;
            }
            return (Core::infinite);
        }
    };

    class Job : public Core::IDispatch {
    private:
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;

    public:
        Job()
        {
        }
        ~Job()
        {
        }
        virtual void Dispatch() override
        {
            EXPECT_NE(g_parentTid, std::this_thread::get_id());
            g_threadDone = true;
        }
    };

    TEST(Core_Thread, SimpleThread)
    {
        g_parentTid = std::this_thread::get_id();
        g_threadDone = false;
        ThreadClass object;
        object.Run();
        usleep(1000);
        EXPECT_EQ(object.State(), Core::Thread::RUNNING);
        EXPECT_TRUE(g_threadDone);
        object.Stop();
        EXPECT_EQ(object.State(), Core::Thread::STOPPING);
        object.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED | Core::Thread::STOPPING, Core::infinite);
    }

    TEST(Core_Thread, ThreadPool)
    {
        g_parentTid = std::this_thread::get_id();
        g_threadDone = false;
        Core::ProxyType<Core::IDispatch> job(Core::ProxyType<Job>::Create());
        Core::ThreadPoolType<Core::Job, 1> executor(0, _T("TestPool"));
        executor.Submit(Core::Job(job), Core::infinite);
        usleep(1000);
        EXPECT_TRUE(g_threadDone);
        Core::Singleton::Dispose();
    }
}
}
