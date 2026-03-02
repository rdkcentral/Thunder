# GitHub Copilot Instructions — Thunder Framework

## Architecture Overview

Thunder is a **plugin-based device abstraction framework** for embedded platforms (ARM/MIPS), developed by Metrological (a Comcast company). All business logic is implemented as plugins loaded and managed by a central `PluginHost::Server`. Thunder itself provides no end-user functionality — plugins do.

### Repository Map
| Repo | Role |
|------|------|
| `Thunder/` | Core daemon, libraries (`core/`, `com/`, `plugins/`), `ThunderPlugin` host binary |
| `ThunderTools/` | Code generators: `ProxyStubGenerator`, `JsonGenerator`, `ConfigGenerator`, `LuaGenerator` |
| `ThunderInterfaces/` | Shared COM-RPC interface headers (`interfaces/I*.h`) and generated JSON-RPC wrappers (`jsonrpc/`) |
| `ThunderNanoServices/` | Metrological-maintained plugins (not used in RDK) |
| `rdkservices/` | RDK-specific plugins consuming ThunderInterfaces |
| `ThunderUI/` | Dev/test web UI running over JSON-RPC |

### Key Layers (bottom-up)
| Layer | Location | Role |
|-------|----------|------|
| `core/` | `Source/core/` | Platform primitives: threads, sockets, JSON, IPC, timers, `ResourceMonitor` |
| `com/` | `Source/com/` | COM-RPC: `RPC::Communicator`, `Core::IUnknown`, proxy/stub registration |
| `plugins/` | `Source/plugins/` | Plugin framework API: `IPlugin`, `IShell`, `ISubSystem`, `JSONRPC` |
| `Thunder/` | `Source/Thunder/` | Runtime host: `Server`, `ServiceMap`, `Controller`, `PluginServer.h` |
| `ThunderInterfaces/interfaces/` | — | Cross-plugin COM interfaces (e.g. `IStore2.h`, `IBrowser.h`) |
| `ThunderNanoServices/` | — | Plugin implementations: `Dictionary`, `NetworkControl`, `TimeSync`, etc. |
| `rdkservices/` | — | RDK-specific plugins consuming ThunderInterfaces |

### Data Flow: Incoming Request → Plugin
1. `Server::ChannelMap` (TCP/WebSocket) accepts connection → `Channel`
2. `Channel` deserializes JSON-RPC / HTTP, enforces `ISecurity` (token-based)
3. `ServiceMap::FromLocator()` resolves URL callsign → `Service` wrapping the plugin
4. `Service::Invoke()` dispatches via `IDispatcher` (JSONRPC) or `IWeb`
5. Jobs are throttled via `ThrottleQueueType` and dispatched on the `WorkerPool`

### Plugin Lifecycle States
```
UNAVAILABLE → DEACTIVATED → PRECONDITION → ACTIVATION → ACTIVATED → DEACTIVATION → DEACTIVATED
                                                             └── SUSPENDED / RESUMED (if IStateControl)
                                                             └── HIBERNATED (if hibernate support built)
```
- **PRECONDITION**: plugin waits until declared subsystem preconditions are all active; auto-transitions to ACTIVATED when met.
- **Activation** always calls `Initialize()` / library load; **Deactivation** always calls `Deinitialize()` / library unload — making live library upgrades possible while deactivated.
- All plugins are briefly loaded/unloaded at startup (regardless of `startmode`) to read version metadata.

### Subsystem System
Subsystems (PLATFORM, NETWORK, SECURITY, INTERNET, GRAPHICS, TIME, IDENTIFIER, BLUETOOTH, …) are a global boolean bitmap tracking which platform capabilities are available. Plugins set subsystems active/inactive; other plugins gate on them via `precondition`/`termination` config. Only subsystems listed in a plugin's `control` metadata may be toggled by that plugin (`SubSystemsControlled` enforcement).

---

## Critical Developer Workflows

### Build System
All builds use **CMake + Ninja** with a shared `install/` prefix. **Build order is mandatory** — each repo's installed artifacts are required by the next:

```
ThunderTools → Thunder → ThunderInterfaces → ThunderNanoServices / rdkservices
```

