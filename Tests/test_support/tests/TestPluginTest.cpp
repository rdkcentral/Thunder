// ==========================================================================
// TestPluginTest — validates that the test runtime can load a plugin
// and interact with it via both COM-RPC and JSON-RPC.
//
// COM-RPC tests use QueryInterface<ITestPlugin> to call methods directly.
// JSON-RPC tests use ThunderTestRuntime::Invoke() and JSONRPCLink.
//
// The TestPlugin is built as a shared library and placed in
// ${CMAKE_BINARY_DIR}/test_plugins/. The test passes that directory as
// the systemPath so the embedded server can dlopen it.
// ==========================================================================

#include "Module.h"
#include "ThunderTestRuntime.h"
#include <interfaces/ITestPlugin.h>
#include <gtest/gtest.h>
#include <string>

#ifndef TEST_PLUGIN_PATH
#error "TEST_PLUGIN_PATH must be defined by CMake"
#endif

namespace Thunder {
namespace TestCore {
namespace Tests {

    class TestPluginTest : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        static void SetUpTestSuite()
        {
            ThunderTestRuntime::PluginConfig dummyConfig;
            dummyConfig.Callsign = "TestPlugin";
            dummyConfig.ClassName = "TestPlugin";
            dummyConfig.Locator = "libThunderTestPlugin.so";
            dummyConfig.StartMode = PluginHost::IShell::startmode::ACTIVATED;

            std::vector<ThunderTestRuntime::PluginConfig> plugins;
            plugins.push_back(dummyConfig);

            uint32_t result = _runtime.Initialize(plugins, TEST_PLUGIN_PATH);
            ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize runtime with TestPlugin";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }
    };

    ThunderTestRuntime TestPluginTest::_runtime;

    // ==================================================================
    // Plugin lifecycle
    // ==================================================================

    TEST_F(TestPluginTest, PluginIsActivated)
    {
        auto shell = _runtime.GetShell("TestPlugin");
        EXPECT_TRUE(shell.IsValid()) << "TestPlugin IShell must be available";
        if (shell.IsValid()) {
            EXPECT_EQ(shell->State(), PluginHost::IShell::state::ACTIVATED);
        }
    }

    // ==================================================================
    // COM-RPC path (QueryInterface<ITestPlugin>)
    // ==================================================================

    TEST_F(TestPluginTest, COMRPC_QueryInterfaceSucceeds)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr) << "QueryInterface<ITestPlugin> must succeed";
        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_EchoReturnsInput)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string output;
        uint32_t result = iface->Echo("hello", output);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(output, "hello");

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_GreetReturnsMessage)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string message;
        uint32_t result = iface->Greet("Thunder", message);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(message, "Hello, Thunder!");

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_GreetDefaultsToWorld)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string message;
        uint32_t result = iface->Greet("", message);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(message, "Hello, World!");

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_EchoEmptyString)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string output;
        uint32_t result = iface->Echo("", output);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_TRUE(output.empty());

        iface->Release();
    }

    // ==================================================================
    // JSON-RPC path (full designator via Invoke)
    // ==================================================================

    TEST_F(TestPluginTest, JSONRPC_EchoReturnsInput)
    {
        string response;
        uint32_t result = _runtime.Invoke("TestPlugin.echo", R"({"input":"hello"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "echo returned: " << result;
        EXPECT_FALSE(response.empty());
        EXPECT_NE(response.find("hello"), string::npos) << "response: " << response;
    }

    TEST_F(TestPluginTest, JSONRPC_GreetReturnsMessage)
    {
        string response;
        uint32_t result = _runtime.Invoke("TestPlugin.greet", R"({"name":"Thunder"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "greet returned: " << result;
        EXPECT_NE(response.find("Hello, Thunder!"), string::npos) << "response: " << response;
    }

    TEST_F(TestPluginTest, JSONRPC_UnknownMethodReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("TestPlugin.nonexistent", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_KEY);
    }

    // ==================================================================
    // JSON-RPC path (JSONRPCLink — callsign-bound)
    // ==================================================================

    TEST_F(TestPluginTest, JSONRPC_EchoViaLink)
    {
        auto* link = _runtime.CreateJSONRPCLink("TestPlugin");
        ASSERT_NE(link, nullptr);

        string response;
        uint32_t result = link->Invoke("echo", R"({"input":"linked"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("linked"), string::npos) << "response: " << response;

        delete link;
    }

    TEST_F(TestPluginTest, JSONRPC_GreetViaLink)
    {
        auto* link = _runtime.CreateJSONRPCLink("TestPlugin");
        ASSERT_NE(link, nullptr);

        string response;
        uint32_t result = link->Invoke("greet", R"({"name":"Link"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("Hello, Link!"), string::npos) << "response: " << response;

        delete link;
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
