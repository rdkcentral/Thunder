---
applyTo: '**'
---

# 12 — Testing Guidelines

> Cross-reference: `10-plugin-development.md` for plugin structure, `06-comrpc-fundamentals.md` for COM-RPC client setup.

## Test Tiers

Thunder uses two test tiers, defined by CMake option files:

| Tier | CMake file | What it tests | Requires daemon? |
|------|-----------|--------------|-----------------|
| L1 | `l1tests.cmake` | Unit tests for plugin logic in isolation | No |
| L2 | `l2tests.cmake` | Integration tests with a running Thunder instance | Yes |

L1 tests live alongside plugin source code or in a dedicated `Tests/` directory. L2 tests communicate with Thunder via JSON-RPC or COM-RPC.

## L1 Tests — Unit Testing Plugin Logic

L1 tests use **Google Test** (`gtest`/`gmock`). They instantiate the plugin class directly without a daemon.

### Mock `IShell` for L1 Tests

```cpp
#include <gmock/gmock.h>
#include <plugins/IPlugin.h>

class MockShell : public PluginHost::IShell {
public:
    MOCK_METHOD(const string&, Callsign, (), (const, override));
    MOCK_METHOD(const string&, ConfigLine, (), (const, override));
    MOCK_METHOD(string, DataPath, (const string&), (const, override));
    MOCK_METHOD(string, PersistentPath, (const string&), (const, override));
    MOCK_METHOD(string, VolatilePath, (const string&), (const, override));
    MOCK_METHOD(void, Register, (PluginHost::IPlugin::INotification*), (override));
    MOCK_METHOD(void, Unregister, (PluginHost::IPlugin::INotification*), (override));
    MOCK_METHOD(Core::hresult, AddRef, (), (override));
    MOCK_METHOD(uint32_t, Release, (), (override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t), (override));
    // ... add other methods as needed by the plugin under test
};
```

### L1 Test Structure

```cpp
#include <gtest/gtest.h>
#include "MyPlugin.h"

using namespace Thunder;

class MyPluginTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        _shell = std::make_unique<MockShell>();
        // Configure mock expectations
        ON_CALL(*_shell, ConfigLine())
            .WillByDefault(::testing::Return(R"({"retries":5,"endpoint":"localhost:8080"})"));
        ON_CALL(*_shell, DataPath(::testing::_))
            .WillByDefault(::testing::ReturnArg<0>());
    }

    void TearDown() override
    {
        _shell.reset();
    }

    std::unique_ptr<MockShell> _shell;
};

TEST_F(MyPluginTest, InitializeSucceeds)
{
    Plugin::MyPlugin plugin;
    EXPECT_CALL(*_shell, Register(::testing::_)).Times(1);

    const string result = plugin.Initialize(_shell.get());
    EXPECT_EQ(result, "");  // empty = success
}

TEST_F(MyPluginTest, DeinitializeReleasesResources)
{
    Plugin::MyPlugin plugin;
    plugin.Initialize(_shell.get());

    EXPECT_CALL(*_shell, Unregister(::testing::_)).Times(1);
    plugin.Deinitialize(_shell.get());
}
```

### What to Test in L1

- `Initialize()` / `Deinitialize()` cycle: must succeed and not leak
- `Initialize()` error paths: return correct non-empty string for each failure mode
- JSON-RPC handler logic: input validation, correct return codes, output correctness
- Config parsing: default values when fields absent, correct parsing when present
- State machine transitions internal to the plugin
- Subsystem interaction (using a mock `ISubSystem`)

### What NOT to Mock in L1

- `Core::WorkerPool::Instance()` — use the real singleton; L1 tests must link against `ThunderCore`
- `Core::ResourceMonitor::Instance()` — use the real singleton
- `Core::CriticalSection` — always test with the real lock (tests thread-safety)

## Mock COM Interface Pattern

For interfaces that a plugin acquires from other plugins:

