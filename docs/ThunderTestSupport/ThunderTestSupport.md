# Thunder Test Support Library

## Overview

The **thunder_test_support** library enables in-process integration testing of Thunder plugins without launching the Thunder daemon. It embeds the Thunder `PluginHost::Server` into a static archive (`.a`) that test binaries link against, allowing GTest-based tests to boot a real Thunder runtime, load plugins as shared libraries, and exercise both JSON-RPC and COM-RPC interfaces — all within a single process.

### Key Properties

| Property | Value |
|----------|-------|
| Library type | Static archive (`libthunder_test_support.a`) |
| CMake option | `ENABLE_TEST_RUNTIME=ON` |
| Location | `Tests/test_support/` |
| Public header | `ThunderTestRuntime.h` |
| Install paths | `lib/libthunder_test_support.a`, `include/thunder_test_support/ThunderTestRuntime.h` |

---

## Architecture

### Production Thunder vs Test Support

```
┌─────────────────────────────────────┐     ┌─────────────────────────────────────────┐
│       PRODUCTION DEPLOYMENT         │     │          TEST DEPLOYMENT                │
│                                     │     │                                         │
│  ┌───────────────────────────────┐  │     │  ┌─────────────────────────────────┐    │
│  │    Thunder Daemon (binary)    │  │     │  │   GTest Binary (plugin_test)    │    │
│  │                               │  │     │  │                                 │    │
│  │  PluginHost.cpp (main)        │  │     │  │  main() from gmock_main         │    │
│  │        ↓                      │  │     │  │        ↓                        │    │
│  │  PluginHost::Server           │  │     │  │  ThunderTestRuntime             │    │
│  │  ├── Controller               │  │     │  │  ├── PluginHost::Server         │    │
│  │  ├── PluginServer             │  │     │  │  │   ├── Controller             │    │
│  │  ├── SystemInfo               │  │     │  │  │   ├── PluginServer           │    │
│  │  └── HTTP/WS Listener         │  │     │  │  │   ├── SystemInfo             │    │
│  │        ↓                      │  │     │  │  │   └── (no HTTP listener)     │    │
│  │  Plugin .so (dynamic load)    │  │     │  │  └── Plugin .so (dynamic load)  │    │
│  └───────────────────────────────┘  │     │  └─────────────────────────────────┘    │
│                                     │     │                                         │
│  Communication: HTTP/WS/COMRPC      │     │  Communication: Direct in-process calls │
└─────────────────────────────────────┘     └─────────────────────────────────────────┘
```

### How It Works

In production, Thunder runs as a standalone daemon (`PluginHost.cpp` → `main()`). The test support library takes a different approach:

1. **Excludes `PluginHost.cpp`** — this file contains `main()` and daemon lifecycle logic (signal handling, daemonization, etc.). The test binary provides its own `main()` via GTest.

2. **Statically links server internals** — `PluginServer.cpp`, `Controller.cpp`, `SystemInfo.cpp`, `PostMortem.cpp`, and `Probe.cpp` are compiled directly into the static library.

3. **Wraps server lifecycle** — `ThunderTestRuntime` provides `Initialize()` and `Shutdown()` to manage the server, replacing the daemon's startup/shutdown sequence.

4. **Generates config on the fly** — instead of reading `/etc/Thunder/config.json`, the runtime builds a minimal JSON config programmatically and writes it to a temporary directory.

5. **Plugins load normally** — plugin `.so` files are still loaded dynamically via `dlopen()`, exactly as in production. The `systempath` config entry points to the directory containing plugin shared libraries.

---

## Files Added to Thunder

### Directory Structure

```
Tests/
├── CMakeLists.txt                       ← Modified: added ENABLE_TEST_RUNTIME option
└── test_support/
    ├── CMakeLists.txt                   ← Build definitions for the static library
    ├── Module.cpp                       ← MODULE_NAME definition (ThunderTestRuntime)
    ├── ThunderTestRuntime.h             ← Public API header
    └── ThunderTestRuntime.cpp           ← Implementation
```

