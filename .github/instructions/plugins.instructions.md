---
name: Thunder-Plugin-Framework-API
description: 'Rules for working in Source/plugins/ — IPlugin, IShell, JSONRPC, ISubSystem, Channel, Service'
applyTo: 'Source/plugins/**'
---

# Thunder `plugins/` Layer — Plugin Framework API

`Source/plugins/` defines the **public contract between the Thunder daemon and all plugins**: `IPlugin`, `IShell`, `ISubSystem`, `JSONRPC`, `Channel`, and `Service`. This is the API surface that every plugin author consumes. Changes here affect the entire plugin ecosystem.

## Layer Scope
- Depends on `Source/core/` and `Source/com/`.
- **Never** include `Source/Thunder/` headers — the runtime implementation is above this layer.
- Public API exported via `plugins/plugins.h`.

## `IPlugin` — Mandatory Plugin Interface
Every plugin class must implement `IPlugin`:
```cpp
const string Initialize(PluginHost::IShell* service) override;  // "" on success
void Deinitialize(PluginHost::IShell* service) override;
string Information() const override;                             // may return {}
```
- `Initialize()` returns **empty string on success**, or a human-readable error string on failure (which causes the plugin to stay deactivated).
- All resource acquisition (sockets, threads, interface pointers, subscriptions) happens in `Initialize()` — **never in the constructor**.
- All resource release happens in `Deinitialize()` — **never in the destructor**.
- The constructor and destructor must be trivial — Thunder loads/unloads plugins briefly at startup just to read version metadata.

## `IShell*` Lifetime Rules
- `IShell*` passed to `Initialize()` is **not** automatically ref-counted for the plugin's use.
- If you store `IShell*` past the `Initialize()` call, you **must** call `service->AddRef()`.
- Release your stored `IShell*` in `Deinitialize()` with `_service->Release(); _service = nullptr;`.
- `IShell*` is only valid between `Initialize()` and `Deinitialize()` — never call it after `Deinitialize()` returns.
- `IShell::ICOMLink` is only available in the main process — it returns `nullptr` in OOP plugins. Always null-check.

## `JSONRPC` — JSON-RPC Dispatcher Mixin
Inherit `PluginHost::JSONRPC` alongside `IPlugin` to expose JSON-RPC methods:
```cpp
class MyPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC { ... };
```

**Registration** (in `Initialize()`):
```cpp
Register<Params, Result>(_T("mymethod"), &MyPlugin::MyMethod, this);
// property (getter + setter):
Property<Core::JSON::String>(_T("myprop"), &MyPlugin::GetProp, &MyPlugin::SetProp, this);
```

**Unregistration** (in `Deinitialize()`):
```cpp
Unregister(_T("mymethod"));
Unregister(_T("myprop"));
```
- Every `Register` must have a matching `Unregister` — leaking registrations causes method collisions if the plugin is re-activated.
- Built-in methods (`versions`, `exists`, `register`, `unregister`) are reserved — never register handlers with those names.

**Events**:
```cpp
Notify(_T("myevent"), params);          // broadcast to all subscribed WS clients
Notify(_T("myevent"), params, sendIf); // conditional — sendIf(clientId) filters recipients
```

## `IPlugin::INotification` — Plugin Lifecycle Observer
```cpp
class MySink : public PluginHost::IPlugin::INotification {
public:
    void Activated(const string& callsign, PluginHost::IShell*) override { ... }
    void Deactivated(const string& callsign, PluginHost::IShell*) override { ... }
    void Unavailable(const string&, PluginHost::IShell*) override {}

    BEGIN_INTERFACE_MAP(MySink)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
    END_INTERFACE_MAP
};
```
- Register with `service->Register(&_sink)` in `Initialize()`.
- Unregister with `service->Unregister(&_sink)` in `Deinitialize()`.
- Store notification sinks as `Core::SinkType<MySink> _sink` in the plugin — it participates in `QueryInterface` without heap allocation.
- Incoming `IShell*` pointers in `Activated`/`Deactivated` are not ref-counted for you — `AddRef()` if you store them.

## `ISubSystem` — Subsystem State
- Access via `service->SubSystems()` which returns `ISubSystem*` (ref-counted, must `Release()`).
- Set a subsystem active: `subsystems->Set(ISubSystem::NETWORK, nullptr)`.
- Set a subsystem inactive: `subsystems->Set(ISubSystem::NOT_NETWORK, nullptr)`.
- A plugin may only control subsystems listed in its `Metadata<>` `control` parameter — enforced at runtime.
- Subscribe to subsystem changes via `ISubSystem::INotification` — unsubscribe in `Deinitialize()`.

