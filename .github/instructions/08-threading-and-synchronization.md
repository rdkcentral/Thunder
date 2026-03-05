---
applyTo: '**'
---

# 08 — Threading and Synchronization

> Thunder's threading model is simple but strict. Violating the rules in this file causes deadlocks, race conditions, or priority inversions that are difficult to diagnose.

## The Two Core Singletons

Thunder provides exactly **two** managed concurrency primitives that plugins must use:

| Singleton | Purpose | Access |
|-----------|---------|--------|
| `Core::IWorkerPool` | Thread pool for all async CPU work | `Core::IWorkerPool::Instance()` |
| `Core::ResourceMonitor` | I/O event loop for all file descriptor readiness | `Core::ResourceMonitor::Instance()` |

**Never** create raw `std::thread`, `pthread_t`, or call `fork()` from plugin code. Submit work to the `WorkerPool` instead.  
**Never** call `select()`, `poll()`, `epoll_wait()`, or any blocking I/O wait directly — register with `ResourceMonitor` instead.

## `Core::IWorkerPool` — Submitting Async Work

All asynchronous work must be submitted as a `Core::ProxyType<Core::IDispatch>` job:

```cpp
// Define a job — implements Core::IDispatch
class MyJob : public Core::IDispatch {
public:
    MyJob(MyPlugin& plugin) : _plugin(plugin) {}
    void Dispatch() override
    {
        // This runs on a WorkerPool thread
        _plugin.DoBackgroundWork();
    }
private:
    MyPlugin& _plugin;
};

// Submit in Initialize() or in a timer/event callback
Core::ProxyType<Core::IDispatch> job = Core::ProxyType<MyJob>::Create(*this);
Core::IWorkerPool::Instance().Submit(job);
```

For recurring or cancelable jobs, use `Core::WorkerPool::JobType<T>`:
```cpp
Core::WorkerPool::JobType<MyPlugin&> _job;

// In Initialize():
_job = Core::WorkerPool::JobType<MyPlugin&>::Create(*this);
// Trigger:
_job.Submit();
// Cancel:
_job.Revoke();
```

### Job and Parent Lifetime

A job that captures a raw reference or pointer to its parent (e.g. `MyPlugin& _plugin`) **does not** extend the parent's lifetime. If the parent is destroyed while a submitted job is still queued or running, the callback will read freed memory.

**Rules**:
- Call `_job.Revoke()` in `Deinitialize()` **before** releasing any resources the job touches. `Revoke()` waits for any in-progress `Dispatch()` call to complete and prevents future dispatches.
- When using `Core::ProxyType<MyJob>`, the proxy type's reference count keeps the *job object* alive but says nothing about the parent — the parent must outlive the job or the job must not reference the parent after `Revoke()`.
- Prefer `Core::WorkerPool::JobType<T>` (which embeds the parent by reference/pointer) over ad-hoc `IDispatch` lambdas that capture `this` — `JobType` makes it clearer that `Revoke()` is required.

```cpp
void MyPlugin::Deinitialize(PluginHost::IShell* service)
{
    _job.Revoke();  // ← must come before releasing any state the job accesses
    // ... rest of teardown ...
}
```

## `Core::ResourceMonitor` — I/O Event Registration

Implement `Core::IResource` and register with `ResourceMonitor`:

```cpp
class MySocket : public Core::IResource {
public:
    Core::IResource::handle Descriptor() const override { return _fd; }
    uint16_t Events() override { return POLLIN | POLLERR; }
    void Handle(const uint16_t events) override
    {
        if (events & POLLIN) { /* data available */ }
        if (events & POLLERR) { /* error */ }
    }
    void Open()
    {
        // ... open _fd ...
        Core::ResourceMonitor::Instance().Register(*this);
    }
    void Close()
    {
        Core::ResourceMonitor::Instance().Unregister(*this);
        // safe to destroy _fd now
    }
private:
    int _fd { -1 };
};
```

**Rules**:
- `Unregister()` must be called before the resource object is destroyed — failure causes use-after-free.
- `Unregister()` may be called from within `Handle()` (re-entrant safe).
- Do not destroy the resource object until after `Unregister()` returns.

## `Core::TimerType<T>` — Timed Callbacks

Never use `sleep()`, `usleep()`, `std::this_thread::sleep_for()`, or `nanosleep()` in any Thunder code:

```cpp
class MyPlugin {
public:
    MyPlugin() : _timer(*this) {}

    void OnTimer()
    {
        // Runs on WorkerPool thread at the scheduled time
        DoPeriodicWork();
    }

private:
    Core::TimerType<MyPlugin> _timer;
};

// In Initialize():
_timer.Schedule(Core::Time::Now().Add(5000), std::bind(&MyPlugin::OnTimer, this));

// In Deinitialize():
_timer.Revoke();
```

## Synchronization Primitives

