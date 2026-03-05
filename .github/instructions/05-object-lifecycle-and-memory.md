---
applyTo: '**'
---

# 05 — Object Lifecycle and Memory Management

> COM-RPC reference counting is the most common source of bugs in Thunder. Every rule in this file is concrete and must be followed without exception.

## `Core::IUnknown` — The Reference Counting Contract

All COM objects in Thunder inherit from `Core::IUnknown`, which provides:
- `AddRef()` — increments the reference count; returns the new count
- `Release()` — decrements the reference count; destroys the object when count reaches 0; returns the new count
- `QueryInterface(id)` — returns a typed pointer to a requested interface, **with one added reference**

**The contract**:
1. Every `IUnknown*` pointer that is **returned to a caller** carries exactly **one reference** owned by the caller.
2. The caller is responsible for calling `Release()` on every pointer it receives from `QueryInterface()` or any other factory method.
3. When a callee **stores** a received pointer for later use, it must call `AddRef()` to take ownership.
4. When a stored pointer is no longer needed, call `Release()` and set it to `nullptr`.

## `QueryInterface` — Caller Always Releases

```cpp
// ✅ Correct
Exchange::INetworkControl* iface = service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
if (iface != nullptr) {
    iface->DoSomething();
    iface->Release();   // caller owns the ref; must release
    iface = nullptr;
}

// ✅ Correct — storing for later use
void Initialize(...) {
    _iface = service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
    // _iface already carries one ref from QueryInterfaceByCallsign — no extra AddRef needed
}
void Deinitialize(...) {
    if (_iface != nullptr) {
        _iface->Release();
        _iface = nullptr;
    }
}

// ❌ Wrong — leaked ref
Exchange::INetworkControl* iface = service->QueryInterface<Exchange::INetworkControl>();
iface->DoSomething();
// Missing Release() — memory/ref leak
```

## `IShell*` Lifetime Rules

`IShell*` is passed to `Initialize()` and `Deinitialize()`. It is **not** automatically ref-counted for the plugin's use.

```cpp
// ✅ Correct — store with AddRef, release with Release
const string Initialize(PluginHost::IShell* service) override
{
    _service = service;
    _service->AddRef();  // take ownership for storage
    return {};
}

void Deinitialize(PluginHost::IShell* service) override
{
    _service->Release();
    _service = nullptr;
}

// ❌ Wrong — stored without AddRef
const string Initialize(PluginHost::IShell* service) override
{
    _service = service;  // dangling pointer risk — no AddRef
    return {};
}
```

`IShell*` is only valid between `Initialize()` and `Deinitialize()`. Never call it after `Deinitialize()` returns.

## Interface Pointers in Notification Callbacks

Interface pointers received in `Activated()` / `Deactivated()` callbacks are **not** ref-counted for the observer:

```cpp
void Activated(const string& callsign, PluginHost::IShell* plugin) override
{
    if (callsign == _T("TargetPlugin")) {
        Exchange::IMyInterface* iface = plugin->QueryInterface<Exchange::IMyInterface>();
        if (iface != nullptr) {
            // iface carries one ref from QueryInterface — store it; no extra AddRef needed
            _storedIface = iface;
            // Do NOT release here — we are keeping it
        }
    }
}

void Deactivated(const string& callsign, PluginHost::IShell*) override
{
    if (callsign == _T("TargetPlugin") && _storedIface != nullptr) {
        _storedIface->Release();
        _storedIface = nullptr;
    }
}
```

## `Core::ProxyType<T>` — RAII Reference-Counted Smart Pointer

`Core::ProxyType<T>` wraps a ref-counted object and manages its lifetime automatically.

```cpp
// Creation — allocates on heap, ref count starts at 1
Core::ProxyType<MyMessage> msg = Core::ProxyType<MyMessage>::Create();

// Copy — increments ref count
Core::ProxyType<MyMessage> msg2 = msg;

// Destruction — decrements ref count; object destroyed when count reaches 0
// (happens automatically when ProxyType goes out of scope or is reassigned)

// Access — like a raw pointer
msg->DoSomething();

// Validity check
if (msg.IsValid()) { ... }
```

