/*
 * test_binder_svcmgr.cpp
 *
 * In-process BinderServiceManager test.
 * Starts the context manager, registers a service, looks it up, and
 * enumerates the service list.  Everything runs inside one process.
 *
 * Expected output:
 *   [PASS] ServiceManager::Start
 *   [PASS] AddService "test.service" handle=1
 *   [PASS] GetService "test.service" returns handle=1
 *   [PASS] ListService index 0 = "test.service"
 *   [PASS] ListService index 1 out-of-range
 *   [PASS] ServiceManager::Stop
 */

#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>

#include "../../Source/core/Errors.h"
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

    uint32_t result = svcmgr.Start(2);
    CHECK(result == ERROR_NONE, "ServiceManager::Start");
    if (result != ERROR_NONE) {
        printf("Cannot continue without a running context manager\n");
        return 1;
    }

    // Give looper threads a moment to register
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // ---- AddService ----
    result = svcmgr.AddService("test.service", /*handle=*/1, /*allowIsolated=*/true);
    CHECK(result == ERROR_NONE, "AddService \"test.service\" handle=1");

    // ---- GetService ----
    uint32_t handle = 0;
    result = svcmgr.GetService("test.service", handle);
    CHECK(result == ERROR_NONE && handle == 1,
          "GetService \"test.service\" returns handle=1");

    // ---- GetService missing ----
    uint32_t handle2 = 0;
    result = svcmgr.GetService("no.such.service", handle2);
    CHECK(result != ERROR_NONE, "GetService missing returns error");

    // ---- ListService ----
    string name;
    result = svcmgr.ListService(0, name);
    CHECK(result == ERROR_NONE && name == "test.service",
          "ListService index 0 = \"test.service\"");

    result = svcmgr.ListService(1, name);
    CHECK(result != ERROR_NONE, "ListService index 1 out-of-range");

    // ---- Stop ----
    result = svcmgr.Stop();
    CHECK(result == ERROR_NONE, "ServiceManager::Stop");
    CHECK(!svcmgr.IsRunning(), "IsRunning() is false after Stop");

    printf("\n%s (%d failure(s))\n",
           failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           failures);
    return (failures == 0) ? 0 : 1;
}
