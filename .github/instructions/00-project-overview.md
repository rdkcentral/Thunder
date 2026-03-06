---
applyTo: '**'
---

# 00 — Project Overview

## What Thunder Is

Thunder is a **plugin-based device abstraction framework** for embedded platforms (primarily ARM/MIPS running Linux). It is developed by Metrological, a Comcast company, and is the backbone of RDK (Reference Design Kit) deployments worldwide.

Thunder provides:
- A daemon (`Thunder` binary) that loads, manages, and isolates plugins
- A layered C++ framework (`core/`, `com/`, `plugins/`) for building those plugins
- COM-RPC (Component Object Model Remote Procedure Call) for cross-process plugin communication
- JSON-RPC over WebSocket for external client access to plugin APIs
- Code-generation tooling (`ThunderTools`) for generating proxy/stubs and JSON-RPC schema

## What Thunder Is NOT

- Thunder itself provides **zero end-user functionality** — all business logic lives in plugins
- It is not a general-purpose microservice framework — do not import general server patterns into it
- It is not a scripting host — no embedded scripting engines in the core framework
- It is not an event bus — inter-plugin communication goes through COM-RPC interfaces, not a pub/sub broker

## Repository Map

| Repo | Role | Build Order |
|------|------|-------------|
| `ThunderTools/` | Code generators: `ProxyStubGenerator`, `JsonGenerator`, `ConfigGenerator`, `LuaGenerator` (all Python) | 1st — must be installed before any other build |
| `Thunder/` | Core daemon + libraries (`core/`, `com/`, `plugins/`, `Source/Thunder/`) | 2nd |
| `ThunderInterfaces/` | Shared COM-RPC interface headers (`interfaces/I*.h`) + generated JSON-RPC wrappers (`jsonrpc/`) | 3rd |
| `ThunderNanoServices/` | Metrological-maintained plugins (not used in RDK deployments) | 4th |
| `rdkservices/` | RDK-specific plugins consuming ThunderInterfaces | 4th (parallel with NanoServices) |
| `ThunderUI/` | Browser-based dev/test UI communicating over JSON-RPC | Optional, any time after Thunder |

**The build order is mandatory.** Each repo's installed artifacts are required by the next. Never attempt to build out of order.

## Key Binary and Library Outputs

| Output | Location | Purpose |
|--------|----------|---------|
| `Thunder` | `install/bin/Thunder` | Main daemon binary — the process that hosts all plugins |
| `ThunderPlugin` | `install/bin/ThunderPlugin` | OOP plugin host — child process for `Local`/`Container` mode plugins |
| `libThunderCore.so/.dylib` | `install/lib/` | Platform primitive library — threads, sockets, JSON, IPC, timers |
| `libThunderCOM.so/.dylib` | `install/lib/` | COM-RPC infrastructure — proxy/stub registration, communicator |
| `libThunderPlugins.so/.dylib` | `install/lib/` | Plugin framework API — `IPlugin`, `IShell`, `JSONRPC` |
| `libThunderCOMProcessStubs.so/.dylib` | `install/lib/` | Core COM proxy/stub library |
| Plugin `.so`/`.dylib` files | `install/lib/thunder/plugins/` | Individual plugin implementations |
| ProxyStub `.so`/`.dylib` files | `install/lib/thunder/proxystubs/` | COM-RPC proxy/stub pairs for plugin interfaces |
| Plugin `.json` configs | `install/etc/Thunder/plugins/` | Per-plugin configuration files |

## Layer Stack (bottom-up dependency order)

```
┌──────────────────────────────────────────────┐
│  rdkservices / ThunderNanoServices           │  Plugin implementations
├──────────────────────────────────────────────┤
│  Source/Thunder/                             │  Daemon runtime: Server, ServiceMap, Controller
├──────────────────────────────────────────────┤
│  Source/plugins/                             │  Plugin API: IPlugin, IShell, JSONRPC, ISubSystem
├──────────────────────────────────────────────┤
│  Source/com/                                 │  COM-RPC: IUnknown, proxy/stub, Communicator
├──────────────────────────────────────────────┤
│  Source/core/                                │  Platform primitives: threads, sockets, JSON, IPC
└──────────────────────────────────────────────┘
```

Each layer may only include headers from layers **below** it. Violating this direction is a hard error.

## Developer Personas

### Plugin Author
Primarily works in `rdkservices/` or `ThunderNanoServices/`. Consumes `Source/plugins/` API (`IPlugin`, `IShell`, `JSONRPC`) and `ThunderInterfaces/` for shared exchange interfaces. Must understand: plugin lifecycle, `Initialize`/`Deinitialize` discipline, COM-RPC ref-counting, JSONRPC registration, and subsystem preconditions.

**Start with**: `10-plugin-development.md` → `07-interface-driven-development.md` → `06-comrpc-fundamentals.md`

### Interface Designer
Works in `ThunderInterfaces/interfaces/`. Designs COM interface headers that are shared across plugins and clients. Must understand: interface immutability rules, ID registration, proxy/stub generation, JSON-RPC schema tags.

**Start with**: `07-interface-driven-development.md` → `06-comrpc-fundamentals.md` → `03-code-style.md`

### Framework Contributor
Works in `Thunder/Source/` (any layer). Must understand the full architecture, inter-layer rules, and how changes propagate across all consumers.

**Start with**: `01-architecture.md` → layer-specific `*.instructions.md` → `05-object-lifecycle-and-memory.md`

### Platform Porter
Works on platform-specific code paths (macOS, Windows, new Linux variants). Must understand platform guards, `core/` portability conventions, and cross-dylib `dynamic_cast` caveats.

**Start with**: `01-architecture.md` → `core.instructions.md` → `05-object-lifecycle-and-memory.md`

## Key Files for Initial Orientation

| Purpose | File |
|---------|------|
| Plugin API contracts | `Source/plugins/IPlugin.h`, `IShell.h`, `ISubSystem.h` |
| Server + ServiceMap (core runtime) | `Source/Thunder/PluginServer.h` (~4800 lines — read before touching lifecycle/dispatch) |
| COM-RPC communicator | `Source/com/Communicator.h` |
| COM-RPC administrator (stub dispatch) | `Source/com/Administrator.h` |
| IPC message types | `Source/core/IPCConnector.h` |
| Platform portability macros | `Source/core/Portability.h` |
| Ref-counted smart pointer | `Source/core/Proxy.h` |
| Network interface abstraction | `Source/core/NetworkInfo.h` |
| Synchronization primitives | `Source/core/Sync.h` |
| COM interface IDs (core) | `Source/com/Ids.h` |
| Plugin/Exchange interface IDs | `ThunderInterfaces/interfaces/Ids.h` |
| Daemon config schema (C++) | `Source/Thunder/Config.h` |
| Plugin config schema (C++) | `Source/plugins/Configuration.h` |
| JSONRPC dispatcher | `Source/plugins/JSONRPC.h` |
