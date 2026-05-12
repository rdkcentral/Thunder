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

            const uint32_t result = _runtime.Initialize(plugins);
            ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize Thunder runtime";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }
    };

    ThunderTestRuntime SmokeTest::_runtime;

    TEST_F(SmokeTest, ControllerStatusViaFullDesignator)
    {
        string response;
        const uint32_t result = _runtime.Invoke("Controller.status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "Controller.status returned: " << result;
        EXPECT_FALSE(response.empty());
    }

    TEST_F(SmokeTest, ControllerSubsystemsViaFullDesignator)
    {
        string response;
        const uint32_t result = _runtime.Invoke("Controller.subsystems", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "Controller.subsystems returned: " << result;
        EXPECT_FALSE(response.empty());
    }

    TEST_F(SmokeTest, ControllerStatusViaJSONRPCLink)
    {
        auto* controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_NE(controller, nullptr);

        string response;
        const uint32_t result = controller->Invoke("status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "status via link returned: " << result;
        EXPECT_FALSE(response.empty());

        controller->Release();
    }

    TEST_F(SmokeTest, GetControllerShell)
    {
        auto shell = _runtime.GetShell("Controller");
        EXPECT_TRUE(shell.IsValid()) << "Controller IShell must be available";
    }

    TEST_F(SmokeTest, UnknownMethodReturnsError)
    {
        string response;
        const uint32_t result = _runtime.Invoke("Controller.thisMethodDoesNotExist", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_KEY);
    }

    TEST_F(SmokeTest, UnknownMethodViaJSONRPCLinkReturnsError)
    {
        auto* controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_NE(controller, nullptr);

        string response;
        const uint32_t result = controller->Invoke("thisMethodDoesNotExist", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_KEY);

        controller->Release();
    }

    TEST_F(SmokeTest, MissingCallsignReturnsError)
    {
        string response;
        const uint32_t result = _runtime.Invoke("noCallsignDot", "{}", response);
        EXPECT_NE(result, Core::ERROR_NONE);
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder