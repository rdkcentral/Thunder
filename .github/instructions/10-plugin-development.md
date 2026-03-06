---
applyTo: '**'
---

# 10 — Plugin Development

> This is the primary reference for writing a Thunder plugin from scratch. Follow this checklist in order. Cross-reference the detailed files for each topic as indicated.

## Plugin Development Checklist

1. Design the COM interface in `ThunderInterfaces/interfaces/IMyFeature.h` (see `07-interface-driven-development.md`)
2. Allocate an interface ID in `ThunderInterfaces/interfaces/Ids.h`
3. Rebuild `ThunderInterfaces` to generate proxy/stub files
4. Create the plugin directory with the standard layout (see Structure section below)
5. Implement `IPlugin` in `MyPlugin.h` / `MyPlugin.cpp`
6. Register the plugin class with `Plugin::Metadata<>` (or `SERVICE_REGISTRATION` for the implementation class)
7. Write the plugin config JSON file
8. Add the plugin to the CMakeLists.txt build
9. Test `Initialize()` → `Deinitialize()` cycle, including error paths

## Standard Plugin Directory Layout

```
MyPlugin/
├── CMakeLists.txt
├── Module.h             ← MODULE_NAME define + common includes
├── Module.cpp           ← MODULE_NAME registration
├── MyPlugin.h           ← IPlugin implementation class declaration
├── MyPlugin.cpp         ← Initialize/Deinitialize, metadata registration
└── MyPlugin.conf.in     ← Plugin config template (processed by ConfigGenerator)
```

## `Module.h` and `Module.cpp`

```cpp
// Module.h
#ifndef __MODULE_MYPLUGIN_H__
#define __MODULE_MYPLUGIN_H__
#ifndef MODULE_NAME
#define MODULE_NAME MyPlugin
#endif
#include <core/core.h>
#include <plugins/plugins.h>
#endif

// Module.cpp
#include "Module.h"
MODULE_NAME_DECLARATION(BUILD_REFERENCE)
```

`Module.h` must be the **first** include in every `.cpp` file in the plugin.

## Plugin Registration and Metadata

The daemon locates a plugin by `dlopen`-ing the library named in the top-level `locator` config field, then calling the exported `GetModuleServices()` symbol to obtain the `IService` registry. It then calls `ServiceAdministrator::Instantiate<IPlugin>()` with the class name from the config. There is **no `PLUGINHOST_ENTRY_POINT` macro** — the registration is done through one of the two mechanisms below.

### Modern: `Plugin::Metadata<T>` (preferred)

Declare a static `Plugin::Metadata<>` object at file scope in a `.cpp` file. This registers both the class factory **and** embeds the precondition/termination/control subsystem metadata in the library:

```cpp
// In MyPlugin.cpp, at file scope (inside an anonymous namespace is fine)
namespace {
    static Plugin::Metadata<Plugin::MyPlugin> metadata(
        1, 0, 0,                                 // Major.Minor.Patch version
        { ISubSystem::NETWORK },                 // preconditions (AND logic — all must be active)
        { ISubSystem::NOT_NETWORK },             // terminations (OR logic — any triggers deactivation)
        { ISubSystem::TIME }                     // subsystems this plugin controls (enforced)
    );
}
```

The `Plugin::Metadata<T>` template is defined in `Source/plugins/Metadata.h`. It inherits from `Core::PublishedServiceType<T, MetadataType<T>>`, which both registers the class factory and exposes `IMetadata` so the daemon can read preconditions, terminations, and control lists at startup — before `Initialize()` is ever called.

The subsystem lists use AND logic for preconditions and OR logic for terminations. The `control` list is enforced — the plugin may only set/clear subsystems listed there.

### Legacy: `SERVICE_REGISTRATION` macro

Older plugins and implementation classes use the `SERVICE_REGISTRATION` macro from `Source/core/Services.h`:

```cpp
// At namespace scope in a .cpp file:
SERVICE_REGISTRATION(MyPlugin, 1, 0)          // major, minor (patch defaults to 0)
SERVICE_REGISTRATION(MyPlugin, 1, 0, 3)       // or with patch
```