#### 1. ThunderTools (code generators — must be first)
```bash
cmake -G Ninja -S ThunderTools -B build/ThunderTools \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PWD}/install"
cmake --build build/ThunderTools --target install
```

#### 2. Thunder (core daemon)
```bash
cmake -G Ninja -S Thunder -B build/Thunder \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PWD}/install" \
  -DTOOLS_SYSROOT="${PWD}/install" \
  -DBINDING="127.0.0.1" \
  -DPORT=55555 \
  -DINITV_SCRIPT=OFF \
  -DENABLED_TRACING_LEVEL=3
cmake --build build/Thunder --target install
```

#### 3. ThunderInterfaces
```bash
cmake -G Ninja -S ThunderInterfaces -B build/ThunderInterfaces \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PWD}/install" \
  -DCMAKE_PREFIX_PATH="${PWD}/install"
cmake --build build/ThunderInterfaces --target install
```

#### Rebuild a single repo (clean)
```bash
cmake --build build/Thunder --target clean
cmake --build build/Thunder --target install
```

#### macOS-Specific Build Notes
- Use the same CMake commands as Linux — no platform-specific flags needed for basic builds.
- Add `-DHIDE_NON_EXTERNAL_SYMBOLS=ON` to enable `-fvisibility=hidden` (recommended, matches production builds).
- Proxy stubs and plugins are `.dylib` on macOS, `.so` on Linux — CMake handles this automatically.
- For development, set `CMAKE_INSTALL_RPATH` to include `${CMAKE_INSTALL_PREFIX}/lib` so dylibs resolve at runtime without `DYLD_LIBRARY_PATH`.
- ThunderTools generators (`ProxyStubGenerator`, `JsonGenerator`) are Python scripts — ensure the venv is activated (`source bin/activate`) before building repos that depend on code generation.
- Apple Clang warnings may differ from GCC — fix any `-Wshorten-64-to-32` or `-Wunused-private-field` warnings that GCC doesn't emit.

### Running Thunder
```bash
install/bin/Thunder -c install/etc/Thunder/config.json
```
Default config is at `/etc/Thunder/config.json` on Linux. Key config fields: `port`, `binding`, `communicator` (COM-RPC socket), `persistentpath`, `datapath`, `systempath`, `proxystubpath`, `configs` (plugin config dir).

### Plugin Config Files
Plugin JSON configs live in `install/etc/Thunder/plugins/` (or wherever `configs` points). These are **hot-reloaded** at runtime via `ConfigObserver` if the directory is watched (`observe.configpath`). Similarly, proxy stubs are hot-reloaded via `observe.proxystubpath`.

---

## Project-Specific Patterns

> Layer-specific rules for this repo are in `.github/instructions/` and activate automatically:
> - `core.instructions.md` → `Source/core/**`
> - `com.instructions.md` → `Source/com/**`
> - `plugins.instructions.md` → `Source/plugins/**`
> - `thunder-runtime.instructions.md` → `Source/Thunder/**`
>
> Scoped instructions for other repos live in their own `.github/instructions/`:
> - `ThunderInterfaces/.github/instructions/interfaces.instructions.md`
> - `rdkservices/.github/instructions/rdkservices.instructions.md`

### Cross-Cutting Rules (apply everywhere)
- **All COM interface methods must return `Core::hresult`** — `Core::ERROR_NONE` (0) on success.
- **Interfaces are immutable** once published — only append new methods, never change or remove existing ones.
- **No STL containers across COM boundaries** — use `RPC::IIteratorType<T, ID>` iterators.
- **No exceptions** (`-fno-exceptions`) — signal errors via return codes only; use `ASSERT()` for invariants.
- **Virtual inheritance mandatory**: all COM interfaces use `virtual public Core::IUnknown`.
- **All setup/teardown in `Initialize()`/`Deinitialize()`** — never in constructors/destructors.
- **`IShell*` ref-count**: call `AddRef()` when storing it; `Release()` in `Deinitialize()`.
- **Config path tokens**: use `%datapath%`, `%persistentpath%`, `%volatilepath%`, `%systempath%`, `%proxystubpath%` — never hardcode absolute paths.

