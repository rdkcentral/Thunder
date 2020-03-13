#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    int g_done = 0;

    class TimeHandler {
    public:
        TimeHandler()
        {
        }
        ~TimeHandler()
        {
        }

    public:
        uint64_t Timed(const uint64_t scheduledTime)
        {
            if (!g_done) {
                Core::Time nextTick = Core::Time::Now();
                uint32_t time = 100; // 0.1 second
                nextTick.Add(time);
                g_done++;
                return nextTick.Ticks();
            }
            g_done++;
            return 0;
        }
    };

    class WatchDogHandler : Core::WatchDogType<WatchDogHandler&> {
    private:
        WatchDogHandler& operator=(const WatchDogHandler&) = delete;

        typedef Core::WatchDogType<WatchDogHandler&> BaseClass;
    public:
        WatchDogHandler()
            : BaseClass(Core::Thread::DefaultStackSize(), _T("WatchDogTimer"), *this)
            , _event(false, false)
        {
        }
        ~WatchDogHandler()
        {
        }
        void Start(uint32_t delay)
        {
            BaseClass::Arm(delay);
        }
        uint32_t Expired()
        {
            _event.SetEvent();
            return Core::infinite;
        }
        int Wait(unsigned int milliseconds) const
        {
            return _event.Lock(milliseconds);
        }
    private:
        uint32_t _delay;
        mutable Core::Event _event;
    };

    TEST(Core_Timer, LoopedTimer)
    {
        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T("LoopedTimer"));
        uint32_t time = 100;

        Core::Time nextTick = Core::Time::Now();
        nextTick.Add(time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());
        sleep(2);
        while(!(g_done == 2));
    }

    TEST(Core_Timer, QueuedTimer)
    {
        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T("QueuedTimer"));
        uint32_t time = 100;

        Core::Time nextTick = Core::Time::Now();
        nextTick.Add(time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());

        nextTick.Add(2 * time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());

        nextTick.Add(3 * time);
        timer.Schedule(nextTick.Ticks(), TimeHandler());
        sleep(2);
        while(!(g_done == 5));
    }

    TEST(Core_Timer, PastTime)
    {
        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T("PastTime"));
        uint32_t time = 100; // 0.1 second

        Core::Time pastTime = Core::Time::Now();
        pastTime.Sub(time);
        timer.Schedule(pastTime.Ticks(), TimeHandler());
        sleep(2);
        while(!(g_done == 6));
    }

    TEST(Core_Timer, WatchDogType)
    {
        WatchDogHandler timer;
        timer.Start(100); // 100 milliseconds delay
        int ret = timer.Wait(200); // Wait for 200 milliseconds
        EXPECT_EQ(ret, Core::ERROR_NONE);
    }
} // Tests
} // WPEFramework