This creates a `Core::PublishedServiceType<MyPlugin>` which registers the class factory but **does not embed subsystem metadata**. When `SERVICE_REGISTRATION` is used, the daemon reads precondition, termination, and control subsystem lists from the plugin config JSON instead — they are not compiled into the library. This macro is also the correct choice for **implementation classes** (the object returned by `service->Root<T>()`) that are not `IPlugin` derivatives.


## Complete `IPlugin` Implementation Template

```cpp
// MyPlugin.h
#pragma once  // Exception — headers that are never included by other headers may use this
              // But the preferred style is the __GUARD__ pattern (see 03-code-style.md)
#include "Module.h"
#include <interfaces/IMyFeature.h>

namespace Thunder {
namespace Plugin {

    class MyPlugin final
        : public PluginHost::IPlugin
        , public PluginHost::JSONRPC {
    public:
        MyPlugin(const MyPlugin&) = delete;
        MyPlugin& operator=(const MyPlugin&) = delete;
        MyPlugin(MyPlugin&&) = delete;
        MyPlugin& operator=(MyPlugin&&) = delete;

    private:
        // Notification sink inner class (see 04-design-patterns.md)
        class Notification : public PluginHost::IPlugin::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            explicit Notification(MyPlugin* parent) : _parent(*parent) { ASSERT(parent != nullptr); }
            ~Notification() override = default;
            void Activated(const string& callsign, PluginHost::IShell* plugin) override;
            void Deactivated(const string& callsign, PluginHost::IShell*) override;
            void Unavailable(const string&, PluginHost::IShell*) override {}

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP
        private:
            MyPlugin& _parent;
        };

    public:
        MyPlugin()
            : _adminLock()
            , _service(nullptr)
            , _sink(this)
        {
        }
        ~MyPlugin() override = default;

        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

        BEGIN_INTERFACE_MAP(MyPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        // JSONRPC method implementations
        Core::hresult GetStatus(Core::JSON::String& response);

        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Core::SinkType<Notification> _sink;
    };

} // namespace Plugin
} // namespace Thunder
```

```cpp
// MyPlugin.cpp
#include "MyPlugin.h"

namespace Thunder {
namespace Plugin {

    const string MyPlugin::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);  // Initialize must only be called once

        _service = service;
        _service->AddRef();

        // Register JSON-RPC handlers
        Register<void, Core::JSON::String>(_T("getstatus"), &MyPlugin::GetStatus, this);

        // Subscribe to plugin lifecycle notifications
        _service->Register(&_sink);

        return {};  // empty = success
    }

    void MyPlugin::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        // Unregister in reverse order of registration
        _service->Unregister(&_sink);
        Unregister(_T("getstatus"));

        _service->Release();
        _service = nullptr;
    }

    Core::hresult MyPlugin::GetStatus(Core::JSON::String& response)
    {
        Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
        response = _T("OK");
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder

// At file scope — registers the class factory and embeds subsystem metadata
namespace { static Plugin::Metadata<Plugin::MyPlugin> metadata(1, 0, 0, {}, {}, {}); }
```

## JSON-RPC Method Registration

```cpp
// Regular method: void params, typed result
Register<void, Core::JSON::DecUInt32>(_T("getcount"), &MyPlugin::GetCount, this);

// Typed params and result
Register<MyParams, MyResult>(_T("process"), &MyPlugin::Process, this);

// Property (getter + optional setter)
Property<Core::JSON::String>(_T("name"), &MyPlugin::GetName, &MyPlugin::SetName, this);

// Property getter-only
Property<Core::JSON::String>(_T("version"), &MyPlugin::GetVersion, nullptr, this);
```

**Every `Register` must have a matching `Unregister` in `Deinitialize()`** — leaking registrations causes collisions if the plugin is re-activated.

## JSON-RPC Events (Notifications to Clients)

```cpp
// Broadcast to all subscribed clients
void MyPlugin::OnValueChanged(uint32_t newValue)
{
    Core::JSON::DecUInt32 params;
    params = newValue;
    Notify(_T("onvaluechanged"), params);
}

// Conditional broadcast — only to specific clients
void MyPlugin::OnPrivateEvent(uint32_t clientId, const string& data)
{
    Core::JSON::String params;
    params = data;
    Notify(_T("onprivateevent"), params, [clientId](const string& id) {
        return id == std::to_string(clientId);
    });
}
```

## `ISubSystem` — Subsystem Access

```cpp
const string Initialize(PluginHost::IShell* service) override
{
    // Acquire subsystem interface
    PluginHost::ISubSystem* subsystems = service->SubSystems();
    ASSERT(subsystems != nullptr);

    // Set a subsystem active (plugin must declare it in metadata control list)
    subsystems->Set(PluginHost::ISubSystem::TIME, nullptr);

    // Subscribe to changes
    subsystems->Register(&_subsystemSink);

    subsystems->Release();  // Release after use — SubSystems() returns a ref
    return {};
}
```

## Acquiring Interfaces from Other Plugins

> **Read the design note in the Best Practices section first** — direct plugin-to-plugin interface coupling has architectural implications and should not be the default approach. If you have decided it is appropriate, use `PluginSmartInterfaceType<T>` rather than raw `QueryInterfaceByCallsign`.

If you need a raw one-shot interface pointer (e.g. during `Initialize()` to a plugin guaranteed to be active first):

```cpp
networkCtrl = service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
if (networkCtrl == nullptr) {
    return _T("NetworkControl not available");
}
// QueryInterfaceByCallsign already AddRef'd — do not AddRef again

//After usage release
if (networkCtrl != nullptr) {
    networkCtrl->Release();
    networkCtrl = nullptr;
}
```

For plugins that can deactivate independently at any time, the raw pointer approach is unsafe. Use `PluginSmartInterfaceType<T>` (see Best Practices → Direct Plugin-to-Plugin Coupling) which automatically invalidates the interface and fires `Operational(false)` when the target deactivates.

## Plugin Config JSON File

```json
{
  "callsign": "MyPlugin",
  "classname": "MyPlugin",
  "locator": "libThunderMyPlugin.so",
  "autostart": true,
  "startuporder": 50,
  "configuration": {
    "retries": 3,
    "endpoint": "127.0.0.1:8080"
  }
}
```

| Field | Required | Meaning |
|-------|---------|---------|
| `callsign` | Yes | Unique identifier used in URLs and inter-plugin references |
| `classname` | Yes | C++ class name registered via `Plugin::Metadata<>` or `SERVICE_REGISTRATION` |
| `locator` | Yes | Shared library filename (`.so` on Linux, `.dylib` on macOS) |
| `autostart` | No | If `true`, plugin activates at daemon startup (default: `false`) |
| `startuporder` | No | Activation order among autostart plugins — lower number activates first (default: 50) |
| `configuration` | No | Plugin-specific config object; accessed via `service->ConfigLine()` |
| `root` | No | OOP execution config: `{ "mode": "Local", "threads": 4 }` |

For OOP execution (`root` block), see `11-configuration-and-metadata.md`.

## Out-of-Process (OOP) Plugins

Thunder supports running a plugin's **implementation** in a separate process (OOP mode). The `IPlugin` bridge class (which handles `Initialize`/`Deinitialize` and JSONRPC) **always runs in the daemon** — the OOP separation applies to the implementation object acquired via `service->Root<T>()`.

### `mode` Values (in the `root` block)

| `mode` | Meaning |
|---|---|
| `"Off"` (or no `root` block) | In-process — everything in the daemon |
| `"Local"` | OOP — child process spawned by the daemon using the `ThunderPlugin` host binary |
| `"Container"` | OOP inside a container runtime |
| `"Distributed"` | OOP on a remote host (advanced) |

### Library Layout and `service->Root<T>()` — All Combinations

The key config fields are:
- **top-level `locator`**: the library loaded in the **daemon** to get the `IPlugin` class.
- **`root.locator`**: the library loaded into the **ThunderPlugin** (OOP) process to get the implementation class.
- **`root.classname`** (or top-level `classname` if `root.classname` not set): the C++ class name for the implementation inside `root.locator`.

