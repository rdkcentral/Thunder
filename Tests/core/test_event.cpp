#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/Sync.h>
#include <core/Time.h>
#include <core/Thread.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

Event event(false,true);
bool g_threadDone = false;
bool g_lock = true;
bool g_set_event = true;
bool g_pulse_event = true;
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
            if (g_lock) {
                g_threadDone = true;
                g_lock = false;
                event.Unlock();
            }else if (g_set_event) {
                g_threadDone = true;
                g_set_event = false;
                event.SetEvent();
            }else if (g_pulse_event) {
                g_threadDone = true;
                g_pulse_event = false;
                event.PulseEvent();
            }
            ::SleepMs(50);
        }
        return (Core::infinite);
    }
};

TEST(test_event, simple_event)
{
    uint64_t timeOut(Core::Time::Now().Add(3).Ticks());
    uint64_t now(Core::Time::Now().Ticks());
    do
    {
        if (now < timeOut) {
            event.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
            ASSERT_FALSE(event.IsSet());
        }
    } while (timeOut < Core::Time::Now().Ticks());
}
TEST(test_event, unlock_event)
{    
    ThreadClass object;
    object.Run();
    event.Lock();
    ASSERT_FALSE(g_lock);
    object.Stop();
}
TEST(test_event, set_event)
{
    ThreadClass object;
    g_threadDone = false;
    object.Run();
    event.ResetEvent();
    event.Lock();
    ASSERT_FALSE(g_set_event);
    object.Stop();     
}
TEST(test_event, pulse_event)
{
    ThreadClass object;
    object.Run();
    g_threadDone = false;
    event.ResetEvent();
    event.Lock();
    ASSERT_FALSE(g_pulse_event);
    object.Stop();
}