### 1. `Tests/CMakeLists.txt` (Modified)

Two additions were made to the existing file:

```cmake
# New option added alongside existing test options
option(ENABLE_TEST_RUNTIME "Build Thunder test support library for plugin integration tests" OFF)

# New conditional block at the end of the file
if(ENABLE_TEST_RUNTIME)
    add_subdirectory(test_support)
endif()
```

This follows the same pattern used by other test options (`LOADER_TEST`, `WORKERPOOL_TEST`, etc.) — off by default, enabled explicitly.

### 2. `Tests/test_support/CMakeLists.txt` (New)

Builds the `thunder_test_support` static library. Key design decisions:

- **Source files**: Compiles `ThunderTestRuntime.cpp`, `Module.cpp`, plus Thunder server sources directly from `Source/Thunder/` (PluginServer, Controller, SystemInfo, PostMortem, Probe).
- **Excludes `PluginHost.cpp`**: This contains `main()` and would conflict with the test binary's entry point.
- **Compile definitions**: Sets `APPLICATION_NAME=ThunderTestRuntime` and `THREADPOOL_COUNT=4`.
- **Public dependencies**: Exposes `ThunderCore`, `ThunderCOM`, `ThunderPlugins`, `ThunderMessaging`, `ThunderWebSocket`, `ThunderCOMProcess`, and `Threads` as PUBLIC link dependencies, so consumers automatically get all required libraries.
- **Private dependencies**: `CompileSettings` is linked PRIVATE to apply Thunder's compile flags without propagating them.
- **Conditional features**: Supports `WARNING_REPORTING`, `PROCESSCONTAINERS`, and `HIBERNATESUPPORT` when enabled.
- **Install rules**: Installs the `.a` archive to `lib/` and the header to `include/thunder_test_support/`.

### 3. `Tests/test_support/Module.cpp` (New)

Module definition required by Thunder's internal logging/tracing macros. The `MODULE_NAME` value appears in trace output to identify which component emitted a message. Using `ThunderTestRuntime` makes it easy to distinguish test runtime traces from production daemon or plugin traces:

```cpp
#ifndef MODULE_NAME
#define MODULE_NAME ThunderTestRuntime
#endif
```

### 4. `Tests/test_support/ThunderTestRuntime.h` (New)

Public API header. Defines the `Thunder::TestCore::ThunderTestRuntime` class:

| Member | Description |
|--------|-------------|
| `PluginConfig` (struct) | Describes a plugin to load: callsign, locator (.so name), classname, autostart, startuporder, configuration JSON |
| `Initialize()` | Boots the embedded server with given plugins, system path, and proxy stub path |
| `InvokeJSONRPC()` | Calls a JSON-RPC method synchronously via in-process `IDispatcher::Invoke()` |
| `GetInterface<T>()` | Template: obtains a COM-RPC interface from a plugin via `QueryInterface<T>()` |
| `GetShell()` | Returns the `IShell` proxy for a plugin (for activation/deactivation control) |
| `Server()` | Direct access to the underlying `PluginHost::Server` |
| `CommunicatorPath()` | Returns the UNIX domain socket path |
| `Shutdown()` | Stops the server, releases config, cleans up temp directories |

### 5. `Tests/test_support/ThunderTestRuntime.cpp` (New)

Implementation with the following lifecycle:

```
Initialize()
    ├── Create temp dir: /tmp/thunder_test_XXXXXX/
    ├── Create subdirs: persistent/, volatile/, data/
    ├── Build JSON config string from PluginConfig list
    ├── Write config to temp dir
    ├── Parse config into PluginHost::Config
    ├── Construct PluginHost::Server
    └── Call Server::Open()
            ├── Set up security
            ├── ServiceMap::Open()
            ├── Activate Controller plugin
            └── Activate auto-start plugins

Shutdown()
    ├── Server::Close()
    │       ├── Deactivate all plugins
    │       └── Close connections
    ├── Delete Server
    ├── Delete Config
    ├── Remove config file
    └── Remove temp directories
```