| Scenario | `locator` (daemon) | `root.locator` (OOP process) | `service->Root<T>()` behaviour |
|---|---|---|---|
| **In-process, single library** | `libThunderMyPlugin.so` | not set / `mode: Off` | Loads `root.locator` (falls back to top-level `locator`) from disk, calls `ServiceAdministrator::Instantiate()` — returns an in-process interface pointer. No child process. |
| **In-process, split library** | `libThunderMyPlugin.so` | `libThunderMyPluginImplementation.so` (in `root` block with `mode: Off`) | Loads `root.locator` in-process, instantiates implementation class — no child process |
| **OOP, single library** | `libThunderMyPlugin.so` | not set (falls back to `locator`) | Spawns `ThunderPlugin` child; child loads `libThunderMyPlugin.so` and instantiates the class named by `root.classname`. **Same `.so` is loaded in both daemon and child.** |
| **OOP, split library** ✅ preferred | `libThunderMyPlugin.so` | `libThunderMyPluginImplementation.so` | Spawns `ThunderPlugin` child; daemon loads only `libThunderMyPlugin.so` (clean); child loads `libThunderMyPluginImplementation.so` and instantiates the implementation |

**OOP split-library JSON config**:
```json
{
  "callsign": "MyPlugin",
  "classname": "MyPlugin",
  "locator": "libThunderMyPlugin.so",
  "root": {
    "mode": "Local",
    "locator": "libThunderMyPluginImplementation.so",
    "classname": "MyPluginImplementation",
    "threads": 2
  }
}
```

**CMakeLists split-library structure**:
```cmake
# IPlugin bridge — loaded into Thunder daemon
add_library(ThunderMyPlugin SHARED MyPlugin.cpp)
target_link_libraries(ThunderMyPlugin PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

# Implementation — loaded into ThunderPlugin OOP host process
add_library(ThunderMyPluginImplementation SHARED MyPluginImpl.cpp)
target_link_libraries(ThunderMyPluginImplementation PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
```

### `service->Root<T>()` — What Happens

`service->Root<T>()` is called from `IPlugin::Initialize()` **in the daemon** (on a daemon WorkerPool thread). The full sequence (from `Source/plugins/Shell.cpp` + `Source/Thunder/PluginServer.h`):

1. **Reads the `root` block** from the plugin's config (`Plugin::Config::RootConfig`).
2. **Checks mode**: if `Off` (or no `root` block), performs an in-process `dlopen` of `root.locator` + `ServiceAdministrator::Instantiate()` — no child process is spawned.
3. **For OOP modes**: calls `ICOMLink::Instantiate()` on the daemon's `CommunicatorServer`, which builds an `RPC::Object` descriptor (locator, classname, callsign, interface ID, thread count, etc.).
4. **Spawns `ThunderPlugin`** child process, passing the `RPC::Object` descriptor and the daemon's COM-RPC socket path.
5. **`ThunderPlugin` starts**: initialises its own `WorkerPool` (default 1 thread), opens its own `ResourceMonitor`, connects back to the daemon's COM-RPC socket.
6. **ThunderPlugin loads `root.locator`** and calls `ServiceAdministrator::Instantiate()` for the named class.
7. **ThunderPlugin sends an `ANNOUNCE`** message over the COM-RPC socket with the implementation interface pointer.
8. **Daemon receives the proxy**: `ICOMLink::Instantiate()` returns; `Root<T>()` casts the result to `T*` — this is now a COM-RPC proxy to the OOP implementation.
9. **`Initialize()` continues** with the proxy in hand. All calls through it are cross-process COM-RPC.

```cpp
// In Initialize() — runs in daemon WorkerPool context
uint32_t _connectionId = 0;  // receives the OOP connection ID
_impl = service->Root<Exchange::IMyFeature>(
    _connectionId,                    // out: connection ID (used to Terminate() the process)
    2000,                             // wait timeout ms
    _T("MyPluginImplementation")      // classname in root.locator
);
if (_impl == nullptr) {
    return _T("Failed to acquire IMyFeature implementation — OOP spawn failed or timed out");
}
```

