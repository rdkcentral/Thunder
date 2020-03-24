#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

int g_shared = 1;

class ThreadClass : public Core::Thread {
public:
    ThreadClass() = delete;
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

    ThreadClass(Core::CriticalSection* lock,std::thread::id parentId)
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
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
            _lock->Lock();
            g_shared++;
            _lock->Unlock();
            ::SleepMs(50);
        }
        return (Core::infinite);
    }

private:
    Core::CriticalSection* _lock;
    std::thread::id _parentId;
    bool _done;
};


TEST(test_criticalsection, simple_criticalsection)
{
    Core::CriticalSection lock;
    std::thread::id parentId;

    ThreadClass object(&lock,parentId);
    object.Run();
    lock.Lock();
    g_shared++;
    lock.Unlock();
    object.Stop();
    EXPECT_EQ(g_shared,2);
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
    EXPECT_EQ(g_shared,3);
}

TEST(test_binairysemaphore, simple_binairysemaphore)
{
    BinairySemaphore bsem(1,5);
    bsem.Lock();
    g_shared++;
    bsem.Unlock();
    EXPECT_EQ(g_shared,4);
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
    EXPECT_EQ(g_shared,5);
   
    timeOut = Core::Time::Now().Add(3).Ticks();
    now = Core::Time::Now().Ticks();
    do
    {
        if (now < timeOut) {
            csem.TryUnlock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
            g_shared++;
        }
    } while (timeOut < Core::Time::Now().Ticks());
    EXPECT_EQ(g_shared,6);
}
TEST(test_countingsemaphore, simple_countingsemaphore)
{
    CountingSemaphore csem(1,5);
    csem.Lock();
    g_shared++;
    csem.Unlock(1);
    EXPECT_EQ(g_shared,7);
}