| Primitive | Class | When to use |
|-----------|-------|------------|
| Mutual exclusion | `Core::CriticalSection` | Protecting shared mutable state |
| RAII lock guard | `Core::SafeSyncType<CriticalSection>` | Preferred over manual Lock/Unlock |
| Binary semaphore | `Core::BinarySemaphore` | Signal between exactly two parties |
| Counting semaphore | `Core::CountingSemaphore` | Bounded resource pool |
| One-shot event | `Core::Event` | Wait for a single occurrence (e.g. startup sync) |

`Core::SafeSyncType<Core::CriticalSection>` is the **preferred** form for locking because it is RAII and releases the lock on any exit path. While Thunder's existing codebase has many sites that call `Lock()`/`Unlock()` directly, new code should use `SafeSyncType` to avoid lock leaks on early-return or error paths.

```cpp
// ✅ Preferred — SafeSyncType releases the lock on any exit path
{
    Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
    // ... access shared state ...
}   // lock released here

// ⚠️  Acceptable in existing code — but requires discipline on every exit path
_adminLock.Lock();
// ...
_adminLock.Unlock();
```

## Lock Order and Deadlock Prevention

Thunder has two important system-level locks that interact:

| Lock | Scope | Guards |
|------|-------|--------|
| `_adminLock` (in `ServiceMap`) | Global | Plugin registry, service map state |
| `_pluginHandling` (per `Service`) | Per-plugin | Per-plugin interface pointers and state |

**Rule**: these two locks must **never** be held simultaneously. Acquiring both creates a deadlock risk with callbacks that invert the order.

**Rule**: never hold any lock across a call into plugin code (`Initialize`, `Deinitialize`, JSONRPC handlers, notification callbacks). Plugin code may acquire its own locks — holding an outer lock while calling in creates lock inversion.

**Rule**: document the lock order explicitly in comments whenever nesting is unavoidable:
```cpp
// Lock order: always _outerLock before _innerLock — never reverse
Core::SafeSyncType<Core::CriticalSection> outerGuard(_outerLock);
Core::SafeSyncType<Core::CriticalSection> innerGuard(_innerLock);
```

## Thread Identity in Thunder

| Thread | Role | Blocking allowed? |
|--------|------|-----------------|
| Network/IO thread | Reads from WebSocket/TCP sockets, deserializes frames | **No** — must never block |
| WorkerPool threads | Execute plugin handlers, JSON-RPC methods, IDispatch jobs | Yes — but avoid long blocks |
| ResourceMonitor thread | Calls `IResource::Handle()` on I/O readiness | **No** — must return quickly |
| TimerType thread | Fires timer callbacks | Yes — but keep brief |

The network thread submits jobs to the `WorkerPool` immediately after parsing — it never executes plugin code directly. Long-running plugin operations must be further delegated to background jobs submitted from the `WorkerPool` handler.

## `Core::Event` — Startup Synchronization

`Core::Event` is commonly used to synchronize asynchronous initialization:

```cpp
Core::Event _ready { false, true };  // false=not yet set, true=auto-reset

// Background thread sets the event when ready:
void BackgroundThread()
{
    DoInitialization();
    _ready.SetEvent();
}

// Waiter blocks until set:
if (_ready.Lock(5000 /* ms timeout */) != Core::ERROR_NONE) {
    // Timed out waiting
}
```

## Foreground vs Background Mode — Effect on the Main Thread

Thunder can be launched in two modes, controlled by the `-b` flag:

| Mode | Launch | Main thread behaviour |
|------|--------|----------------------|
| **Foreground** (default on macOS/dev) | no `-b` | Main thread runs an interactive key-press loop; does **not** join the `WorkerPool` |
| **Background** (production) | `-b` | Main thread calls `Core::WorkerPool::Instance().Join()` and becomes an **extra WorkerPool worker thread** |

> **Developer note**: In foreground mode the main thread is idle (not a `WorkerPool` worker). In background mode it contributes an extra worker, which changes the effective pool size and can mask thread-starvation bugs or expose races not seen in foreground testing. Always validate plugin behaviour in background mode before declaring a test complete.

## OOP Plugin Threading

Regardless of whether a plugin runs in-process or out-of-process, **`IPlugin::Initialize()` and `IPlugin::Deinitialize()` always execute in the Thunder daemon's context** — on a daemon WorkerPool thread dispatching an `IShell::Job`.

The OOP distinction applies only to the **implementation class** (the object acquired via `service->Root<T>()`):
- The implementation object lives in the `ThunderPlugin` child process, which has its **own `WorkerPool`** (default: one thread), its own `ResourceMonitor`, and its own COM-RPC stub.
- COM-RPC method calls from the daemon to the implementation cross the IPC boundary and are dispatched on the OOP process's WorkerPool thread.
- The daemon-side bridge class (implementing `IPlugin`) still runs `Initialize()`/`Deinitialize()` **in the daemon**.

Therefore:
- Locking in the **bridge class** protects against concurrent daemon WorkerPool calls.
- Locking in the **implementation class** protects against concurrent calls within the OOP process.
- The daemon's locks do **not** protect OOP implementation state.
- If you use `PluginSmartInterfaceType` from OOP code, configure `threads` ≥ 2 in the `root` block — the default single thread will deadlock (see warning in `Source/plugins/Types.h`).
