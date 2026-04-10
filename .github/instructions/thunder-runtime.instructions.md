---
name: Thunder-Daemon-Runtime
description: 'Rules for working in Source/Thunder/ — Server, ServiceMap, Controller, Config, PostMortem, Probe'
applyTo: 'Source/Thunder/**'
---

# Thunder `Source/Thunder/` — Daemon Runtime

`Source/Thunder/` is the **top-level daemon**: it assembles `core/`, `com/`, and `plugins/` into the running `Thunder` process. This directory contains the most security-sensitive and lifecycle-critical code in the framework. Every change here affects all plugins and all clients.

## Layer Scope
- Depends on `Source/core/`, `Source/com/`, `Source/plugins/`.
- **Never** include `ThunderInterfaces/` directly — the daemon itself has no business logic, only infrastructure.
- Entry point: `PluginHost.cpp` → `Server` (in `PluginServer.h` / `PluginServer.cpp`).

## `PluginServer.h` — Master File (~4800+ lines)
- Contains `Server`, `ServiceMap`, inner `Service` class, `Channel`, `ThrottleQueueType`, `ConfigObserver`.
- Read this file before touching any lifecycle or dispatch path — many subtleties are documented in comments there.
- Do not split into smaller files without very careful ABI and linkage consideration.

## `ServiceMap` — Plugin Registry
- All access to the plugin map is guarded by `_adminLock` (`Core::CriticalSection`).
- **Acquire `_adminLock` as briefly as possible** — never call plugin code (`Initialize`, `Deinitialize`, JSON-RPC handlers) while holding it.
- `_pluginHandling` (per-`Service`) guards per-plugin interface pointers — separate from `_adminLock`. Never hold both simultaneously.
- Lookup by callsign: `ServiceMap::FromLocator()` resolves a URL segment to a `Service*`.
- Plugin lifecycle states follow `PluginHost::IShell::state` (see `Source/plugins/IShell.h`): `UNAVAILABLE`, `DEACTIVATED`, `ACTIVATION`, `PRECONDITION`, `ACTIVATED`, `DEACTIVATION`, `HIBERNATED`, `DESTROYED`. Only the framework transitions states — never force-set `_state` directly.

## `Service` — Plugin Wrapper
- `Service` wraps the loaded plugin library and its `IPlugin` implementation.
- Activation path: `Service::Activate()` → loads the plugin shared library → calls `Initialize()` → transitions to `ACTIVATED`.
- Deactivation path: `Service::Deactivate()` → calls `Deinitialize()` → unloads the plugin shared library.
- Deactivation reasons are represented by `PluginHost::IShell::reason` enum: `REQUESTED`, `AUTOMATIC`, `FAILURE`, `STARTUP`, `SHUTDOWN`, `CONDITIONS`, `WATCHDOG_EXPIRED`, `INITIALIZATION_FAILED`. The reason is **not** passed as a parameter to `IPlugin::INotification::Deactivated()` — retrieve it inside the callback via `shell->Reason()`.
- `Service::Submit(job)` enqueues a job through `ThrottleQueueType` — use this for all plugin-context async work.

## Dispatch & Throttle
- `ThrottleQueueType` limits concurrent in-flight jobs per plugin — prevents a misbehaving plugin from starving the `WorkerPool`.
- JSON-RPC invocations arrive on the `WorkerPool` thread, not the network thread — do not block the network thread.
- HTTP / Web requests go through `Service::Process()` → `IWeb::Process()` — also on the `WorkerPool`.
- `Channel::Submit()` posts a response back to the client — always call on the response path even for error cases.

## `Controller` — Built-in Management Plugin

Controller is the only plugin that is part of `Source/Thunder/` itself. Key JSON-RPC methods it exposes:

| Method | Purpose |
|--------|--------|
| `activate` | Activate a plugin by callsign |
| `deactivate` | Deactivate a plugin by callsign |
| `status` | Query plugin status, version, state |
| `configuration` | Read/write plugin config JSON |
| `storeconfig` | Persist plugin config to disk |
| `discovery` | List all known plugins |
| `harakiri` | Request daemon self-termination |

- The Controller's callsign is `"Controller"` (configurable but conventionally fixed).
- Do **not** shortcut `ServiceMap` state transitions by calling internal methods — always go through `Controller`'s JSON-RPC interface or `Service::Activate()`/`Deactivate()` public methods.
- `Controller` also manages `ConfigObserver` (hot-reload of plugin `.json` configs) and proxy stub hot-reload.

## `Config.h` — Daemon Configuration Schema
- Add new daemon-level config options here as `Core::JSON::Container` members — never via environment variables or compile-time constants.
- Key fields: `port`, `binding`, `communicator`, `persistentpath`, `datapath`, `systempath`, `proxystubpath`, `configs`, `exitreasons`, `postmortempath`, `observe`.
- Config is loaded once at startup from the file pointed to by `-c` argument (default `/etc/Thunder/config.json`).
- Path substitution tokens (`%datapath%`, `%persistentpath%`, `%volatilepath%`, `%systempath%`, `%proxystubpath%`) are resolved by `Config::Substitute()` — use them in plugin config strings, never absolute paths.

## `PostMortem.cpp` — Crash Diagnostics
- On abnormal exit, Thunder writes a post-mortem dump to `postmortempath`.
- Add new dump sections to `PostMortem.cpp` via the existing `Dump*()` function pattern — never add ad-hoc `fprintf`/syslog calls for crash data.
- `exitreasons` config controls which shutdown reasons trigger a dump (e.g. `WATCHDOG`, `FAILURE`).

