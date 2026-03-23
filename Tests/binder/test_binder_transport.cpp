/*
 * test_binder_transport.cpp
 *
 * Low-level BinderTransport unit test.
 * Verifies that /dev/binder can be opened/closed and that BecomeContextManager
 * works.  Does NOT require a remote peer.
 *
 * Run as root (or with CAP_SYS_ADMIN on some kernels) because
 * BINDER_SET_CONTEXT_MGR requires the process to hold BPF_PROG_LOAD privilege
 * or be root on older kernels.
 *
 * Expected output (success):
 *   [PASS] Open /dev/binder
 *   [PASS] BecomeContextManager
 *   [PASS] Close
 */

#include <cstdio>
#include <cstdlib>

#include "../../Source/core/Errors.h"
#include "../../Source/core/BinderTransport.h"

using namespace Thunder::Core;

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
    BinderTransport transport;

    uint32_t result = transport.Open();
    CHECK(result == ERROR_NONE, "Open /dev/binder");

    if (result != ERROR_NONE) {
        printf("Cannot continue without an open transport\n");
        return 1;
    }

    CHECK(transport.IsOpen(), "IsOpen() after Open");

    result = transport.BecomeContextManager();
    CHECK(result == ERROR_NONE, "BecomeContextManager");

    result = transport.Close();
    CHECK(result == ERROR_NONE, "Close");

    CHECK(!transport.IsOpen(), "IsOpen() is false after Close");

    printf("\n%s (%d failure(s))\n",
           failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           failures);
    return (failures == 0) ? 0 : 1;
}
