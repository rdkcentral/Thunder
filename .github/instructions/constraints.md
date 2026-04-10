---
applyTo: '**'
---

# Thunder — Additional Constraints

These rules supplement the 15 non-negotiable rules in `.github/copilot-instructions.md`. They cover nuances the model would likely get wrong without being told.

---

## COM Interface Design

- The interface destructor must always be `~IMyInterface() override = default;` — never add a body.
- To extend a published interface, add a **new versioned struct** (`IMyFeature2`) that inherits the old one and appends methods. Never touch the original.
- Every interface struct must carry `EXTERNAL` — missing it causes linker failures in OOP mode.
- Every interface must declare `enum { ID = RPC::ID_XXX };` — the ID must be registered in `ThunderInterfaces/interfaces/Ids.h`.
- All method parameters that are outputs must use references annotated `/* @out */`, not raw pointers.

## Reference-Counting Nuances

- `QueryInterface()` called inside `Activated()` / `Deactivated()` callbacks returns a pointer with **one ref owned by the caller** — the callback must either store it (no extra `AddRef()` needed, but `Release()` required on `Deactivated()`) or release it before returning.
- `IShell*` is valid only between `Initialize()` and `Deinitialize()`. Never invoke it after `Deinitialize()` returns. Never invoke it from a destructor.
- `RPC::IIteratorType<>` pointers returned from COM methods carry one ref — always call `Release()` after iterating.

## Plugin Constructor / Destructor

- Constructors must only zero-initialise members (`nullptr`, `0`, `false`). No subscriptions, no timers, no socket opens, no `AddRef()`.
- Destructors must be empty. Add `ASSERT(_service == nullptr)` to verify `Deinitialize()` ran. Never call COM interfaces or `_service` from the destructor.
- A plugin object may be `Initialize()`d and `Deinitialize()`d multiple times (re-activation). Everything that opens must be closed in `Deinitialize()`.

## Initialize / Deinitialize Contract

- Read config only inside `Initialize()` — never in the constructor or lazily at first use.
- Use `service->PersistentPath()`, `service->DataPath()`, `service->VolatilePath()` for path construction. Never use `service->ConfigLine()` for paths — it returns raw JSON.
- `Deinitialize()` must be the exact inverse of `Initialize()`: every `Register()` must have a `Unregister()`, every `AddRef()` must have a `Release()`, every sink registration must be revoked before the sink is destroyed.
- For notification sinks, call `service->Unregister(&_sink)` **before** releasing any state the sink touches. If the order is reversed, an in-flight callback can access freed memory.

## OOP Plugin Threading

- `IPlugin::Initialize()` and `IPlugin::Deinitialize()` **always execute in the daemon** on a daemon WorkerPool thread — even when the plugin's implementation class runs out-of-process in a `ThunderPlugin` child. There is no exception to this.
- The implementation class returned by `Root<T>()` lives in the child `ThunderPlugin` process; everything else (`IPlugin` bridge, JSON-RPC dispatch) lives in the daemon.
- If the implementation class uses `PluginSmartInterfaceType<T>`, the `root` config block must set `threads` ≥ 2. With the default value of 1, monitor registration deadlocks because the single WorkerPool thread is already blocked waiting for the monitor reply.

## Logging

- Never call `printf()`, `fprintf()`, or `syslog()` directly. Always use `SYSLOG()` (production) or `TRACE()` (debug). In foreground mode the framework routes to `stdout`; in background mode to `syslog()` / unified log. Bypassing the macros breaks this routing.
- `ASSERT()` is for invariants that must never fail in any build — it calls `abort()` in debug builds. Use it to document impossible states, not as runtime error handling.

## ServiceMap / State

- Plugin state transitions (`DEACTIVATED → ACTIVATED`, etc.) are exclusively managed by the framework. Never directly set a plugin's internal `_state` field or call internal `Service` methods to force a transition — always go through `Controller`'s JSON-RPC interface or `Service::Activate()` / `Deactivate()` public methods.

## Configuration / Paths

- Path token substitution (`%datapath%`, `%persistentpath%`, `%volatilepath%`, `%systempath%`, `%proxystubpath%`) is resolved by `Config::Substitute()` at runtime. Use these tokens in all plugin config JSON strings. Never hardcode absolute paths.
- New daemon-level config options belong in `Source/Thunder/Config.h` as `Core::JSON::Container` members — never as environment variables or compile-time constants.

---

## Coding Style and Conventions

These apply to all Thunder source code, with particular emphasis on the framework layers (`core/`, `com/`, `plugins/`, `Thunder/`).

### Naming

- **New feature/module files**: PascalCase — `MyModule.h`, `MyModule.cpp`. Legacy framework-level headers (e.g. `core.h`, `com.h`, `plugins.h`) use lowercase — do not rename them.
- **Namespaces and classes**: PascalCase — `MyNameSpace`, `MyClass`.
- **Class member methods**: PascalCase — `GetValue()`, `SetActive()`.
- **Class member variables**: `_` prefix + camelCase — `_myVariable`, `_adminLock`.

### Comments

Keep comments minimal — code must be self-explanatory. A comment must add context that cannot be read from the code itself. Write comments in correct English. Prefer placing comments on their own line before the relevant code; avoid trailing inline comments where possible (they make diffs and alignment fragile).

### Include Guards

Prefer `#pragma once` for new header files. Existing files that use classic `#ifndef` guards may keep them — there is no need to convert. Never mix both styles in the same file.

### Type Safety

- **Always use C++-style casts** — `static_cast<>`, `reinterpret_cast<>`, `const_cast<>`. Never C-style casts `(Type)x`. C++-style casts are statically checked and grep-searchable.
- **Always use explicit integer widths** — `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, `int32_t`. Never `int`, `char`, `short`, or `long` for any value whose bit-width matters (protocol fields, message sizes, counts).
- **Use `const` everywhere valid** — `const` parameters, `const` methods, `const` local variables. Immutable values are easier to reason about and caught at compile time.
- **Prefer references over pointers** — use a reference when the value cannot be null and does not need re-seating. Use a pointer only when null is a valid state or the pointee may change.

### Initialization

- Always initialize variables at the point of definition — never declare first and assign later.
- Every class member variable must be initialized in the constructor initializer list. No member may be left in an indeterminate state after construction.

### Object Copying

If copying an object is unsafe or meaningless, delete the copy constructor and copy-assignment operator. The deleted declarations **must be in the `public:` section** — if they are `private:`, the compiler reports "inaccessible" instead of "use of a deleted function", which is misleading.

### Class Design

- Use `final` on a class or virtual method only when extension would cause a correctness failure (e.g. mandatory initialization that cannot be delegated). Do not use `final` preemptively.
- Prefer composition over inheritance for implementation reuse. COM interface inheritance (`virtual public` chains) is the explicit, architecturally mandated exception.
- Minimize global mutable state. Never introduce process-global mutable variables outside of the `Core::Singleton<T>` pattern.
- A header must be self-contained: including it alone must compile without requiring the consumer to include additional prerequisites first.

### Multi-threading Discipline

- **`Core::CriticalSection` is for mutual exclusion** — one thread acquires it to protect a shared resource, then releases it. Never use a `CriticalSection` to signal events between threads.
- **`Core::Event` / `Core::Semaphore` is for signaling** — one thread signals, another waits. Never also use it as a resource guard.
- Be aware of **priority inversion**: threads at different priorities coordinating via the same `CriticalSection` create inversion risk. Keep critical sections as short as possible.
- Prevent **deadlocks**: establish and document lock acquisition order. Never acquire lock B while holding lock A if any code path acquires A while holding B.
