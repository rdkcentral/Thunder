// ============================================================================
// Smoke test for thunder_test_support
//
// Verifies the library links, boots the embedded server, and exercises
// basic JSON-RPC calls against the built-in Controller plugin.
// No external plugin .so files are needed.
// ============================================================================

#include <gtest/gtest.h>
#include "ThunderTestRuntime.h"

using namespace Thunder;

class SmokeTest : public ::testing::Test {
protected:
    static TestCore::ThunderTestRuntime _runtime;

    static void SetUpTestSuite() {
        // No plugins — only the built-in Controller is needed
        std::vector<TestCore::ThunderTestRuntime::PluginConfig> plugins;
        uint32_t result = _runtime.Initialize(plugins);
        ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize Thunder runtime";
    }

    static void TearDownTestSuite() {
        _runtime.Deinitialize();
    }
};

TestCore::ThunderTestRuntime SmokeTest::_runtime;

// Verify the server booted and we can query Controller status
TEST_F(SmokeTest, ControllerStatus) {
    string response;
    uint32_t result = _runtime.InvokeJSONRPC("Controller.1.status", "", response);
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_FALSE(response.empty());
    // Response should contain Controller's own entry
    EXPECT_NE(response.find("Controller"), string::npos);
}

// Verify we can query subsystems
TEST_F(SmokeTest, ControllerSubsystems) {
    string response;
    uint32_t result = _runtime.InvokeJSONRPC("Controller.1.subsystems", "", response);
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_FALSE(response.empty());
}

// Verify we can get the IShell for Controller
TEST_F(SmokeTest, GetControllerShell) {
    auto shell = _runtime.GetShell("Controller");
    ASSERT_TRUE(shell.IsValid());
    EXPECT_EQ(shell->State(), PluginHost::IShell::state::ACTIVATED);
}
