#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace WPEFramework;

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
            ASSERT(g_parentTid != std::this_thread::get_id());
            g_threadDone = true;
        }
        return (Core::infinite);
    }
};

TEST(Core_Thread, simpleSet)
{
    g_parentTid = std::this_thread::get_id();
    ThreadClass object;
    object.Run();
    ::SleepMs(300);
    ASSERT_EQ(object.State(), Core::Thread::RUNNING);
    ASSERT_TRUE(g_threadDone);
    object.Stop();
    ASSERT_EQ(object.State(), Core::Thread::STOPPING);
    object.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED | Core::Thread::STOPPING, Core::infinite);
}
