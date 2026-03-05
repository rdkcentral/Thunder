---
applyTo: '**'
---

# 01 — Architecture

> Cross-reference: `00-project-overview.md` for repo map. Layer-specific rules in scoped `*.instructions.md` files.

## Layer Stack and Inter-Layer Dependency Rules

```
Source/Thunder/        → may include: core/, com/, plugins/
Source/plugins/        → may include: core/, com/
Source/com/            → may include: core/
Source/core/           → may include: OS headers only; no other Thunder layers
```

Violating these include boundaries corrupts the abstraction hierarchy and **must never be done**. A `core/` header that includes `com/` or `plugins/` is a defect, not a style issue.

## Full Request Data Flow: Network → Plugin

```
TCP/WebSocket connection accepted
        │
        ▼
Server::ChannelMap  ──── accepts connections, creates Channel objects
        │
        ▼
Channel::Deserialize ─── JSON-RPC / HTTP / raw COM-RPC frame parsed
        │
        ▼
ISecurity::Allowed() ─── token check (SecurityAgent plugin provides impl)
        │
        ▼
ServiceMap::FromLocator() ─── URL callsign → Service object (plugin wrapper)
        │
        ▼
Service::Invoke()  ──────── dispatches via IDispatcher (JSONRPC) or IWeb (HTTP)
        │
        ▼
ThrottleQueueType ───────── limits concurrent in-flight jobs per plugin
        │
        ▼
WorkerPool::Submit() ────── job runs on one of N worker threads
        │
        ▼
Plugin::Handler()  ──────── actual plugin method executes
        │
        ▼
Channel::Submit()  ──────── response posted back to client
```

The **network thread** must never block. It reads frames, validates security, and immediately submits a job to the `WorkerPool`. All plugin code runs on `WorkerPool` threads.

## Plugin Lifecycle State Machine

```
UNAVAILABLE
    │  (library available, config file discovered)
    ▼
DEACTIVATED  ◄────────────────────────────────────────────────────┐
    │  (all preconditions met, or explicit activate request)      │
    ▼                                                             │
PRECONDITION ─── (waiting for declared subsystem preconditions)  │
    │  (all preconditions satisfied)                              │
    ▼                                                             │
[Activation in progress: library load → Initialize()]            │
    │  (Initialize() returns "")                                  │
    ▼                                                             │
ACTIVATED ─── normal operating state                              │
    │  │                                                          │
    │  ├──► SUSPENDED (IStateControl::Suspend())                 │
    │  │       └──► ACTIVATED (IStateControl::Resume())          │
    │  │                                                          │
    │  └──► HIBERNATED (if hibernate support built)              │
    │                                                             │
    │  (deactivate request / termination condition met / failure) │
    ▼                                                             │
[Deactivation in progress: Deinitialize() → library unload]      │
    └─────────────────────────────────────────────────────────────┘
```

**Valid transition triggers**:

| From | To | Trigger |
|------|----|---------|
| `DEACTIVATED` | `PRECONDITION` | All declared `precondition` subsystems active; autostart or explicit activate |
| `PRECONDITION` | `ACTIVATED` | All remaining `precondition` subsystems satisfied |
| `ACTIVATED` | `DEACTIVATION` | Explicit deactivate, `termination` subsystem condition met, or `IPlugin::Initialize()` returned error |
| `DEACTIVATION` | `DEACTIVATED` | `Deinitialize()` + library unload complete |
| `ACTIVATED` | `SUSPENDED` | `IStateControl::Suspend()` |
| `SUSPENDED` | `ACTIVATED` | `IStateControl::Resume()` |

**All plugins are briefly loaded and unloaded at daemon startup** (regardless of `autostart`) to read version metadata from `PLUGINHOST_ENTRY_POINT`. `Initialize()` and `Deinitialize()` are **not** called during this metadata-only pass. This makes trivial constructors and destructors mandatory.

## Subsystem System

Subsystems are a **global boolean bitmap** tracking which platform capabilities are available. They are declared in `ISubSystem` (e.g. `PLATFORM`, `NETWORK`, `SECURITY`, `INTERNET`, `GRAPHICS`, `TIME`, `IDENTIFIER`, `BLUETOOTH`).

**Rules:**
- Precondition logic uses **AND**: all declared `precondition` subsystems must be active before the plugin activates.
- Termination logic uses **NOT_ values with OR**: any declared `termination` subsystem going inactive triggers deactivation.
- A plugin may only set/clear subsystems declared in its metadata `control` list — the runtime enforces this (`SubSystemsControlled`).
- Plugins subscribe to subsystem changes via `ISubSystem::INotification` (not polling).

**Typical pattern**: `NetworkControl` plugin sets `NETWORK` active → all plugins with `precondition: ["NETWORK"]` transition from `PRECONDITION` to `ACTIVATED`.

## Execution Modes

| Mode | `root.mode` value | Plugin runs in | Use when |
|------|------------------|---------------|----------|
| In-process | _(no `root` block)_ | Main Thunder daemon process | Maximum performance, trusted plugins |
| Out-of-process | `Local` | Dedicated `ThunderPlugin` child process | Isolation, crash containment, different privilege |
| Container | `Container` | `ThunderPlugin` + OS container (LXC/runc) | Strict sandbox, minimum attack surface |
| Distributed | `Distributed` | Remote device, connected via COM-RPC over TCP | Multi-device architectures |

