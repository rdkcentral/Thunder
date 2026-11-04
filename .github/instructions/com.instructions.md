---
name: Thunder-COM-RPC-Layer
description: 'Rules for working in Source/com/ — proxy/stub infrastructure, IUnknown, Administrator, Communicator'
applyTo: 'Source/com/**'
---

# Thunder `com/` Layer — COM-RPC Infrastructure

`Source/com/` implements Thunder's **inter-process communication protocol**: `Core::IUnknown`, proxy/stub registration, `RPC::Administrator`, and `RPC::Communicator`. This layer bridges in-process C++ objects to out-of-process (OOP) plugins over Unix domain sockets or TCP.

## Layer Scope
- **Depends on**: `Source/core/` only.
- **Never** include `Source/plugins/` or `Source/Thunder/` headers here.
- Public API is `com/com.h` — all other com headers are implementation details.

## `Core::IUnknown` — The Base of All COM Interfaces
- Every COM interface must inherit **`virtual public Core::IUnknown`** — virtual inheritance is mandatory to avoid diamond-problem reference count collisions.
- `Core::IUnknown` provides `QueryInterface(id)`. `AddRef()` and `Release()` are inherited via `Core::IReferenceCounted` — do not override them in interface definitions, only in final concrete implementations if absolutely required.
- Every interface must declare: `enum { ID = RPC::ID_XXX };` where `ID_XXX` is registered in `ThunderInterfaces/interfaces/Ids.h` (or `Source/com/Ids.h` for core COM IDs).

## Proxy/Stub Pattern
- Proxies and stubs are **auto-generated** by `ProxyStubGenerator` from interface headers annotated with generation tags.
- **Never hand-edit generated proxy/stub files** — edit the source interface header and regenerate.
- A stub handles incoming calls on the server side (deserializes parameters, calls the real implementation).
- A proxy handles outgoing calls on the client side (serializes parameters, sends over IPC).
- Stub classes inherit `ProxyStub::UnknownStubType<INTERFACE, METHODS[]>`.
- Proxy classes inherit `ProxyStub::UnknownProxyType<INTERFACE>`.

## `RPC::Administrator` — Stub Registry
- `Administrator` is a singleton that maps interface IDs → stub factories.
- Register a new stub with the `ANNOUNCE` macro (in the generated `.cpp`):
  ```cpp
  namespace {
      class Instantiation {
      public:
          Instantiation() { RPC::Administrator::Instance().Announce<MyProxy, MyStub>(); }
      } _announcement;
  }
  ```
- Stubs are looked up by interface ID at call dispatch time — an unregistered ID causes a runtime error on the OOP boundary.
- Loading a plugin's proxy/stub `.so` automatically triggers static init which calls `Announce`.

## `RPC::Communicator` — Transport
- `Communicator` owns the Unix domain socket (or TCP socket) for COM-RPC traffic.
- The daemon opens a `CommunicatorServer`; OOP plugin processes open a `CommunicatorClient`.
- Never create raw sockets for IPC — always go through `Communicator`/`CommunicatorClient`.
- Socket path is configured via `communicator` in `config.json` (e.g. `/tmp/communicator` or `127.0.0.1:62000` for TCP debugging).
- `RPC::CommunicationTimeOut` (3000 ms release / infinite debug) applies to all method calls — remote calls that exceed this timeout return `Core::ERROR_TIMEDOUT`.

## Reference Counting Rules on the COM Boundary
- Every raw `IUnknown*` / `IMyInterface*` returned from `QueryInterface()` carries **one ref** — the caller owns it and must call `Release()`.
- When passing an interface pointer into a method: the callee calls `AddRef()` if it stores it; otherwise the caller retains ownership.
- `Core::ProxyType<T>` manages lifetime automatically for heap-allocated objects — use it for all ref-counted storage on the in-process side.
- `Core::SinkType<T>` wraps a stack/member object so it can participate in `QueryInterface` without heap allocation.
- `ERROR_COMPOSIT_OBJECT` returned from `AddRef()` signals a composit (delegate-lifetime) object — `Administrator::RecoverySet` handles this correctly; do not special-case it elsewhere.

