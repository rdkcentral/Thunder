---
applyTo: '**'
---

# GitHub Copilot Instructions — Thunder Framework

Thunder is a **plugin-based device abstraction framework** for embedded platforms. The daemon provides no end-user functionality — all business logic lives in plugins. See `.github/instructions/00-project-overview.md` for full context.

---

## Detailed Guidance Index

All detailed rules live in `.github/instructions/`. These files are loaded automatically by scope or must be consulted when working in those areas:

| File | Topic | Load trigger |
|------|-------|-------------|
| `00-project-overview.md` | Repo map, layer stack, key binaries, developer personas | All files |
| `01-architecture.md` | Request data flow, plugin lifecycle, subsystems, execution modes, COM-RPC, JSON-RPC | All files |
| `02-build-and-run.md` | Mandatory build order, CMake commands, macOS build notes, code generation | CMakeLists, build scripts |
| `03-code-style.md` | clang-format, naming conventions, include guards, `override`, `EXTERNAL` | All `.h` / `.cpp` files |
| `04-design-patterns.md` | Sink pattern, interface map, composite plugin, ProxyPool, iterators, config class | All `.h` / `.cpp` files |
| `05-object-lifecycle-and-memory.md` | `IUnknown` ref-counting, `QueryInterface`, `ProxyType`, `SinkType`, macOS `dynamic_cast` failure | All `.h` / `.cpp` files |
| `06-comrpc-fundamentals.md` | COM-RPC vs JSON-RPC, ID registration, proxy/stub gen, `ANNOUNCE`, announce handshake, ThunderShark | `Source/com/**`, interfaces |
| `07-interface-driven-development.md` | Interface header rules, immutability, method signatures, generator tags, versioning | `ThunderInterfaces/**` |
| `08-threading-and-synchronization.md` | WorkerPool, ResourceMonitor, locks, lock order, TimerType, Event | All `.cpp` files |
| `09-error-handling-and-logging.md` | No exceptions, `hresult`, `ASSERT`, `TRACE`, `SYSLOG`, PostMortem | All `.cpp` files |
| `10-plugin-development.md` | End-to-end plugin checklist, `PLUGINHOST_ENTRY_POINT`, JSONRPC, best practices | Plugin source files |
| `11-configuration-and-metadata.md` | Config hierarchy, path tokens, plugin config schema, hot-reload, OOP `root` block | `.json` config files, `Initialize()` |
| `12-testing-guidelines.md` | L1/L2 tests, mock `IShell`, mock COM interfaces, CI requirements | `*Test*.cpp`, `Tests/` dirs |
| `core.instructions.md` | `Source/core/` rules: deps, ResourceMonitor, threading, JSON, smart pointers, IPC | `Source/core/**` (auto-applied) |
| `com.instructions.md` | `Source/com/` rules: proxy/stub, Administrator, Communicator, RemoteConnectionMap | `Source/com/**` (auto-applied) |
| `plugins.instructions.md` | `Source/plugins/` rules: IPlugin, IShell, JSONRPC, ISubSystem, Channel | `Source/plugins/**` (auto-applied) |
| `thunder-runtime.instructions.md` | `Source/Thunder/` rules: ServiceMap, Controller, Config, PostMortem, OOP launch | `Source/Thunder/**` (auto-applied) |

---

## Non-Negotiable Cross-Cutting Rules

These rules apply to **every file** in every repo. No exceptions.

1. **All COM interface methods must return `Core::hresult`** — `Core::ERROR_NONE` (0) on success.
2. **COM interfaces are immutable once published** — methods must never be changed or removed; only append new methods at the end.
3. **No STL containers across COM boundaries** — use `RPC::IIteratorType<T, ID>` iterators. Never pass `std::vector`, `std::list`, or `std::map` across an interface.
4. **No exceptions** — Thunder is built with `-fno-exceptions`. Never use `throw`, `try`, or `catch`.
5. **Virtual inheritance is mandatory for all COM interfaces** — every interface must inherit `virtual public Core::IUnknown` to avoid diamond-problem ref-count corruption.
6. **All setup in `Initialize()`, all teardown in `Deinitialize()`** — constructors and destructors must be trivial.
7. **`IShell*` ref-count discipline** — call `AddRef()` when storing it; call `Release()` in `Deinitialize()`.
8. **`QueryInterface` always returns a ref** — the caller must always call `Release()` on the returned pointer.
9. **Never delete a COM object** — always call `Release()`.
10. **Path tokens are mandatory** — use `%datapath%`, `%persistentpath%`, `%volatilepath%`, `%systempath%`, `%proxystubpath%`; never hardcode absolute paths.
11. **`EXTERNAL` macro on all cross-dylib symbols** — missing `EXTERNAL` causes linker failures in OOP mode and incorrect `dynamic_cast` on macOS for non-template classes.
12. **Never use `dynamic_cast` on `IPCMessageType<>` specializations across dylib boundaries on macOS** — Apple Clang emits template typeinfo as private-per-dylib. Use `IIPC::Label()` + `static_cast` instead.
13. **Layer include discipline** — `core/` includes nothing from `com/`, `plugins/`, or `Thunder/`; `com/` includes nothing from `plugins/` or `Thunder/`; `plugins/` includes nothing from `Thunder/`. Violations are defects.
14. **Never use `-flat_namespace` on macOS** — it breaks plugin isolation and causes unpredictable symbol conflicts.
15. **Always run `clang-format -i` on every modified C++ file** — style violations block CI.

---

## Quick Navigation by Task

| I want to… | Read first |
|-----------|-----------|
| Write a new plugin from scratch | `10-plugin-development.md` |
| Design a new COM interface | `07-interface-driven-development.md` |
| Fix a build issue | `02-build-and-run.md` |
| Understand the plugin lifecycle | `01-architecture.md` |
| Debug a COM-RPC failure | `06-comrpc-fundamentals.md` |
| Fix a macOS `dynamic_cast` crash | `05-object-lifecycle-and-memory.md` |
| Add plugin configuration | `11-configuration-and-metadata.md` |
| Write a test | `12-testing-guidelines.md` |
| Work in `Source/core/` | `core.instructions.md` (auto-applied) |
| Work in `Source/com/` | `com.instructions.md` (auto-applied) |
| Work in `Source/plugins/` | `plugins.instructions.md` (auto-applied) |
| Work in `Source/Thunder/` | `thunder-runtime.instructions.md` (auto-applied) |
