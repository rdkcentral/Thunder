#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

static int g_done = 0;

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
            ASSERT(Core::Time::Now().ToRFC1123() == _nextTick.ToRFC1123());
            if (!g_done) {
                _nextTick = Core::Time::Now();
                uint32_t time = 1000; // 1 second
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

TEST(Core_Timer, simpleSet)
{
    Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T(""));
    uint32_t time = 1000; // 1 second
    Core::Time nextTick = Core::Time::Now();
    nextTick.Add(time);
    timer.Schedule(nextTick.Ticks(), TimeHandler(nextTick));
    while(!(g_done == 2));
    Core::Singleton::Dispose();
}