### Execution Modes (plugin `configuration.root.mode`)
| Mode | Meaning |
|------|---------|
| `Off` | In-process (default if no `root` block) |
| `Local` | Out-of-process in a dedicated `ThunderPlugin` child process |
| `Container` | Out-of-process in a container |
| `Distributed` | Out-of-process on a remote device via COM-RPC over TCP |

---

## Messaging & Debugging

### Message Types
- **TRACE**: developer-only, compiled out in `MinSizeRel` production builds. Enabled per-module/category in config.
- **SYSLOG/Logging**: production-safe, always present.
- **Warning Reporting**: conditional, requires `WARNING_REPORTING` build option (`REPORT_OUTOFBOUNDS_WARNING`, `REPORT_DURATION_WARNING`).

Enable categories in `config.json` under `defaultmessagingcategories`. Route output via `MessageControl` plugin (console, syslog, file, UDP, WebSocket).

### COM-RPC Traffic Debugging (ThunderShark)
Capture COM-RPC traffic by switching the communicator to TCP, then use Wireshark + the ThunderShark Lua dissector:
```json
{ "communicator": "127.0.0.1:62000" }
```
```bash
tcpdump -i lo port 62000 -w /tmp/comrpc-traffic-dump.pcap
./GenerateLua.sh Thunder/ ThunderInterfaces/   # generates protocol-thunder-comrpc.data
```
Filter in Wireshark: `thunder-comrpc` — shows method names, parameters, results, call duration. Failed calls: `thunder-comrpc.invoke.hresult != 0`.

### Error Handling
- **No exceptions**: Thunder is built with `-fno-exceptions`. Never throw in plugin code.
- Check COM-RPC transport errors vs plugin errors: `if (result & COM_ERROR) { /* transport failure */ }`.
- `Core::ErrorToString(code)` converts error codes to readable strings.
- Post-mortem dumps (worker pool state, callstacks) are written to `postmortempath` on exit reasons listed in `exitreasons` config.

---

## Key Files for Navigation

| Purpose | File |
|---------|------|
| Plugin API contracts | `Source/plugins/IPlugin.h`, `IShell.h`, `ISubSystem.h` |
| JSONRPC dispatcher | `Source/plugins/JSONRPC.h` |
| Plugin config schema (C++) | `Source/plugins/Configuration.h` |
| Server + ServiceMap (core runtime) | `Source/Thunder/PluginServer.h` (~4800 lines) |
| Thunder global config schema (C++) | `Source/Thunder/Config.h` |
| Controller plugin | `Source/Thunder/Controller.h` / `Controller.cpp` |
| COM-RPC communicator | `Source/com/Communicator.h` |
| COM-RPC administrator (stub dispatch) | `Source/com/Administrator.h` |
| IPC message types & IIPC interface | `Source/core/IPCConnector.h` (defines `IIPC`, `IPCMessageType<>`) |
| IPC channel (server & client) | `Source/core/IPCChannel.h` |
| Network interface abstraction | `Source/core/NetworkInfo.h` / `NetworkInfo.cpp` |
| Socket address abstraction | `Source/core/NodeId.h` / `NodeId.cpp` |
| Resource monitor (fd event loop) | `Source/core/ResourceMonitor.h` |
| Ref-counted smart pointer | `Source/core/Proxy.h` (`ProxyType<T>`, `ProxyObject<T>`) |
| Platform portability macros | `Source/core/Portability.h` |
| Synchronization primitives | `Source/core/Sync.h` |
| COM interface IDs (core) | `Source/com/Ids.h` |
| Documentation | `docs/` |

---

## Code Style

### Formatter
Thunder uses **clang-format** (WebKit-based). Always run `clang-format -i <file>` on any modified C++ file before committing.

Key `.clang-format` settings:
| Setting | Value |
|---------|-------|
| `BasedOnStyle` | `WebKit` |
| `IndentWidth` | `4` (spaces only, no tabs) |
| `PointerAlignment` | `Left` — `int* ptr`, not `int *ptr` |
| `ColumnLimit` | `0` (no line-length limit enforced) |
| `BreakBeforeBraces` | `WebKit` — function/class braces on a new line; control-flow braces on same line |
| `AccessModifierOffset` | `-4` — access modifiers dedented to column 0 relative to class body |
| `NamespaceIndentation` | `Inner` — inner namespaces indented |
| `BreakConstructorInitializers` | `BeforeComma` — comma-first style for init lists |
| `AllowShortFunctionsOnASingleLine` | `All` |

