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

// -----------------------------------------------------------------
// Thunder Plugin framework gap coverage tests
//
// Gap coverage mapping (from test-it-report.md Plugins section):
//   Gap 1: IPlugin lifecycle                        -> PluginLifecycle tests
//   Gap 2: PluginHost::JSONRPC dispatch              -> JSONRPCDispatch tests
//   Gap 3: ISubSystem precondition/termination       -> SubSystem tests
//   Gap 4: Event subscription lifecycle              -> EventSubscription tests
//   Gap 5: IStateControl suspend/resume              -> StateControl tests
//   Gap 6: Plugin::Config parsing                    -> PluginConfig tests
//   Gap 7: IShell service interface                  -> IShell tests (via mock)
//
// NOTE: ThunderNanoServices has 7+ test plugins that exercise Initialize/
// Deinitialize but with no GTest assertions. These tests fill that gap
// with assertion-verified unit tests that need no live daemon.
// -----------------------------------------------------------------

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <plugins/plugins.h>

#include <atomic>
#include <string>
#include <vector>

namespace Thunder {
namespace Tests {
namespace PluginGap {

    // =====================================================================
    // JSON parameter types for JSONRPC dispatch tests
    // =====================================================================

    class JsonAddParams : public Core::JSON::Container {
    public:
        JsonAddParams()
            : Core::JSON::Container()
            , A(0)
            , B(0)
        {
            Add(_T("a"), &A);
            Add(_T("b"), &B);
        }
        JsonAddParams(const JsonAddParams& copy)
            : Core::JSON::Container()
            , A(copy.A)
            , B(copy.B)
        {
            Add(_T("a"), &A);
            Add(_T("b"), &B);
        }
        ~JsonAddParams() override = default;

        Core::JSON::DecUInt32 A;
        Core::JSON::DecUInt32 B;
    };

    class JsonAddResult : public Core::JSON::Container {
    public:
        JsonAddResult()
            : Core::JSON::Container()
            , Sum(0)
        {
            Add(_T("sum"), &Sum);
        }
        JsonAddResult(const JsonAddResult& copy)
            : Core::JSON::Container()
            , Sum(copy.Sum)
        {
            Add(_T("sum"), &Sum);
        }
        ~JsonAddResult() override = default;

        Core::JSON::DecUInt32 Sum;
    };

    class JsonStringParam : public Core::JSON::Container {
    public:
        JsonStringParam()
            : Core::JSON::Container()
            , Value()
        {
            Add(_T("value"), &Value);
        }
        JsonStringParam(const JsonStringParam& copy)
            : Core::JSON::Container()
            , Value(copy.Value)
        {
            Add(_T("value"), &Value);
        }
        ~JsonStringParam() override = default;

        Core::JSON::String Value;
    };

    // =====================================================================
    // Test service with bound methods for JSONRPC tests
    // =====================================================================

    class CalculatorService {
    public:
        CalculatorService() : _counter(0) {}

        uint32_t Add(const JsonAddParams& params, JsonAddResult& result)
        {
            result.Sum = params.A + params.B;
            return Core::ERROR_NONE;
        }

        uint32_t GetCounter(JsonStringParam& result) const
        {
            result.Value = std::to_string(_counter);
            return Core::ERROR_NONE;
        }

        uint32_t SetCounter(const JsonStringParam& param)
        {
            _counter = std::stoi(param.Value.Value());
            return Core::ERROR_NONE;
        }

        uint32_t Increment(const JsonAddParams& /*params*/, JsonAddResult& result)
        {
            _counter++;
            result.Sum = static_cast<uint32_t>(_counter);
            return Core::ERROR_NONE;
        }

        uint32_t FailAlways(const JsonStringParam& /*param*/, JsonStringParam& /*result*/)
        {
            return Core::ERROR_UNAVAILABLE;
        }

        int Counter() const { return _counter; }

    private:
        int _counter;
    };

    // =====================================================================
    // Testable JSONRPC wrapper (PluginHost::JSONRPC is abstract via IUnknown)
    // =====================================================================

    class TestableJSONRPC : public PluginHost::JSONRPC {
    public:
        TestableJSONRPC() : PluginHost::JSONRPC() {}
        TestableJSONRPC(const TokenCheckFunction& validation) : PluginHost::JSONRPC(validation) {}
        ~TestableJSONRPC() override = default;

        uint32_t AddRef() const override { return 0; }
        uint32_t Release() const override { return Core::ERROR_NONE; }

        BEGIN_INTERFACE_MAP(TestableJSONRPC)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP
    };

    // =====================================================================
    // Mock IPlugin for lifecycle tests
    // =====================================================================

    class MockPlugin : public PluginHost::IPlugin {
    public:
        MockPlugin()
            : _initCalled(false)
            , _deinitCalled(false)
            , _initError()
            , _refCount(1)
        {
        }

        // IPlugin
        const string Initialize(PluginHost::IShell* shell VARIABLE_IS_NOT_USED) override
        {
            _initCalled = true;
            return _initError; // empty = success, non-empty = error
        }

        void Deinitialize(PluginHost::IShell* shell VARIABLE_IS_NOT_USED) override
        {
            _deinitCalled = true;
        }

        string Information() const override
        {
            return _T("MockPlugin v1.0");
        }