## Plugin Metadata Registration
Register metadata in the plugin's `.cpp` at static initialisation time (not in a header):
```cpp
static Plugin::Metadata<MyPlugin> metadata(
    1, 0, 0,                             // Major, Minor, Patch
    { ISubSystem::NETWORK },             // preconditions (all must be active to activate)
    { ISubSystem::NOT_NETWORK },         // terminations (any triggers deactivation)
    { ISubSystem::TIME }                 // subsystems this plugin controls
);
```

## `PluginHost::IPluginExtended` and `PluginHost::ICompositPlugin`
- `PluginHost::IPluginExtended`: extends `IPlugin` with `Attach(channelId)` / `Detach(channelId)` — called when a WebSocket or HTTP channel connects or disconnects.
- HTTP request handling is provided by `PluginHost::IWeb`, which adds `Inbound(Web::Request&)` and `Process(const Web::Request&)`. Implement `IWeb` (not `IPluginExtended`) when exposing HTTP endpoints.
- `PluginHost::ICompositPlugin`: for plugins that aggregate sub-plugins — rare, check existing examples before implementing.

## Custom Config
Extend `Core::JSON::Container`; parse in `Initialize()`:
```cpp
class Config : public Core::JSON::Container {
public:
    Config()
        : Core::JSON::Container()
        , Retries(3)
    {
        Add(_T("retries"), &Retries);
    }
    Core::JSON::DecUInt32 Retries;
};
// In Initialize():
Config config;
config.FromString(service->ConfigLine());
```
- Always read config in `Initialize()` from `service->ConfigLine()` — never from a global or constructor.
- Use `service->DataPath()`, `service->PersistentPath()`, `service->VolatilePath()` for derived paths — never hardcode paths.

## `Channel` — WebSocket Connection
- `Channel` represents an active client connection (WebSocket or HTTP).
- Each channel has a `uint32_t` ID — used for filtering `Notify()` calls.
- `IShell::IConnectionServer::INotification` delivers `Opened(channelId)` / `Closed(channelId)` — use to track per-client state.

## Interface Map in Plugin Classes
Always declare the interface map at the top of the `public:` section body, covering every interface the class implements:
```cpp
BEGIN_INTERFACE_MAP(MyPlugin)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IDispatcher)   // auto-inherited from JSONRPC
    INTERFACE_ENTRY(Exchange::IMyInterface)
END_INTERFACE_MAP
```

## `DEPRECATED` Methods
- Wrap deprecated methods with the `DEPRECATED` macro — never remove them (ABI stability).
- New code must not call deprecated methods internally.

## Error Codes
- Return `Core::ERROR_NONE` (0) on success.
- `Core::ERROR_UNAVAILABLE`, `Core::ERROR_ILLEGAL_STATE`, `Core::ERROR_BAD_REQUEST` for common failure modes.
- Use `Core::ErrorToString(code)` when logging errors.
- Check for transport vs plugin errors: `if (result & COM_ERROR) { /* transport failure */ }`.
- `Core::ERROR_TIMEDOUT` indicates a COM-RPC call exceeded `CommunicationTimeOut`. It is a distinct code from `Core::ERROR_ASYNC_ABORTED` — do not treat them as equivalent.

## External COM-RPC Clients (Standalone Tools)
For standalone tools or test apps that connect to Thunder as COM-RPC clients (not as plugins):

### Using `SmartInterfaceType<T>`
```cpp
#include <com/com.h>
#include <core/core.h>

class MyClient : public RPC::SmartInterfaceType<Exchange::IMyInterface> {
    void Operational(const bool upAndRunning) override {
        if (upAndRunning) { /* connected */ }
        else { /* disconnected */ }
    }
};

int main() {
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T("/tmp/communicator"));
    MyClient client(
        Core::NodeId("/tmp/communicator"),
        _T("MyPlugin"),  // callsign
        ~0               // version (any)
    );
    client.Open(5000);  // 5s timeout
    auto* iface = client.Interface();
    if (iface) {
        // Use the interface...
        iface->Release();
    }
    client.Close(Core::infinite);
    Core::Singleton::Dispose();
    return 0;
}
```

### CMake for External Clients
```cmake
find_package(Thunder REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
target_link_libraries(mytool
    PRIVATE ${NAMESPACE}Core::${NAMESPACE}Core
            ${NAMESPACE}COM::${NAMESPACE}COM)
```