### Naming Conventions
| Construct | Convention | Examples |
|-----------|-----------|---------|
| Private / protected member variable | `_camelCase` | `_adminLock`, `_handler`, `_refCount`, `_skipURL` |
| Class / struct | `PascalCase` | `CriticalSection`, `ServiceMap`, `WorkerPoolImplementation` |
| COM interface | `I` + `PascalCase` | `IPlugin`, `IShell`, `IWorkerPool`, `IMyInterface` |
| Method (any visibility) | `PascalCase` | `Initialize()`, `AddRef()`, `QueryInterface()` |
| Template parameter | `UPPER_CASE` | `IMPLEMENTATION`, `CONTEXT`, `ELEMENT`, `FORWARDER` |
| Enum value | `UPPER_CASE` | `ACTIVATED`, `DEACTIVATED`, `ERROR_NONE` |
| Macro | `UPPER_CASE` | `EXTERNAL`, `ASSERT`, `TRACE_L1`, `BEGIN_INTERFACE_MAP` |
| Namespace | `PascalCase` | `Thunder`, `Core`, `PluginHost`, `Plugin`, `RPC`, `Exchange` |
| `using` type alias | `PascalCase` | `using Clients = std::map<string, Entry>;` |
| Include guard | `__UPPER_CASE_H__` | `#ifndef __IPLUGIN_H__` |

> **Legacy note**: older files (e.g. `Source/core/Thread.h`) use `m_PascalCase` for members. Do **not** use this in new code — use `_camelCase`.

### Non-Copyable / Non-Movable Classes
Declare all four deleted special members explicitly at the top of the `private:` section (before any other methods):
```cpp
class MyClass {
private:
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;
    MyClass(MyClass&&) = delete;
    MyClass& operator=(MyClass&&) = delete;
    ...
};
```
Alternatively use `= delete` on the public section if the class is value-semantic but non-copyable by design (as seen in `BinarySemaphore`).

### Virtual Methods & Overrides
- Always add `override` on every method that overrides a virtual base — never rely on implicit override.
- Interface destructors must be `= default` (never define a body): `~IMyInterface() override = default;`
- Concrete final classes should be marked `final`: `class MyImpl final : public IMyInterface`.

### Constructor Initializer Lists
Use **comma-first** (BeforeComma) style matching clang-format:
```cpp
Power()
    : _adminLock()
    , _skipURL(0)
    , _clients()
    , _sink(this)
    , _currentState(Exchange::IPower::PCState::On)
{
}
```

### Inner Notification / Sink Class Pattern
Observer/notification inner classes follow this canonical pattern (from `Power.h`):
```cpp
class Notification
    : public PluginHost::IPlugin::INotification {
public:
    Notification() = delete;
    Notification(const Notification&) = delete;
    Notification& operator=(const Notification&) = delete;

    explicit Notification(MyPlugin* parent)
        : _parent(*parent)
    {
        ASSERT(parent != nullptr);
    }
    ~Notification() override = default;

    void Activated(const string& callsign, PluginHost::IShell* plugin) override { ... }
    void Deactivated(const string& callsign, PluginHost::IShell* plugin) override { ... }
    void Unavailable(const string&, PluginHost::IShell*) override {}

    BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
    END_INTERFACE_MAP

private:
    MyPlugin& _parent;   // reference, never pointer
};
```
Store the sink using `Core::SinkType<Notification> _sink;` in the plugin class.

### COM Interface Map
Every class that exposes COM interfaces must declare the interface map immediately before or after public method declarations:
```cpp
BEGIN_INTERFACE_MAP(MyPlugin)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IDispatcher)
    INTERFACE_ENTRY(Exchange::IMyInterface)
    INTERFACE_AGGREGATE(IAnotherInterface, _member)  // delegate to a sub-object
END_INTERFACE_MAP
```

### Warning Suppression
Use Thunder's portable macros — never raw `#pragma`:
```cpp
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
MyPlugin()
    : _sink(this)  // safe here
{
}
POP_WARNING()
```
Other common suppressors: `DISABLE_WARNING_DEPRECATED_USE`, `DISABLE_WARNING_MULTIPLE_INHERITANCE_OF_BASE_CLASS`.

