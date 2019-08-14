#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

bool g_timerDone = false;
string g_currentTime = "";

class TimeHandler {
    public:
        TimeHandler(Core::Time& Tick)
            :_tick(Tick)
        {
        }
        ~TimeHandler()
        {
        }

    public:
        uint64_t Timed(const uint64_t scheduledTime)
        {
            g_currentTime = Core::Time::Now().ToRFC1123();
            g_timerDone = true;
            return 0;
        }
private:
    Core::Time _tick;
};

TEST(Core_Timer, simpleSet)
{
    Core::TimerType<TimeHandler> timer(Core::Thread::DefaultStackSize(), _T(""));
    uint32_t time = 2000; // 2 seconds
    Core::Time NextTick = Core::Time::Now();
    NextTick.Add(time);
    string scheduledTime = NextTick.ToRFC1123();
    timer.Schedule(NextTick.Ticks(), TimeHandler(NextTick));
    while(!g_timerDone);
    ASSERT_STREQ(scheduledTime.c_str(), g_currentTime.c_str());
}