**Conversion between ProxyType types** — uses `dynamic_cast` internally:
```cpp
Core::ProxyType<Core::IIPC> base = ...;

// ✅ Safe for non-template types with EXTERNAL visibility
Core::ProxyType<Core::IPCChannel> channel(base);  // dynamic_cast on non-template — works everywhere

// ❌ BROKEN on macOS for IPCMessageType<> specializations
Core::ProxyType<InvokeMessage> invoke(base);  // dynamic_cast fails on macOS (see below)
```

## macOS `dynamic_cast` Failure for Template Types

**This is the most critical platform-specific rule in the codebase.**

Apple Clang emits template class typeinfo as **"weak private external"** — each `.dylib` gets its own private copy of the typeinfo pointer. Since `libc++` compares typeinfo by **pointer equality** (not by name), `dynamic_cast` across dylib boundaries **silently returns `nullptr`** for any class template instantiation, even when the object genuinely has that type.

**Affected types**: `Core::IPCMessageType<ID, P, R>` and all its specializations (`InvokeMessage`, `AnnounceMessage`).

**The fix** — use `IIPC::Label()` (a virtual call on the non-template base) plus `static_cast`:

```cpp
// ✅ Correct — works on macOS and Linux
void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
{
    ASSERT(data->Label() == InvokeMessage::Id());  // Label() is virtual on non-template IIPC
    InvokeMessage* raw = static_cast<InvokeMessage*>(&(*data));  // static_cast safe: routing guarantees type
    // use raw->Parameters(), raw->Response()
}

// ❌ Broken on macOS — ProxyType<InvokeMessage> uses dynamic_cast internally
void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
{
    Core::ProxyType<InvokeMessage> msg(data);  // dynamic_cast returns nullptr on macOS
    ASSERT(msg.IsValid());  // FAILS
}
```

**Non-template classes are safe**: `dynamic_cast` works correctly for any non-template class decorated with `EXTERNAL` (`visibility("default")`). The issue is specific to C++ class template instantiations on Apple Clang.

## `Core::SinkType<T>` — Stack-Allocated COM Participant

`Core::SinkType<T>` makes a stack/member object participate in `QueryInterface` without heap allocation. The lifetime of the sink is tied to its containing object.

```cpp
class MyPlugin {
    // ...
    Core::SinkType<Notification> _sink;  // NOT heap-allocated
};
```

`_sink` can be passed to `Register()` calls that expect a COM pointer — `SinkType` handles the ref-counting protocol transparently (its `AddRef`/`Release` delegate to the enclosing object).

## Never Delete a COM Object Directly

```cpp
// ✅ Correct
iface->Release();

// ❌ Wrong — undefined behavior
delete iface;

// ❌ Wrong — ProxyType hides a Release, but do not use delete on the backing object
delete &(*proxy);
```

## OOP Plugin Boundary: Proxy Lifetime ≠ Remote Object Lifetime

When a plugin runs out-of-process, interface pointers held in the daemon process are **proxies**, not the real objects. Calling `Release()` on a proxy decrements the proxy's ref count — when it reaches zero, the proxy is destroyed and a corresponding `Release` RPC call is sent to the remote object. The remote object is only destroyed when **its own** ref count reaches zero.

This means:
- A ref leak in the daemon (forgetting to call `Release()`) keeps a proxy alive, which keeps the remote object alive, which prevents the OOP plugin process from shutting down cleanly.
- Always verify ref-counting discipline at process shutdown using the post-mortem dump.

## Object Creation on the COM Boundary

For creating COM objects that will be returned across an interface, use `Core::Service<T>::Create<I>()`:

```cpp
// Creates a MyIterator object and returns it as IMyIterator* (with one ref)
IMyIterator* out = Core::Service<MyIteratorImpl>::Create<IMyIterator>(/* constructor args */);
return out;  // caller owns the ref
```

Never use `new` directly for COM objects that will cross interface boundaries — use `Core::Service<>` to ensure proper ref-counting setup.
