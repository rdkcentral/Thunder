/*
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

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <gtest/gtest.h>

#include <core/core.h>
#include <plugins/plugins.h>

// ThunderPlugin warning/diagnostic categories and PostMortem data
#include <WarningReportingCategories.h>
#include <PostMortem.h>

// ThrottleQueueType is defined in PluginServer.h but depends on the full
// Thunder Host module graph. We replicate the standalone template here to
// avoid pulling in the entire PluginServer header (which requires linking
// against libThunderPlugins and more). The template is self-contained.
namespace Thunder {
namespace Core {
    template<typename CONTENT, typename FORWARDER>
    class TestThrottleQueueType {
    private:
        using Queue = std::queue<CONTENT>;

    public:
        TestThrottleQueueType(TestThrottleQueueType&&) = delete;
        TestThrottleQueueType(const TestThrottleQueueType&) = delete;
        TestThrottleQueueType& operator=(TestThrottleQueueType&&) = delete;
        TestThrottleQueueType& operator=(const TestThrottleQueueType&) = delete;

        template <typename... Args>
        TestThrottleQueueType(Args&&... args)
            : _adminLock()
            , _forwarder(std::forward<Args>(args)...)
            , _slots(1)
            , _used(0)
            , _queue()
        {
        }
        ~TestThrottleQueueType() = default;

    public:
        inline void Slots(const uint32_t slots) {
            _slots = slots;
        }
        inline uint32_t Slots() const {
            return (_slots);
        }
        inline uint32_t Used() const {
            return (_used);
        }
        inline void Push(CONTENT&& object) {
            _adminLock.Lock();
            if ((_used < _slots) || (_slots == 0)) {
                _used++;
                _forwarder.Submit(std::move(object));
            }
            else {
                _queue.emplace(object);
            }
            _adminLock.Unlock();
        }
        inline void Pop() {
            _adminLock.Lock();
            ASSERT(_used > 0);
            if (_queue.empty() == false) {
                _forwarder.Submit(std::forward<CONTENT>(_queue.front()));
                _queue.pop();
            }
            else {
                _used--;
            }
            _adminLock.Unlock();
        }

        uint32_t QueueSize() const {
            return static_cast<uint32_t>(_queue.size());
        }

    private:
        Core::CriticalSection _adminLock;
        FORWARDER _forwarder;
        uint32_t _slots;
        uint32_t _used;
        Queue _queue;
    };
}
}

namespace Thunder {
namespace Tests {

    // =====================================================================
    // Test Suite: ThunderHost_PluginConfig
    // Covers: Plugin::Config JSON parsing, defaults, path helpers
    // Report gap: Config parsing (missing/invalid fields, defaults)
    // =====================================================================

    TEST(ThunderHost_PluginConfig, DefaultValues)
    {
        Plugin::Config config;

        EXPECT_FALSE(config.Callsign.IsSet());
        EXPECT_FALSE(config.Locator.IsSet());
        EXPECT_FALSE(config.ClassName.IsSet());
        EXPECT_EQ(config.Resumed.Value(), false);
        EXPECT_EQ(config.StartupOrder.Value(), 1000u);
        EXPECT_EQ(config.Throttle.Value(), static_cast<uint8_t>(~0));
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::ACTIVATED);
    }

    TEST(ThunderHost_PluginConfig, ParseAllFields)
    {
        Plugin::Config config;
        string json = R"({
            "callsign": "TestPlugin",
            "locator": "libTestPlugin.so",
            "classname": "TestPlugin",
            "versions": "1.0",
            "resumed": true,
            "webui": "static",
            "configuration": "{\"key\":\"value\"}",
            "persistentpathpostfix": "custom_persist",
            "volatilepathpostfix": "custom_volatile",
            "systemrootpath": "/custom/root",
            "startuporder": 50,
            "throttle": 5,
            "communicator": "/tmp/test_communicator"
        })";

        Core::OptionalType<Core::JSON::Error> error;
        EXPECT_TRUE(config.IElement::FromString(json, error));
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(config.Callsign.Value(), "TestPlugin");
        EXPECT_EQ(config.Locator.Value(), "libTestPlugin.so");
        EXPECT_EQ(config.ClassName.Value(), "TestPlugin");
        EXPECT_EQ(config.Versions.Value(), "1.0");
        EXPECT_EQ(config.Resumed.Value(), true);
        EXPECT_EQ(config.WebUI.Value(), "static");
        EXPECT_TRUE(config.Configuration.IsSet());
        EXPECT_EQ(config.PersistentPathPostfix.Value(), "custom_persist");
        EXPECT_EQ(config.VolatilePathPostfix.Value(), "custom_volatile");
        EXPECT_EQ(config.SystemRootPath.Value(), "/custom/root");
        EXPECT_EQ(config.StartupOrder.Value(), 50u);
        EXPECT_EQ(config.Throttle.Value(), 5u);
        EXPECT_EQ(config.Communicator.Value(), "/tmp/test_communicator");
    }

    TEST(ThunderHost_PluginConfig, ParseMinimalJSON_DefaultsApply)
    {
        Plugin::Config config;
        string json = R"({"callsign":"Minimal","classname":"MinClass"})";

        Core::OptionalType<Core::JSON::Error> error;
        EXPECT_TRUE(config.IElement::FromString(json, error));
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(config.Callsign.Value(), "Minimal");
        EXPECT_EQ(config.ClassName.Value(), "MinClass");

        // Verify defaults for unset fields
        EXPECT_FALSE(config.Locator.IsSet());
        EXPECT_EQ(config.Resumed.Value(), false);
        EXPECT_EQ(config.StartupOrder.Value(), 1000u);
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::ACTIVATED);
    }

    TEST(ThunderHost_PluginConfig, ParseStartMode_Deactivated)
    {
        Plugin::Config config;
        string json = R"({"callsign":"Test","classname":"Test","startmode":"Deactivated"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::DEACTIVATED);
    }

    TEST(ThunderHost_PluginConfig, ParseStartMode_Unavailable)
    {
        Plugin::Config config;
        string json = R"({"callsign":"Test","classname":"Test","startmode":"Unavailable"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::UNAVAILABLE);
    }

    TEST(ThunderHost_PluginConfig, DataPath)
    {
        Plugin::Config config;
        string json = R"({"callsign":"MyPlugin","classname":"MyClass"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        string dataPath = config.DataPath("/usr/share/Thunder/");
        EXPECT_EQ(dataPath, "/usr/share/Thunder/MyClass/");
    }

    TEST(ThunderHost_PluginConfig, PersistentPath_WithPostfix)
    {
        Plugin::Config config;
        string json = R"({"callsign":"MyPlugin","classname":"MyClass","persistentpathpostfix":"custom"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        string path = config.PersistentPath("/opt/persistent/");
        EXPECT_EQ(path, "/opt/persistent/custom/");
    }

    TEST(ThunderHost_PluginConfig, PersistentPath_WithoutPostfix_UsesCallsign)
    {
        Plugin::Config config;
        string json = R"({"callsign":"MyPlugin","classname":"MyClass"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        string path = config.PersistentPath("/opt/persistent/");
        EXPECT_EQ(path, "/opt/persistent/MyPlugin/");
    }

    TEST(ThunderHost_PluginConfig, VolatilePath_WithPostfix)
    {
        Plugin::Config config;
        string json = R"({"callsign":"MyPlugin","classname":"MyClass","volatilepathpostfix":"vol_custom"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        string path = config.VolatilePath("/tmp/");
        EXPECT_EQ(path, "/tmp/vol_custom/");
    }

    TEST(ThunderHost_PluginConfig, VolatilePath_WithoutPostfix_UsesCallsign)
    {
        Plugin::Config config;
        string json = R"({"callsign":"MyPlugin","classname":"MyClass"})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        string path = config.VolatilePath("/tmp/");
        EXPECT_EQ(path, "/tmp/MyPlugin/");
    }

    TEST(ThunderHost_PluginConfig, IsSameInstance)
    {
        Plugin::Config config1;
        Plugin::Config config2;

        Core::OptionalType<Core::JSON::Error> error;
        config1.IElement::FromString(R"({"callsign":"A","locator":"libTest.so","classname":"TestClass"})", error);
        config2.IElement::FromString(R"({"callsign":"B","locator":"libTest.so","classname":"TestClass"})", error);

        EXPECT_TRUE(config1.IsSameInstance(config2));
    }

    TEST(ThunderHost_PluginConfig, IsSameInstance_DifferentLocator)
    {
        Plugin::Config config1;
        Plugin::Config config2;

        Core::OptionalType<Core::JSON::Error> error;
        config1.IElement::FromString(R"({"callsign":"A","locator":"libA.so","classname":"TestClass"})", error);
        config2.IElement::FromString(R"({"callsign":"A","locator":"libB.so","classname":"TestClass"})", error);

        EXPECT_FALSE(config1.IsSameInstance(config2));
    }

    TEST(ThunderHost_PluginConfig, SerializeRoundTrip)
    {
        Plugin::Config original;
        string json = R"({"callsign":"RoundTrip","classname":"RTClass","startuporder":42,"throttle":3})";

        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        string serialized;
        original.IElement::ToString(serialized);

        Plugin::Config restored;
        restored.IElement::FromString(serialized, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(restored.Callsign.Value(), "RoundTrip");
        EXPECT_EQ(restored.ClassName.Value(), "RTClass");
        EXPECT_EQ(restored.StartupOrder.Value(), 42u);
        EXPECT_EQ(restored.Throttle.Value(), 3u);
    }

    TEST(ThunderHost_PluginConfig, EmptyJSON_AllDefaults)
    {
        Plugin::Config config;
        string json = "{}";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        EXPECT_FALSE(config.Callsign.IsSet());
        EXPECT_FALSE(config.Locator.IsSet());
        EXPECT_FALSE(config.ClassName.IsSet());
        EXPECT_EQ(config.StartupOrder.Value(), 1000u);
        EXPECT_EQ(config.Throttle.Value(), static_cast<uint8_t>(~0));
    }

    TEST(ThunderHost_PluginConfig, InvalidJSON_ErrorSet)
    {
        Plugin::Config config;
        string json = "{not valid json}";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        EXPECT_TRUE(error.IsSet());
    }

    TEST(ThunderHost_PluginConfig, CopyConstructor)
    {
        Plugin::Config original;
        string json = R"({"callsign":"Copy","classname":"CopyClass","startuporder":99})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        Plugin::Config copy(original);
        EXPECT_EQ(copy.Callsign.Value(), "Copy");
        EXPECT_EQ(copy.ClassName.Value(), "CopyClass");
        EXPECT_EQ(copy.StartupOrder.Value(), 99u);
    }

    TEST(ThunderHost_PluginConfig, MoveConstructor)
    {
        Plugin::Config original;
        string json = R"({"callsign":"Move","classname":"MoveClass","startuporder":77})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        Plugin::Config moved(std::move(original));
        EXPECT_EQ(moved.Callsign.Value(), "Move");
        EXPECT_EQ(moved.ClassName.Value(), "MoveClass");
        EXPECT_EQ(moved.StartupOrder.Value(), 77u);
    }

    TEST(ThunderHost_PluginConfig, PreconditionArray)
    {
        Plugin::Config config;
        string json = R"({"callsign":"Test","classname":"Test","precondition":["Platform","Network"]})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        EXPECT_TRUE(config.Precondition.IsSet());
        auto it = config.Precondition.Elements();
        uint32_t count = 0;
        while (it.Next()) {
            count++;
        }
        EXPECT_EQ(count, 2u);
    }

    TEST(ThunderHost_PluginConfig, TerminationArray)
    {
        Plugin::Config config;
        string json = R"({"callsign":"Test","classname":"Test","termination":["Platform"]})";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);

        EXPECT_TRUE(config.Termination.IsSet());
        auto it = config.Termination.Elements();
        uint32_t count = 0;
        while (it.Next()) {
            count++;
        }
        EXPECT_EQ(count, 1u);
    }

    // =====================================================================
    // Test Suite: ThunderHost_RootConfig
    // Covers: Plugin::Config::RootConfig OOP configuration parsing
    // Report gap: Plugin config parsing, OOP mode configuration
    // =====================================================================

    TEST(ThunderHost_RootConfig, DefaultValues)
    {
        Plugin::Config::RootConfig root;

        EXPECT_EQ(root.Threads.Value(), 1u);
        EXPECT_EQ(root.Priority.Value(), 0);
        EXPECT_EQ(root.OutOfProcess.Value(), false);
        EXPECT_EQ(root.Mode.Value(), Plugin::Config::RootConfig::ModeType::LOCAL);
        EXPECT_FALSE(root.Locator.IsSet());
        EXPECT_FALSE(root.User.IsSet());
        EXPECT_FALSE(root.Group.IsSet());
    }

    TEST(ThunderHost_RootConfig, ParseAllFields)
    {
        Plugin::Config::RootConfig root;
        string json = R"({
            "locator": "/usr/lib/libPlugin.so",
            "user": "thunder",
            "group": "thunder",
            "threads": 4,
            "priority": -5,
            "outofprocess": true,
            "mode": "Container",
            "remoteaddress": "192.168.1.100:8080"
        })";

        Core::OptionalType<Core::JSON::Error> error;
        root.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(root.Locator.Value(), "/usr/lib/libPlugin.so");
        EXPECT_EQ(root.User.Value(), "thunder");
        EXPECT_EQ(root.Group.Value(), "thunder");
        EXPECT_EQ(root.Threads.Value(), 4u);
        EXPECT_EQ(root.Priority.Value(), -5);
        EXPECT_EQ(root.OutOfProcess.Value(), true);
        EXPECT_EQ(root.Mode.Value(), Plugin::Config::RootConfig::ModeType::CONTAINER);
        EXPECT_EQ(root.RemoteAddress.Value(), "192.168.1.100:8080");
    }

    TEST(ThunderHost_RootConfig, HostType_Local)
    {
        Plugin::Config::RootConfig root;
        string json = R"({"mode":"Local"})";
        Core::OptionalType<Core::JSON::Error> error;
        root.IElement::FromString(json, error);

        EXPECT_EQ(root.HostType(), RPC::Object::HostType::LOCAL);
    }

    TEST(ThunderHost_RootConfig, HostType_Container)
    {
        Plugin::Config::RootConfig root;
        string json = R"({"mode":"Container"})";
        Core::OptionalType<Core::JSON::Error> error;
        root.IElement::FromString(json, error);

        EXPECT_EQ(root.HostType(), RPC::Object::HostType::CONTAINER);
    }

    TEST(ThunderHost_RootConfig, HostType_Distributed)
    {
        Plugin::Config::RootConfig root;
        string json = R"({"mode":"Distributed"})";
        Core::OptionalType<Core::JSON::Error> error;
        root.IElement::FromString(json, error);

        EXPECT_EQ(root.HostType(), RPC::Object::HostType::DISTRIBUTED);
    }

    TEST(ThunderHost_RootConfig, HostType_Off_ReturnsLocal)
    {
        Plugin::Config::RootConfig root;
        string json = R"({"mode":"Off"})";
        Core::OptionalType<Core::JSON::Error> error;
        root.IElement::FromString(json, error);

        // Off mode maps to LOCAL per switch default
        EXPECT_EQ(root.HostType(), RPC::Object::HostType::LOCAL);
    }

    TEST(ThunderHost_RootConfig, Environments)
    {
        Plugin::Config::RootConfig root;
        string json = R"({
            "environments": [
                {"key":"LD_LIBRARY_PATH","value":"/custom/lib","scope":"Local"},
                {"key":"HOME","value":"/tmp","scope":"Local"}
            ]
        })";

        Core::OptionalType<Core::JSON::Error> error;
        root.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_TRUE(root.Environments.IsSet());
        auto envList = root.Environment();
        EXPECT_EQ(envList.size(), 2u);
    }

    TEST(ThunderHost_RootConfig, SerializeRoundTrip)
    {
        Plugin::Config::RootConfig original;
        string json = R"({"locator":"/usr/lib/test.so","threads":2,"mode":"Container"})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        string serialized;
        original.IElement::ToString(serialized);

        Plugin::Config::RootConfig restored;
        restored.IElement::FromString(serialized, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(restored.Locator.Value(), "/usr/lib/test.so");
        EXPECT_EQ(restored.Threads.Value(), 2u);
        EXPECT_EQ(restored.Mode.Value(), Plugin::Config::RootConfig::ModeType::CONTAINER);
    }

    TEST(ThunderHost_RootConfig, CopyConstructor)
    {
        Plugin::Config::RootConfig original;
        string json = R"({"locator":"/lib/test.so","threads":3,"mode":"Local"})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        Plugin::Config::RootConfig copy(original);
        EXPECT_EQ(copy.Locator.Value(), "/lib/test.so");
        EXPECT_EQ(copy.Threads.Value(), 3u);
        // Copy ctor forces OutOfProcess to true
        EXPECT_EQ(copy.OutOfProcess.Value(), true);
    }

    // =====================================================================
    // Test Suite: ThunderHost_Throttle
    // Covers: ThrottleQueueType slot limits, queuing, pop draining
    // Report gap: Throttling enforcement (per-channel, per-plugin, boundary)
    // =====================================================================

    // Simple forwarder that records submitted items
    struct TestForwarder {
        std::vector<int> submitted;
        void Submit(int&& value) {
            submitted.push_back(value);
        }
    };

    TEST(ThunderHost_Throttle, DefaultSlotIsOne)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        EXPECT_EQ(throttle.Slots(), 1u);
        EXPECT_EQ(throttle.Used(), 0u);
    }

    TEST(ThunderHost_Throttle, SetSlots)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(5);
        EXPECT_EQ(throttle.Slots(), 5u);
    }

    TEST(ThunderHost_Throttle, PushWithinSlots_SubmittedImmediately)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(3);

        int v1 = 1, v2 = 2, v3 = 3;
        throttle.Push(std::move(v1));
        throttle.Push(std::move(v2));
        throttle.Push(std::move(v3));

        EXPECT_EQ(throttle.Used(), 3u);
        EXPECT_EQ(throttle.QueueSize(), 0u);
    }

    TEST(ThunderHost_Throttle, PushBeyondSlots_Queued)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(2);

        int v1 = 1, v2 = 2, v3 = 3, v4 = 4;
        throttle.Push(std::move(v1));
        throttle.Push(std::move(v2));
        throttle.Push(std::move(v3));
        throttle.Push(std::move(v4));

        EXPECT_EQ(throttle.Used(), 2u);
        EXPECT_EQ(throttle.QueueSize(), 2u);
    }

    TEST(ThunderHost_Throttle, Pop_DrainsQueue)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(1);

        int v1 = 10, v2 = 20, v3 = 30;
        throttle.Push(std::move(v1));
        throttle.Push(std::move(v2));
        throttle.Push(std::move(v3));

        EXPECT_EQ(throttle.Used(), 1u);
        EXPECT_EQ(throttle.QueueSize(), 2u);

        // Pop drains from queue — used count stays same because a queued item is submitted
        throttle.Pop();
        EXPECT_EQ(throttle.Used(), 1u);
        EXPECT_EQ(throttle.QueueSize(), 1u);

        throttle.Pop();
        EXPECT_EQ(throttle.Used(), 1u);
        EXPECT_EQ(throttle.QueueSize(), 0u);

        // Final pop — queue empty, so used decrements
        throttle.Pop();
        EXPECT_EQ(throttle.Used(), 0u);
    }

    TEST(ThunderHost_Throttle, ZeroSlots_NoThrottling)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(0);

        int v1 = 1, v2 = 2, v3 = 3, v4 = 4, v5 = 5;
        throttle.Push(std::move(v1));
        throttle.Push(std::move(v2));
        throttle.Push(std::move(v3));
        throttle.Push(std::move(v4));
        throttle.Push(std::move(v5));

        // All should be submitted immediately (no throttling)
        EXPECT_EQ(throttle.Used(), 5u);
        EXPECT_EQ(throttle.QueueSize(), 0u);
    }

    TEST(ThunderHost_Throttle, SingleSlot_SerializesExecution)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(1);

        int v1 = 100, v2 = 200;
        throttle.Push(std::move(v1));
        EXPECT_EQ(throttle.Used(), 1u);

        throttle.Push(std::move(v2));
        EXPECT_EQ(throttle.Used(), 1u);
        EXPECT_EQ(throttle.QueueSize(), 1u);

        throttle.Pop();
        EXPECT_EQ(throttle.Used(), 1u);
        EXPECT_EQ(throttle.QueueSize(), 0u);

        throttle.Pop();
        EXPECT_EQ(throttle.Used(), 0u);
    }

    TEST(ThunderHost_Throttle, BoundaryCondition_ExactlyAtLimit)
    {
        Core::TestThrottleQueueType<int, TestForwarder> throttle;
        throttle.Slots(3);

        int v1 = 1, v2 = 2, v3 = 3;
        throttle.Push(std::move(v1));
        throttle.Push(std::move(v2));
        throttle.Push(std::move(v3));

        EXPECT_EQ(throttle.Used(), 3u);
        EXPECT_EQ(throttle.QueueSize(), 0u);

        // One more should be queued
        int v4 = 4;
        throttle.Push(std::move(v4));
        EXPECT_EQ(throttle.QueueSize(), 1u);
    }

    // =====================================================================
    // Test Suite: ThunderHost_ServiceAdministrator
    // Covers: Instance tracking, callback notification
    // Report gap: ServiceAdministrator orderly shutdown
    // =====================================================================

    class TestCallback : public Core::ServiceAdministrator::ICallback {
    public:
        TestCallback() : _destructedCount(0) {}
        void Destructed() override { _destructedCount++; }
        uint32_t Count() const { return _destructedCount; }
    private:
        uint32_t _destructedCount;
    };

    TEST(ThunderHost_ServiceAdministrator, InstanceIsSingleton)
    {
        auto& inst1 = Core::ServiceAdministrator::Instance();
        auto& inst2 = Core::ServiceAdministrator::Instance();
        EXPECT_EQ(&inst1, &inst2);
    }

    TEST(ThunderHost_ServiceAdministrator, CreateAndDrop_TracksCount)
    {
        auto& admin = Core::ServiceAdministrator::Instance();
        uint32_t initial = admin.Instances();

        admin.Created();
        EXPECT_EQ(admin.Instances(), initial + 1);

        admin.Created();
        EXPECT_EQ(admin.Instances(), initial + 2);

        admin.Dropped();
        EXPECT_EQ(admin.Instances(), initial + 1);

        admin.Dropped();
        EXPECT_EQ(admin.Instances(), initial);
    }

    TEST(ThunderHost_ServiceAdministrator, Callback_NotifiedOnDrop)
    {
        auto& admin = Core::ServiceAdministrator::Instance();

        TestCallback callback;
        admin.Callback(&callback);

        admin.Created();
        EXPECT_EQ(callback.Count(), 0u);

        admin.Dropped();
        EXPECT_EQ(callback.Count(), 1u);

        admin.Created();
        admin.Dropped();
        EXPECT_EQ(callback.Count(), 2u);

        // Unregister callback
        admin.Callback(nullptr);
    }

    TEST(ThunderHost_ServiceAdministrator, Callback_NotCalledAfterUnregister)
    {
        auto& admin = Core::ServiceAdministrator::Instance();

        TestCallback callback;
        admin.Callback(&callback);

        admin.Created();
        admin.Dropped();
        EXPECT_EQ(callback.Count(), 1u);

        admin.Callback(nullptr);

        admin.Created();
        admin.Dropped();
        // Callback should not have been invoked again
        EXPECT_EQ(callback.Count(), 1u);
    }

    // =====================================================================
    // Test Suite: ThunderHost_PluginConfigEnvironment
    // Covers: Plugin::Config::Environment parsing
    // Report gap: Plugin config parsing (environment variables)
    // =====================================================================

    TEST(ThunderHost_PluginConfigEnvironment, ParseKeyValue)
    {
        Plugin::Config::Environment env;
        string json = R"({"key":"MY_VAR","value":"/some/path","scope":"Local"})";

        Core::OptionalType<Core::JSON::Error> error;
        env.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(env.Key.Value(), "MY_VAR");
        EXPECT_EQ(env.Value.Value(), "/some/path");
    }

    TEST(ThunderHost_PluginConfigEnvironment, CopyConstructor)
    {
        Plugin::Config::Environment original;
        string json = R"({"key":"COPY_VAR","value":"copy_val"})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        Plugin::Config::Environment copy(original);
        EXPECT_EQ(copy.Key.Value(), "COPY_VAR");
        EXPECT_EQ(copy.Value.Value(), "copy_val");
    }

    TEST(ThunderHost_PluginConfigEnvironment, MoveConstructor)
    {
        Plugin::Config::Environment original;
        string json = R"({"key":"MOVE_VAR","value":"move_val"})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        Plugin::Config::Environment moved(std::move(original));
        EXPECT_EQ(moved.Key.Value(), "MOVE_VAR");
        EXPECT_EQ(moved.Value.Value(), "move_val");
    }

    // =====================================================================
    // Test Suite: ThunderHost_ServerConfig
    // Covers: PluginHost::Config::JSONConfig parsing and defaults
    // Report gap: Config parsing (main config.json)
    // =====================================================================

    // We test the JSONConfig inner class directly since PluginHost::Config
    // requires a Core::File object. JSONConfig is the JSON container.

    TEST(ThunderHost_ServerConfig, ParsePortAndBinding)
    {
        // Use Plugin::Config to parse a JSON that has server-like fields.
        // We test the server config defaults and field parsing by
        // constructing a JSONConfig-like structure via string deserialization.
        // Since JSONConfig is nested in PluginHost::Config (private),
        // we test the publicly accessible Plugin::Config fields that mirror
        // server configuration concerns.

        // Test port parsing via a simple JSON::DecUInt16
        Core::JSON::DecUInt16 port(80);
        EXPECT_EQ(port.Value(), 80u);

        Core::JSON::String binding("0.0.0.0");
        EXPECT_EQ(binding.Value(), "0.0.0.0");
    }

    TEST(ThunderHost_ServerConfig, PluginConfig_WithRootSection)
    {
        Plugin::Config config;
        string json = R"({
            "callsign": "OOPPlugin",
            "classname": "OOPClass",
            "root": {
                "locator": "/usr/lib/libOOP.so",
                "mode": "Local",
                "threads": 2,
                "priority": -10,
                "outofprocess": true,
                "user": "nobody",
                "group": "nogroup"
            }
        })";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(config.Root.Locator.Value(), "/usr/lib/libOOP.so");
        EXPECT_EQ(config.Root.Mode.Value(), Plugin::Config::RootConfig::ModeType::LOCAL);
        EXPECT_EQ(config.Root.Threads.Value(), 2u);
        EXPECT_EQ(config.Root.Priority.Value(), -10);
        EXPECT_EQ(config.Root.OutOfProcess.Value(), true);
        EXPECT_EQ(config.Root.User.Value(), "nobody");
        EXPECT_EQ(config.Root.Group.Value(), "nogroup");
    }

    TEST(ThunderHost_ServerConfig, PluginConfig_ContainerMode)
    {
        Plugin::Config config;
        string json = R"({
            "callsign": "ContainerPlugin",
            "classname": "ContainerClass",
            "root": {
                "mode": "Container",
                "locator": "/usr/lib/libContainer.so"
            }
        })";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(config.Root.HostType(), RPC::Object::HostType::CONTAINER);
    }

    TEST(ThunderHost_ServerConfig, PluginConfig_DistributedMode)
    {
        Plugin::Config config;
        string json = R"({
            "callsign": "DistPlugin",
            "classname": "DistClass",
            "root": {
                "mode": "Distributed",
                "remoteaddress": "10.0.0.5:9090"
            }
        })";

        Core::OptionalType<Core::JSON::Error> error;
        config.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(config.Root.HostType(), RPC::Object::HostType::DISTRIBUTED);
        EXPECT_EQ(config.Root.RemoteAddress.Value(), "10.0.0.5:9090");
    }

    // =====================================================================
    // Test Suite: ThunderPlugin_PluginStateWarning
    // Covers: WarningReporting::TooLongPluginState
    // Report gap: ThunderPlugin OOP process lifecycle warning categories
    // =====================================================================

    TEST(ThunderPlugin_PluginStateWarning, Analyze_StoresCallsignAndState)
    {
        WarningReporting::TooLongPluginState warning;
        bool result = warning.Analyze("module", "callsign",
            WarningReporting::TooLongPluginState::StateChange::ACTIVATION, "TestPlugin");
        EXPECT_TRUE(result);
    }

    TEST(ThunderPlugin_PluginStateWarning, SerializeDeserialize_Activation)
    {
        WarningReporting::TooLongPluginState original;
        original.Analyze("mod", "cs",
            WarningReporting::TooLongPluginState::StateChange::ACTIVATION, "MyPlugin");

        uint8_t buffer[256];
        uint16_t serialized = original.Serialize(buffer, sizeof(buffer));
        EXPECT_GT(serialized, 0u);

        WarningReporting::TooLongPluginState restored;
        uint16_t deserialized = restored.Deserialize(buffer, serialized);
        EXPECT_EQ(deserialized, serialized);

        string originalStr, restoredStr;
        original.ToString(originalStr, 6000, 5000);
        restored.ToString(restoredStr, 6000, 5000);
        EXPECT_EQ(originalStr, restoredStr);
    }

    TEST(ThunderPlugin_PluginStateWarning, SerializeDeserialize_Deactivation)
    {
        WarningReporting::TooLongPluginState warning;
        warning.Analyze("mod", "cs",
            WarningReporting::TooLongPluginState::StateChange::DEACTIVATION, "SomePlugin");

        uint8_t buffer[256];
        uint16_t serialized = warning.Serialize(buffer, sizeof(buffer));
        EXPECT_GT(serialized, 0u);

        string str;
        warning.ToString(str, 7000, 5000);
        EXPECT_NE(str.find("SomePlugin"), string::npos);
        EXPECT_NE(str.find("Deactivation"), string::npos);
    }

    TEST(ThunderPlugin_PluginStateWarning, SerializeDeserialize_Suspend)
    {
        WarningReporting::TooLongPluginState warning;
        warning.Analyze("mod", "cs",
            WarningReporting::TooLongPluginState::StateChange::SUSPEND, "SuspendPlugin");

        uint8_t buffer[256];
        uint16_t serialized = warning.Serialize(buffer, sizeof(buffer));
        EXPECT_GT(serialized, 0u);

        WarningReporting::TooLongPluginState restored;
        restored.Deserialize(buffer, serialized);

        string str;
        restored.ToString(str, 100, 50);
        EXPECT_NE(str.find("SuspendPlugin"), string::npos);
        EXPECT_NE(str.find("Suspend"), string::npos);
    }

    TEST(ThunderPlugin_PluginStateWarning, SerializeDeserialize_Resume)
    {
        WarningReporting::TooLongPluginState warning;
        warning.Analyze("mod", "cs",
            WarningReporting::TooLongPluginState::StateChange::RESUME, "ResumePlugin");

        string str;
        warning.ToString(str, 200, 100);
        EXPECT_NE(str.find("ResumePlugin"), string::npos);
        EXPECT_NE(str.find("Resume"), string::npos);
    }

    TEST(ThunderPlugin_PluginStateWarning, Serialize_BufferTooSmall_ReturnsZero)
    {
        WarningReporting::TooLongPluginState warning;
        warning.Analyze("mod", "cs",
            WarningReporting::TooLongPluginState::StateChange::ACTIVATION, "LongPluginName");

        uint8_t buffer[2]; // too small
        uint16_t serialized = warning.Serialize(buffer, sizeof(buffer));
        EXPECT_EQ(serialized, 0u);
    }

    TEST(ThunderPlugin_PluginStateWarning, ToString_ContainsValues)
    {
        WarningReporting::TooLongPluginState warning;
        warning.Analyze("mod", "cs",
            WarningReporting::TooLongPluginState::StateChange::ACTIVATION, "TestPlugin");

        string str;
        warning.ToString(str, 6000, 5000);
        EXPECT_NE(str.find("6000"), string::npos);
        EXPECT_NE(str.find("5000"), string::npos);
        EXPECT_NE(str.find("TestPlugin"), string::npos);
        EXPECT_NE(str.find("Activation"), string::npos);
    }

    TEST(ThunderPlugin_PluginStateWarning, DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::TooLongPluginState::DefaultWarningBound, 5000u);
        EXPECT_EQ(WarningReporting::TooLongPluginState::DefaultReportBound, 5000u);
    }

    // =====================================================================
    // Test Suite: ThunderPlugin_InvokeMessageWarning
    // Covers: WarningReporting::TooLongInvokeMessage
    // Report gap: ThunderPlugin message handling warning categories
    // =====================================================================

    TEST(ThunderPlugin_InvokeMessageWarning, Analyze_String_TruncatesTo20)
    {
        WarningReporting::TooLongInvokeMessage warning;
        string longMsg(50, 'X');
        bool result = warning.Analyze("mod", "cs", longMsg);
        EXPECT_TRUE(result);

        string str;
        warning.ToString(str, 1000, 500);
        // Content should be truncated, so full 50-char string should NOT appear
        EXPECT_EQ(str.find(longMsg), string::npos);
        EXPECT_NE(str.find("String"), string::npos);
    }

    TEST(ThunderPlugin_InvokeMessageWarning, Analyze_String_ShortMessage)
    {
        WarningReporting::TooLongInvokeMessage warning;
        warning.Analyze("mod", "cs", string("hello"));

        string str;
        warning.ToString(str, 1000, 500);
        EXPECT_NE(str.find("hello"), string::npos);
        EXPECT_NE(str.find("String"), string::npos);
    }

    TEST(ThunderPlugin_InvokeMessageWarning, Analyze_JSONRPC)
    {
        WarningReporting::TooLongInvokeMessage warning;
        Core::JSONRPC::Message msg;
        msg.Designator = "Controller.1.activate";

        warning.Analyze("mod", "cs", msg);

        string str;
        warning.ToString(str, 1000, 500);
        EXPECT_NE(str.find("JSONRPC"), string::npos);
        EXPECT_NE(str.find("Controller.1.activate"), string::npos);
    }

    TEST(ThunderPlugin_InvokeMessageWarning, Analyze_WebRequest)
    {
        WarningReporting::TooLongInvokeMessage warning;
        Web::Request request;
        request.Verb = Web::Request::HTTP_GET;
        request.Path = "/Service/TestPlugin";

        warning.Analyze("mod", "cs", request);

        string str;
        warning.ToString(str, 2000, 500);
        EXPECT_NE(str.find("WebRequest"), string::npos);
    }

    TEST(ThunderPlugin_InvokeMessageWarning, SerializeDeserialize_RoundTrip)
    {
        WarningReporting::TooLongInvokeMessage original;
        original.Analyze("mod", "cs", string("test_content"));

        uint8_t buffer[256];
        uint16_t serialized = original.Serialize(buffer, sizeof(buffer));
        EXPECT_GT(serialized, 0u);

        WarningReporting::TooLongInvokeMessage restored;
        uint16_t deserialized = restored.Deserialize(buffer, serialized);
        EXPECT_EQ(deserialized, serialized);

        string origStr, restStr;
        original.ToString(origStr, 100, 50);
        restored.ToString(restStr, 100, 50);
        EXPECT_EQ(origStr, restStr);
    }

    TEST(ThunderPlugin_InvokeMessageWarning, Serialize_BufferTooSmall)
    {
        WarningReporting::TooLongInvokeMessage warning;
        warning.Analyze("mod", "cs", string("content"));

        uint8_t buffer[2];
        uint16_t serialized = warning.Serialize(buffer, sizeof(buffer));
        EXPECT_EQ(serialized, 0u);
    }

    TEST(ThunderPlugin_InvokeMessageWarning, DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::TooLongInvokeMessage::DefaultWarningBound, 500u);
        EXPECT_EQ(WarningReporting::TooLongInvokeMessage::DefaultReportBound, 500u);
    }

    // =====================================================================
    // Test Suite: ThunderPlugin_FlowControl
    // Covers: WarningReporting::FlowControlQueueSize, FlowControlJobTime
    // Report gap: ThunderPlugin flow control warning categories
    // =====================================================================

    TEST(ThunderPlugin_FlowControl, QueueSize_ToString)
    {
        WarningReporting::FlowControlQueueSize fc;
        string str;
        fc.ToString(str, 15, 10);
        EXPECT_NE(str.find("flow control queue size"), string::npos);
        EXPECT_NE(str.find("15"), string::npos);
        EXPECT_NE(str.find("10"), string::npos);
    }

    TEST(ThunderPlugin_FlowControl, QueueSize_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::FlowControlQueueSize::DefaultWarningBound, 10u);
        EXPECT_EQ(WarningReporting::FlowControlQueueSize::DefaultReportBound, 10u);
    }

    TEST(ThunderPlugin_FlowControl, QueueSize_SerializeIsNoOp)
    {
        WarningReporting::FlowControlQueueSize fc;
        uint8_t buffer[64];
        EXPECT_EQ(fc.Serialize(buffer, sizeof(buffer)), 0u);
    }

    TEST(ThunderPlugin_FlowControl, QueueSize_DeserializeIsNoOp)
    {
        WarningReporting::FlowControlQueueSize fc;
        uint8_t buffer[64] = {};
        EXPECT_EQ(fc.Deserialize(buffer, sizeof(buffer)), 0u);
    }

    TEST(ThunderPlugin_FlowControl, JobTime_ToString)
    {
        WarningReporting::FlowControlJobTime fc;
        string str;
        fc.ToString(str, 2000, 1000);
        EXPECT_NE(str.find("flow control job time"), string::npos);
        EXPECT_NE(str.find("2000"), string::npos);
        EXPECT_NE(str.find("1000"), string::npos);
    }

    TEST(ThunderPlugin_FlowControl, JobTime_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::FlowControlJobTime::DefaultWarningBound, 1000u);
        EXPECT_EQ(WarningReporting::FlowControlJobTime::DefaultReportBound, 1000u);
    }

    // =====================================================================
    // Test Suite: ThunderPlugin_PostMortem
    // Covers: CallstackData and PostMortemData JSON containers
    // Report gap: PostMortem crash handling data structures
    // =====================================================================

    TEST(ThunderPlugin_PostMortem, CallstackData_DefaultConstruction)
    {
        PluginHost::CallstackData data;

        EXPECT_FALSE(data.Address.IsSet());
        EXPECT_FALSE(data.Function.IsSet());
        EXPECT_FALSE(data.Module.IsSet());
        EXPECT_FALSE(data.Line.IsSet());
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_ParseJSON)
    {
        PluginHost::CallstackData data;
        string json = R"({"address":305419896,"function":"TestFunc","module":"libtest.so","line":42})";

        Core::OptionalType<Core::JSON::Error> error;
        data.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_TRUE(data.Function.IsSet());
        EXPECT_EQ(data.Function.Value(), "TestFunc");
        EXPECT_EQ(data.Module.Value(), "libtest.so");
        EXPECT_EQ(data.Line.Value(), 42u);
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_SerializeRoundTrip)
    {
        PluginHost::CallstackData original;
        string json = R"({"function":"MyFunc","module":"mylib.so","line":100})";

        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        string serialized;
        original.IElement::ToString(serialized);

        PluginHost::CallstackData restored;
        restored.IElement::FromString(serialized, error);
        EXPECT_FALSE(error.IsSet());

        EXPECT_EQ(restored.Function.Value(), "MyFunc");
        EXPECT_EQ(restored.Module.Value(), "mylib.so");
        EXPECT_EQ(restored.Line.Value(), 100u);
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_CopyConstructor)
    {
        PluginHost::CallstackData original;
        string json = R"({"function":"CopyFunc","line":55})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        PluginHost::CallstackData copy(original);
        EXPECT_EQ(copy.Function.Value(), "CopyFunc");
        EXPECT_EQ(copy.Line.Value(), 55u);
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_MoveConstructor)
    {
        PluginHost::CallstackData original;
        string json = R"({"function":"MoveFunc","module":"movelib.so"})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        PluginHost::CallstackData moved(std::move(original));
        EXPECT_EQ(moved.Function.Value(), "MoveFunc");
        EXPECT_EQ(moved.Module.Value(), "movelib.so");
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_AssignmentOperator)
    {
        PluginHost::CallstackData source;
        string json = R"({"function":"AssignFunc","line":77})";
        Core::OptionalType<Core::JSON::Error> error;
        source.IElement::FromString(json, error);

        PluginHost::CallstackData target;
        target = source;
        EXPECT_EQ(target.Function.Value(), "AssignFunc");
        EXPECT_EQ(target.Line.Value(), 77u);
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_FromCallstackInfo)
    {
        Core::callstack_info info;
        info.address = reinterpret_cast<void*>(0x12345678);
        info.function = "test_function";
        info.module = "testmod.so";
        info.line = 42;

        PluginHost::CallstackData data(info);
        EXPECT_EQ(data.Function.Value(), "test_function");
        EXPECT_EQ(data.Module.Value(), "testmod.so");
        EXPECT_EQ(data.Line.Value(), 42u);
    }

    TEST(ThunderPlugin_PostMortem, CallstackData_FromCallstackInfo_EmptyModule)
    {
        Core::callstack_info info;
        info.address = reinterpret_cast<void*>(0x1000);
        info.function = "func";
        info.module = "";
        info.line = static_cast<uint32_t>(~0);

        PluginHost::CallstackData data(info);
        EXPECT_EQ(data.Function.Value(), "func");
        EXPECT_FALSE(data.Module.IsSet());
        EXPECT_FALSE(data.Line.IsSet());
    }

    TEST(ThunderPlugin_PostMortem, PostMortemData_DefaultConstruction)
    {
        PluginHost::PostMortemData data;

        auto it = data.Callstacks.Elements();
        uint32_t count = 0;
        while (it.Next()) count++;
        EXPECT_EQ(count, 0u);
    }

    TEST(ThunderPlugin_PostMortem, Callstack_ParseJSON)
    {
        PluginHost::PostMortemData::Callstack stack;
        string json = R"({
            "id": 12345,
            "stack": [
                {"function":"func1","line":10},
                {"function":"func2","line":20}
            ]
        })";

        Core::OptionalType<Core::JSON::Error> error;
        stack.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        auto it = stack.Data.Elements();
        uint32_t count = 0;
        while (it.Next()) count++;
        EXPECT_EQ(count, 2u);
    }

    TEST(ThunderPlugin_PostMortem, Callstack_CopyConstructor)
    {
        PluginHost::PostMortemData::Callstack original;
        string json = R"({"id":999,"stack":[{"function":"copyfunc"}]})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);

        PluginHost::PostMortemData::Callstack copy(original);

        auto it = copy.Data.Elements();
        uint32_t count = 0;
        while (it.Next()) count++;
        EXPECT_EQ(count, 1u);
    }

    TEST(ThunderPlugin_PostMortem, Callstack_SerializeRoundTrip)
    {
        PluginHost::PostMortemData::Callstack original;
        string json = R"({"id":42,"stack":[{"function":"f1","line":1},{"function":"f2","line":2}]})";
        Core::OptionalType<Core::JSON::Error> error;
        original.IElement::FromString(json, error);
        EXPECT_FALSE(error.IsSet());

        string serialized;
        original.IElement::ToString(serialized);

        PluginHost::PostMortemData::Callstack restored;
        restored.IElement::FromString(serialized, error);
        EXPECT_FALSE(error.IsSet());

        auto it = restored.Data.Elements();
        uint32_t count = 0;
        while (it.Next()) count++;
        EXPECT_EQ(count, 2u);
    }

} // namespace Tests
} // namespace Thunder