#### JSON-RPC Invocation Path

`InvokeJSONRPC()` bypasses HTTP/WebSocket entirely:

```
InvokeJSONRPC("<Callsign>", "<Callsign>.1.<method>", params, response)
    ├── Services().FromIdentifier("<Callsign>") → IShell proxy
    ├── shell->QueryInterface<IDispatcher>() → dispatcher
    └── dispatcher->Invoke(0, 0, "", method, params, response)
```

This calls the plugin's JSON-RPC handler directly in-process, with zero network overhead.

#### COM-RPC Interface Access

`GetInterface<T>()` provides direct COM-RPC access:

```
GetInterface<Exchange::IMyInterface>("<Callsign>")
    ├── GetShell("<Callsign>") → IShell proxy
    └── shell->QueryInterface<Exchange::IMyInterface>() → interface pointer
```

The caller receives a real COM-RPC interface pointer and must call `Release()` when done.

---

## Build Configuration

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_TEST_RUNTIME` | `OFF` | Build the thunder_test_support static library |
| `BUILD_TESTS` | `OFF` | Build Thunder unit tests (independent of test runtime) |

### Build Command

```bash
cmake -S Thunder -B build/Thunder \
    -DCMAKE_INSTALL_PREFIX=/path/to/install \
    -DCMAKE_MODULE_PATH=/path/to/install/tools/cmake \
    -DENABLE_TEST_RUNTIME=ON

cmake --build build/Thunder --target install -j$(nproc)
```

After installation, the library and header are available at:
```
<install_prefix>/lib/libthunder_test_support.a
<install_prefix>/include/thunder_test_support/ThunderTestRuntime.h
```

---

## Usage Guide

### Writing a Plugin Integration Test

#### 1. CMakeLists.txt for the test

```cmake
find_package(Thunder REQUIRED)
find_package(ThunderDefinitions REQUIRED)