```cpp
class MockNetworkControl : public Exchange::INetworkControl {
public:
    MOCK_METHOD(Core::hresult, Up, (const string& interface), (override));
    MOCK_METHOD(Core::hresult, Down, (const string& interface), (override));
    MOCK_METHOD(Core::hresult, AddRef, (), (override));
    MOCK_METHOD(uint32_t, Release, (), (override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t), (override));
};

// In test setup:
auto* mockNetwork = new MockNetworkControl();
ON_CALL(*_shell, QueryInterface(Exchange::INetworkControl::ID))
    .WillByDefault(::testing::Return(static_cast<void*>(mockNetwork)));
```

## L2 Tests — Integration Testing with a Running Daemon

L2 tests run against a live Thunder instance. They connect via JSON-RPC (WebSocket) or COM-RPC.

### JSON-RPC L2 Client

```cpp
// Use Thunder's built-in JSON-RPC client
#include <plugins/IPlugin.h>

// Or use any WebSocket JSON-RPC 2.0 client library
// Connect to ws://127.0.0.1:55555/MyPlugin
// Send: {"jsonrpc":"2.0","id":1,"method":"MyPlugin.getstatus"}
// Expect: {"jsonrpc":"2.0","result":"OK","id":1}
```

### COM-RPC L2 Client

See `06-comrpc-fundamentals.md` — `SmartInterfaceType<T>` for connecting to a live daemon.

### L2 Test Lifecycle

1. Start a minimal Thunder instance with only the plugin under test activated
2. Connect via JSON-RPC or COM-RPC
3. Exercise the plugin's public API
4. Verify state, return codes, and events
5. Deactivate the plugin via Controller
6. Verify clean shutdown (no crash, no leaked resources in post-mortem dump)

### Minimal Test Config for L2

```json
{
  "binding": "127.0.0.1",
  "port": 55555,
  "communicator": "/tmp/thundertest",
  "persistentpath": "/tmp/thunder-test/persistent/",
  "datapath": "/tmp/thunder-test/data/",
  "systempath": "/usr/lib/thunder/",
  "proxystubpath": "/usr/lib/thunder/proxystubs/",
  "volatilepath": "/tmp/thunder-test/volatile/"
}
```

Plugin config for L2 (minimal):
```json
{
  "callsign": "MyPlugin",
  "classname": "MyPlugin",
  "locator": "libThunderMyPlugin.so",
  "autostart": true
}
```

## Testing OOP Plugins

To test out-of-process execution:
1. Add `"root": { "mode": "Local" }` to the plugin config.
2. Verify that the `ThunderPlugin` child process spawns and terminates cleanly.
3. Verify that COM-RPC calls across the process boundary work correctly.
4. Use ThunderShark (TCP communicator mode) to capture and inspect COM-RPC traffic during tests.

## Testing `Initialize` / `Deinitialize` Cycle Stability

Every plugin must survive repeated activation/deactivation without leaking resources:

```cpp
TEST_F(MyPluginL2Test, RepeatedActivationDeactivation)
{
    for (int i = 0; i < 10; ++i) {
        ActivatePlugin("MyPlugin");
        // Verify plugin is operational
        EXPECT_EQ(QueryMethod("MyPlugin.getstatus"), "OK");
        DeactivatePlugin("MyPlugin");
        // Verify clean state
    }
}
```

## ThunderUI — Manual Exploration

ThunderUI is a browser-based tool for manually testing JSON-RPC:
```
open http://localhost:55555/ThunderUI/index.html
```
Connect to `ws://127.0.0.1:55555` and explore plugin APIs interactively. Useful for L2 test case discovery.

## CMakeLists.txt for L1 Tests

```cmake
find_package(GTest REQUIRED)

add_executable(MyPluginTests
    MyPluginTest.cpp
)

target_link_libraries(MyPluginTests
    PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ThunderMyPlugin   # link against the plugin library directly
)

gtest_discover_tests(MyPluginTests)
```

## CI Requirements

Before a PR can merge:
1. All L1 tests must pass with zero failures
2. No new compiler warnings (treat warnings as errors in CI: `-DCMAKE_CXX_FLAGS="-Werror"`)
3. `clang-format --dry-run --Werror` must report no style violations
4. L2 tests (where configured) must pass on the CI test device
5. Build must succeed with both `Debug` and `MinSizeRel` build types
6. Build must succeed on both Linux (GCC) and macOS (Apple Clang) if platform-specific code was modified