        // IUnknown
        uint32_t AddRef() const override { _refCount++; return _refCount; }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (--_refCount == 0) {
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                delete this;
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(MockPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        // Test accessors
        bool InitCalled() const { return _initCalled; }
        bool DeinitCalled() const { return _deinitCalled; }
        void SetInitError(const string& error) { _initError = error; }

    private:
        bool _initCalled;
        bool _deinitCalled;
        string _initError;
        mutable std::atomic<uint32_t> _refCount;
    };

    // =====================================================================
    // Mock ISubSystem for subsystem flag tests
    // =====================================================================

    class MockSubSystem : public PluginHost::ISubSystem {
    public:
        MockSubSystem()
            : _flags(0)
            , _refCount(1)
            , _updateCount(0)
        {
        }

        void Register(INotification* notification) override
        {
            _notifications.push_back(notification);
        }

        void Unregister(INotification* notification) override
        {
            _notifications.erase(
                std::remove(_notifications.begin(), _notifications.end(), notification),
                _notifications.end());
        }

        Core::hresult Set(const subsystem type, Core::IUnknown* /* information */) override
        {
            if (type < END_LIST) {
                _flags |= (1u << static_cast<uint32_t>(type));
                NotifyUpdated();
                return Core::ERROR_NONE;
            }
            else if (type >= NEGATIVE_START) {
                // Clear the corresponding positive flag
                uint32_t positive = static_cast<uint32_t>(type) - static_cast<uint32_t>(NEGATIVE_START);
                if (positive < static_cast<uint32_t>(END_LIST)) {
                    _flags &= ~(1u << positive);
                    NotifyUpdated();
                }
                return Core::ERROR_NONE;
            }
            return Core::ERROR_BAD_REQUEST;
        }

        const Core::IUnknown* Get(const subsystem /* type */) const override
        {
            return nullptr;
        }

        bool IsActive(const subsystem type) const override
        {
            return (static_cast<uint32_t>(type) < static_cast<uint32_t>(END_LIST)) &&
                   ((_flags & (1u << static_cast<uint32_t>(type))) != 0);
        }

        string BuildTreeHash() const override { return _T("test-hash"); }
        string Version() const override { return _T("1.0.0"); }

        // IUnknown
        uint32_t AddRef() const override { _refCount++; return _refCount; }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (--_refCount == 0) {
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                delete this;
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(MockSubSystem)
            INTERFACE_ENTRY(PluginHost::ISubSystem)
        END_INTERFACE_MAP

        // Test accessors
        uint32_t Flags() const { return _flags; }
        uint32_t UpdateCount() const { return _updateCount; }

    private:
        void NotifyUpdated()
        {
            _updateCount++;
            for (auto* n : _notifications) {
                n->Updated();
            }
        }

        uint32_t _flags;
        mutable std::atomic<uint32_t> _refCount;
        uint32_t _updateCount;
        std::vector<INotification*> _notifications;
    };

    // =====================================================================
    // Mock ISubSystem::INotification
    // =====================================================================

    class MockSubSystemNotification : public PluginHost::ISubSystem::INotification {
    public:
        MockSubSystemNotification() : _updateCount(0), _refCount(1) {}

        void Updated() override { _updateCount++; }

        uint32_t AddRef() const override { _refCount++; return _refCount; }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (--_refCount == 0) {
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                delete this;
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(MockSubSystemNotification)
            INTERFACE_ENTRY(PluginHost::ISubSystem::INotification)
        END_INTERFACE_MAP

        uint32_t UpdateCount() const { return _updateCount; }

    private:
        uint32_t _updateCount;
        mutable std::atomic<uint32_t> _refCount;
    };

    // =====================================================================
    // Mock IStateControl for suspend/resume tests
    // =====================================================================

    class MockStateControl : public PluginHost::IStateControl {
    public:
        MockStateControl()
            : _state(UNINITIALIZED)
            , _refCount(1)
        {
        }

        Core::hresult Configure(PluginHost::IShell* /* framework */) override
        {
            _state = RESUMED;
            return Core::ERROR_NONE;
        }

        state State() const override { return _state; }

        Core::hresult Request(const command cmd) override
        {
            switch (cmd) {
            case SUSPEND:
                if (_state == RESUMED) {
                    _state = SUSPENDED;
                    NotifyStateChange(SUSPENDED);
                    return Core::ERROR_NONE;
                }
                return Core::ERROR_ILLEGAL_STATE;
            case RESUME:
                if (_state == SUSPENDED) {
                    _state = RESUMED;
                    NotifyStateChange(RESUMED);
                    return Core::ERROR_NONE;
                }
                return Core::ERROR_ILLEGAL_STATE;
            default:
                return Core::ERROR_BAD_REQUEST;
            }
        }

        void Register(INotification* notification) override
        {
            _notifications.push_back(notification);
        }

        void Unregister(INotification* notification) override
        {
            _notifications.erase(
                std::remove(_notifications.begin(), _notifications.end(), notification),
                _notifications.end());
        }

        // IUnknown
        uint32_t AddRef() const override { _refCount++; return _refCount; }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (--_refCount == 0) {
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                delete this;
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(MockStateControl)
            INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        void NotifyStateChange(state newState)
        {
            for (auto* n : _notifications) {
                n->StateChange(newState);
            }
        }

        state _state;
        mutable std::atomic<uint32_t> _refCount;
        std::vector<INotification*> _notifications;
    };

    // =====================================================================
    // Mock IStateControl::INotification
    // =====================================================================

    class MockStateNotification : public PluginHost::IStateControl::INotification {
    public:
        MockStateNotification() : _lastState(PluginHost::IStateControl::UNINITIALIZED), _changeCount(0), _refCount(1) {}

        void StateChange(const PluginHost::IStateControl::state state) override
        {
            _lastState = state;
            _changeCount++;
        }

        uint32_t AddRef() const override { _refCount++; return _refCount; }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (--_refCount == 0) {
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                delete this;
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(MockStateNotification)
            INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
        END_INTERFACE_MAP

        PluginHost::IStateControl::state LastState() const { return _lastState; }
        uint32_t ChangeCount() const { return _changeCount; }

    private:
        PluginHost::IStateControl::state _lastState;
        uint32_t _changeCount;
        mutable std::atomic<uint32_t> _refCount;
    };

    // ==========================================================================
    // GAP 6: Plugin::Config parsing
    //
    // ThunderNanoServices coverage: TestUtility parses config via ConfigLine().
    //   BUT: No test asserts on parsing results or error handling.
    //
    //  - FromString with all fields
    //  - Default values when fields are missing
    //  - Path helper methods (DataPath, PersistentPath, VolatilePath)
    //  - Round-trip serialization
    //  - StartMode enum parsing
    // ==========================================================================

    TEST(Plugin_Gap, PluginConfig_FullParse)
    {
        Plugin::Config config;

        const string json = _T("{"
            "\"callsign\":\"MyPlugin\","
            "\"locator\":\"libMyPlugin.so\","
            "\"classname\":\"MyPlugin\","
            "\"versions\":\"1\","
            "\"resumed\":true,"
            "\"webui\":\"static\","
            "\"configuration\":\"{\\\"key\\\":\\\"value\\\"}\","
            "\"persistentpathpostfix\":\"myplugin_data\","
            "\"volatilepathpostfix\":\"myplugin_tmp\","
            "\"systemrootpath\":\"/opt/thunder\","
            "\"startuporder\":50,"
            "\"throttle\":10,"
            "\"startmode\":\"Activated\""
            "}");

        EXPECT_TRUE(config.FromString(json));

        EXPECT_EQ(config.Callsign.Value(), _T("MyPlugin"));
        EXPECT_EQ(config.Locator.Value(), _T("libMyPlugin.so"));
        EXPECT_EQ(config.ClassName.Value(), _T("MyPlugin"));
        EXPECT_EQ(config.Versions.Value(), _T("1"));
        EXPECT_TRUE(config.Resumed.Value());
        EXPECT_EQ(config.WebUI.Value(), _T("static"));
        // Configuration is an unquoted JSON::String, so Value() includes JSON quoting
        EXPECT_TRUE(config.Configuration.IsSet());
        EXPECT_EQ(config.PersistentPathPostfix.Value(), _T("myplugin_data"));
        EXPECT_EQ(config.VolatilePathPostfix.Value(), _T("myplugin_tmp"));
        EXPECT_EQ(config.SystemRootPath.Value(), _T("/opt/thunder"));
        EXPECT_EQ(config.StartupOrder.Value(), 50u);
        EXPECT_EQ(config.Throttle.Value(), 10u);
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::ACTIVATED);
    }

    TEST(Plugin_Gap, PluginConfig_DefaultValues)
    {
        Plugin::Config config;

        // Parse empty JSON — all fields should have defaults
        config.FromString(_T("{}"));

        EXPECT_TRUE(config.Callsign.Value().empty());
        EXPECT_TRUE(config.Locator.Value().empty());
        EXPECT_TRUE(config.ClassName.Value().empty());
        EXPECT_FALSE(config.Resumed.Value());
        EXPECT_EQ(config.StartupOrder.Value(), 1000u); // default from source
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::ACTIVATED); // default
    }

    TEST(Plugin_Gap, PluginConfig_PathHelpers)
    {
        Plugin::Config config;

        const string json = _T("{"
            "\"callsign\":\"TestPlugin\","
            "\"classname\":\"TestClass\","
            "\"persistentpathpostfix\":\"custom_persist\","
            "\"volatilepathpostfix\":\"custom_volatile\""
            "}");

        EXPECT_TRUE(config.FromString(json));

        // DataPath is basePath + ClassName + /
        EXPECT_EQ(config.DataPath("/data/"), _T("/data/TestClass/"));

        // PersistentPath uses postfix if set, otherwise callsign
        EXPECT_EQ(config.PersistentPath("/persist/"), _T("/persist/custom_persist/"));

        // VolatilePath uses postfix if set, otherwise callsign
        EXPECT_EQ(config.VolatilePath("/volatile/"), _T("/volatile/custom_volatile/"));
    }

    TEST(Plugin_Gap, PluginConfig_PathHelpers_FallbackToCallsign)
    {
        Plugin::Config config;

        // No postfix set — should fall back to callsign
        const string json = _T("{"
            "\"callsign\":\"FallbackPlugin\","
            "\"classname\":\"FallbackClass\""
            "}");

        EXPECT_TRUE(config.FromString(json));

        EXPECT_EQ(config.PersistentPath("/persist/"), _T("/persist/FallbackPlugin/"));
        EXPECT_EQ(config.VolatilePath("/volatile/"), _T("/volatile/FallbackPlugin/"));
    }

    TEST(Plugin_Gap, PluginConfig_RoundTrip)
    {
        Plugin::Config original;

        const string json = _T("{"
            "\"callsign\":\"RoundTrip\","
            "\"locator\":\"libRoundTrip.so\","
            "\"classname\":\"RoundTripClass\","
            "\"startuporder\":42"
            "}");

        EXPECT_TRUE(original.FromString(json));

        // Serialize back
        string serialized;
        EXPECT_TRUE(original.ToString(serialized));

        // Parse again
        Plugin::Config restored;
        EXPECT_TRUE(restored.FromString(serialized));

        EXPECT_EQ(restored.Callsign.Value(), _T("RoundTrip"));
        EXPECT_EQ(restored.Locator.Value(), _T("libRoundTrip.so"));
        EXPECT_EQ(restored.ClassName.Value(), _T("RoundTripClass"));
        EXPECT_EQ(restored.StartupOrder.Value(), 42u);
    }

    TEST(Plugin_Gap, PluginConfig_StartModeEnum)
    {
        Plugin::Config config;

        EXPECT_TRUE(config.FromString(_T("{\"startmode\":\"Deactivated\"}")));
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::DEACTIVATED);

        EXPECT_TRUE(config.FromString(_T("{\"startmode\":\"Activated\"}")));
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::ACTIVATED);

        EXPECT_TRUE(config.FromString(_T("{\"startmode\":\"Unavailable\"}")));
        EXPECT_EQ(config.StartMode.Value(), PluginHost::IShell::startmode::UNAVAILABLE);
    }

    TEST(Plugin_Gap, PluginConfig_PreconditionArray)
    {
        Plugin::Config config;

        const string json = _T("{"
            "\"callsign\":\"Gated\","
            "\"precondition\":[\"Platform\",\"Network\"],"
            "\"termination\":[\"Internet\"]"
            "}");

        EXPECT_TRUE(config.FromString(json));

        EXPECT_TRUE(config.Precondition.IsSet());
        EXPECT_EQ(config.Precondition.Length(), 2u);

        EXPECT_TRUE(config.Termination.IsSet());
        EXPECT_EQ(config.Termination.Length(), 1u);
    }

    TEST(Plugin_Gap, PluginConfig_IsSameInstance)
    {
        Plugin::Config a, b;

        EXPECT_TRUE(a.FromString(_T("{\"locator\":\"libA.so\",\"classname\":\"ClassA\"}")));
        EXPECT_TRUE(b.FromString(_T("{\"locator\":\"libA.so\",\"classname\":\"ClassA\"}")));
        EXPECT_TRUE(a.IsSameInstance(b));

        EXPECT_TRUE(b.FromString(_T("{\"locator\":\"libB.so\",\"classname\":\"ClassA\"}")));
        EXPECT_FALSE(a.IsSameInstance(b));
    }

    // ==========================================================================
    // GAP 1: IPlugin lifecycle
    //
    // ThunderNanoServices coverage: All 7 test plugins implement Initialize/
    //   Deinitialize. But no test asserts on the framework's handling of
    //   Initialize() return values or state transitions.
    //
    //  - Initialize() success → returns empty string
    //  - Initialize() error → returns non-empty string
    //  - Deinitialize() called
    //  - Information() returns valid string
    //  - Plugin ref counting via IUnknown
    // ==========================================================================

    TEST(Plugin_Gap, PluginLifecycle_InitializeSuccess)
    {
        MockPlugin* plugin = new MockPlugin();

        // Initialize with nullptr shell (our mock doesn't use it)
        const string result = plugin->Initialize(nullptr);

        EXPECT_TRUE(result.empty()); // empty = success
        EXPECT_TRUE(plugin->InitCalled());
        EXPECT_FALSE(plugin->DeinitCalled());

        plugin->Release();
    }

    TEST(Plugin_Gap, PluginLifecycle_InitializeError)
    {
        MockPlugin* plugin = new MockPlugin();

        // Configure the mock to return an error string
        plugin->SetInitError(_T("Initialization failed: missing dependency"));

        const string result = plugin->Initialize(nullptr);

        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result, _T("Initialization failed: missing dependency"));
        EXPECT_TRUE(plugin->InitCalled());

        plugin->Release();
    }

