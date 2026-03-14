---
applyTo: '**'
---

# Thunder — Navigation

Use this file to find where detailed documentation or canonical examples live. This file does not define new rules — consult `copilot-instructions.md` and `constraints.md` for cross-cutting rules, the four scoped instruction files (`core.instructions.md`, `com.instructions.md`, `plugins.instructions.md`, `thunder-runtime.instructions.md`) for layer-specific rules, and `common-mistakes.md` for anti-patterns.

---

## Official Documentation (`docs/`)

| Topic | Document |
|-------|----------|
| Plugin lifecycle (activation, deactivation, states) | `docs/plugin/lifecycle.md` |
| Out-of-process (OOP) execution mode | `docs/plugin/execution-modes/outofprocess.md` |
| In-process execution mode | `docs/plugin/execution-modes/inprocess.md` |
| All execution modes overview | `docs/plugin/execution-modes/execution-modes.md` |
| Plugin configuration schema (`root`, `configuration` blocks) | `docs/plugin/config.md` |
| Subsystem declarations and preconditions | `docs/plugin/subsystems.md` |
| Interface design guidelines | `docs/plugin/interfaces/guidelines.md` |
| ProxyStubGenerator / JsonGenerator annotation tags | `docs/plugin/interfaces/tags.md` |
| Interface versioning and immutability | `docs/plugin/interfaces/interfaces.md` |
| COM-RPC internals and debugging (ThunderShark) | `docs/plugin/private-comrpc.md` |
| Composite plugins | `docs/plugin/composite.md` |
| ProxyPool pattern | `docs/plugin/proxypool.md` |
| Plugin messaging and tracing | `docs/plugin/messaging.md` |
| Architecture overview | `docs/introduction/architecture/` |
| Build and run (Linux) | `docs/introduction/build_linux.md` |
| Repository map and mandatory build order | `docs/introduction/repos.md` |
| Plugin skeleton generator | `docs/plugin/devtools/pluginskeletongenerator.md` |

---

## Key Source Pointers

| What | Where |
|------|-------|
| `PluginSmartInterfaceType<T>` (safe plugin coupling) | `Source/plugins/Types.h` |
| `Plugin::Metadata<T>` (modern plugin registration) | `Source/plugins/Metadata.h` |
| `SERVICE_REGISTRATION` macro (legacy registration) | `Source/core/Services.h` |
| Interface ID allocation | `ThunderInterfaces/interfaces/Ids.h` |
| Foreground / background log routing logic | `Source/Thunder/PluginHost.cpp` (`_background` flag) |
| OOP spawn sequence (`Root<T>()`) | `Source/plugins/Shell.cpp`, `Source/Thunder/PluginServer.h` |
| WorkerPool / ResourceMonitor | `Source/core/WorkerPool.h` |
| `Core::TimerType<T>` | `Source/core/Timer.h` |
| `Core::SinkType<T>` | `Source/core/IUnknown.h` |
| `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` | `Source/com/Ids.h` |

---

## Canonical Plugin Examples

| Pattern | Path |
|---------|------|
| Simple in-process plugin | `ThunderNanoServices/examples/` |
| OOP split-library plugin (recommended) | `ThunderNanoServices/examples/` (`Pollux` example) |
| Nano-service (single responsibility, minimal plugin) | Any plugin under `ThunderNanoServices/` |
| L1 test with mock `IShell` | `rdkservices/Tests/L1Tests/` |

---

## Repo Map

| Repo | Role |
|------|------|
| `Thunder/` | Framework daemon, `core/`, `com/`, `plugins/`, `Thunder/` layers |
| `ThunderInterfaces/` | Shared COM interface headers and generated JSON-RPC schemas |
| `ThunderTools/` | `ProxyStubGenerator`, `JsonGenerator`, skeleton generator |
| `ThunderNanoServices/` | Reference plugin implementations and examples |
| `rdkservices/` | RDK-specific plugin collection (uses Thunder as framework) |

---

## Build Order

`ThunderTools` → `Thunder` → `ThunderInterfaces` → `ThunderNanoServices` / `rdkservices`

Each step must be installed before the next is configured. See `docs/introduction/repos.md` for full CMake commands.
