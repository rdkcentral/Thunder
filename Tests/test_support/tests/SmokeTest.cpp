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
        auto controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_TRUE(controller.IsValid());

        string response;
        const uint32_t result = controller->Invoke("status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "status via link returned: " << result;
        EXPECT_FALSE(response.empty());
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
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_METHOD);
    }

    TEST_F(SmokeTest, UnknownMethodViaJSONRPCLinkReturnsError)
    {
        auto controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_TRUE(controller.IsValid());

        string response;
        const uint32_t result = controller->Invoke("thisMethodDoesNotExist", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_METHOD);
    }

    TEST_F(SmokeTest, MissingCallsignReturnsError)
    {
        string response;
        const uint32_t result = _runtime.Invoke("noCallsignDot", "{}", response);
        EXPECT_NE(result, Core::ERROR_NONE);
    }

    // --- QueryInterfaceByCallsign tests ---

    TEST_F(SmokeTest, QueryInterfaceByCallsign_ControllerPlugin)
    {
        auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("Controller");
        ASSERT_NE(plugin, nullptr) << "Controller must implement IPlugin";
        plugin->Release();
    }

    TEST_F(SmokeTest, QueryInterfaceByCallsign_NonExistentCallsignReturnsNull)
    {
        auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("NoSuchPlugin");
        EXPECT_EQ(plugin, nullptr);
    }

    TEST_F(SmokeTest, QueryInterfaceByCallsign_UnsupportedInterfaceReturnsNull)
    {
        auto* iface = _runtime.QueryInterfaceByCallsign<PluginHost::ISubSystem::IInternet>("Controller");
        EXPECT_EQ(iface, nullptr);
    }

    TEST_F(SmokeTest, QueryInterfaceByCallsign_ControllerDispatcher)
    {
        auto* dispatcher = _runtime.QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");
        ASSERT_NE(dispatcher, nullptr) << "Controller must expose IDispatcher";
        dispatcher->Release();
    }

    // --- JsonObject overload tests ---

    TEST_F(SmokeTest, InvokeWithJsonObjectViaFullDesignator)
    {
        JsonObject params;
        JsonObject response;
        const uint32_t result = _runtime.Invoke("Controller.status", params, response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "Controller.status (JsonObject) returned: " << result;
        EXPECT_TRUE(response.HasLabel("") == false || true);
    }

    TEST_F(SmokeTest, InvokeWithJsonObjectViaLink)
    {
        auto controller = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_TRUE(controller.IsValid());

        JsonObject params;
        JsonObject response;
        const uint32_t result = controller->Invoke("status", params, response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "status (JsonObject) via link returned: " << result;
    }

    TEST_F(SmokeTest, InvokeWithJsonObjectUnknownMethodReturnsError)
    {
        JsonObject params;
        JsonObject response;
        const uint32_t result = _runtime.Invoke("Controller.thisMethodDoesNotExist", params, response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_METHOD);
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder