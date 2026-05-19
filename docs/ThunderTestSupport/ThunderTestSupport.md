# Thunder Test Support Library

## Overview

The **thunder_test_support** library enables in-process integration testing of Thunder plugins without launching the Thunder daemon. It embeds the Thunder `PluginHost::Server` into a static archive (`.a`) that test binaries link against, allowing GTest-based tests to boot a real Thunder runtime, load plugins as shared libraries, and exercise both JSON-RPC and COM-RPC interfaces — all within a single process.

### Key Properties

| Property | Value |
|----------|-------|
| Library type | Static archive (`libthunder_test_support.a`) |
| CMake option | `ENABLE_TEST_RUNTIME=ON` (POSIX platforms only) |
| Location | `Tests/test_support/` |
| Public header | `ThunderTestRuntime.h` |
| Install paths | `${CMAKE_INSTALL_LIBDIR}/libthunder_test_support.a`, `${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/test_support/ThunderTestRuntime.h` |
| Installed CMake package | `find_package(thunder_test_support)` — exports `thunder_test_support::thunder_test_support` target with whole-archive link options |

---

## Why This Exists

This library was added to eliminate the need for repo-local Thunder mock layers in tests.

Without a reusable embedded Thunder runtime, repositories typically end up recreating enough Thunder behavior locally to get code under test running at all. That usually means carrying custom mocks or shims for host services, COM link behavior, worker-pool setup, factories, module plumbing, JSON-RPC wiring, and other framework-owned pieces.

`thunder_test_support` replaces that Thunder-specific scaffolding with a reusable in-process Thunder runtime:

- tests link one reusable library instead of maintaining local Thunder mocks
- code under test runs against a real embedded `PluginHost::Server`
- JSON-RPC and COM-RPC calls use real Thunder plumbing rather than hand-built host/service stubs
- consumers no longer need to recreate Thunder host behavior just to exercise their code

The goal is to stop faking Thunder itself.

Tests may still keep mocks for their own non-Thunder dependencies, but the Thunder runtime layer should be provided by this library rather than rebuilt in each repository.

---

## Architecture

### Production Thunder vs Test Support

```
┌─────────────────────────────────────┐    ┌─────────────────────────────────────────┐
│       PRODUCTION DEPLOYMENT         │    │          TEST DEPLOYMENT                │
│                                     │    │                                         │
│  ┌───────────────────────────────┐  │    │  ┌─────────────────────────────────┐    │
│  │    Thunder Daemon (binary)    │  │    │  │  GTest Binary (plugin_test)     │    │
│  │                               │  │    │  │                                 │    │
│  │  PluginHost.cpp (main)        │  │    │  │  main() from gtest_main         │    │
│  │        ↓                      │  │    │  │        ↓                        │    │
│  │  PluginHost::Server           │  │    │  │  ThunderTestRuntime             │    │
│  │  ├── Controller               │  │    │  │  ├── PluginHost::Server         │    │
│  │  ├── PluginServer             │  │    │  │  │   ├── Controller             │    │
│  │  ├── SystemInfo               │  │    │  │  │   ├── PluginServer           │    │
│  │  └── HTTP/WS Listener         │  │    │  │  │   ├── SystemInfo             │    │
│  │        ↓                      │  │    │  │  │   └── HTTP/WS Listener       │    │
│  │  Plugin .so (dynamic load)    │  │    │  │  └── Plugin .so (dynamic load)  │    │
│  └───────────────────────────────┘  │    │  └─────────────────────────────────┘    │
│                                     │    │                                         │
│  Communication: HTTP/WS/COMRPC      │    │  Communication: HTTP/WS/COMRPC          │
│                                     │    │  Tests use in-process direct calls      │
└─────────────────────────────────────┘    └─────────────────────────────────────────┘
```

### How It Works

In production, Thunder runs as a standalone daemon (`PluginHost.cpp` → `main()`). The test support library takes a different approach:

1. **Excludes `PluginHost.cpp`** — this file contains `main()` and daemon lifecycle logic (signal handling, daemonization, and similar startup behavior). The test binary provides its own `main()` via GTest.

2. **Statically links server internals** — `PluginServer.cpp`, `Controller.cpp`, `SystemInfo.cpp`, `PostMortem.cpp`, and `Probe.cpp` are compiled directly into the static library.

3. **Wraps server lifecycle** — `ThunderTestRuntime` provides `Initialize()` and `Deinitialize()` to manage the server, replacing the daemon's startup/shutdown sequence.

4. **Generates config on the fly** — instead of reading `/etc/Thunder/config.json`, the runtime builds a minimal JSON config using `Core::JSON` containers and `Plugin::Config` serialization, then writes it to a temporary directory.

5. **Still opens the listener** — `PluginHost::Server::Open()` still starts the HTTP/WebSocket listener, bound to `127.0.0.1` on an ephemeral port (`PORT=0`).

6. **Plugins load normally** — plugin `.so` files are still loaded dynamically via `dlopen()`, exactly as in production. The `systempath` config entry points to the directory containing plugin shared libraries.

---

## Directory Structure

```
Tests/
├── CMakeLists.txt              ← Modified: added ENABLE_TEST_RUNTIME option
└── test_support/
    ├── CMakeLists.txt          ← Build definitions for the static library
    ├── Module.cpp              ← MODULE_NAME_ARCHIVE_DECLARATION
    ├── ThunderTestRuntime.h    ← Public API header
    ├── ThunderTestRuntime.cpp  ← Implementation
    └── tests/
        ├── CMakeLists.txt      ← Smoke test build
        ├── Module.cpp          ← MODULE_NAME_DECLARATION for smoke test binary
        └── SmokeTest.cpp       ← Self-contained Controller smoke test
```

---

## Public API

### `ThunderTestRuntime`

Defined in `ThunderTestRuntime.h` under `Thunder::TestCore`.

| Member | Description |
|--------|-------------|
| `PluginConfig` | Type alias for `Plugin::Config`. Key fields: `Callsign`, `Locator`, `ClassName`, `StartupOrder`, `StartMode`, `Configuration`. |
| `Initialize()` | Boots the embedded server with given plugins, system path, and proxy stub path. |
| `Deinitialize()` | Stops the server, closes messaging, releases config, cleans up temp directories. |
| `Invoke(method, params, response)` | Calls a JSON-RPC method synchronously via in-process `IDispatcher::Invoke()`. Method format: `"Callsign.method"`. String variant. |
| `Invoke(method, params, response)` | JsonObject overload — handles serialization/deserialization automatically. |
| `CreateJSONRPCLink()` | Returns a callsign-bound `JSONRPCLink` for repeated calls and event subscriptions. Caller must `Release()` when done. |
| `Exists()` | Checks whether a JSON-RPC method is registered on a plugin, using the framework's built-in `"exists"` endpoint. |
| `QueryInterfaceByCallsign<T>()` | Template: obtains a COM-RPC interface from a plugin via `QueryInterface<T>()`. Caller must `Release()` when done. |
| `GetShell()` | Returns the `IShell` proxy for a plugin. |
| `Server()` | Direct access to the underlying `PluginHost::Server`. |
| `CommunicatorPath()` | Returns the UNIX domain socket path used by the communicator. |

### `ThunderTestRuntime::JSONRPCLink`

Callsign-bound helper for JSON-RPC invocations and event subscriptions. Implements `PluginHost::IDispatcher::ICallback` with proper ref-counting — use `Release()` to destroy, do not `delete` directly.

| Member | Description |
|--------|-------------|
| `Invoke(method, params, response)` | Invoke a method by bare name (for example `"echo"`), without repeating the callsign. String variant. |
| `Invoke(method, params, response)` | JsonObject overload — handles serialization/deserialization automatically. |
| `Subscribe(event, handler, index)` | Subscribe to a JSON-RPC event by name with a `std::function<void(const string& designator, const string& index, const string& params)>` callback. Optional `index` parameter for indexed events. |
| `Unsubscribe(event, index)` | Unsubscribe from a previously subscribed event. Optional `index` parameter for indexed events. |
| `Callsign()` | Returns the callsign this link is bound to. |

#### JSON-RPC invocation path

```
Invoke("Callsign.method", params, response)
    ├── Core::JSONRPC::Message::Callsign()  → callsign
    ├── Core::JSONRPC::Message::Method()    → methodName
    ├── Services().FromIdentifier()         → IShell proxy
    ├── shell->QueryInterface<IDispatcher>()
    └── dispatcher->Invoke(0, 0, "", method, params, response)
```

#### COM-RPC interface access

```
QueryInterfaceByCallsign<Exchange::IMyInterface>("Callsign")
    ├── GetShell("Callsign")               → IShell proxy
    └── shell->QueryInterface<T>()         → interface pointer
```

Caller must `Release()` the returned pointer when done.

---

## Build Configuration

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_TEST_RUNTIME` | `OFF` | Build the `thunder_test_support` static library and smoke test. POSIX platforms only. |

### Build Command

```bash
cmake -S Thunder -B build/Thunder \
    -DCMAKE_INSTALL_PREFIX=/path/to/install \
    -DCMAKE_MODULE_PATH=/path/to/install/include/Thunder/Modules \
    -DBINDING=127.0.0.1 \
    -DPORT=0 \
    -DENABLE_TEST_RUNTIME=ON

cmake --build build/Thunder --target install
```

After installation, the library, header, and CMake package config are available at:

```
<prefix>/lib/libthunder_test_support.a
<prefix>/include/Thunder/test_support/ThunderTestRuntime.h
<prefix>/lib/cmake/thunder_test_support/thunder_test_supportConfig.cmake
```

External consumers can then use:

```cmake
find_package(thunder_test_support REQUIRED)
target_link_libraries(my_test PRIVATE thunder_test_support::thunder_test_support)
```

The exported target carries the whole-archive link options automatically, so consumers do not need to add them manually.

---

## Usage Guide

### Writing a Plugin Integration Test

#### 1. CMakeLists.txt (external consumer, after `find_package`)

```cmake
find_package(Thunder REQUIRED)
find_package(ThunderDefinitions REQUIRED)
find_package(thunder_test_support REQUIRED)
find_package(GTest REQUIRED)

add_library(ThunderMyPlugin SHARED
    MyPlugin.cpp
    Module.cpp
)
target_link_libraries(ThunderMyPlugin PRIVATE
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

add_executable(my_plugin_test
    MyPluginTest.cpp
    Module.cpp
)
target_link_libraries(my_plugin_test PRIVATE
    thunder_test_support::thunder_test_support
    GTest::gtest
    GTest::gtest_main
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

if(BUILD_REFERENCE)
    target_compile_definitions(my_plugin_test PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()
```

Each test executable needs a dedicated `Module.cpp`:

```cpp
#define MODULE_NAME MyPluginTest
#include <core/core.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)
```

#### 2. Test fixture

```cpp
#include <gtest/gtest.h>
#include <test_support/ThunderTestRuntime.h>
#include <interfaces/IMyInterface.h>

using namespace Thunder;

class MyPluginTest : public ::testing::Test {
protected:
    static TestCore::ThunderTestRuntime _runtime;

    static void SetUpTestSuite()
    {
        TestCore::ThunderTestRuntime::PluginConfig cfg;
        cfg.Callsign  = "MyPlugin";
        cfg.Locator   = "libThunderMyPlugin.so";
        cfg.ClassName = "MyPlugin";
        cfg.StartMode = PluginHost::IShell::startmode::ACTIVATED;

        std::vector<TestCore::ThunderTestRuntime::PluginConfig> plugins;
        plugins.push_back(cfg);

        ASSERT_EQ(Core::ERROR_NONE,
            _runtime.Initialize(plugins, "/path/to/plugins"));
    }

    static void TearDownTestSuite()
    {
        _runtime.Deinitialize();
    }
};

TestCore::ThunderTestRuntime MyPluginTest::_runtime;
```

#### 3. JSON-RPC via `Invoke()`

```cpp
TEST_F(MyPluginTest, JsonRpcFullDesignator)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE,
        _runtime.Invoke("MyPlugin.echo", R"({"input":"hello"})", response));
    EXPECT_NE(response.find("hello"), string::npos);
}
```

#### 4. JSON-RPC via `JSONRPCLink`

```cpp
TEST_F(MyPluginTest, JsonRpcViaLink)
{
    auto* link = _runtime.CreateJSONRPCLink("MyPlugin");
    ASSERT_NE(link, nullptr);

    string response;
    EXPECT_EQ(Core::ERROR_NONE,
        link->Invoke("echo", R"({"input":"hello"})", response));

    link->Release();
}
```

#### 5. JSON-RPC event subscription

Events do not require a dedicated "trigger" method. Any plugin operation can fire an event internally as a side-effect. Subscribe before calling the operation, then wait for the callback to arrive.

```cpp
TEST_F(MyPluginTest, JSONRPC_OperationTriggersEvent)
{
    auto* link = _runtime.CreateJSONRPCLink("MyPlugin");
    ASSERT_NE(link, nullptr);

    std::mutex mtx;
    std::condition_variable cv;
    string receivedParams;
    bool fired = false;

    link->Subscribe("onStateChanged", [&](const string& /*designator*/, const string& /*index*/, const string& params) {
        std::lock_guard<std::mutex> lock(mtx);
        receivedParams = params;
        fired = true;
        cv.notify_one();
    });

    string response;
    link->Invoke("performAction", R"({"param":"value"})", response);

    {
        std::unique_lock<std::mutex> lock(mtx);
        EXPECT_TRUE(cv.wait_for(lock, std::chrono::milliseconds(2000),
            [&] { return fired; }));
        EXPECT_FALSE(receivedParams.empty());
    }

    link->Unsubscribe("onStateChanged");
    link->Release();
}
```

#### 6. COM-RPC

```cpp
TEST_F(MyPluginTest, ComRpc)
{
    Exchange::IMyInterface* iface =
        _runtime.QueryInterfaceByCallsign<Exchange::IMyInterface>("MyPlugin");
    ASSERT_NE(iface, nullptr);

    string result;
    EXPECT_EQ(Core::ERROR_NONE, iface->SomeMethod("input", result));

    iface->Release();
}
```

#### 7. COM-RPC event subscription

The same principle applies over COM-RPC: register a notification sink before calling the operation, then wait for the callback. The event is fired by the plugin internally as a side-effect of the operation — the test does not need to (and should not) call the plugin's event dispatch directly.

```cpp
class MySink : public Exchange::IMyInterface::INotification {
public:
    void OnStateChanged(const string& state) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _lastState = state;
        _count++;
        _cv.notify_one();
    }

    bool WaitForEvent(uint32_t timeoutMs = 2000, uint32_t expectedCount = 1)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
            [this, expectedCount] { return _count >= expectedCount; });
    }

    string LastState()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _lastState;
    }

    uint32_t Count()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _count;
    }

    BEGIN_INTERFACE_MAP(MySink)
        INTERFACE_ENTRY(Exchange::IMyInterface::INotification)
    END_INTERFACE_MAP

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    string _lastState;
    uint32_t _count = 0;
};