### Include Guards
Use the `__UPPER_CASE_H__` double-underscore pattern — **not** `#pragma once`:
```cpp
#ifndef __MYPLUGIN_H__
#define __MYPLUGIN_H__
...
#endif // __MYPLUGIN_H__
```

### Custom Config Class
Extend `Core::JSON::Container`. Member fields are public JSON types, registered in the constructor with `Add()`:
```cpp
class Config : public Core::JSON::Container {
private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

public:
    Config()
        : Core::JSON::Container()
        , Retries(3)
        , Endpoint(_T(""))
    {
        Add(_T("retries"), &Retries);
        Add(_T("endpoint"), &Endpoint);
    }

public:
    Core::JSON::DecUInt32 Retries;
    Core::JSON::String Endpoint;
};
```

### Smart Pointers & Reference Counting
- Use `Core::ProxyType<T>` for heap-allocated ref-counted objects.
- Raw `IUnknown*` pointers obtained via `QueryInterface()` **must** be manually `Release()`d.
- Use `Core::SinkType<T>` to embed a notification sink that participates in `QueryInterface`.
- Never delete a COM object directly — always call `Release()`.

### No Exceptions
Thunder is compiled with **`-fno-exceptions`**. Never use `throw`, `try`, or `catch`. Signal errors exclusively via `Core::hresult` return values. Use `ASSERT()` for invariant violations during development.

---

## macOS / Apple Clang Portability

Thunder must build and run on macOS (Apple Clang + libc++) in addition to the primary Linux (GCC + libstdc++) target. macOS introduces several critical differences:

### Cross-Dylib `dynamic_cast` Failure
**Apple Clang unconditionally emits template class typeinfo as "weak private external"** regardless of any visibility attributes, `#pragma`, or explicit template instantiation. Each `.dylib` gets its own private copy of the typeinfo pointer. Since `libc++` compares typeinfo by **pointer only** (unlike `libstdc++` which falls back to `strcmp`), `dynamic_cast` across dylib boundaries **silently returns `nullptr`** for template types.

**Affected pattern in Thunder**: `Core::IPCMessageType<ID, PARAMS, RESPONSE>` (a class template inheriting both `IIPC` and `IReferenceCounted`). Downcasting from `Core::IIPC*` → `IPCMessageType<...>*` or cross-casting to `IReferenceCounted*` fails on macOS even though the object genuinely has that type.

**Rule**: Never use `dynamic_cast` to downcast or cross-cast `Core::ProxyType<Core::IIPC>` to a specific `IPCMessageType<>` specialization (e.g. `InvokeMessage`, `AnnounceMessage`) across shared library boundaries. Instead, use the `Label()` discriminator + `static_cast`:
```cpp
// ✅ Correct — works on both macOS and Linux
ASSERT(data->Label() == InvokeMessage::Id());
InvokeMessage* raw = static_cast<InvokeMessage*>(&(*data));

// ❌ Broken on macOS — dynamic_cast returns nullptr across dylibs for template types
Core::ProxyType<InvokeMessage> message(data);  // uses dynamic_cast internally
```

**Non-template classes are fine**: `dynamic_cast` works correctly across dylibs for non-template classes with `visibility("default")` (the `EXTERNAL` macro). The issue is **specific to C++ class templates** on Apple Clang.