## Collections Across COM Boundaries
- `std::list` and `std::map` cannot be marshalled across a COM interface — never use them as interface parameters.
- `std::vector` is supported with the `@restrict` annotation on the parameter (Thunder 5.x+); for new interfaces prefer `RPC::IIteratorType<ELEMENT, ID>` which works across all versions.
- `RPC::IIteratorType<ELEMENT, ID>` — a COM-safe forward iterator with `Next()` / `Current()` / `Reset()`.
- Generate iterators: declare `using IMyIterator = RPC::IIteratorType<IMyElement, RPC::ID_MY_ITERATOR>;` and register `ID_MY_ITERATOR` in `Ids.h`.
- `IteratorType<CONTAINER, INTERFACE>` provides a concrete in-process implementation of an iterator interface.

## `IValueIterator` / `IStringIterator`
- `IStringIterator` (string collections) and `IValueIterator` (uint32 collections) are pre-defined in `com/` — reuse them for simple scalar iteration.

## COM-RPC Debugging
- Switch `communicator` to TCP (`"127.0.0.1:62000"`) to capture traffic.
- Use `thunder-comrpc` Wireshark filter; inspect `invoke.hresult` for failures.

## IPC Message Dispatch Pattern (`Administrator.h`, `Communicator.h`)
The COM-RPC layer dispatches incoming IPC frames through a handler registry:

### Message Types
- **`InvokeMessage`** (`IPCMessageType<0, ...>`): carries method invocation data (interface ID, method index, parameters).
- **`AnnounceMessage`** (`IPCMessageType<1, ...>`): carries process announcement / interface registration data.
- Both are registered with the IPC channel via `BaseClass::Register(MessageType::Id(), handler)`.

### Handler Pattern
`IIPCServer::Procedure(IPCChannel& channel, Core::ProxyType<Core::IIPC>& data)` is the dispatch entry point. The handler receives a type-erased `IIPC` pointer and must determine the concrete message type.

**Safe pattern** (works cross-platform):
```cpp
void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override {
    // Label() is a virtual call on IIPC — always works, no dynamic_cast needed
    ASSERT(data->Label() == AnnounceMessage::Id());
    // static_cast is safe because the IPC channel routing guarantees type by label
    AnnounceMessage* message = static_cast<AnnounceMessage*>(&(*data));
    // ... use message->Parameters(), message->Response()
}
```

**Avoid this pattern** (fragile for template message types across dylib boundaries):
```cpp
void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override {
    Core::ProxyType<AnnounceMessage> message(data);  // uses dynamic_cast internally
    ASSERT(message.IsValid());  // FAILS — dynamic_cast on template types is not reliable across dylibs
}
```

### `Administrator::Invoke` and `ExtractInvokeMessage`
- `Job::Invoke()` dispatches `InvokeMessage` frames to the `Administrator` for method call execution.
- `Administrator::ExtractInvokeMessage()` extracts the concrete `InvokeMessage` from a `ProxyType<IIPC>`.
- `Administrator::Identifier()` reads the interface ID from the message to route to the correct stub.
- These methods must use `Label()` check + `static_cast` — the IPC channel routing guarantees type correctness by label, making `static_cast` safe.

### `Communicator::ChannelServer::AnnounceHandler`
- Receives `AnnounceMessage` frames from OOP plugin processes and new COM-RPC client connections.
- Extracts `Data::Init` parameters (process ID, class name, interface ID, version).
- Routes to `RemoteConnectionMap::Announce()` which handles process registration and interface acquisition.

### `CommunicatorClient::AnnounceHandler`
- Client-side handler for server-initiated announce requests (server asking client for an interface).
- Uses `Acquire()` virtual to obtain the requested implementation.

