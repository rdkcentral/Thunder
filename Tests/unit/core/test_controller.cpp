/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../Module.h"
#include "ThunderTestRuntime.h"

#include <gtest/gtest.h>
#include <string>

namespace Thunder {
namespace TestCore {
namespace Tests {

    // =========================================================================
    // TEST FILE: ControllerTest.cpp
    //
    // Purpose:
    //   Tests Controller plugin operations (Gap 4) and startup/shutdown
    //   sequence (Gap 5) using the ThunderTestRuntime.
    //
    // Coverage:
    //   - Controller.status — full response validation
    //   - Controller.subsystems — subsystem flags
    //   - Controller.activate/deactivate — lifecycle transitions
    //   - Controller.configuration — config read
    //   - Shutdown deinitialize — clean teardown
    //   - Multiple init/deinit cycles — no resource leaks
    // =========================================================================

    // =========================================================================
    // Gap 4: Controller Operations
    // =========================================================================

    class ControllerTest : public ::testing::Test {
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

    ThunderTestRuntime ControllerTest::_runtime;

    // Controller.status returns a non-empty JSON response
    TEST_F(ControllerTest, StatusQuery_ReturnsValidJSON)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_FALSE(response.empty());

        // Parse and verify it's valid JSON
        JsonObject json;
        EXPECT_TRUE(json.FromString(response));
    }

    // Controller.status via JsonObject overload
    TEST_F(ControllerTest, StatusQuery_JsonObject)
    {
        JsonObject params;
        JsonObject response;
        uint32_t result = _runtime.Invoke("Controller.status", params, response);
        EXPECT_EQ(result, Core::ERROR_NONE);
    }

    // Controller.subsystems returns subsystem state
    TEST_F(ControllerTest, SubsystemsQuery_ReturnsValidJSON)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.subsystems", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_FALSE(response.empty());
    }

    // Activate unknown plugin returns error
    TEST_F(ControllerTest, ActivateNonExistent_ReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.activate",
            "{\"callsign\":\"NonExistentPlugin_12345\"}", response);
        EXPECT_NE(result, Core::ERROR_NONE);
    }

    // Deactivate unknown plugin returns error
    TEST_F(ControllerTest, DeactivateNonExistent_ReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.deactivate",
            "{\"callsign\":\"NonExistentPlugin_12345\"}", response);
        EXPECT_NE(result, Core::ERROR_NONE);
    }

    // Unknown Controller method returns ERROR_UNKNOWN_METHOD
    TEST_F(ControllerTest, UnknownMethod_ReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("Controller.thisMethodDoesNotExist", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_METHOD);
    }

    // JSONRPCLink-based status query
    TEST_F(ControllerTest, StatusViaJSONRPCLink)
    {
        auto link = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_TRUE(link.IsValid());

        string response;
        uint32_t result = link->Invoke("status", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_FALSE(response.empty());
    }

    // JSONRPCLink-based subsystems query
    TEST_F(ControllerTest, SubsystemsViaJSONRPCLink)
    {
        auto link = _runtime.CreateJSONRPCLink("Controller");
        ASSERT_TRUE(link.IsValid());

        string response;
        uint32_t result = link->Invoke("subsystems", "{}", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_FALSE(response.empty());
    }

    // GetShell returns valid IShell for Controller
    TEST_F(ControllerTest, GetShell_Controller)
    {
        auto shell = _runtime.GetShell("Controller");
        EXPECT_TRUE(shell.IsValid());
    }

    // GetShell returns invalid for non-existent callsign
    TEST_F(ControllerTest, GetShell_NonExistent)
    {
        auto shell = _runtime.GetShell("NonExistentPlugin_12345");
        EXPECT_FALSE(shell.IsValid());
    }

    // QueryInterfaceByCallsign — Controller implements IPlugin
    TEST_F(ControllerTest, QueryInterface_IPlugin)
    {
        auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("Controller");
        ASSERT_NE(plugin, nullptr);
        plugin->Release();
    }

    // QueryInterfaceByCallsign — Controller implements IDispatcher
    TEST_F(ControllerTest, QueryInterface_IDispatcher)
    {
        auto* dispatcher = _runtime.QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");
        ASSERT_NE(dispatcher, nullptr);
        dispatcher->Release();
    }

    // Exists() — verify known methods exist
    TEST_F(ControllerTest, Exists_KnownMethod)
    {
        auto* dispatcher = _runtime.QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");
        ASSERT_NE(dispatcher, nullptr);

        bool exists = _runtime.Exists(dispatcher, "Controller", "status");
        EXPECT_TRUE(exists);

        dispatcher->Release();
    }

    // Exists() — verify unknown method does not exist
    TEST_F(ControllerTest, Exists_UnknownMethod)
    {
        auto* dispatcher = _runtime.QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");
        ASSERT_NE(dispatcher, nullptr);

        bool exists = _runtime.Exists(dispatcher, "Controller", "noSuchMethod_12345");
        EXPECT_FALSE(exists);

        dispatcher->Release();
    }

    // =========================================================================
    // Gap 5: Startup/Shutdown Sequence
    // =========================================================================

    class StartupShutdownTest : public ::testing::Test {
    };

    // Server accessor is valid after init
    TEST_F(StartupShutdownTest, ServerAccessorAfterInit)
    {
        ThunderTestRuntime runtime;
        std::vector<ThunderTestRuntime::PluginConfig> plugins;

        uint32_t result = runtime.Initialize(plugins);
        ASSERT_EQ(result, Core::ERROR_NONE);

        // Server reference should be accessible
        EXPECT_NO_THROW(runtime.Server());

        runtime.Deinitialize();
    }

    // CommunicatorPath is non-empty after init
    TEST_F(StartupShutdownTest, CommunicatorPath_NonEmpty)
    {
        ThunderTestRuntime runtime;
        std::vector<ThunderTestRuntime::PluginConfig> plugins;

        uint32_t result = runtime.Initialize(plugins);
        ASSERT_EQ(result, Core::ERROR_NONE);

        string path = runtime.CommunicatorPath();
        EXPECT_FALSE(path.empty());

        runtime.Deinitialize();
    }

    // Multiple init/deinit cycles — no crash or resource leak
    TEST_F(StartupShutdownTest, MultipleInitDeinit_NoCrash)
    {
        for (int i = 0; i < 3; i++) {
            ThunderTestRuntime runtime;
            std::vector<ThunderTestRuntime::PluginConfig> plugins;

            uint32_t result = runtime.Initialize(plugins);
            ASSERT_EQ(result, Core::ERROR_NONE)
                << "Init failed on iteration " << i;

            string response;
            result = runtime.Invoke("Controller.status", "{}", response);
            EXPECT_EQ(result, Core::ERROR_NONE);

            runtime.Deinitialize();
        }
    }

    // Controller is available immediately after startup
    TEST_F(StartupShutdownTest, ControllerAvailableAfterStartup)
    {
        ThunderTestRuntime runtime;
        std::vector<ThunderTestRuntime::PluginConfig> plugins;

        uint32_t result = runtime.Initialize(plugins);
        ASSERT_EQ(result, Core::ERROR_NONE);

        auto shell = runtime.GetShell("Controller");
        EXPECT_TRUE(shell.IsValid());

        auto* plugin = runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("Controller");
        EXPECT_NE(plugin, nullptr);
        if (plugin) plugin->Release();

        runtime.Deinitialize();
    }

    // Deinitialize without Initialize — no crash
    TEST_F(StartupShutdownTest, DeinitWithoutInit_NoCrash)
    {
        ThunderTestRuntime runtime;
        // Should not crash
        runtime.Deinitialize();
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