**Architectural implication of OOP mode**: every method call on the plugin crosses a process boundary through COM-RPC serialization. `IShell::ICOMLink()` returns `nullptr` in OOP mode — always null-check it. Platform primitives like `Core::ProcessInfo` refer to the host process, not the plugin process.

## COM-RPC Architecture

```
┌────────────────────────────────────────────┐
│  Plugin process (ThunderPlugin)            │
│  ┌──────────────────────────────────────┐  │
│  │  Plugin implementation (real object) │  │
│  └──────────┬───────────────────────────┘  │
│             │ Stub (serializes params)      │
│  ┌──────────▼───────────────────────────┐  │
│  │  RPC::Administrator (stub registry)  │  │
│  └──────────┬───────────────────────────┘  │
│             │ IPC frame                     │
│  ┌──────────▼───────────────────────────┐  │
│  │  RPC::CommunicatorClient (socket)    │  │
│  └──────────┬───────────────────────────┘  │
└─────────────│──────────────────────────────┘
              │ Unix domain socket / TCP
┌─────────────│──────────────────────────────┐
│  Daemon process (Thunder)                  │
│  ┌──────────▼───────────────────────────┐  │
│  │  RPC::Communicator (socket server)   │  │
│  └──────────┬───────────────────────────┘  │
│             │ IPC frame                     │
│  ┌──────────▼───────────────────────────┐  │
│  │  RPC::Administrator (proxy lookup)   │  │
│  └──────────┬───────────────────────────┘  │
│             │ Proxy (deserializes result)   │
│  ┌──────────▼───────────────────────────┐  │
│  │  Caller (ServiceMap / other plugin)  │  │
│  └──────────────────────────────────────┘  │
└────────────────────────────────────────────┘
```

**Proxy/stub registration** happens automatically at library load time via static initializers generated by `ProxyStubGenerator`. Loading a proxy/stub `.so`/`.dylib` registers the stubs in `RPC::Administrator`.

**OOP Announce Handshake** (step-by-step):
1. `Service::Activate()` → `Communicator::Create()` → `RemoteConnectionMap::Create()` forks `ThunderPlugin` with CLI args (`-l locator -c classname -C callsign -r socket -i ifaceId -x exchangeId`).
2. Parent blocks on a `Core::Event`.
3. Child opens `CommunicatorClient` connection to the parent's COM-RPC socket.
4. Child sends `AnnounceMessage` containing: PID, class name, interface ID, version, implementation pointer.
5. Parent's `ChannelServer::AnnounceHandler::Procedure()` receives it → `RemoteConnectionMap::Announce()`.
6. `Announce()` matches `exchangeId` to the pending `Create()` → sets the `Core::Event`.
7. Parent unblocks, receives the proxied interface pointer, completes `Initialize()` on the plugin side.

## JSON-RPC Layer Architecture

JSON-RPC sits on top of COM-RPC as a thin deserialization layer. `PluginHost::JSONRPC` (a mixin inherited by plugins) handles method dispatch:

```
WebSocket frame (JSON string)
        │
        ▼
Channel::Deserialize() ─── parses JSON-RPC 2.0 envelope {method, params, id}
        │
        ▼
JSONRPC::Invoke() ──────── looks up registered handler by method name
        │
        ▼
Handler deserialization ─── params JSON → C++ typed struct (generated schema)
        │
        ▼
Plugin method ──────────── executes; returns result
        │
        ▼
Handler serialization ──── C++ result → params JSON
        │
        ▼
Channel::Submit() ──────── {"jsonrpc":"2.0","result":...,"id":N} sent back
```

JSON-RPC events (notifications) flow in the reverse direction: plugin calls `Notify("eventname", params)` → `Channel::Notify()` → sends to all subscribed WebSocket clients. Subscription is managed by the built-in `register`/`unregister` JSON-RPC methods.

## Component Diagram

```
                    ┌─────────────┐
    WebSocket/HTTP  │   Channel   │◄─── external JSON-RPC clients (ThunderUI, apps)
    clients ───────►│   (TCP)     │
                    └──────┬──────┘
                           │
                    ┌──────▼──────────────────────────┐
                    │         Server                   │
                    │  ┌────────────────────────────┐  │
                    │  │       ServiceMap            │  │◄── Controller plugin (built-in)
                    │  │  (plugin registry +         │  │
                    │  │   state machine)            │  │
                    │  └────────────────────────────┘  │
                    │  ┌───────────┐  ┌─────────────┐  │
                    │  │WorkerPool │  │ResourceMon. │  │
                    │  └───────────┘  └─────────────┘  │
                    │  ┌───────────────────────────┐   │
                    │  │   RPC::Communicator        │   │◄── OOP plugin processes
                    │  │  (COM-RPC socket server)   │   │    (ThunderPlugin)
                    │  └───────────────────────────┘   │
                    └─────────────────────────────────-─┘
```