## `RemoteConnectionMap` — Connection Lifecycle
- Tracks all active OOP connections via `std::unordered_map<uint32_t, RemoteConnection*>`.
- **`Announce()`**: called when a new process connects — creates `RemoteConnection`, links it to the channel.
- **`Closed()`**: called on channel disconnect — notifies observers, releases connection, calls `Terminate()`.
- **`Create()`**: spawns an OOP process and waits for its announce message (with timeout via `Core::Event`).
- Observer pattern: `IRemoteConnection::INotification` (`Activated`, `Deactivated`, `Terminated`) — register in plugin `Initialize()`, unregister in `Deinitialize()`.

## `Channel` vs. `Communicator` Lifecycle

- `Communicator::Server` owns the listening socket — it lives for the duration of the daemon.
- Each connected `Channel` is created by `Communicator::Server` on accept and destroyed on disconnect.
- `Channel` holds the per-connection state: IPC framing, in-flight call tracking, reference to `RemoteConnection`.
- When a channel disconnects (socket error, remote close), `RemoteConnectionMap::Closed()` is called automatically.
- Do not cache `Channel*` pointers across calls — channels can be destroyed at any time on disconnect.

## Manual Proxy/Stub Registration (Edge Case)

For unusual cases where the generator is not used (e.g. a synthesized interface for testing):
```cpp
// In a .cpp file, in an anonymous namespace:
namespace {
    class Instantiation {
    public:
        Instantiation()
        {
            RPC::Administrator::Instance().Announce<MyManualProxy, MyManualStub>();
        }
    } _announcement;
}
```
- `MyManualProxy` must inherit `ProxyStub::UnknownProxyType<IMyInterface>`.
- `MyManualStub` must inherit `ProxyStub::UnknownStubType<IMyInterface, N>` where `N` = number of methods.
- This is a last resort — always prefer the generator for correctness and maintainability.

## COM-RPC Client Helpers

Several helper types exist depending on context:

| Helper | Where to use |
|--------|--------------|
| `RPC::SmartInterfaceType<T>` | **External standalone tools** connecting to a running Thunder daemon — handles connection, reconnect, and interface lifecycle |
| `PluginHost::PluginSmartInterfaceType<T>` | **Plugins** acquiring another plugin's interface — lifecycle-aware, nullifies pointer automatically on target deactivation (see `plugins.instructions.md`) |

```cpp
// External tool example using RPC::SmartInterfaceType:
class MyClient : public RPC::SmartInterfaceType<Exchange::INetworkControl> {
    void Operational(const bool upAndRunning) override { ... }
};
MyClient client(_T("/tmp/communicator"), _T("NetworkControl"), ~0);
client.Open(5000);  // connect with 5s timeout
auto* iface = client.Interface();  // returns Exchange::INetworkControl*
```
- The COM-RPC socket path must match the daemon's `communicator` config value.
- Never use `RPC::SmartInterfaceType` from within a daemon plugin — use `PluginSmartInterfaceType` or `QueryInterfaceByCallsign` instead.

## Adding a New COM Interface (within `com/`)
1. Define interface in the appropriate header, inheriting `virtual public Core::IUnknown`.
2. Assign a unique `ID` from `Source/com/Ids.h` (core IDs only — plugin-level IDs go in `ThunderInterfaces/interfaces/Ids.h`).
3. Add `@stubgen:omit` on any method that cannot be marshalled (raw pointers, callbacks not expressible as COM).
4. Run `ProxyStubGenerator` to regenerate proxy/stub pair.
5. Verify the stub is announced in `ProxyStubs.h` / the generated `.cpp`.

## Cross-Reference

- For interface design rules and annotation tags: see `navigation.md`.
- For ref-counting contract and `ProxyType` usage: see `constraints.md`.
- For COM-RPC usage from the plugin layer: see `plugins.instructions.md`.
- For the OOP announce handshake sequence: see `thunder-runtime.instructions.md`.
