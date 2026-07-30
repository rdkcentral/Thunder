#pragma once
#include <string>

namespace Thunder {
namespace TestSupport {
    extern std::string g_observedCallsign; // set by the test before deactivating
    extern bool g_deactivatedFired;
    extern bool g_shellQiWorked;           // QI<IShell> during the Deactivated notification
    extern bool g_handlerQiResolved;       // QI<IPlugin> during it (forwards to live handler -> non-null)
}
}