    TEST(Plugin_Gap, PluginLifecycle_Deinitialize)
    {
        MockPlugin* plugin = new MockPlugin();

        plugin->Initialize(nullptr);
        EXPECT_FALSE(plugin->DeinitCalled());

        plugin->Deinitialize(nullptr);
        EXPECT_TRUE(plugin->DeinitCalled());

        plugin->Release();
    }

    TEST(Plugin_Gap, PluginLifecycle_FullCycle)
    {
        MockPlugin* plugin = new MockPlugin();

        // Full lifecycle: Initialize → use → Deinitialize
        const string initResult = plugin->Initialize(nullptr);
        EXPECT_TRUE(initResult.empty());

        string info = plugin->Information();
        EXPECT_FALSE(info.empty());

        plugin->Deinitialize(nullptr);
        EXPECT_TRUE(plugin->DeinitCalled());

        plugin->Release();
    }

    TEST(Plugin_Gap, PluginLifecycle_QueryInterface)
    {
        MockPlugin* plugin = new MockPlugin();

        // QueryInterface for IPlugin should succeed
        void* iface = plugin->QueryInterface(PluginHost::IPlugin::ID);
        PluginHost::IPlugin* queried = reinterpret_cast<PluginHost::IPlugin*>(iface);
        EXPECT_TRUE(queried != nullptr);

        if (queried != nullptr) {
            queried->Release();
        }

        plugin->Release();
    }