```cpp
// In Deinitialize() — proper OOP teardown
if (_impl != nullptr) {
    RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
    _impl->Release();   // Release proxy — triggers COMRPC teardown
    _impl = nullptr;

    if (connection != nullptr) {
        connection->Terminate();  // Ask ThunderPlugin process to exit
        connection->Release();
    }
}
```

Subscribe to `RPC::IRemoteConnection::INotification` to detect unexpected OOP process death and trigger self-deactivation (see `ThunderNanoServices/examples/PluginSmartInterfaceType/Pollux/` for a complete working example).

## `IShell`, `Service` and Plugin Object Lifecycle

Understanding the relationship between `IShell`, `Service`, and the plugin object is essential for writing correct plugin code.

### Hierarchy

```
Core::IUnknown
  └── PluginHost::IShell          ← COM interface exposed to plugins
        └── PluginHost::Service   ← Abstract base in Source/plugins/Service.h
              └── Server::Service ← Concrete implementation in Source/Thunder/PluginServer.h
                                    (also implements IShell::ICOMLink, IConnectionServer)
```

- **`IShell`** is the interface your plugin receives in `Initialize(PluginHost::IShell* service)`. It gives access to paths, config, subsystems, COM-RPC, and plugin management.
- **`PluginHost::Service`** (in `Source/plugins/`) is the abstract base holding common state: callsign, locator, config JSON, state machine (`_state`), path resolution, and web-server prefix logic.
- **`Server::Service`** (in `Source/Thunder/PluginServer.h`) is the concrete class owned by `ServiceMap`. It holds the `IPlugin*` pointer (`_handler`), the `ThrottleQueue` of incoming JSONRPC jobs, and the COM-RPC `ExternalAccess` endpoint.

### Plugin Object Lifecycle vs. Service Lifetime

| Phase | `Service` object | Plugin (`IPlugin*`) object |
|---|---|---|
| **Config file parsed at startup** | `Service` created, state = `DEACTIVATED` or `PRECONDITION` | Not yet instantiated |
| **`LoadMetadata()`** | Daemon `dlopen`s `locator`, calls `GetModuleServices()` to read `Plugin::Metadata<>` data (version, preconditions, control list); library may be unloaded immediately after | Not yet instantiated |
| **`Activate()` called** | State → activated path begins; `AcquireInterfaces()` runs | `ServiceAdministrator::Instantiate<IPlugin>()` called — **plugin object constructed** |
| **`Initialize()` called** | State → `ACTIVATED` on success; back to `DEACTIVATED` on non-empty return | Plugin `Initialize()` runs on a daemon WorkerPool thread |
| **Running** | State = `ACTIVATED` | Plugin object alive, processing JSONRPC and COM-RPC calls |
| **`Deactivate()` called** | State → deactivation path begins | `Deinitialize()` called, then `IPlugin*` released; library potentially unloaded |
| **Service destroyed** (daemon shutdown) | `ServiceMap` cleared | Plugin object long gone |

**Key rules that follow from this**:
- The `Service` object **outlives** the `IPlugin` object across many activate/deactivate cycles on the same `Service`.
- Never cache the `IPlugin*` pointer from outside the plugin. Use callsign lookup each time.
- Plugin metadata (preconditions, version) is read **before** `Initialize()` — changing metadata requires a config change and daemon restart.
- The `IShell*` passed to `Initialize()` is the `Service`'s `IShell` face. Storing it with `AddRef()` is safe for the duration of the activated lifetime.



- **No global state** — all state belongs in the plugin class instance
- **No blocking in JSON-RPC handlers** — if an operation takes >1ms, dispatch it as a background job
- **Prefer COM iterator interfaces over raw STL across COM boundaries** — `std::vector` is now supported by the code generator in Thunder 5.x, but `RPC::IIteratorType<>` is still the canonical ABI-stable form (see `06-comrpc-fundamentals.md`)
- **Minimise direct plugin-to-plugin coupling** — see the design note below
- **Always `Unregister` what you `Register`** — in `Deinitialize()`, in reverse order
- **Always `Release` what you `QueryInterface`** — no exceptions
- **Use `ASSERT` for invariants, not runtime checks** — runtime conditions return `Core::hresult`
- **Keep constructors and destructors trivial** — all setup in `Initialize()`, all teardown in `Deinitialize()`