TEST_F(MyPluginTest, COMRPC_OperationTriggersNotification)
{
    auto* iface = _runtime.QueryInterfaceByCallsign<Exchange::IMyInterface>("MyPlugin");
    ASSERT_NE(iface, nullptr);

    Core::SinkType<MySink> sink;
    iface->Register(&sink);

    EXPECT_EQ(Core::ERROR_NONE, iface->PerformAction("active"));

    EXPECT_TRUE(sink.WaitForEvent());
    EXPECT_EQ(sink.LastState(), "active");

    iface->Unregister(&sink);
    iface->Release();
}

TEST_F(MyPluginTest, COMRPC_MultipleOperationsTriggerMultipleNotifications)
{
    auto* iface = _runtime.QueryInterfaceByCallsign<Exchange::IMyInterface>("MyPlugin");
    ASSERT_NE(iface, nullptr);

    Core::SinkType<MySink> sink;
    iface->Register(&sink);

    iface->PerformAction("first");
    iface->PerformAction("second");
    iface->PerformAction("third");

    EXPECT_TRUE(sink.WaitForEvent(2000, 3)) << "Expected 3 notifications";
    EXPECT_EQ(sink.Count(), 3u);
    EXPECT_EQ(sink.LastState(), "third");

    iface->Unregister(&sink);
    iface->Release();
}
```

#### 8. Running the tests

```bash
LD_LIBRARY_PATH=/path/to/install/lib:/path/to/install/lib/thunder/plugins:$LD_LIBRARY_PATH \
    ./my_plugin_test --gtest_color=yes
```

---

## CI Integration

```yaml
- name: Build Thunder with test support
  run: |
    source venv/bin/activate
    cmake -G Ninja -S Thunder -B build/Thunder \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_INSTALL_PREFIX=install/usr \
      -DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules" \
      -DBINDING=127.0.0.1 \
      -DPORT=0 \
      -DENABLE_TEST_RUNTIME=ON
    cmake --build build/Thunder --target install

- name: Build plugin tests
  run: |
    source venv/bin/activate
    cmake -G Ninja -S MyRepo/tests -B build/tests \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_INSTALL_PREFIX=install/usr \
      -DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules" \
      -DCMAKE_PREFIX_PATH="${PWD}/install/usr" \
      -DPLUGIN_MYPLUGIN=ON \
      -DTEST_PLUGIN_PATH="${PWD}/install/usr/lib/thunder/plugins"
    cmake --build build/tests --target install

- name: Run tests
  run: |
    LD_LIBRARY_PATH="install/usr/lib:install/usr/lib/thunder/plugins:$LD_LIBRARY_PATH" \
    ./build/tests/MyPlugin/test/my_plugin_test \
      --gtest_output="xml:test-results.xml" \
      --gtest_color=yes
