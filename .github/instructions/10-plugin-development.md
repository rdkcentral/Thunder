---
applyTo: '**'
---

# 10 — Plugin Development

> This is the primary reference for writing a Thunder plugin from scratch. It supersedes all earlier ad-hoc plugin guidance. Each section below links to a dedicated sub-file that owns that topic in full. Follow the checklist in order and cross-reference the sub-files as indicated.

---

## Plugin Development Checklist

1. Design the COM interface in `ThunderInterfaces/interfaces/IMyFeature.h` (see `07-interface-driven-development.md`)
2. Allocate an interface ID in `ThunderInterfaces/interfaces/Ids.h`
3. Rebuild `ThunderInterfaces` to generate proxy/stub files
4. Create the plugin directory with the standard layout (see [10.1 — Module Files](10.1-plugin-module.md))
5. Write `Module.h` and `Module.cpp` (see [10.1 — Module Files](10.1-plugin-module.md))
6. Apply code style, copyright headers, and naming conventions (see [10.2 — Code Style](10.2-plugin-codestyle.md))
7. Implement `IPlugin` in `MyPlugin.h` / `MyPlugin.cpp` — class structure, interface map, OOP notification sink (see [10.3 — Plugin Class & Registration](10.3-plugin-class-registration.md))
8. Register the plugin via `Plugin::Metadata<>` (see [10.3 — Plugin Class & Registration](10.3-plugin-class-registration.md))
9. Implement `Initialize()` and `Deinitialize()` following the lifecycle rules (see [10.4 — Lifecycle](10.4-plugin-lifecycle.md))
10. Apply internal implementation patterns: notifications, JSON config class, thread safety, memory management (see [10.5 — Implementation Patterns](10.5-plugin-implementation.md))
11. Write the plugin config `.conf.in` file (see [10.6 — Configuration Files](10.6-plugin-config.md))
12. Write the plugin `CMakeLists.txt` and register in the root build (see [10.7 — CMake Build](10.7-plugin-cmake.md))
13. Test `Initialize()` → `Deinitialize()` cycle, including error paths and OOP process failure scenarios

---

## Standard Plugin Directory Layout

```
MyPlugin/
├── CMakeLists.txt              ← see 10.7
├── Module.h                    ← MODULE_NAME define + common includes (see 10.1)
├── Module.cpp                  ← MODULE_NAME_DECLARATION (see 10.1)
├── MyPlugin.h                  ← IPlugin implementation class declaration (see 10.3)
├── MyPlugin.cpp                ← Initialize/Deinitialize, metadata registration (see 10.3, 10.4)
├── MyPluginImplementation.h    ← OOP implementation class (if split-library)
├── MyPluginImplementation.cpp  ← OOP implementation (if split-library)
└── MyPlugin.conf.in            ← Plugin config template (see 10.6)
```

---

## Sub-File Index

| File | Topic |
|------|-------|
| [10.1-plugin-module.md](10.1-plugin-module.md) | `Module.h` and `Module.cpp` — `MODULE_NAME`, include order, `MODULE_NAME_DECLARATION` |
| [10.2-plugin-codestyle.md](10.2-plugin-codestyle.md) | Copyright headers, clang-format rules, naming conventions, error handling, general C++ rules |
| [10.3-plugin-class-registration.md](10.3-plugin-class-registration.md) | `IPlugin` class structure, interface map, `Plugin::Metadata<>`, `SERVICE_REGISTRATION`, OOP connection notification |
| [10.4-plugin-lifecycle.md](10.4-plugin-lifecycle.md) | `Initialize()`, `Deinitialize()`, `Deactivated()` — ordering, guards, OOP teardown |
| [10.5-plugin-implementation.md](10.5-plugin-implementation.md) | Notification/sink pattern, JSON config class, `IWeb` REST handling, thread safety, memory management |
| [10.6-plugin-config.md](10.6-plugin-config.md) | `.conf.in` template rules, root-level fields, `configuration` object, validation rules |
| [10.7-plugin-cmake.md](10.7-plugin-cmake.md) | `${NAMESPACE}` usage, CMake structure, root registration, compiler settings |

---

## Key Cross-References to Other Core Files

- **COM interface design** → `07-interface-driven-development.md`
- **COM-RPC proxy/stub fundamentals** → `06-comrpc-fundamentals.md`
- **Object lifecycle and `AddRef`/`Release`** → `05-object-lifecycle-and-memory.md`
- **Design patterns (notification sink, inner classes)** → `04-design-patterns.md`
- **Threading and `WorkerPool`** → `08-threading-and-synchronization.md`
- **`ASSERT`, `TRACE`, `SYSLOG`, `Core::hresult`** → `09-error-handling-and-logging.md`
- **OOP `root` block and `IShell`/`Service` hierarchy** → `11-configuration-and-metadata.md`
- **Testing `Initialize`/`Deinitialize` cycles** → `12-testing-guidelines.md`

---

## Best Practices Summary

- **No global state** — all state belongs in the plugin class instance
- **No blocking in JSON-RPC handlers** — dispatch work >1 ms to the `WorkerPool`
- **Always `Unregister` what you `Register`** in `Deinitialize()`, in reverse order
- **Always `Release` what you `QueryInterface`** — no exceptions
- **`Deinitialize()` is the single authoritative cleanup path** — null-check every resource before releasing; never duplicate cleanup inside `Initialize()` failure branches
- **Constructors and destructors must be stateless** — all logic belongs in `Initialize()` / `Deinitialize()`
- **Minimise direct plugin-to-plugin coupling** — use `PluginSmartInterfaceType<T>` if inter-plugin COM access is unavoidable; never use JSON-RPC for plugin-to-plugin calls
- **Never use `RPC::CommunicatorClient` to connect back to the daemon's own socket** from within a plugin — use `IShell::QueryInterfaceByCallsign<T>()` instead
- **Prefer `Plugin::Metadata<>` over `SERVICE_REGISTRATION`** for new plugins

---

## Common Mistakes Table

| Mistake | Consequence | Fix |
|---------|-------------|-----|
| Storing `IShell*` without `AddRef()` | Dangling pointer | Always `AddRef()` when storing `IShell*` |
| Not unregistering JSON-RPC methods in `Deinitialize()` | Method collision on re-activation | Unregister every method by matching name |
| Blocking a `WorkerPool` thread | Stalls all plugin dispatch | Use `Core::TimerType<T>` for delays |
| Calling plugin methods while holding `_adminLock` | Deadlock | Release lock before calling out |
| Leaking iterator pointers from COM methods | Reference leak | Always `Release()` iterators after use |
| Accessing `_service` in destructor | Undefined behaviour | Never access `_service` in destructor |
| Putting activation logic in the constructor | WorkerPool not ready yet | Move all logic to `Initialize()` |
| Using JSON-RPC for plugin-to-plugin calls | Unnecessary serialisation overhead | Use `QueryInterfaceByCallsign<T>()` |
| Creating `CommunicatorClient` inside the daemon | Bypasses in-process short-circuit | Use `IShell::QueryInterfaceByCallsign<T>()` |