## `Probe.cpp` — Process Health Monitor
- `Probe` implements the watchdog for OOP plugin processes — it detects unresponsive child processes.
- Do not bypass the probe mechanism with custom liveness checks.

## `IRemoteInstantiation.h` — OOP Plugin Launch Protocol
- Defines the interface between `Server` and the `ThunderPlugin` launcher process for `Local` / `Container` execution mode.
- Changes here must be coordinated with `ThunderPlugin/` changes — they share the same IPC protocol version.

## Hot-Reload
- Plugin config `.json` files in `configs` directory are watched by `FileObserver`. New files cause `ConfigObserver` to register additional plugins via `ServiceMap::Insert()` — changes to already-loaded plugin configs are **not** applied at runtime; a daemon restart is required.
- Proxy stub shared libraries in `proxystubpath` are hot-loaded — new stubs are registered, stale ones are left loaded (never unloaded at runtime to avoid use-after-free).
- Never cache the contents of `proxystubpath` — always query at load time.

## OOP Plugin Launch & Announce Protocol
When activating an out-of-process (OOP) plugin (`mode: Local` or `Container`):

### Launch Sequence
1. `Service::Activate()` → `Communicator::Create()` → `RemoteConnectionMap::Create()`.
2. `CreateStarter()` creates a `LocalProcess` or `ContainerProcess` (both inherit `MonitorableProcess` → `RemoteConnection`).
3. `Process::Launch()` forks the `ThunderPlugin` host binary with CLI args: `-l` (locator/library), `-c` (classname), `-C` (callsign), `-r` (communicator socket), `-i` (interface ID), `-x` (exchange ID).
4. The parent waits on a `Core::Event` for the child to announce itself.

### Announce Handshake
5. The child process opens a `CommunicatorClient` connection to the parent's COM-RPC socket.
6. Child sends an `AnnounceMessage` containing: process ID, class name, interface ID, version, and the implementation pointer.
7. Parent's `ChannelServer::AnnounceHandler::Procedure()` receives the message → calls `RemoteConnectionMap::Announce()`.
8. `Announce()` distinguishes two cases:
   - **Requested** (`info.IsRequested()`): matches to a pending `Create()` via exchange ID, completes the `Core::Event`.
   - **Spontaneous** (external client connecting): creates a new `RemoteConnection`, notifies observers.
9. `Handle()` processes the interface offer/revoke/acquire based on `Data::Init` flags.

### Process Teardown
- `MonitorableProcess::Destruct()` sends `SIGTERM` first (soft kill), waits `SoftKillCheckWaitTime()`, then `SIGKILL` (hard kill).
- Container processes use `IContainer::Stop()` instead of POSIX signals.
- `ProcessShutdown` (background timer) manages graceful shutdown of multiple OOP processes.
- `RemoteConnectionMap::Closed()` is called when a channel disconnects — notifies `IRemoteConnection::INotification` observers, then calls `Terminate()`.

### `ThunderPlugin` Process
`ThunderPlugin` is the OOP host binary that runs plugin implementation classes out-of-process. It is **always spawned by Thunder itself** — never launched manually. Its internal CLI arguments (`-l`, `-c`, `-C`, `-r`, `-i`, `-x`, etc.) are implementation details and should not be referenced in plugin code or configuration.

## Security
- All JSON-RPC calls pass through `ISecurity::Allowed()` (token-based) before dispatch.
- `SecurityAgent` plugin provides the `ISecurity` implementation — the daemon uses it via `QueryInterface`.
- Never bypass `ISecurity` for convenience — gate every new channel type through it.

## Hot-Reload Details

- **Plugin config hot-reload**: `FileObserver` watches the `configs` directory. On change, `ConfigObserver` calls `ServiceMap::ConfigReload()`, which scans `*.json` files and calls `Insert()` for each. `Insert()` **only registers new plugins** — if a plugin with that callsign is already registered, the updated config is silently ignored (an error is logged). Hot-reload does **not** deactivate or reconfigure already-running plugins; a daemon restart is required to apply config changes to existing plugins.
- **Proxy stub hot-load**: when a new `.so`/`.dylib` appears in `proxystubpath`, it is loaded via `Core::Library` and its `Instantiation` static object fires, registering stubs. Already-loaded stubs are **never unloaded** — removing a proxy stub file does not deregister it.
- **Daemon config**: `config.json` is read once at startup — changes require a daemon restart.

## Adding a New Daemon-Level Feature Checklist
1. Add config fields to `Config.h` (daemon config) or `PluginServer.h` (runtime state).
2. Lock with `_adminLock` when accessing `ServiceMap` state; `_pluginHandling` when accessing plugin interface pointers.
3. Add throttled dispatch through `ThrottleQueueType` — do not dispatch directly to `WorkerPool`.
4. Add post-mortem dump section to `PostMortem.cpp` if the feature has recoverable state.
5. Add the new feature to `Controller`'s JSON-RPC interface if external management is needed.
6. Update the repository's example/default daemon configuration file(s) with the new config option.

## Cross-Reference

- For the complete OOP plugin lifecycle: see `docs/plugin/execution-modes/outofprocess.md`.
- For `_adminLock` / `_pluginHandling` locking discipline: see `constraints.md` (Multi-threading Discipline).
- For adding daemon config fields: see `constraints.md` (Configuration / Paths).
- For security and ISecurity interface: see `Source/plugins/IPlugin.h`.
