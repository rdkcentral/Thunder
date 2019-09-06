#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    int g_done = 0;

    class TimeHandler {
        public:
            TimeHandler(Core::Time& tick)
                : _nextTick(tick)
            {
            }
            ~TimeHandler()
            {
            }

        public:
            uint64_t Timed(const uint64_t scheduledTime)
            {
                if (!g_done) {
                    _nextTick = Core::Time::Now();
                    uint32_t time = 100; // 0.1 second
                    _nextTick.Add(time);
                    g_done++;
                    return _nextTick.Ticks();
                }
                g_done++;
                return 0;
            }
    private:
        Core::Time _nextTick;
    };

    TEST(Core_Timer, PastTime)
    {
        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T(""));
        uint32_t time = 100; // 0.1 second

        Core::Time pastTime = Core::Time::Now();
        pastTime.Sub(time);
        timer.Schedule(pastTime.Ticks(), TimeHandler(pastTime));
        usleep(100);
    }

    TEST(Core_Timer, LoopedTimer)
    {
        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T(""));
        uint32_t time = 100;

        Core::Time nextTick = Core::Time::Now();
        nextTick.Add(time);
        timer.Schedule(nextTick.Ticks(), TimeHandler(nextTick));
        while(!(g_done == 2));
    }

    TEST(Core_Timer, QueuedTimer)
    {
        Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T(""));
        uint32_t time = 100;

        Core::Time nextTick = Core::Time::Now();
        nextTick.Add(time);
        timer.Schedule(nextTick.Ticks(), TimeHandler(nextTick));

        nextTick.Add(2 * time);
        timer.Schedule(nextTick.Ticks(), TimeHandler(nextTick));

        nextTick.Add(3 * time);
        timer.Schedule(nextTick.Ticks(), TimeHandler(nextTick));
        while(!(g_done == 5));
    }
}
}
