#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/Sync.h>
#include <core/Thread.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

int g_shared = 1;
bool g_threadCritDone = false;
static std::thread::id g_parentId;
Core::CriticalSection lock;

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
        while (IsRunning() && (!g_threadCritDone)) {
            ASSERT(g_parentId != std::this_thread::get_id());
            g_threadCritDone = true;
            lock.Lock();
            g_shared++;
            lock.Unlock();
            ::SleepMs(50);
        }
        return (Core::infinite);
    }
};


TEST(test_criticalsection, simple_criticalsection)
{
    ThreadClass object;
    object.Run();
    lock.Lock();
    g_shared++;
    lock.Unlock();
    object.Stop();
    ASSERT_EQ(g_shared,2);
}
TEST(test_binairysemaphore, simple_binairysemaphore_timeout)
{
    BinairySemaphore bsem(true);
    uint64_t timeOut(Core::Time::Now().Add(3).Ticks());
    uint64_t now(Core::Time::Now().Ticks());
    do
    {
        if (now < timeOut) {
            bsem.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
            g_shared++;
        }
    } while (timeOut < Core::Time::Now().Ticks());
    ASSERT_EQ(g_shared,3);
}

TEST(test_binairysemaphore, simple_binairysemaphore)
{
    BinairySemaphore bsem(1,5);
    bsem.Lock();
    g_shared++;
    bsem.Unlock();
    ASSERT_EQ(g_shared,4);
}
TEST(test_countingsemaphore, simple_countingsemaphore_timeout)
{
    CountingSemaphore csem(1,5);
    uint64_t timeOut(Core::Time::Now().Add(3).Ticks());
    uint64_t now(Core::Time::Now().Ticks());
    do
    {
        if (now < timeOut) {
            csem.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
            g_shared++;
        }
    } while (timeOut < Core::Time::Now().Ticks());
    ASSERT_EQ(g_shared,5);
   
    timeOut = Core::Time::Now().Add(3).Ticks();
    now = Core::Time::Now().Ticks();
    do
    {
        if (now < timeOut) {
            csem.TryUnlock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
            g_shared++;
        }
    } while (timeOut < Core::Time::Now().Ticks());
    ASSERT_EQ(g_shared,6);
}
TEST(test_countingsemaphore, simple_countingsemaphore)
{
    CountingSemaphore csem(1,5);
    csem.Lock();
    g_shared++;
    csem.Unlock(1);
    ASSERT_EQ(g_shared,7);
}