### Direct Plugin-to-Plugin Coupling — Design Note

Thunder's original nano-services philosophy intentionally avoided direct dependencies between plugins. Each plugin is meant to be autonomous, replaceable, and testable in isolation. Using `QueryInterfaceByCallsign<T>()` from one plugin to acquire another plugin's COM interface creates a hard callsign dependency and an activation ordering requirement — this undermines the nano-services model.

That said, this pattern has become common in RDK deployments. **If you do use it**:
- Do **not** use raw `QueryInterfaceByCallsign<T>()` and store the pointer indefinitely — the target plugin can deactivate at any time.
- Use `PluginHost::PluginSmartInterfaceType<T>` (from `Source/plugins/Types.h`) instead. It sets up a plugin monitor, automatically invalidates the interface when the target deactivates, and provides an `Operational()` callback for state changes.
- See the complete worked example in `ThunderNanoServices/examples/PluginSmartInterfaceType/Pollux/`.

```cpp
// ✅ If inter-plugin interface access is unavoidable — use PluginSmartInterfaceType
class MyPlugin : public PluginHost::IPlugin
               , public PluginHost::JSONRPC {
private:
    PluginHost::PluginSmartInterfaceType<Exchange::INetworkControl> _networkCtrl;
public:
    const string Initialize(PluginHost::IShell* service) override {
        // Open() registers the monitor and acquires the interface when available
        _networkCtrl.Open(service, _T("NetworkControl"));
        // ...
        return {};
    }
    void Deinitialize(PluginHost::IShell* service) override {
        _networkCtrl.Close();  // unregisters monitor and releases interface
        // ...
    }
};
```

> **Warning**: If you use `PluginSmartInterfaceType` from **OOP code** (the implementation class running in `ThunderPlugin`), configure `threads` ≥ 2 in the `root` block — the default single thread will deadlock during monitor registration.

### `Deinitialize()` as the Inverted Mirror of `Initialize()`

Thunder has a configuration option that causes `Deinitialize()` to be called when `Initialize()` failed partway through and returns a non-empty string. **Design `Deinitialize()` to handle partial initialisation gracefully** — null-check every resource before releasing it.

Structure `Initialize()` and `Deinitialize()` so that they are exact inverses of each other:

```cpp
// Initialize acquires resources in order: A, B, C
const string MyPlugin::Initialize(PluginHost::IShell* service)
{
    _service = service;
    _service->AddRef();                          // A

    _impl = service->Root<Exchange::IMyFeature>(_pid, 2000, _T("MyPluginImpl"));
    if (_impl == nullptr) {
        return _T("Failed to acquire implementation"); // B fails → A still held, Deinitialize will clean A
    }

    _service->Register(&_sink);                  // C
    return {};
}

// Deinitialize releases in reverse order: C, B, A — and null-checks each
void MyPlugin::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    if (_service != nullptr) {
        _service->Unregister(&_sink);            // C (no-op if C never registered)
    }

    if (_impl != nullptr) {
        _impl->Release();                        // B
        _impl = nullptr;
    }

    _service->Release();                         // A
    _service = nullptr;
}
```

Do **not** duplicate cleanup code inside the failure branches of `Initialize()`. Let `Deinitialize()` be the single authoritative cleanup path.

### Constructor and Destructor Must Be Stateless

A plugin object is instantiated once and may be `Initialize()`d and `Deinit­ialize()`d many times (e.g. on re-activation). **The constructor and destructor must not contain any logic that depends on plugin activation state.**

**Rules**:
- Constructors must only zero-initialise member variables (set pointers to `nullptr`, counters to `0`, etc.).
- Destructors must not access `_service`, COM interfaces, or any resource acquired in `Initialize()`.
- All meaningful work belongs between `Initialize()` and `Deinitialize()`.

