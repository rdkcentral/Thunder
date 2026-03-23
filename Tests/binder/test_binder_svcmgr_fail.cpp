/*
 * test_binder_svcmgr_fail.cpp
 *
 * Negative tests for BinderServiceManager and BinderServiceManagerProxy:
 *  1. Double-start is rejected.
 *  2. AddService with empty name is rejected.
 *  3. AddService with handle=0 is rejected.
 *  4. GetService for an unknown service returns an error.
 *  5. ListService past the end returns an error.
 *  6. BinderServiceManagerProxy returns error when transport is not open.
 */

#include <cstdio>
#include <thread>
#include <chrono>

#include "../../Source/com/BinderServiceManager.h"
#include "../../Source/core/Errors.h"

using namespace Thunder;
using namespace Thunder::Core;
using namespace Thunder::RPC;

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s\n", msg); \
            ++failures; \
        } else { \
            printf("[PASS] %s\n", msg); \
        } \
    } while (0)

int main()
{
    BinderServiceManager& svcmgr = BinderServiceManager::Instance();

    // ---- 1. Start ----
    uint32_t result = svcmgr.Start(1);
    CHECK(result == ERROR_NONE, "First Start succeeds");
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // ---- 2. Double-start is InProgress ----
    result = svcmgr.Start(1);
    CHECK(result == ERROR_INPROGRESS, "Second Start returns INPROGRESS");

    // ---- 3. Add with empty name ----
    result = svcmgr.AddService("", 1, true);
    CHECK(result != ERROR_NONE, "AddService with empty name returns error");

    // ---- 4. Add with handle=0 ----
    result = svcmgr.AddService("dummy", 0, true);
    CHECK(result != ERROR_NONE, "AddService handle=0 returns error");

    // ---- 5. GetService unknown ----
    uint32_t handle = 999;
    result = svcmgr.GetService("does.not.exist", handle);
    CHECK(result == ERROR_UNAVAILABLE, "GetService unknown returns UNAVAILABLE");
    CHECK(handle == 999, "GetService output handle unchanged on failure");

    // ---- 6. ListService past end (no services registered) ----
    string name;
    result = svcmgr.ListService(0, name);
    CHECK(result == ERROR_UNAVAILABLE, "ListService index 0 on empty registry");

    // ---- 7. Add and list ----
    result = svcmgr.AddService("good.service", 42, false);
    CHECK(result == ERROR_NONE, "AddService good.service handle=42");

    result = svcmgr.ListService(0, name);
    CHECK(result == ERROR_NONE && name == "good.service",
          "ListService index 0 returns good.service");

    result = svcmgr.ListService(1, name);
    CHECK(result == ERROR_UNAVAILABLE, "ListService index 1 out-of-range");

    // ---- 8. Proxy errors when not connected ----
    {
        BinderServiceManagerProxy proxy;
        // Not opened yet
        uint32_t h = 0;
        result = proxy.GetService("any", h);
        CHECK(result == ERROR_ILLEGAL_STATE, "Proxy GetService without Open returns ILLEGAL_STATE");

        result = proxy.AddService("any", 1, true);
        CHECK(result == ERROR_ILLEGAL_STATE, "Proxy AddService without Open returns ILLEGAL_STATE");

        result = proxy.ListService(0, name);
        CHECK(result == ERROR_ILLEGAL_STATE, "Proxy ListService without Open returns ILLEGAL_STATE");
    }

    // ---- 9. Stop ----
    result = svcmgr.Stop();
    CHECK(result == ERROR_NONE, "ServiceManager::Stop");
    CHECK(!svcmgr.IsRunning(), "IsRunning() false after Stop");

    printf("\n%s (%d failure(s))\n",
           failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           failures);
    return (failures == 0) ? 0 : 1;
}
