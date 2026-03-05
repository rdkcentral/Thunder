---
applyTo: '**'
---

# 04 — Design Patterns

> These are the canonical patterns used throughout Thunder. When writing new code, replicate these patterns exactly — do not invent alternatives.

## 1. Sink / Notification Inner Class Pattern

Used to receive lifecycle or domain events from another subsystem. The inner class is a COM object itself, participates in `QueryInterface`, and is stored as a member (not heap-allocated).

```cpp
class MyPlugin
    : public PluginHost::IPlugin {

private:
    // Inner notification sink — non-copyable
    class Notification
        : public PluginHost::IPlugin::INotification {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        explicit Notification(MyPlugin* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~Notification() override = default;

        void Activated(const string& callsign, PluginHost::IShell* plugin) override
        {
            _parent.OnActivated(callsign, plugin);
        }
        void Deactivated(const string& callsign, PluginHost::IShell* plugin) override
        {
            _parent.OnDeactivated(callsign, plugin);
        }
        void Unavailable(const string&, PluginHost::IShell*) override
        {
            // Must be implemented even if empty — pure virtual in base
        }

        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    private:
        MyPlugin& _parent;   // reference, never pointer
    };

public:
    // ...
    const string Initialize(PluginHost::IShell* service) override
    {
        service->Register(&_sink);   // register in Initialize()
        return {};
    }
    void Deinitialize(PluginHost::IShell* service) override
    {
        service->Unregister(&_sink); // unregister in Deinitialize()
    }

    BEGIN_INTERFACE_MAP(MyPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        // SinkType allows QueryInterface to find Notification via _sink:
        INTERFACE_AGGREGATE(PluginHost::IPlugin::INotification, &_sink)
    END_INTERFACE_MAP

private:
    Core::SinkType<Notification> _sink;   // stack-allocated, participates in QI
};
```

**Rules**:
- Always store sinks as `Core::SinkType<T>` — never heap-allocate notification sinks.
- The inner class must hold a **reference** (`T&`) to the parent, never a pointer.
- `Unavailable()` must be implemented even if the body is empty — it is pure virtual.
- Register in `Initialize()`, unregister in `Deinitialize()` — never in constructor/destructor.

## 2. Interface Map

Every class that exposes COM interfaces must declare an interface map. This is what `QueryInterface` dispatches through.

```cpp
BEGIN_INTERFACE_MAP(MyPlugin)
    INTERFACE_ENTRY(PluginHost::IPlugin)           // directly implemented by this class
    INTERFACE_ENTRY(PluginHost::IDispatcher)       // inherited via JSONRPC mixin
    INTERFACE_ENTRY(Exchange::IMyInterface)        // directly implemented by this class
    INTERFACE_AGGREGATE(Exchange::IOther, _other)  // implemented by _other sub-object
    INTERFACE_RELAY(Exchange::IRelay, _relayPtr)   // relayed to another COM object
END_INTERFACE_MAP
```

| Macro | Use when |
|-------|---------|
| `INTERFACE_ENTRY(I)` | This class directly implements `I` |
| `INTERFACE_AGGREGATE(I, member)` | `member` (a `Core::SinkType<T>` or sub-object) implements `I` |
| `INTERFACE_RELAY(I, ptr)` | Delegate `QueryInterface` for `I` to a different COM object pointer |

The interface map must cover **every** interface the class exposes. Missing an entry causes `QueryInterface` to return `nullptr` for that interface.

## 3. Composite Plugin Pattern

A plugin that acquires and wraps another plugin's interface via `IShell`:

```cpp
const string Initialize(PluginHost::IShell* service) override
{
    // Acquire interface from another plugin by callsign
    _other = service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
    if (_other == nullptr) {
        return _T("NetworkControl plugin not available");
    }
    // _other carries one ref from QueryInterfaceByCallsign — store it, it is now owned
    return {};
}

void Deinitialize(PluginHost::IShell* service) override
{
    if (_other != nullptr) {
        _other->Release();
        _other = nullptr;
    }
}

private:
    Exchange::INetworkControl* _other { nullptr };
```

**Rules**:
- `QueryInterfaceByCallsign<T>()` returns a ref-counted pointer — the caller owns the ref.
- Always `Release()` and null the pointer in `Deinitialize()`.
- If the target plugin can deactivate while this plugin is active, subscribe to `IPlugin::INotification` and release in `Deactivated()`.

## 4. `ProxyPool<T>` — Recycled Object Pool

For heavyweight objects that are frequently allocated and freed (e.g. IPC messages, buffers), use `Core::ProxyPoolType<T>`:

