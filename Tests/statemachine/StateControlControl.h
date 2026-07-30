#pragma once
#include <string>
#include <vector>

namespace Thunder {
namespace TestSupport {
    // Observation surface for the in-process StateControl mock. Because the mock
    // runs in the test process, the test reads these directly. (An OOP variant
    // cannot, which is why these tests do not transfer to OOP.)
    extern std::vector<int> g_stateControlRequests; // command per Request() call: SUSPEND=0, RESUME=1
    extern int g_stateControlSinkCount;             // currently registered INotification sinks
    extern int g_stateControlNotifyCount;           // StateChange notifications emitted to sinks

    inline void ResetStateControlObservation()
    {
        g_stateControlRequests.clear();
        g_stateControlSinkCount = 0;
        g_stateControlNotifyCount = 0;
    }
}
}