### RPATH Considerations
- On Linux, use `$ORIGIN/../lib` or absolute paths via `CMAKE_INSTALL_RPATH`.

## Plugin Example Structure (`ThunderNanoServices/examples/`)
Plugin examples follow a standard layout:
```
examples/MyPlugin/
    CMakeLists.txt
    MyPlugin.h          # IPlugin implementation
    MyPlugin.cpp         # Initialize/Deinitialize, metadata registration
    MyPlugin.conf.in     # Plugin config template (processed by ConfigGenerator)
    Module.h             # MODULE_NAME define
    Module.cpp           # Module registration
```

Standalone test apps (non-plugin, COM-RPC clients) have a simpler structure:
```
examples/MyTest/
    CMakeLists.txt
    MyTest.cpp           # main() using CommunicatorClient or SmartInterfaceType
```

## `ISubSystem` — Full Pattern

```cpp
class Notification : public PluginHost::ISubSystem::INotification {
public:
    explicit Notification(MyPlugin* parent) : _parent(*parent) { ASSERT(parent != nullptr); }
    ~Notification() override = default;
    void Updated() override { _parent.OnSubsystemUpdate(); }

    BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::ISubSystem::INotification)
    END_INTERFACE_MAP
private:
    MyPlugin& _parent;
};

const string Initialize(PluginHost::IShell* service) override
{
    PluginHost::ISubSystem* subsystems = service->SubSystems();
    ASSERT(subsystems != nullptr);

    // Query a subsystem's current state
    bool networkReady = (subsystems->IsActive(PluginHost::ISubSystem::NETWORK) == true);

    // Set a subsystem active (plugin must declare it in metadata 'control' list)
    subsystems->Set(PluginHost::ISubSystem::TIME, nullptr);

    // Subscribe to subsystem state changes
    subsystems->Register(&_subSink);

    subsystems->Release();  // SubSystems() returns a ref — must Release
    return {};
}

void Deinitialize(PluginHost::IShell* service) override
{
    PluginHost::ISubSystem* subsystems = service->SubSystems();
    if (subsystems != nullptr) {
        subsystems->Unregister(&_subSink);
        subsystems->Release();
    }
}
```

## `IShell::COMLink()` — In-Process Direct COM Access

`IShell::ICOMLink` provides direct COM-RPC access within the same process (bypassing JSON-RPC overhead):

```cpp
// Only available in in-process mode — always null-check
PluginHost::IShell::ICOMLink* link = service->QueryInterface<PluginHost::IShell::ICOMLink>();
if (link != nullptr) {
    // Connect directly to a remote Thunder instance or another in-process service
    Exchange::IMyInterface* iface = link->Acquire<Exchange::IMyInterface>(
        5000 /* timeout ms */,
        Core::NodeId("/tmp/communicator"),
        _T("MyPlugin"),
        ~0 /* any version */
    );
    if (iface != nullptr) {
        iface->DoSomething();
        iface->Release();
    }
    link->Release();
}
```

## `IShell::QueryInterfaceByCallsign<T>()` — Cross-Plugin Interface Acquisition

```cpp
// ✅ Correct — standard pattern for acquiring another plugin's interface
Exchange::INetworkControl* ctrl =
    service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
if (ctrl == nullptr) {
    return _T("NetworkControl plugin not available or not activated");
}
// ctrl carries one ref — store it; no extra AddRef needed
_networkCtrl = ctrl;

// In Deinitialize():
if (_networkCtrl != nullptr) {
    _networkCtrl->Release();
    _networkCtrl = nullptr;
}
```

If the target plugin can deactivate independently, subscribe to `IPlugin::INotification` and release in the `Deactivated()` callback for that callsign.

## `IPlugin::INotification::Unavailable()` — Must Always Be Implemented

```cpp
// ✅ Must implement — it is pure virtual even if behavior is a no-op
void Unavailable(const string& callsign, PluginHost::IShell* plugin) override
{
    // Implementation may be empty, but the method must be present
}

// ❌ Wrong — not providing this method causes a linker error
```

## Cross-Reference

- For complete plugin development workflow: see `navigation.md` and `docs/plugin/`.
- For subsystem config (`precondition`, `termination`): see `constraints.md` (Initialize/Deinitialize Contract) and `docs/plugin/subsystems.md`.
- For JSON-RPC event notification patterns: see `docs/plugin/messaging.md`.
- For interface design: see `docs/plugin/interfaces/guidelines.md`.
