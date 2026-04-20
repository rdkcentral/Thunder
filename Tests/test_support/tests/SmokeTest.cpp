// ==========================================================================
// SmokeTest — validates the ThunderTestRuntime boots and tears down cleanly.
//
// These tests exercise:
//   1. Invoke() with the full "Callsign.method" form
//   2. JSONRPCLink (callsign-bound) invocation
//   3. GetShell() COM-RPC path
// ==========================================================================

#include "Module.h"
#include "ThunderTestRuntime.h"
#include <gtest/gtest.h>
#include <string>

namespace Thunder {
namespace TestCore {
namespace Tests {

    class SmokeTest : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        static void SetUpTestSuite()
        {
            std::vector<ThunderTestRuntime::PluginConfig> plugins;

            uint32_t result = _runtime.Initialize(plugins);
            ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize Thunder runtime";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }
    };

    ThunderTestRuntime SmokeTest::_runtime;

    // ------------------------------------------------------------------
    // Full-designator Invoke: "Controller.status"
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, ControllerStatusViaFullDesignator)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "Controller.status returned: " << result;
        EXPECT_FALSE(response.empty());
    }

    // ------------------------------------------------------------------
    // Full-designator Invoke: "Controller.subsystems"
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, ControllerSubsystemsViaFullDesignator)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.subsystems", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "Controller.subsystems returned: " << result;
        EXPECT_FALSE(response.empty());
    }

    // ------------------------------------------------------------------
    // JSONRPCLink (callsign-bound): invoke without repeating callsign
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, ControllerStatusViaJSONRPCLink)
    {
        auto* controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_NE(controller, nullptr);

        string response;
        uint32_t result = controller->Invoke("status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "status via link returned: " << result;
        EXPECT_FALSE(response.empty());

        delete controller;
    }

    // ------------------------------------------------------------------
    // GetShell: obtain the IShell for the Controller
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, GetControllerShell)
    {
        auto shell = _runtime.GetShell("Controller");
        EXPECT_TRUE(shell.IsValid()) << "Controller IShell must be available";
    }

    // ------------------------------------------------------------------
    // Unknown method returns ERROR_UNKNOWN_KEY
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, UnknownMethodReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.thisMethodDoesNotExist", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_KEY);
    }

    // ------------------------------------------------------------------
    // Unknown method via JSONRPCLink
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, UnknownMethodViaJSONRPCLinkReturnsError)
    {
        auto* controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_NE(controller, nullptr);

        string response;
        uint32_t result = controller->Invoke("thisMethodDoesNotExist", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_KEY);

        delete controller;
    }

    // ------------------------------------------------------------------
    // Missing callsign in full-designator form returns error
    // ------------------------------------------------------------------
    TEST_F(SmokeTest, MissingCallsignReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("noCallsignDot", "{}", response);
        EXPECT_NE(result, Core::ERROR_NONE);
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
