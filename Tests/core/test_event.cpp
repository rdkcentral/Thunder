#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

class ThreadClass : public Core::Thread {
public:
    ThreadClass() = delete;
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

    ThreadClass(Event& event, std::thread::id parentTid, bool& threadDone, volatile bool& lock, volatile bool& setEvent, volatile bool& pulseEvent)
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
        , _threadDone(threadDone)
        , _lock(lock)
        , _setEvent(setEvent)
        , _pulseEvent(pulseEvent)
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
            if (_lock) {
                _threadDone = true;
                _lock = false;
                _event.Unlock();
            }else if (_setEvent) {
                _threadDone = true;
                _setEvent = false;
                _event.SetEvent();
            }else if (_pulseEvent) {
                _threadDone = true;
                _pulseEvent = false;
                _event.PulseEvent();
            }
            ::SleepMs(50);
        }
        return (Core::infinite);
    }
private:
    volatile bool&  _threadDone;
    volatile bool&  _lock;
    volatile bool&  _setEvent;
    volatile bool&  _pulseEvent;
    Event& _event;
    std::thread::id _parentTid;
};

TEST(test_event, simple_event)
{
    Event event(false,true);
    uint64_t timeOut(Core::Time::Now().Add(3).Ticks());
    uint64_t now(Core::Time::Now().Ticks());
    do
    {
        if (now < timeOut) {
            event.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
            EXPECT_FALSE(event.IsSet());
        }
    } while (timeOut < Core::Time::Now().Ticks());
}

TEST(test_event, unlock_event)
{    
    Event event(false,true);
    std::thread::id parentTid;
    bool threadDone = false;
    volatile bool lock = true;
    volatile bool setEvent = false;
    volatile bool pulseEvent = false;

    ThreadClass object(event, parentTid, threadDone, lock, setEvent, pulseEvent);
    object.Run();
    event.Lock();
    EXPECT_FALSE(lock);
    object.Stop();
}

TEST(test_event, set_event)
{
    Event event(false,true);
    std::thread::id parentTid;
    bool threadDone = false;
    volatile bool lock = false;
    volatile bool setEvent = true;
    volatile bool pulseEvent = false;

    ThreadClass object(event, parentTid, threadDone, lock, setEvent, pulseEvent);
    object.Run();
    event.ResetEvent();
    event.Lock();
    EXPECT_FALSE(setEvent);
    object.Stop();     
}

TEST(test_event, pulse_event)
{
    Event event(false,true);
    std::thread::id parentTid;
    bool threadDone = false;
    volatile bool lock = false;
    volatile bool setEvent = false;
    volatile bool pulseEvent = true;

    ThreadClass object(event, parentTid, threadDone, lock, setEvent, pulseEvent);
    object.Run();
    event.ResetEvent();
    event.Lock();
    EXPECT_FALSE(pulseEvent);
    object.Stop();
}