```cpp
// Declare the pool (typically as a static or module-level object)
static Core::ProxyPoolType<MyMessage> _messagePool(8); // initial capacity 8

// Acquire from pool (creates if pool is empty)
Core::ProxyType<MyMessage> msg = _messagePool.Element();

// Use msg...

// Return to pool automatically when last ProxyType<> holding it is destroyed
```

**Rules**:
- Only use `ProxyPoolType<T>` for objects that are expensive to construct and have clear ownership boundaries.
- Never manually `delete` pool-managed objects — the pool reclaims them on `Release()`.

## 5. COM-Safe Iterator Pattern (`RPC::IIteratorType<>`)

The standard pattern for returning collections across COM boundaries:

**Interface definition** (in `ThunderInterfaces/interfaces/`):
```cpp
// Declare the iterator interface
using IMyItemIterator = RPC::IIteratorType<IMyItem*, RPC::ID_MY_ITEM_ITERATOR>;
```

**In-process implementation** (returning from a method):
```cpp
// Build the container in-process
std::vector<IMyItem*> items;
// ... populate items ...

// Wrap in a COM-safe iterator
Core::hresult Items(IMyItemIterator*& out) const override
{
    using Iterator = RPC::IteratorType<std::vector<IMyItem*>, IMyItemIterator>;
    out = Core::Service<Iterator>::Create<IMyItemIterator>(items);
    return Core::ERROR_NONE;
}
```

**Caller usage**:
```cpp
IMyItemIterator* it = nullptr;
plugin->Items(it);
if (it != nullptr) {
    while (it->Next() == true) {
        IMyItem* item = it->Current();
        // use item...
        item->Release();
    }
    it->Release();
}
```

**Rules**:
- Never pass `std::vector`, `std::list`, `std::map`, or any STL container across a COM interface.
- Always `Release()` the iterator when done.
- Pre-built types `IStringIterator` and `IValueIterator` are available for `string` and `uint32_t` collections — reuse them rather than defining new iterator interfaces for those types.

## 6. Config Inner Class Pattern

Every plugin that has configuration must parse it via a `Core::JSON::Container` subclass, not raw string manipulation:

```cpp
class Config : public Core::JSON::Container {
private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

public:
    Config()
        : Core::JSON::Container()
        , Retries(3)
        , Endpoint(_T(""))
        , Timeout(5000)
    {
        Add(_T("retries"), &Retries);
        Add(_T("endpoint"), &Endpoint);
        Add(_T("timeout"), &Timeout);
    }

public:
    Core::JSON::DecUInt32 Retries;
    Core::JSON::String Endpoint;
    Core::JSON::DecUInt32 Timeout;
};

// Usage in Initialize():
Config config;
config.FromString(service->ConfigLine());
uint32_t retries = config.Retries.Value();
```

**Rules**:
- Always read config in `Initialize()` via `service->ConfigLine()` — never in a constructor or from a global.
- All config member fields are `public` JSON type members — do not add getters.
- Always provide defaults in the constructor — never assume fields will be present in the JSON.
- Extend `Core::JSON::Container` — never hand-parse JSON strings.

## 7. `DEPRECATED` Method Wrapper

When a method must be removed logically but retained for ABI compatibility:

```cpp
DEPRECATED virtual Core::hresult OldMethod(const string& param) = 0;
```

**Rules**:
- Never remove a deprecated method — only mark it `DEPRECATED`.
- New code must not call deprecated methods internally.
- Document the replacement method in a comment above the deprecated one.

## 8. `/* @stubgen:omit */` — Exclude from Proxy/Stub Generation

For methods that are in-process only (cannot be marshalled or are not needed OOP):

```cpp
/* @stubgen:omit */
virtual Core::hresult InProcessOnlyMethod(Core::IUnknown* rawCallback) = 0;
```

The generator will skip this method entirely — no proxy or stub code is emitted for it. Use this for:
- Methods with raw pointer parameters that cannot be marshalled
- Methods that are only meaningful in the same process (e.g. `IShell::ICOMLink`)
- Callbacks that are registered differently in OOP contexts

## 9. `EXTERNAL` on Class vs. Function Level

Apply `EXTERNAL` at the class level for COM classes that are instantiated across dylib boundaries:

```cpp
// ✅ Correct — whole class exported
class EXTERNAL MyPlugin : public PluginHost::IPlugin { ... };

// ✅ Correct — single free function exported
EXTERNAL void MyFactoryFunction();

// ❌ Wrong — do not apply EXTERNAL to individual methods of an already-EXTERNAL class
class EXTERNAL MyPlugin {
public:
    EXTERNAL void SomeMethod();  // redundant and wrong
};
```