```cpp
// ✅ Correct
MyPlugin::MyPlugin()
    : _adminLock()
    , _service(nullptr)
    , _impl(nullptr)
    , _sink(this)
{
    // Nothing else — no subscriptions, no allocations, no AddRef
}

MyPlugin::~MyPlugin()
{
    // Nothing — Deinitialize() already ran
    ASSERT(_service == nullptr);  // Prove it
}

// ❌ Wrong
MyPlugin::MyPlugin()
{
    _timer.Schedule(...);   // No WorkerPool yet at construction time
    _connection = OpenSocket(); // Acquired resource that Deinitialize() won't know to close
}
```

| Mistake | Consequence | Fix |
|---------|------------|-----|
| Storing `IShell*` without `AddRef()` | Dangling pointer after `Initialize()` scope | Always `AddRef()` when storing `IShell*` |
| Not unregistering JSON-RPC methods in `Deinitialize()` | Method collision on re-activation | Unregister every method; use matching names |
| Blocking a `WorkerPool` thread (e.g. `sleep()`) | Stalls all plugin dispatch on that thread | Use `Core::TimerType<T>` for delays |
| Calling plugin methods while holding `_adminLock` | Deadlock if plugin callback tries to acquire `_adminLock` | Release lock before calling out |
| Leaking iterator pointers from COM methods | Reference leak, memory not freed | Always `Release()` iterators after use |
| Using `dynamic_cast` on `IPCMessageType<>` across dylib boundaries on macOS | Silent `nullptr`, crash | Use `Label()` + `static_cast` (see `05-object-lifecycle-and-memory.md`) |
| Accessing `_service` in destructor | Undefined behavior — `_service` is released in `Deinitialize()` | Never access `_service` in destructor |
| Putting activation logic in the constructor | Constructor runs before `Initialize()` — WorkerPool and subsystems are not ready | Move all logic to `Initialize()` / `Deinitialize()` |
| **Using JSON-RPC to communicate between in-process plugins** | Every call is serialised to JSON, dispatched through the `WorkerPool`, then deserialised — adds latency, increases `WorkerPool` load, and bypasses COM-RPC short-circuiting | Use `QueryInterfaceByCallsign<T>()` to get a COM interface pointer and call it directly |
| **Using `RPC::CommunicatorClient` to connect to the daemon's default socket (`/tmp/communicator`) from within a plugin** | Defeats COM-RPC in-process short-circuiting: even if the target plugin is in-process, the call goes out-of-process over the socket, adding unnecessary IPC overhead | Use `IShell::QueryInterfaceByCallsign<T>()` for in-process calls; `CommunicatorClient` is only for external client applications outside the daemon process |

### JSONRPC Between Plugins — Anti-Pattern Detail

Using the JSONRPC layer for plugin-to-plugin communication is a common mistake:

```cpp
// ❌ Wrong — JSONRPC round-trip through WorkerPool for every call
string result;
service->Invoke(_T("NetworkControl"), _T("getstatus"), "{}", result);

// ✅ Correct — direct COM interface call, zero IPC overhead when in-process
Exchange::INetworkControl* net =
    service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
if (net != nullptr) {
    // Call directly on the COM interface
    net->DoSomething();
    net->Release();
}
```

### `CommunicatorClient` from Inside the Daemon — Anti-Pattern Detail

```cpp
// ❌ Wrong — creates a new socket connection back to the daemon even though we ARE the daemon
RPC::CommunicatorClient client(
    Core::NodeId(_T("/tmp/communicator")),
    Core::ProxyType<RPC::InvokeServerType<1,0,4>>::Create());
client.Open(2000);
Exchange::IMyInterface* iface = client.Acquire<Exchange::IMyInterface>(2000, _T("MyPlugin"), ~0);

// ✅ Correct — stay inside the process
Exchange::IMyInterface* iface =
    service->QueryInterfaceByCallsign<Exchange::IMyInterface>(_T("MyPlugin"));
```

COM-RPC's short-circuit mechanism works only when both caller and callee are in the same daemon process. Creating a `CommunicatorClient` to `localhost` or `/tmp/communicator` from within plugin code bypasses this entirely and treats an in-process call as a remote call.
