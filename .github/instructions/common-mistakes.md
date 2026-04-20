---
applyTo: '**'
---

# Thunder — Common Mistakes

These are the most frequent wrong patterns in Thunder plugins. Each entry names the mistake, its consequence, and the correct alternative.

---

## 1. JSON-RPC between in-process plugins

**Mistake**: Calling another plugin via the JSON-RPC layer (`service->Invoke(...)`, `JSONRPC::Link`, etc.) from within a plugin.

**Consequence**: Every call serialises to JSON, dispatches through the WorkerPool queue, then deserialises — adding round-trip latency. In-process COM-RPC short-circuiting (which turns the call into a direct C++ vtable call) is bypassed entirely. Under load, this starves the WorkerPool.

**Fix**: Use `service->QueryInterfaceByCallsign<T>(callsign)` to obtain a COM interface pointer and call it directly. Release the pointer when done.

---

## 2. `CommunicatorClient` inside the daemon process

**Mistake**: Creating an `RPC::CommunicatorClient` pointing at `/tmp/communicator` (or the TCP equivalent) from within a plugin that is running inside the daemon.

**Consequence**: Forces every call out over a Unix socket even though the target plugin is in the same process. Defeats COM-RPC's in-process short-circuit, adds IPC overhead, and can cause timeouts if the daemon's socket queue is full.

**Fix**: Use `service->QueryInterfaceByCallsign<T>(callsign)` for intra-daemon calls. `CommunicatorClient` is only for **external client applications** that are not part of the daemon process.

---

## 3. Raw `QueryInterfaceByCallsign` without `PluginSmartInterfaceType`

**Mistake**: Storing the result of `service->QueryInterfaceByCallsign<T>()` in a member variable across activation events, without using `PluginSmartInterfaceType<T>`.

**Consequence**: If the target plugin deactivates and reactivates independently, the stored pointer becomes dangling. Calling through it is undefined behaviour.

**Fix**: Use `PluginHost::PluginSmartInterfaceType<T>` (`Source/plugins/Types.h`) — it subscribes to plugin lifecycle events and nullifies the internal pointer automatically on deactivation.

## 4. Storing `IShell*` without `AddRef()`

**Mistake**: Assigning `_service = service` inside `Initialize()` without calling `service->AddRef()`.

**Consequence**: The pointer becomes dangling once the framework's own reference is released. Subsequent use via `_service` in a notification callback or delayed job is undefined behaviour.

**Fix**: Always `_service->AddRef()` when storing the pointer. Always `_service->Release(); _service = nullptr;` in `Deinitialize()`.

---

## 5. Not unregistering JSON-RPC handlers in `Deinitialize()`

**Mistake**: Calling `Register(...)` for a JSON-RPC method in `Initialize()` but omitting the matching `Unregister(...)` in `Deinitialize()`.

**Consequence**: On plugin re-activation, `Register()` is called again for the same method name. The framework detects the collision and may assert or fail activation, making the plugin permanently broken until the daemon restarts.

**Fix**: Every `Register(methodName, ...)` in `Initialize()` must have a matching `Unregister(methodName)` in `Deinitialize()`.

---

## 6. Activation logic in the constructor

**Mistake**: Opening sockets, scheduling timers, allocating COM objects, or calling `AddRef()` inside the plugin constructor.

**Consequence**: The WorkerPool, ResourceMonitor, and subsystem infrastructure are not yet available at construction time. Any such allocation has no corresponding teardown path in the destructor (which must be empty), creating a leak or crash if `Initialize()` is never called.

**Fix**: Move all activation logic to `Initialize()`. Constructors must only zero-initialise member variables.

---

## 7. Accessing `_service` in the destructor

**Mistake**: Calling `_service->Something()` or releasing a COM interface via `_service` inside the plugin destructor.

**Consequence**: `_service` was already released in `Deinitialize()` and is `nullptr`. The dereference is undefined behaviour.

**Fix**: `Deinitialize()` is the correct teardown point. The destructor should contain only `ASSERT(_service == nullptr)` to verify teardown completed.

---

## 8. `PluginSmartInterfaceType` from OOP implementation class with 1 thread

**Mistake**: Using `PluginSmartInterfaceType<T>` inside the implementation class that runs in a `ThunderPlugin` child process, while the `root` config block leaves `threads` at its default value of 1.

**Consequence**: Monitor registration sends a message back to the daemon and then waits for a reply — on the single WorkerPool thread that is already processing the request. The thread cannot service its own reply. The deadlock is silent and permanent until the watchdog kills the process.

**Fix**: Set `"threads": 2` (or higher) in the plugin's `root` configuration block whenever the OOP implementation class uses `PluginSmartInterfaceType<T>`.

---

## 9. Leaking iterator pointers from COM methods

**Mistake**: Calling a COM method that returns an `RPC::IIteratorType<>*` (or similar iterator interface), iterating it, and then not calling `Release()` on the iterator pointer.

**Consequence**: Every call to such a method leaks a ref-counted iterator object. Under sustained load, this causes the daemon's heap to grow without bound.

**Fix**: Always call `iterator->Release()` (or wrap in `Core::ProxyType<>`) after you are done iterating.