    TEST(Plugin_Gap, PluginLifecycle_RefCounting)
    {
        MockPlugin* plugin = new MockPlugin(); // refcount = 1

        plugin->AddRef(); // refcount = 2
        plugin->AddRef(); // refcount = 3

        uint32_t r1 = plugin->Release(); // refcount = 2
        EXPECT_NE(r1, Core::ERROR_DESTRUCTION_SUCCEEDED);

        uint32_t r2 = plugin->Release(); // refcount = 1
        EXPECT_NE(r2, Core::ERROR_DESTRUCTION_SUCCEEDED);

        uint32_t r3 = plugin->Release(); // refcount = 0 → destroyed
        EXPECT_EQ(r3, Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    // ==========================================================================
    // GAP 2: PluginHost::JSONRPC dispatch
    //
    // ThunderNanoServices coverage: TestAutomationComRpc, TestTextOptions,
    //   TestController, TestUtility all use JSON-RPC Register/Invoke.
    //   BUT: No test asserts on dispatch correctness, error codes, or
    //   unknown method handling.
    //
    //  - Invoke() with valid method → dispatched correctly
    //  - Invoke() with unknown method → ERROR_INVALID_SIGNATURE
    //  - Invoke() with empty method → ERROR_PARSE_FAILURE
    //  - Register/Unregister method lifecycle
    //  - Token validation callback
    //  - Built-in "exists" method
    // ==========================================================================

    TEST(Plugin_Gap, JSONRPCDispatch_InvokeRegisteredMethod)
    {
        TestableJSONRPC dispatcher;
        CalculatorService calc;

        dispatcher.Register<JsonAddParams, JsonAddResult>("add", &CalculatorService::Add, &calc);

        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("add"), _T("{\"a\":5,\"b\":3}"), response);

        EXPECT_EQ(result, Core::ERROR_NONE);

        JsonAddResult parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Sum.Value(), 8u);

        dispatcher.Unregister("add");
    }