**Reference**: [Qt blog — dynamic_cast across library boundaries](https://www.qt.io/quality-assurance/blog/one-way-dynamic_cast-across-library-boundaries-can-fail-and-how-to-fix-it). Their fix (import macro with `visibility("default")`) works for non-template classes but does **not** fix template typeinfo on Apple Clang.

### IPC Message Architecture (`IPCConnector.h`)
The IPC message hierarchy in `Source/core/IPCConnector.h` is central to COM-RPC:
```
IIPC (interface)                     ← has Label(), IParameters(), IResponse()
IReferenceCounted                    ← has AddRef(), Release()
  └── IPCMessageType<ID, P, R>      ← class template inheriting BOTH; Label() == ID
        └── InvokeMessage  = IPCMessageType<0, ...>
        └── AnnounceMessage = IPCMessageType<1, ...>
```
- `IIPC` and `IReferenceCounted` are **non-template, EXTERNAL** → their typeinfo is properly exported.
- `IPCMessageType<N, P, R>` is a **template** → its typeinfo is private-per-dylib on macOS.
- `Core::ProxyType<IIPC>` uses `dynamic_cast` internally for type conversion (via `ProxyType` constructor).
- `IIPC::Label()` returns a `uint32_t` discriminator (`InvokeMessage::Id() == 0`, `AnnounceMessage::Id() == 1`) and is the **safe way** to identify message type at runtime.

### macOS-Specific Platform Abstractions
- **NetworkInfo** (`Source/core/NetworkInfo.h`/`.cpp`): macOS uses `getifaddrs()` + `sysctl()` (via `PF_ROUTE`/`RTM_IFLIST2`) instead of Linux `netlink` / `/proc/net`. The `AdapterIterator` implementation is fully platform-switched.
- **`getaddrinfo` portability**: `EAI_NONAME` is not available on all platforms as a macro — use runtime error code checking rather than compile-time constants in preprocessor guards.
- **DHCPClient**: requires conditional compilation for macOS (no raw sockets for DHCP without root).
- **Shared library extension**: `.dylib` on macOS, `.so` on Linux — CMake handles this via `CMAKE_SHARED_LIBRARY_SUFFIX`.
- **RPATH**: macOS requires `@rpath` / `@loader_path` for dylib resolution; use `CMAKE_INSTALL_RPATH` and `CMAKE_BUILD_WITH_INSTALL_RPATH` in CMake.
- **`LD_LIBRARY_PATH`**: does not work on macOS — use `DYLD_LIBRARY_PATH` or set rpaths at build time.

### Symbol Visibility on macOS
- Thunder uses `-fvisibility=hidden -fvisibility-inlines-hidden` (behind `HIDE_NON_EXTERNAL_SYMBOLS=ON`).
- The `EXTERNAL` macro applies `__attribute__((visibility("default")))` — use it on all classes/functions that cross dylib boundaries.
- **Template visibility caveat**: even with `EXTERNAL` on a template class, Apple Clang emits its typeinfo as "weak private external". This is an Apple Clang compiler behavior, not a configuration issue. `nm` will show `(was a private external)` for template typeinfo symbols.

---

## Conventions & Gotchas

- **`EXTERNAL` macro**: marks symbols for shared library export. Missing it causes linker failures in OOP scenarios. On macOS, it also affects `dynamic_cast` behavior for non-template classes (see macOS section above).
- **`/* @stubgen:omit */`**: excludes a class/method from proxy/stub generation — used for in-process-only APIs (e.g. `IShell::ICOMLink`).
- **`DEPRECATED` macro**: wraps deprecated methods kept for ABI compatibility — never remove, wrap only.
- **Virtual inheritance**: all COM interfaces must use `virtual public Core::IUnknown` to avoid diamond-problem reference count issues.
- **Thread safety**: `_adminLock` guards the `ServiceMap`; `_pluginHandling` guards per-plugin interface pointers. Never hold both simultaneously.
- **`Core::ProxyType<T>`**: reference-counted smart pointer. Uses `dynamic_cast` internally for type conversion — be aware of the macOS template typeinfo limitation (see above). Raw pointers from `QueryInterface` must be manually `Release()`d.
- **Subsystem preconditions** use AND logic (all must be set); **terminations** use NOT_ values with OR logic — see `Service::Condition` in `PluginServer.h`.
- **Interfaces are immutable**: never change or remove existing COM interface methods — only append. Bump JSON-RPC version tag on breaking changes.
- **`startuporder`** in plugin config (default 50): lower values activate earlier among auto-start plugins.
- **`BUILD_REFERENCE`**: pass the git hash at configure time (`-DBUILD_REFERENCE=$(git rev-parse HEAD)`) so version is traceable at runtime via Controller.
- **`-flat_namespace`** (macOS linker flag): **never use** — it merges all symbol namespaces across all dylibs, breaking plugin isolation and causing unpredictable symbol conflicts in Thunder's plugin architecture.
