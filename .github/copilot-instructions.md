---
applyTo: '**'
---

# GitHub Copilot Instructions — Thunder Framework

Thunder is a **plugin-based device abstraction framework** for embedded platforms. The daemon provides no end-user functionality — all business logic lives in plugins.

## Core Philosophy

1. **Plugin isolation** — the daemon is infrastructure only. All domain logic lives in plugins behind COM interfaces. Never add business logic to `Source/Thunder/` or `Source/core/`.
2. **COM-RPC over everything** — inter-plugin calls go through COM interface pointers. The framework short-circuits in-process calls to direct vtable calls automatically. Never use JSON-RPC, raw sockets, or function pointers to call between plugins.
3. **No exceptions, no STL across ABI boundaries** — Thunder is built with `-fno-exceptions`. RTTI is available but avoid `dynamic_cast` and typeid-dependent patterns across dylib boundaries. COM ref-counting (`AddRef`/`Release`/`Core::ProxyType`) replaces smart pointers at interface boundaries.
4. **Interfaces are contracts** — a published COM interface is a binary ABI. Design every interface as if the implementation will be replaced and the binary will be deployed independently. It cannot change after publication.

---

## Instruction Files

All model-steering rules live in `.github/instructions/`. Four files are applied automatically by path scope; three apply to all files.

| File | Topic | Applied |
|------|-------|---------|
| `constraints.md` | Additional rules beyond the 15 below — nuances the model would likely get wrong | All files |
| `common-mistakes.md` | Anti-patterns with consequences and correct alternatives | All files |
| `navigation.md` | Where to find documentation and canonical examples by topic | All files |
| `core.instructions.md` | `Source/core/` rules: deps, ResourceMonitor, threading, JSON, smart pointers, IPC | `Source/core/**` (auto-applied) |
| `com.instructions.md` | `Source/com/` rules: proxy/stub, Administrator, Communicator, IPC message dispatch | `Source/com/**` (auto-applied) |
| `plugins.instructions.md` | `Source/plugins/` rules: IPlugin, IShell, JSONRPC, ISubSystem, Channel | `Source/plugins/**` (auto-applied) |
| `thunder-runtime.instructions.md` | `Source/Thunder/` rules: ServiceMap, Controller, Config, PostMortem, OOP launch | `Source/Thunder/**` (auto-applied) |

---

## Non-Negotiable Cross-Cutting Rules

These rules apply to **every file** in every repo. No exceptions.

1. **Prefer `Core::hresult` return type for COM interface methods** — `Core::ERROR_NONE` (0) on success. This is the strong convention and effectively required for any method reachable via JSON-RPC; for pure COM-RPC interfaces other return types are technically supported.
2. **COM interfaces are immutable once published** — methods should not be changed or removed without carefully considering backwards compatibility.
3. **Prefer `RPC::IIteratorType<T, ID>` iterators over STL containers across COM interfaces** — `std::list` and `std::map` cannot be marshalled across a COM/process boundary. `std::vector` is supported with the `@restrict` annotation on the parameter; use `IIteratorType` for new interfaces.
4. **No exceptions** — Thunder is built with `-fno-exceptions`. Never use `throw`, `try`, or `catch`.
5. **Virtual inheritance is mandatory for all COM interfaces** — every interface must inherit `virtual public Core::IUnknown` to avoid diamond-problem ref-count corruption.
6. **All setup in `Initialize()`, all teardown in `Deinitialize()`** — constructors and destructors must be trivial.
7. **`IShell*` ref-count discipline** — call `AddRef()` when storing it; call `Release()` in `Deinitialize()`.
8. **`QueryInterface` always returns a ref** — the caller must always call `Release()` on the returned pointer.
9. **Never delete a COM object** — always call `Release()`.
10. **Path tokens are mandatory** — use `%datapath%`, `%persistentpath%`, `%volatilepath%`, `%systempath%`, `%proxystubpath%`; never hardcode absolute paths.
11. **`EXTERNAL` macro on all cross-dylib symbols** — missing `EXTERNAL` causes linker failures in OOP mode.
12. **Layer include discipline** — `core/` includes nothing from `com/`, `plugins/`, or `Thunder/`; `com/` includes nothing from `plugins/` or `Thunder/`; `plugins/` includes nothing from `Thunder/`. Violations are defects.
13. **No platform-specific code without guards** — use `#ifdef __LINUX__`, `#ifdef __WINDOWS__`, etc. Always provide an `#else` or `#error` for unsupported platforms in new platform-abstraction code.
14. **Use `SYSLOG()` / `TRACE()` for all logging** — never call `printf()`, `fprintf()`, or `syslog()` directly. Bypassing the macros breaks log routing.
15. **`ASSERT()` for invariants, not runtime errors** — use `ASSERT()` to document impossible states; use error codes for recoverable errors. Never use `assert()` from `<cassert>`.

---

## Prohibited Patterns

The model must **never** generate the following, regardless of context:

- `throw`, `try`, or `catch` — Thunder builds with `-fno-exceptions`
- `delete` on a COM object — always `Release()`; never `delete`
- `dynamic_cast` on `IPCMessageType<>` template specialisations — use `Label()` + `static_cast`
- JSON-RPC calls between plugins running in the same daemon process — use `QueryInterfaceByCallsign<T>()` directly
- `RPC::CommunicatorClient` constructed from within a daemon plugin — defeats in-process short-circuiting
- Hardcoded absolute paths — use `%datapath%`, `%persistentpath%`, `%volatilepath%`, `%systempath%`, `%proxystubpath%` tokens
- `std::list` or `std::map` as parameters on any interface that crosses a process boundary — these cannot be marshalled. `std::vector` is supported with the `@restrict` parameter annotation; still prefer `RPC::IIteratorType<>` for new interfaces
- `printf()`, `fprintf()`, or `syslog()` directly — use `SYSLOG()` or `TRACE()`
- `assert()` from `<cassert>` — use `ASSERT()`
- Domain or business logic in `Source/Thunder/` or `Source/core/` — those layers are infrastructure only
- Modifying the signature, return type, or parameter list of an existing published COM interface method — always extend via a new versioned interface
- Process-global mutable state outside of `Core::Singleton<T>`

---

> For coding style and naming conventions, see the **Coding Style and Conventions** section in `constraints.md`.

## When to Ask

Pause and request clarification rather than guessing when:

- Ownership of a returned COM interface pointer is ambiguous — it is not clear who is responsible for calling `Release()`
- A change touches an existing interface method signature or adds a method to an existing interface — ABI impact must be confirmed
- A new feature requires choosing between in-process, split-library, or out-of-process execution mode — the right answer depends on isolation, stability, and deployment constraints
- The plugin coupling strategy is unclear — whether to use a one-shot `QueryInterfaceByCallsign<T>()` or the persistent `PluginSmartInterfaceType<T>` pattern depends on whether the target can deactivate independently
- Which thread context a callback, `Initialize()`, or `Deinitialize()` runs on is not evident from the snippet — getting this wrong causes deadlocks

---

## Quick Navigation by Task

For documentation links and source pointers by topic, see `navigation.md`.

| I want to… | Start here |
|-----------|----------|
| Find documentation or examples for any topic | `navigation.md` |
| Check additional rules and nuances | `constraints.md` |
| Identify what I'm doing wrong | `common-mistakes.md` |
| Work in `Source/core/` | `core.instructions.md` (auto-applied) |
| Work in `Source/com/` | `com.instructions.md` (auto-applied) |
| Work in `Source/plugins/` | `plugins.instructions.md` (auto-applied) |
| Work in `Source/Thunder/` | `thunder-runtime.instructions.md` (auto-applied) |