```

---

## Module.cpp Requirement

Every test executable that links `thunder_test_support` must provide exactly one dedicated `Module.cpp` with:

```cpp
#define MODULE_NAME MyTestBinaryName
#include <core/core.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)
```

This is a per-binary requirement. `thunder_test_support` itself uses `MODULE_NAME_ARCHIVE_DECLARATION` (archive-level, no full declaration), so each consumer binary must supply the full declaration. `BUILD_REFERENCE` should be guarded in CMake:

```cmake
if(BUILD_REFERENCE)
    target_compile_definitions(my_test PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()
```

---

## Design Decisions

### Why a static library?

A static archive ensures all Thunder server symbols are available to the test binary at link time. Shared libraries would require careful `RPATH` management and could conflict with the installed Thunder daemon libraries.

### Why whole-archive semantics?

Thunder uses static initializers extensively (`MODULE_NAME_DECLARATION`, `SERVICE_REGISTRATION`, singletons). Without whole-archive semantics, the linker would discard these symbols because the test binary doesn't reference them directly.

For in-tree builds, this is enforced automatically via `target_link_options(INTERFACE)` scoped to the archive using `$<TARGET_FILE:...>`. The installed CMake package config (`thunder_test_supportConfig.cmake`) applies the same whole-archive flags to the installed `.a` via the `thunder_test_support::thunder_test_support` imported target, so external consumers get correct linking automatically without manual flags.

### Why exclude `PluginHost.cpp`?

`PluginHost.cpp` contains:
- `main()` — conflicts with GTest's `main()`
- Signal handler setup — unwanted in test context
- Daemonization logic — not applicable to tests
- Command-line argument parsing — replaced by programmatic config

### Why generate config programmatically?

- **Isolation**: Each test run gets a unique temp directory (`thunder_test_<pid>_<ticks>_<n>`), preventing interference between parallel test runs.
- **Simplicity**: Tests declare plugins as C++ structs rather than maintaining JSON config files.
- **Portability**: Paths are resolved at runtime, making tests work across build environments and CI systems.

### Why port 0?

Using port `0` tells the OS to assign an available ephemeral port, avoiding conflicts when multiple test processes run simultaneously. Tests typically invoke JSON-RPC in-process via `IDispatcher::Invoke()` rather than through the HTTP listener, so the actual bound port is rarely needed.

### `JSONRPCLink` ref-counting

`JSONRPCLink` implements `IDispatcher::ICallback`. Thunder's dispatcher calls `AddRef()` / `Release()` on the callback when subscribing and unsubscribing. The callback therefore uses a real atomic ref-count with `delete this` on zero. Always call `Release()` when you are done with a `JSONRPCLink`, never `delete` it directly.

---

## Smoke Test

A self-contained smoke test (`Tests/test_support/tests/SmokeTest.cpp`) is included. It verifies the library links, boots the embedded server, and can exercise JSON-RPC against the built-in Controller plugin — no external plugin `.so` files needed.

Covered cases:

- **ControllerStatusViaFullDesignator** — calls `Controller.status` and verifies a non-empty response
- **ControllerSubsystemsViaFullDesignator** — calls `Controller.subsystems` and verifies a non-empty response
- **ControllerStatusViaJSONRPCLink** — same via `JSONRPCLink`
- **GetControllerShell** — obtains the Controller `IShell` and verifies it is valid
- **UnknownMethodReturnsError** — verifies `Core::ERROR_UNKNOWN_METHOD` for unregistered methods
- **UnknownMethodViaJSONRPCLinkReturnsError** — same via `JSONRPCLink`
- **MissingCallsignReturnsError** — verifies error when callsign is missing from designator
- **QueryInterfaceByCallsign_ControllerPlugin** — obtains `IPlugin` from Controller via `QueryInterfaceByCallsign<T>()`
- **QueryInterfaceByCallsign_NonExistentCallsignReturnsNull** — verifies `nullptr` for a non-existent callsign
- **QueryInterfaceByCallsign_UnsupportedInterfaceReturnsNull** — verifies `nullptr` when querying an unsupported interface
- **QueryInterfaceByCallsign_ControllerDispatcher** — obtains `IDispatcher` from Controller via `QueryInterfaceByCallsign<T>()`
- **InvokeWithJsonObjectViaFullDesignator** — calls `Controller.status` using `JsonObject` overload
- **InvokeWithJsonObjectViaLink** — same via `JSONRPCLink` with `JsonObject` overload
- **InvokeWithJsonObjectUnknownMethodReturnsError** — verifies `Core::ERROR_UNKNOWN_METHOD` with `JsonObject` overload

Running:

```bash
LD_LIBRARY_PATH=/path/to/install/lib:$LD_LIBRARY_PATH \
    ./thunder_test_runtime_smoke --gtest_color=yes
```