# Build the plugin as a shared library
add_library(ThunderMyPluginImpl SHARED
    plugin/MyPlugin.cpp
    plugin/MyPluginImpl.cpp
    plugin/Module.cpp
)
target_link_libraries(ThunderMyPluginImpl PRIVATE
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

# Build the test executable
add_executable(my_plugin_test MyPluginTest.cpp)
target_link_libraries(my_plugin_test PRIVATE
    -Wl,--whole-archive
    thunder_test_support
    -Wl,--no-whole-archive
    gmock_main
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)
```

> **Note**: `--whole-archive` is required for `thunder_test_support` to ensure Thunder's static initializers (MODULE_NAME_DECLARATION, service registrations, etc.) are linked even though the test binary doesn't reference them directly.

#### 2. Test fixture

```cpp
#include <gtest/gtest.h>
#include "ThunderTestRuntime.h"
#include <interfaces/IMyInterface.h>

using namespace Thunder;

class MyPluginTest : public ::testing::Test {
protected:
    static TestCore::ThunderTestRuntime _runtime;

    static void SetUpTestSuite() {
        std::vector<TestCore::ThunderTestRuntime::PluginConfig> plugins;

        TestCore::ThunderTestRuntime::PluginConfig cfg;
        cfg.callsign     = "MyPlugin";
        cfg.locator       = "libThunderMyPluginImpl.so";
        cfg.classname     = "MyPlugin";
        cfg.autostart     = true;
        cfg.startuporder  = 50;
        cfg.configuration = R"({"root":{"mode":"Off","locator":"libThunderMyPluginImpl.so"}})";
        plugins.push_back(cfg);

        uint32_t result = _runtime.Initialize(plugins,
            "/path/to/plugins/",
            "/path/to/proxystubs/");
        ASSERT_EQ(result, Core::ERROR_NONE);
    }

    static void TearDownTestSuite() {
        _runtime.Shutdown();
    }
};

TestCore::ThunderTestRuntime MyPluginTest::_runtime;

// JSON-RPC test
TEST_F(MyPluginTest, JsonRpcCall) {
    string response;
    EXPECT_EQ(Core::ERROR_NONE,
        _runtime.InvokeJSONRPC("MyPlugin", "MyPlugin.1.someMethod", R"({"param":1})", response));
    // Validate response...
}

// COM-RPC test
TEST_F(MyPluginTest, ComRpcCall) {
    Exchange::IMyInterface* iface = _runtime.GetInterface<Exchange::IMyInterface>("MyPlugin");
    ASSERT_NE(iface, nullptr);
    // Call interface methods...
    iface->Release();
}

// Lifecycle test
TEST_F(MyPluginTest, DeactivateReactivate) {
    auto shell = _runtime.GetShell("MyPlugin");
    ASSERT_TRUE(shell.IsValid());
    EXPECT_EQ(Core::ERROR_NONE, shell->Deactivate(PluginHost::IShell::reason::REQUESTED));
    EXPECT_EQ(Core::ERROR_NONE, shell->Activate(PluginHost::IShell::reason::REQUESTED));
}
```

#### 3. Running

```bash
export LD_LIBRARY_PATH=/path/to/install/lib:/path/to/install/lib/thunder/plugins:/path/to/install/lib/thunder/proxystubs
export THUNDER_PLUGIN_PATH=/path/to/install/lib/thunder/plugins/
export THUNDER_PROXYSTUB_PATH=/path/to/install/lib/thunder/proxystubs/

./my_plugin_test --gtest_color=yes
```

---

## CI Integration

The library is designed for use in GitHub Actions workflows. A typical workflow:

```yaml
- name: Build Thunder (with test runtime)
  run: >
    cmake -S Thunder -B build/Thunder
    -DCMAKE_INSTALL_PREFIX=${{github.workspace}}/install/usr
    -DCMAKE_MODULE_PATH=${{github.workspace}}/install/tools/cmake
    -DENABLE_TEST_RUNTIME=ON
    &&
    cmake --build build/Thunder --target install -j8

- name: Build plugin and tests
  run: >
    cmake -S my-plugin/tests -B build/tests
    -DCMAKE_INSTALL_PREFIX=${{github.workspace}}/install/usr
    -DCMAKE_MODULE_PATH=${{github.workspace}}/install/tools/cmake
    &&
    cmake --build build/tests --target install -j8

- name: Run tests
  run: |
    export LD_LIBRARY_PATH=${{github.workspace}}/install/usr/lib:...
    my_plugin_test --gtest_output="xml:test-results.xml"
```

---

## Design Decisions

### Why a static library?

A static archive ensures that all Thunder server symbols are available to the test binary at link time. Shared libraries would require careful `RPATH` management and could conflict with the installed Thunder daemon libraries.

### Why `--whole-archive`?

Thunder uses static initializers extensively (`MODULE_NAME_DECLARATION`, `SERVICE_REGISTRATION`, singletons). Without `--whole-archive`, the linker would discard these symbols because the test binary doesn't reference them directly. The `--whole-archive` flag forces all object files from the archive to be included.

### Why exclude `PluginHost.cpp`?

`PluginHost.cpp` contains:
- `main()` — conflicts with GTest's `main()`
- Signal handler setup — unwanted in test context
- Daemonization logic — not applicable to tests
- Command-line argument parsing — replaced by programmatic config

### Why generate config programmatically?

- **Isolation**: Each test run gets a unique temp directory, preventing interference between parallel test runs.
- **Simplicity**: Tests declare plugins as C++ structs rather than maintaining JSON config files.
- **Portability**: Paths are resolved at runtime, making tests work across different build environments and CI systems.

### Why port 0?

Using port 0 in the config tells the OS to assign an available port, avoiding conflicts when multiple test processes run simultaneously.

---