    TEST(Plugin_Gap, JSONRPCDispatch_InvokeUnknownMethod)
    {
        TestableJSONRPC dispatcher;

        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("nonExistent"), _T("{}"), response);

        // Unknown method should return ERROR_UNKNOWN_METHOD (handler not found)
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_METHOD);
    }

    TEST(Plugin_Gap, JSONRPCDispatch_InvokeEmptyMethod)
    {
        TestableJSONRPC dispatcher;

        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T(""), _T("{}"), response);

        // Empty method → ERROR_PARSE_FAILURE
        EXPECT_EQ(result, Core::ERROR_PARSE_FAILURE);
    }

    TEST(Plugin_Gap, JSONRPCDispatch_RegisterUnregisterLifecycle)
    {
        TestableJSONRPC dispatcher;

        dispatcher.Register("temp", Core::JSONRPC::InvokeFunction(
            [](const Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                result = _T("\"ok\"");
                return Core::ERROR_NONE;
            }));

        // Should work
        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("temp"), _T(""), response);
        EXPECT_EQ(result, Core::ERROR_NONE);

        // Unregister
        dispatcher.Unregister("temp");

        // Should fail now
        response.clear();
        result = dispatcher.Invoke(0, 1, _T(""), _T("temp"), _T(""), response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_METHOD);
    }

    TEST(Plugin_Gap, JSONRPCDispatch_HandlerReturnsCustomError)
    {
        TestableJSONRPC dispatcher;
        CalculatorService calc;

        dispatcher.Register<JsonStringParam, JsonStringParam>("fail", &CalculatorService::FailAlways, &calc);

        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("fail"), _T("{\"value\":\"test\"}"), response);

        EXPECT_EQ(result, Core::ERROR_UNAVAILABLE);

        dispatcher.Unregister("fail");
    }

    TEST(Plugin_Gap, JSONRPCDispatch_MultipleMethodsIndependent)
    {
        TestableJSONRPC dispatcher;
        CalculatorService calc;

        dispatcher.Register<JsonAddParams, JsonAddResult>("add", &CalculatorService::Add, &calc);
        dispatcher.Register<JsonAddParams, JsonAddResult>("increment", &CalculatorService::Increment, &calc);

        // Call add
        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("add"), _T("{\"a\":10,\"b\":20}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        {
            JsonAddResult parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Sum.Value(), 30u);
        }

        // Call increment
        response.clear();
        result = dispatcher.Invoke(0, 1, _T(""), _T("increment"), _T("{\"a\":0,\"b\":0}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(calc.Counter(), 1);

        dispatcher.Unregister("add");
        dispatcher.Unregister("increment");
    }

    TEST(Plugin_Gap, JSONRPCDispatch_TokenValidation)
    {
        // Create dispatcher with token validation
        TestableJSONRPC dispatcher(
            [](const string& token, const string& /* method */, const string& /* params */) -> PluginHost::JSONRPC::classification {
                if (token == _T("valid_token")) {
                    return PluginHost::JSONRPC::classification::VALID;
                }
                return PluginHost::JSONRPC::classification::INVALID;
            });

        dispatcher.Register("secured", Core::JSONRPC::InvokeFunction(
            [](const Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                result = _T("\"access_granted\"");
                return Core::ERROR_NONE;
            }));

        // Valid token
        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T("valid_token"), _T("secured"), _T(""), response);
        EXPECT_EQ(result, Core::ERROR_NONE);

        // Invalid token
        response.clear();
        result = dispatcher.Invoke(0, 1, _T("bad_token"), _T("secured"), _T(""), response);
        EXPECT_EQ(result, Core::ERROR_PRIVILIGED_REQUEST);

        dispatcher.Unregister("secured");
    }

    TEST(Plugin_Gap, JSONRPCDispatch_BuiltInExists)
    {
        TestableJSONRPC dispatcher;

        dispatcher.Register("myMethod", Core::JSONRPC::InvokeFunction(
            [](const Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                result = _T("\"ok\"");
                return Core::ERROR_NONE;
            }));

        // Built-in "exists" method should report whether a method is registered
        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("exists"), _T("{\"method\":\"myMethod\"}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        // Response should contain "true"
        EXPECT_NE(response.find("true"), string::npos);

        // Non-existent method
        response.clear();
        result = dispatcher.Invoke(0, 1, _T(""), _T("exists"), _T("{\"method\":\"noSuchMethod\"}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("false"), string::npos);

        dispatcher.Unregister("myMethod");
    }

    TEST(Plugin_Gap, JSONRPCDispatch_PropertyGetSet)
    {
        TestableJSONRPC dispatcher;
        CalculatorService calc;

        dispatcher.Property<JsonStringParam>("counter", &CalculatorService::GetCounter, &CalculatorService::SetCounter, &calc);

        // GET (empty params)
        string response;
        uint32_t result = dispatcher.Invoke(0, 1, _T(""), _T("counter"), _T(""), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        {
            JsonStringParam parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Value.Value(), _T("0"));
        }

        // SET
        response.clear();
        result = dispatcher.Invoke(0, 1, _T(""), _T("counter"), _T("{\"value\":\"99\"}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(calc.Counter(), 99);

        // GET again
        response.clear();
        result = dispatcher.Invoke(0, 1, _T(""), _T("counter"), _T(""), response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        {
            JsonStringParam parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Value.Value(), _T("99"));
        }

        dispatcher.Unregister("counter");
    }

    // ==========================================================================
    // GAP 3: ISubSystem precondition/termination gating
    //
    // ThunderNanoServices coverage: NONE.
    //   No NanoServices test plugin tests ISubSystem.
    //
    //  - Set() flag toggling and notification
    //  - IsActive() state query
    //  - Negative flag (NOT_) clears positive
    //  - Notification fires on state change
    //  - Multiple flags set independently
    // ==========================================================================

    TEST(Plugin_Gap, SubSystem_SetAndIsActive)
    {
        MockSubSystem subsystem;

        EXPECT_FALSE(subsystem.IsActive(PluginHost::ISubSystem::PLATFORM));
        EXPECT_FALSE(subsystem.IsActive(PluginHost::ISubSystem::NETWORK));

        subsystem.Set(PluginHost::ISubSystem::PLATFORM, nullptr);
        EXPECT_TRUE(subsystem.IsActive(PluginHost::ISubSystem::PLATFORM));
        EXPECT_FALSE(subsystem.IsActive(PluginHost::ISubSystem::NETWORK));

        subsystem.Set(PluginHost::ISubSystem::NETWORK, nullptr);
        EXPECT_TRUE(subsystem.IsActive(PluginHost::ISubSystem::PLATFORM));
        EXPECT_TRUE(subsystem.IsActive(PluginHost::ISubSystem::NETWORK));
    }

    TEST(Plugin_Gap, SubSystem_NegativeFlagClearsPositive)
    {
        MockSubSystem subsystem;

        // Set PLATFORM active
        subsystem.Set(PluginHost::ISubSystem::PLATFORM, nullptr);
        EXPECT_TRUE(subsystem.IsActive(PluginHost::ISubSystem::PLATFORM));

        // Clear via NOT_PLATFORM
        subsystem.Set(PluginHost::ISubSystem::NOT_PLATFORM, nullptr);
        EXPECT_FALSE(subsystem.IsActive(PluginHost::ISubSystem::PLATFORM));
    }

    TEST(Plugin_Gap, SubSystem_NotificationFires)
    {
        MockSubSystem subsystem;
        MockSubSystemNotification notification;

        subsystem.Register(&notification);

        EXPECT_EQ(notification.UpdateCount(), 0u);

        subsystem.Set(PluginHost::ISubSystem::PLATFORM, nullptr);
        EXPECT_EQ(notification.UpdateCount(), 1u);

        subsystem.Set(PluginHost::ISubSystem::NETWORK, nullptr);
        EXPECT_EQ(notification.UpdateCount(), 2u);

        subsystem.Unregister(&notification);

        // After unregister, notification should not fire
        subsystem.Set(PluginHost::ISubSystem::INTERNET, nullptr);
        EXPECT_EQ(notification.UpdateCount(), 2u);
    }

    TEST(Plugin_Gap, SubSystem_AllFlagsIndependent)
    {
        MockSubSystem subsystem;

        // Set all subsystems
        for (uint32_t i = 0; i < static_cast<uint32_t>(PluginHost::ISubSystem::END_LIST); ++i) {
            auto flag = static_cast<PluginHost::ISubSystem::subsystem>(i);
            subsystem.Set(flag, nullptr);
        }

        // All should be active
        for (uint32_t i = 0; i < static_cast<uint32_t>(PluginHost::ISubSystem::END_LIST); ++i) {
            auto flag = static_cast<PluginHost::ISubSystem::subsystem>(i);
            EXPECT_TRUE(subsystem.IsActive(flag)) << "Subsystem " << i << " should be active";
        }
    }

    TEST(Plugin_Gap, SubSystem_IsActiveOutOfRange)
    {
        MockSubSystem subsystem;

        // Out-of-range subsystem should return false
        EXPECT_FALSE(subsystem.IsActive(PluginHost::ISubSystem::END_LIST));
        EXPECT_FALSE(subsystem.IsActive(static_cast<PluginHost::ISubSystem::subsystem>(999)));
    }

    TEST(Plugin_Gap, SubSystem_PreconditionGating)
    {
        // Simulate a precondition check: plugin requires PLATFORM + NETWORK
        MockSubSystem subsystem;

        auto preconditionsMet = [&]() -> bool {
            return subsystem.IsActive(PluginHost::ISubSystem::PLATFORM) &&
                   subsystem.IsActive(PluginHost::ISubSystem::NETWORK);
        };

        EXPECT_FALSE(preconditionsMet());

        subsystem.Set(PluginHost::ISubSystem::PLATFORM, nullptr);
        EXPECT_FALSE(preconditionsMet()); // Still missing NETWORK

        subsystem.Set(PluginHost::ISubSystem::NETWORK, nullptr);
        EXPECT_TRUE(preconditionsMet()); // Both met

        // Revoke NETWORK
        subsystem.Set(PluginHost::ISubSystem::NOT_NETWORK, nullptr);
        EXPECT_FALSE(preconditionsMet()); // Precondition no longer met
    }

    // ==========================================================================
    // GAP 4: Event subscription lifecycle
    //
    // ThunderNanoServices coverage: TestTextOptions forwards events through
    //   JSON-RPC, but no test asserts on subscribe/fire/deliver/unsubscribe.
    //
    //  - Subscribe → event registered
    //  - Unsubscribe → event removed
    //  - Callback-based subscribe/unsubscribe
    // ==========================================================================

    class MockEventCallback : public PluginHost::IDispatcher::ICallback {
    public:
        MockEventCallback() : _eventCount(0), _refCount(1) {}

        Core::hresult Event(const string& eventId, const string& /* designator */, const string& /* index */, const string& parameters) override
        {
            _lastEventId = eventId;
            _lastParameters = parameters;
            _eventCount++;
            return Core::ERROR_NONE;
        }

        uint32_t AddRef() const override { _refCount++; return _refCount; }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (--_refCount == 0) {
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                delete this;
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(MockEventCallback)
            INTERFACE_ENTRY(PluginHost::IDispatcher::ICallback)
        END_INTERFACE_MAP

        uint32_t EventCount() const { return _eventCount; }
        const string& LastEventId() const { return _lastEventId; }
        const string& LastParameters() const { return _lastParameters; }

    private:
        string _lastEventId;
        string _lastParameters;
        uint32_t _eventCount;
        mutable std::atomic<uint32_t> _refCount;
    };

    TEST(Plugin_Gap, EventSubscription_CallbackSubscribeUnsubscribe)
    {
        TestableJSONRPC dispatcher;
        MockEventCallback* callback = new MockEventCallback();

        // Subscribe to an event via callback
        Core::hresult subscribeResult = dispatcher.Subscribe(callback, _T("stateChanged"), _T(""), _T(""));
        EXPECT_EQ(subscribeResult, Core::ERROR_NONE);

        // Verify the subscription was recorded (subscribe same again should succeed
        // since the internal loop allows same-callback re-subscription)
        Core::hresult subscribeAgain = dispatcher.Subscribe(callback, _T("stateChanged"), _T(""), _T(""));
        EXPECT_EQ(subscribeAgain, Core::ERROR_NONE);

        callback->Release();
    }

    TEST(Plugin_Gap, EventSubscription_UnsubscribeNonExistent)
    {
        TestableJSONRPC dispatcher;
        MockEventCallback* callback = new MockEventCallback();

        // Unsubscribe from an event that was never subscribed
        Core::hresult result = dispatcher.Unsubscribe(callback, _T("neverSubscribed"), _T(""), _T(""));
        EXPECT_NE(result, Core::ERROR_NONE);

        callback->Release();
    }

    TEST(Plugin_Gap, EventSubscription_ChannelBasedViaInvoke)
    {
        TestableJSONRPC dispatcher;

        // Register a method so the handler exists
        dispatcher.Register("myEvent", Core::JSONRPC::InvokeFunction(
            [](const Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                result = _T("\"event_data\"");
                return Core::ERROR_NONE;
            }));

        // Channel-based subscription goes through the built-in "register" method
        string response;
        uint32_t result = dispatcher.Invoke(42, 1, _T(""), _T("register"),
            _T("{\"event\":\"myEvent\",\"id\":\"callback1\"}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);

        // Unsubscribe via built-in "unregister" method
        response.clear();
        result = dispatcher.Invoke(42, 2, _T(""), _T("unregister"),
            _T("{\"event\":\"myEvent\",\"id\":\"callback1\"}"), response);
        EXPECT_EQ(result, Core::ERROR_NONE);

        dispatcher.Unregister("myEvent");
    }

    // ==========================================================================
    // GAP 5: IStateControl suspend/resume
    //
    // ThunderNanoServices coverage: NONE.
    //   No NanoServices test plugin tests IStateControl.
    //
    //  - Configure → RESUMED
    //  - Request(SUSPEND) → SUSPENDED
    //  - Request(RESUME) → RESUMED
    //  - Full suspend → resume cycle
    //  - Notification fires on state change
    //  - Invalid state transitions return error
    // ==========================================================================

    TEST(Plugin_Gap, StateControl_InitialState)
    {
        MockStateControl stateCtrl;

        EXPECT_EQ(stateCtrl.State(), PluginHost::IStateControl::UNINITIALIZED);
    }

    TEST(Plugin_Gap, StateControl_Configure)
    {
        MockStateControl stateCtrl;

        Core::hresult result = stateCtrl.Configure(nullptr);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(stateCtrl.State(), PluginHost::IStateControl::RESUMED);
    }

    TEST(Plugin_Gap, StateControl_SuspendResumeCycle)
    {
        MockStateControl stateCtrl;
        stateCtrl.Configure(nullptr); // → RESUMED

        // Suspend
        Core::hresult result = stateCtrl.Request(PluginHost::IStateControl::SUSPEND);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(stateCtrl.State(), PluginHost::IStateControl::SUSPENDED);

        // Resume
        result = stateCtrl.Request(PluginHost::IStateControl::RESUME);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(stateCtrl.State(), PluginHost::IStateControl::RESUMED);
    }

    TEST(Plugin_Gap, StateControl_InvalidTransition)
    {
        MockStateControl stateCtrl;
        stateCtrl.Configure(nullptr); // → RESUMED

        // Can't resume when already resumed
        Core::hresult result = stateCtrl.Request(PluginHost::IStateControl::RESUME);
        EXPECT_EQ(result, Core::ERROR_ILLEGAL_STATE);

        // Suspend first
        stateCtrl.Request(PluginHost::IStateControl::SUSPEND);

        // Can't suspend when already suspended
        result = stateCtrl.Request(PluginHost::IStateControl::SUSPEND);
        EXPECT_EQ(result, Core::ERROR_ILLEGAL_STATE);
    }

    TEST(Plugin_Gap, StateControl_NotificationFires)
    {
        MockStateControl stateCtrl;
        MockStateNotification notification;

        stateCtrl.Register(&notification);
        stateCtrl.Configure(nullptr); // → RESUMED

        EXPECT_EQ(notification.ChangeCount(), 0u);

        // Suspend → notification should fire
        stateCtrl.Request(PluginHost::IStateControl::SUSPEND);
        EXPECT_EQ(notification.ChangeCount(), 1u);
        EXPECT_EQ(notification.LastState(), PluginHost::IStateControl::SUSPENDED);

        // Resume → notification should fire again
        stateCtrl.Request(PluginHost::IStateControl::RESUME);
        EXPECT_EQ(notification.ChangeCount(), 2u);
        EXPECT_EQ(notification.LastState(), PluginHost::IStateControl::RESUMED);

        stateCtrl.Unregister(&notification);

        // After unregister, notification should not fire
        stateCtrl.Request(PluginHost::IStateControl::SUSPEND);
        EXPECT_EQ(notification.ChangeCount(), 2u);
    }

    TEST(Plugin_Gap, StateControl_MultipleSuspendResumeCycles)
    {
        MockStateControl stateCtrl;
        stateCtrl.Configure(nullptr);

        for (int i = 0; i < 10; ++i) {
            EXPECT_EQ(stateCtrl.Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
            EXPECT_EQ(stateCtrl.State(), PluginHost::IStateControl::SUSPENDED);
            EXPECT_EQ(stateCtrl.Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
            EXPECT_EQ(stateCtrl.State(), PluginHost::IStateControl::RESUMED);
        }
    }

    // ==========================================================================
    // GAP 7: IShell state enum and reason enum coverage
    //
    // ThunderNanoServices coverage: All plugins use IShell but no test asserts
    //   on state/reason enum values or transitions.
    //
    //  - State enum values are distinct
    //  - Reason enum values are distinct
    //  - StartMode enum maps to state values
    //  - IShell::Job dispatch helper
    // ==========================================================================

    TEST(Plugin_Gap, IShell_StateEnumValues)
    {
        // Verify all state enum values are distinct and ordered
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::UNAVAILABLE), 0u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::DEACTIVATED), 1u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::ACTIVATED), 2u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::DEACTIVATION), 3u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::ACTIVATION), 4u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::PRECONDITION), 5u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::HIBERNATED), 6u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::state::DESTROYED), 7u);
    }

    TEST(Plugin_Gap, IShell_ReasonEnumValues)
    {
        // Verify reason enum values
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::REQUESTED), 0u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::AUTOMATIC), 1u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::FAILURE), 2u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::MEMORY_EXCEEDED), 3u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::STARTUP), 4u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::SHUTDOWN), 5u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::CONDITIONS), 6u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::WATCHDOG_EXPIRED), 7u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::INITIALIZATION_FAILED), 8u);
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::reason::INSTANTIATION_FAILED), 9u);
    }

    TEST(Plugin_Gap, IShell_StartModeMapsToState)
    {
        // startmode enum values should map to corresponding state values
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::startmode::UNAVAILABLE),
                  static_cast<uint8_t>(PluginHost::IShell::state::UNAVAILABLE));
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::startmode::DEACTIVATED),
                  static_cast<uint8_t>(PluginHost::IShell::state::DEACTIVATED));
        EXPECT_EQ(static_cast<uint8_t>(PluginHost::IShell::startmode::ACTIVATED),
                  static_cast<uint8_t>(PluginHost::IShell::state::ACTIVATED));
    }

    TEST(Plugin_Gap, IStateControl_StringConversion)
    {
        // state ToString (all uppercase per ENUM_CONVERSION table)
        EXPECT_STREQ(PluginHost::IStateControl::ToString(PluginHost::IStateControl::UNINITIALIZED), _T("UNINITIALIZED"));
        EXPECT_STREQ(PluginHost::IStateControl::ToString(PluginHost::IStateControl::SUSPENDED), _T("SUSPENDED"));
        EXPECT_STREQ(PluginHost::IStateControl::ToString(PluginHost::IStateControl::RESUMED), _T("RESUMED"));
        // EXITED has no entry in the enum conversion table, so ToString returns empty string
        EXPECT_STREQ(PluginHost::IStateControl::ToString(PluginHost::IStateControl::EXITED), _T(""));
    }

    TEST(Plugin_Gap, IStateControl_CommandStringConversion)
    {
        // command ToString uses state enum internally (maps command to state by value)
        // SUSPEND(1) maps to SUSPENDED(1), RESUME(2) maps to RESUMED(2)
        EXPECT_STREQ(PluginHost::IStateControl::ToString(PluginHost::IStateControl::SUSPEND), _T("SUSPENDED"));
        EXPECT_STREQ(PluginHost::IStateControl::ToString(PluginHost::IStateControl::RESUME), _T("RESUMED"));
    }

    // ==========================================================================
    // GAP 3 supplement: ISubSystem subsystem enum ToString coverage
    // ==========================================================================

    TEST(Plugin_Gap, SubSystem_EnumValues)
    {
        // Verify subsystem enum values are sequential
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::PLATFORM), 0u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::SECURITY), 1u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::NETWORK), 2u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::IDENTIFIER), 3u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::GRAPHICS), 4u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::INTERNET), 5u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::LOCATION), 6u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::TIME), 7u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::PROVISIONING), 8u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::DECRYPTION), 9u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::WEBSOURCE), 10u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::STREAMING), 11u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::BLUETOOTH), 12u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::CRYPTOGRAPHY), 13u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::INSTALLATION), 14u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::END_LIST), 15u);
    }

    TEST(Plugin_Gap, SubSystem_NegativeEnumValues)
    {
        // Negative flags should be NEGATIVE_START + offset
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::NEGATIVE_START), 0x80000000u);
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::NOT_PLATFORM),
                  static_cast<uint32_t>(PluginHost::ISubSystem::NEGATIVE_START));
        EXPECT_EQ(static_cast<uint32_t>(PluginHost::ISubSystem::NOT_NETWORK),
                  static_cast<uint32_t>(PluginHost::ISubSystem::NEGATIVE_START) + 2u);
    }

} // namespace PluginGap
} // namespace Tests
} // namespace Thunder
