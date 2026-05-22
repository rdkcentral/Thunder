## Context

`CommunicationChannel` is a `private` inner class of the `LinkType<INTERFACE>` class template, defined entirely in `JSONRPCLink.h`. Its `Instance()` static method holds `channelMap` as a function-local static:

```cpp
static Core::ProxyType<CommunicationChannel> Instance(...)
{
    static Core::ProxyMapType<string, CommunicationChannel> channelMap;
    ...
}
```

In RDK, platform-specific linker flags (e.g. `--export-dynamic` / COMDAT merging behaviour) ensure that only **one** copy of the static exists per process — channel sharing does work. The problem is not duplication; it is **ownership and lifetime**.

The single `channelMap` instance ends up owned by whichever plugin shared library first triggers the template instantiation. Subsequent plugins that call `Instance()` for the same endpoint receive a `Core::ProxyType` wrapping a channel whose vtable pointers point into that first library. The vtable problem is deeper than just `CommunicationChannel`: `ChannelImpl<INTERFACE>` inherits through `StreamJSONType<WebSocketClientType<SocketStream>, FactoryImpl&, INTERFACE>`, which contains a nested `HandlerType<WebSocketClientType<SocketStream>>`. `HandlerType` implements `IResource` via `SocketStream`, and this is what `ResourceMonitor` holds as an `IResource*`. Because `HandlerType` is nested inside `StreamJSONType<..., INTERFACE>`, its vtable is also instantiated in the plugin DSO.

When the owning plugin is **deactivated and its shared library is unloaded**, both the `channelMap` static and the vtables of `CommunicationChannel`, `ChannelImpl`, and `HandlerType` become dangling. Any later use of the shared channel — including the `ResourceMonitor` callback that still holds a reference — dereferences the unmapped vtable address and produces a SIGSEGV (see [rdkcentral/Thunder#2040](https://github.com/rdkcentral/Thunder/issues/2040)).

### Root architectural tension

`StreamJSONType<SOURCE, ALLOCATOR, INTERFACE>` fuses two unrelated concerns into one template class:

- **Socket ownership** — `HandlerType<SOURCE>` manages the TCP socket and implements `IResource`. This has process lifetime.
- **Protocol framing** — `Serializer`/`Deserializer` handle JSON text (`IElement`) or binary MessagePack (`IMessagePack`). This is INTERFACE-dependent.

Because both are in one template parameterised by `INTERFACE`, the socket vtable is tied to the instantiating DSO.

## Goals / Non-Goals

**Goals (Stage 1):**
- Anchor `channelMap`, `CommunicationChannel`, and all related vtables in the websocket shared library.
- Prevent use-after-free of shared `CommunicationChannel` objects when the instantiating plugin is deactivated.
- Preserve the existing `CommunicationChannel::Instance()` signature unchanged.
- Require no changes to callers or CMakeLists.

**Goals (Stage 2):**
- Eliminate the per-INTERFACE-type registration requirement entirely.
- Move socket ownership (`WebSocketConnection`) into a permanently-stable non-template class in `.cpp`.
- Ensure `ResourceMonitor` always holds a pointer whose vtable is in the websocket library, regardless of INTERFACE type.
- Separate framing logic (`ChannelImpl<INTERFACE>`) from socket management so framing can live in plugin DSOs safely.

**Non-Goals (both stages):**
- Changing the channel sharing policy (identity key remains `hostAddress@callsign`).
- Adding thread-safety above what `Core::ProxyMapType` already provides.
- Changing the public `LinkType<INTERFACE>` API visible to callers.

## Decisions

### Decision 1 (Stage 1) — Whole-class explicit instantiation in `.cpp` + `extern template` in header

**Constraint:** `CommunicationChannel` and all its nested types (`ChannelImpl`, `FactoryImpl`, `HandlerType`) are dependent on `INTERFACE`. A single non-template `.cpp` function cannot own the type-specific map or its vtables. The fix must control where the compiler emits the instantiation.

**Chosen:** Whole-class **explicit instantiation** (not specialization) of `LinkType<Core::JSON::IElement>` in `JSONRPCLink.cpp`, paired with `extern template class` in the header.

Explicit instantiation (`template class T<U>` / `extern template class T<U>`) is the C++ standardised mechanism for controlling which TU owns a template instantiation. It is semantically distinct from explicit specialization (`template<> class T<U>`) — specialization implies behavioural difference, while instantiation is purely about DSO placement. Using explicit instantiation is correct here because there is no behavioural difference between INTERFACE types; only the framing types differ.

In `JSONRPCLink.h` (after the `LinkType` class body):
```cpp
// Suppress implicit instantiation in all consumer TUs.
// The websocket library provides the single authoritative instantiation.
// Add a matching `template class LinkType<...>` line in JSONRPCLink.cpp for each new INTERFACE type.
extern template class LinkType<Core::JSON::IElement>;
```

In `JSONRPCLink.cpp`:
```cpp
template class LinkType<Core::JSON::IElement>;
```

This causes the compiler to emit every symbol for `LinkType<IElement>` — including `CommunicationChannel`'s vtable, `ChannelImpl`'s vtable, `HandlerType`'s vtable, and the `channelMap` function-local static — exactly once, into the websocket library's object file. Plugin libraries see `extern template` and do not emit their own copies.

**INTERFACE types to register:** Only `Core::JSON::IElement` is instantiated in the current codebase. `Core::JSON::IMessagePack` is protocol-supported but has no callers; a matching line MAY be added for defensive completeness. The total INTERFACE space is bounded: only types that `Core::JSONRPC::Message` implements are valid (currently exactly these two), enforced by the `Core::ProxyType<INTERFACE>(message)` cast in `Send()`.

**Alternative considered: per-method explicit specialization**  
Specializing only `CommunicationChannel::Instance()` anchors the `channelMap` static but leaves `HandlerType`'s vtable in the plugin DSO. `ResourceMonitor` still holds a pointer whose vtable dangles on plugin unload. This alternative does not fully fix the bug.

**Alternative considered: non-template free function with type-erased registry**  
A single free function in `.cpp` using a `std::unordered_map<std::type_index, void*>` registry can serve all INTERFACE types, but introduces type-erasure complexity, requires a factory lambda, and has RTTI/visibility risks across shared libraries. Not warranted given the finite set of INTERFACE types in use.

### Decision 2 (Stage 1) — Keep `Instance()` signature unchanged

The generic template definition of `Instance()` in the header remains as-is for any INTERFACE type that does not have a registered explicit instantiation. The `extern template class` declaration is additive. No change to call sites.

### Decision 3 (Stage 1 & 2) — Map key scheme unchanged

The `ProxyMapType` key remains `remoteNode.HostAddress() + '@' + callsign`. No change to the channel identity or sharing policy.

### Decision 4 (Stage 2) — Separate `WebSocketConnection` (non-template) from `ChannelImpl<INTERFACE>` (framing adapter)

**Problem with Stage 1:** Even with all vtables in the websocket lib, the socket ownership and framing are still fused in `StreamJSONType`. Adding a new INTERFACE type requires two registration lines. The design is correct but not minimal.

**Chosen:** Extract socket management into a new non-template class `WebSocketConnection` defined in `JSONRPCLink.cpp`. `ChannelImpl<INTERFACE>` becomes a thin framing adapter that does not own the socket and does not implement `IResource`.

**`WebSocketConnection`** (non-template, defined in `JSONRPCLink.cpp`):
- Owns `Web::WebSocketClientType<Core::SocketStream>` by value.
- Owns the `channelMap` static (`Core::ProxyMapType<string, WebSocketConnection>`).
- Implements `IResource` (via `SocketStream`); `ResourceMonitor` holds `IResource*` here — always in the websocket lib, never in a plugin.
- Calls back via `IFramingCallback` (new narrow interface, ~6 lines, in the header).
- `Instance()` is a plain non-template `EXTERNAL` static method.

**`IFramingCallback`** (new, in header):
```cpp
class EXTERNAL IFramingCallback {
public:
    virtual ~IFramingCallback() = default;
    virtual void OnReceived(const Core::ProxyType<Core::JSONRPC::Message>&) = 0;
    virtual void OnStateChange(bool open) = 0;
};
```

**`ChannelImpl<INTERFACE>`** (remains in header, simplified):
- No longer inherits from `StreamJSONType` — no longer owns the socket.
- Implements `IFramingCallback` — handles JSON text or binary deserialisation only.
- Its vtable lives in the plugin DSO, but `ResourceMonitor` never touches it — only `WebSocketConnection`'s vtable matters.

**`IChannelClient`** (new, in header, ~8 lines):
```cpp
class EXTERNAL IChannelClient {
public:
    virtual ~IChannelClient() = default;
    virtual void Opened() = 0;
    virtual void Closed() = 0;
    virtual uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>&) = 0;
    virtual uint64_t Timed() = 0;
};
```
`LinkType<INTERFACE>` inherits `IChannelClient` and adds `virtual` to its four existing implementations.

**`CommunicationChannel`** (non-template, defined in `JSONRPCLink.cpp`):
- `_observers` is `std::list<IChannelClient*>` — no INTERFACE dependency.
- `channelMap` holds `Core::ProxyType<WebSocketConnection>`.
- Stage 1 `extern template` / `template class` lines are removed.

**`FactoryImpl`** (non-template, defined in `JSONRPCLink.cpp`):
- `WatchDog` stores `IChannelClient*` instead of `LinkType<INTERFACE>*`.
- One `Core::TimerType<WatchDog>` per process, not one per INTERFACE type.

**Alternative considered: keep `ChannelImpl` inheriting `StreamJSONType`, just move `CommunicationChannel` out**  
Still requires template code for socket-related vtables and does not eliminate the per-type registration requirement for Stage 1's successor. The clean break is to have the socket class not be a template at all.

## Alternatives Considered

All alternatives below address the same root problem: template code instantiated in a plugin DSO produces symbols (vtables, statics) that dangle when the plugin is unloaded.

---

### Alternative 1 — CommunicationChannelBase as a type-erased handle

Introduce a non-template `CommunicationChannelBase` class defined in `JSONRPCLink.cpp` with a virtual destructor. `CommunicationChannel<INTERFACE>` inherits from it. `Instance()` moves to `JSONRPCLink.cpp` and owns the `channelMap` as a static of type `Core::ProxyMapType<string, CommunicationChannelBase>`. `LinkType<INTERFACE>` holds `Core::ProxyType<CommunicationChannelBase>` as `_channel`, downcasting to the concrete type when it needs INTERFACE-specific operations.

```cpp
// header
class EXTERNAL CommunicationChannelBase {
public:
    virtual ~CommunicationChannelBase() = default;
    // virtual IResource methods (SendData, ReceiveData, StateChange, IsIdle) declared here
    // so ResourceMonitor's IResource* points into this class whose vtable is in the websocket lib
};

// .cpp — Instance() owns the map; channelMap static is in the websocket library
EXTERNAL Core::ProxyType<CommunicationChannelBase>
CommunicationChannelBase::Instance(const Core::NodeId& remoteNode, const string& callsign, const string& query)
{
    static Core::ProxyMapType<string, CommunicationChannelBase> channelMap;
    ...
}
```

**What type erasure achieves here:**
- The `channelMap` static is inside a non-template function in `.cpp` — it is owned by the websocket library and never duplicated. ✓
- `CommunicationChannelBase`'s own vtable (the virtual destructor and any virtual `IResource` forwarders declared on it) is emitted once into the websocket library. ✓

**Why it still fails to fix the crash:**

1. **`IResource` implementation location.** `ResourceMonitor` calls `SendData()`, `ReceiveData()`, `StateChange()`, `IsIdle()` through the `IResource*` it holds. For these calls to be safe after plugin unload, their vtable entry must point into the websocket library. Two sub-cases:

   - *Make them pure virtual on `CommunicationChannelBase`, implemented in `CommunicationChannel<INTERFACE>`.* The override vtable is emitted in whichever DSO instantiates the template — still the plugin DSO. `ResourceMonitor`'s call still goes through a vtable in the plugin DSO. **Crash not fixed.**
   - *Implement them concretely on `CommunicationChannelBase` by owning the socket directly.* This requires `CommunicationChannelBase` to hold a `WebSocketClientType<SocketStream>` by value or pointer. But `WebSocketClientType<SocketStream>` is not template-dependent — this is precisely what Stage 2 does. If you pursue this sub-case, `CommunicationChannelBase` _becomes_ `WebSocketConnection` and Alternative 1 converges on Stage 2.

2. **Object construction.** `CommunicationChannelBase::Instance()` in `.cpp` must construct the correct derived object (`CommunicationChannel<IElement>` or `CommunicationChannel<IMessagePack>`). The `.cpp` TU does not know INTERFACE. Constructing the right derived type requires either: (a) a per-type registered factory (converging on Alternative 4), (b) explicit instantiation of `CommunicationChannel<INTERFACE>` in `.cpp` (making the base class redundant — why not just use whole-class instantiation as in Alternative 5?), or (c) the base class owns the socket entirely and there is no derived type (Stage 2).

3. **`_channel` member type in `LinkType<INTERFACE>`.** `LinkType<INTERFACE>` holds `_channel` and calls `_channel->Submit(Core::ProxyType<INTERFACE>(message))`, `_channel->Register(*this)`, etc. These methods are INTERFACE-specific and cannot be on `CommunicationChannelBase`. Every call site must `static_cast<CommunicationChannel<INTERFACE>*>(_channel.operator->())` before use. This cast is valid only if the dynamic type is `CommunicationChannel<INTERFACE>` — which the compiler cannot verify — and is conceptually identical to just keeping `_channel` typed as `Core::ProxyType<CommunicationChannel<INTERFACE>>` (which is what the code already has). The downcast buys nothing.

**Summary:** The type erasure correctly anchors the `channelMap` static. The `IResource` vtable problem — the direct cause of the crash — is not fixed unless the socket is also moved into the base class, at which point this alternative becomes Stage 2. The required downcast at every call site and the object construction ambiguity make this a more complex path to a partial fix than Alternative 5.

---

### Alternative 2 — CommunicationChannelBase with type-independent members extracted

Extract all members of `CommunicationChannel` that do not depend on INTERFACE into a non-template base: `_adminLock`, `_sequence`, `Open()`, `Close()`, `StateChange()`. Move `Instance()` to `.cpp` returning `Core::ProxyType<CommunicationChannelBase>`.

**Drawbacks:**
- `_observers` is `std::list<LinkType<INTERFACE>*>` — type-dependent. To move it to the base it must become `std::list<IChannelClient*>`, which requires defining `IChannelClient`. Once `IChannelClient` exists this alternative converges on Stage 2 — making it Stage 2 with extra indirection.
- `ChannelImpl` is still a data member of the template-derived class. `HandlerType`'s vtable is still in the plugin DSO. **`ResourceMonitor`'s `IResource*` still dangles.** The socket vtable problem is not fixed by moving lock/sequence/open/close to a base.
- `Instance()` in `.cpp` has the same construction problem as Alternative 1: the concrete derived type must be named to construct the object, requiring the INTERFACE type to be visible in the `.cpp` TU.
- This is a significant refactoring investment for a partial fix. It does not satisfy the requirement that `ResourceMonitor` references remain valid after plugin unload.

---

### Alternative 3 — Per-method explicit specialization of Instance() only

Specialize only the static `CommunicationChannel::Instance()` method in `JSONRPCLink.cpp` for each INTERFACE type using `template <>`:

```cpp
// .cpp
template <>
EXTERNAL Core::ProxyType<LinkType<Core::JSON::IElement>::CommunicationChannel>
LinkType<Core::JSON::IElement>::CommunicationChannel::Instance(
    const Core::NodeId& remoteNode, const string& callsign, const string& query)
{
    static Core::ProxyMapType<string,
        LinkType<Core::JSON::IElement>::CommunicationChannel> channelMap;
    ...
}
```

This was the approach in the original draft of this change.

**Drawbacks:**
- **Fixes `channelMap` ownership but does not fix the vtable problem.** `HandlerType` (inside `StreamJSONType`, inside `ChannelImpl`) is still implicitly instantiated in the plugin DSO. `ResourceMonitor` holds `IResource*` into `HandlerType::SocketStream` — that vtable still dangles when the plugin is unloaded. The SIGSEGV is not fully eliminated.
- Explicit specialization (`template <>`) is the wrong semantic tool. Specialization declares a behavioural difference for a specific type. Here the behavior is identical for all INTERFACE types; only the DSO placement needs to change. Using `template <>` expresses the wrong intent and misleads future maintainers.
- More verbose than whole-class instantiation: each INTERFACE type requires spelling out the full method body (including the `searchLine` key construction) rather than a single `template class` line.
- Accessing a `private` nested type in a specialization at namespace scope is technically access-controlled; some compilers reject it or require a `friend` declaration workaround.

---

### Alternative 4 — Non-template free function with type-erased registry

A single non-template function in `JSONRPCLink.cpp` owns a `std::unordered_map<std::type_index, void*>` registry. Each INTERFACE type registers a factory lambda on first use; subsequent calls return the cached channel.

```cpp
// .cpp
static std::unordered_map<std::type_index, std::function<void*(...)>> registry;

template <typename INTERFACE>
Core::ProxyType<CommunicationChannel>
GetOrCreate(const Core::NodeId& remoteNode, const string& callsign, const string& query)
{
    auto& factory = registry[std::type_index(typeid(INTERFACE))];
    ...
}
```

**Drawbacks:**
- Requires RTTI (`typeid`, `std::type_index`) across shared library boundaries, which has known fragility in multi-DSO environments when symbol visibility is restricted.
- The registry stores `void*` — requires `static_cast` back to the concrete `CommunicationChannel<INTERFACE>` type, introducing the same cross-DSO downcast risk as Alternative 1.
- `ChannelImpl` and `HandlerType` vtables are still in the plugin DSO. **The `ResourceMonitor` vtable crash is not fixed.**
- Adds significant complexity (registry, factory lambdas, RTTI) for no benefit over the chosen approach.
- `std::function` allocation and `unordered_map` lookup per-call introduce non-trivial overhead on the hot path.

---

### Alternative 5 — Whole-class explicit instantiation (chosen for Stage 1)

`extern template class LinkType<Core::JSON::IElement>` in the header suppresses implicit instantiation in all consumer TUs. `template class LinkType<Core::JSON::IElement>` in `JSONRPCLink.cpp` anchors every symbol — vtables, statics, function bodies — for the entire class in the websocket library.

**Why chosen:** Semantically correct (DSO placement, not behavioural override), minimal (2 lines per INTERFACE type), fixes both `channelMap` and all vtables simultaneously, no access-control issues, no RTTI, no cast risk. Two lines required per new INTERFACE type, but the INTERFACE value space is bounded to 2 values (`IElement`, `IMessagePack`).

**Residual limitation:** `FactoryImpl` is a different singleton per INTERFACE instantiation — two active INTERFACE types produce two timer threads. Addressed by Stage 2.

---

### Alternative 6 — Socket/framing separation with non-template WebSocketConnection (chosen for Stage 2)

Separate `WebSocketConnection` (non-template, owns the socket, `IResource` in websocket lib) from `ChannelImpl<INTERFACE>` (thin framing adapter, in plugin DSO). `ResourceMonitor` holds `IResource*` into `WebSocketConnection` permanently.

**Why chosen:** Eliminates the per-INTERFACE-type registration requirement entirely (zero lines per new type), ensures `ResourceMonitor` references are structurally stable regardless of plugin lifetime, and corrects the root architectural tension (`StreamJSONType` fusing socket ownership and framing). The added `IFramingCallback` and `IChannelClient` interfaces are small and well-bounded.

**Trade-off:** Substantially more refactoring than Stage 1. Stage 1 is safe to ship independently while Stage 2 is designed.

## Risks / Trade-offs

### Stage 1

- **`extern template class` must appear before any use in a consumer TU** — If a consumer TU instantiates `LinkType<IElement>` before parsing the `extern template class` line (e.g., through a prior include), the compiler may silently generate local vtables and the `channelMap` static. Place the `extern template class` declaration immediately after the `LinkType` class body — before any other code in the header — so it is always visible when the class is first seen.

- **Whole-class instantiation pulls all `LinkType<IElement>` symbols into the websocket lib** — This is the desired behaviour, but it means the websocket lib must be able to compile all of `LinkType`'s methods. Confirm no methods use types only available in consumer plugin headers; none currently do.

- **ODR for `FactoryImpl`** — `FactoryImpl` is accessed via `Core::SingletonType<FactoryImpl>::Instance()` — ODR-safe by the Singleton machinery. `extern template class LinkType<IElement>` will anchor `FactoryImpl` inside `LinkType<IElement>::CommunicationChannel` in the websocket lib. A subsequent `extern template class LinkType<IMessagePack>` would produce a distinct `FactoryImpl` singleton (different template instantiation), resulting in two timer threads. Stage 2 eliminates this by making `FactoryImpl` non-template.

- **New INTERFACE types** — Without Stage 2, a new INTERFACE type requires 2 registration lines. Document this requirement with a comment at the `extern template` site.

### Stage 2

- **`IFramingCallback` and `IChannelClient` are published COM-like interfaces** — Once added to a shipped header they are part of the ABI. Design them carefully and mark them `EXTERNAL`. Do not add methods after publication without versioning.

- **`WebSocketConnection` is not a `Core::IUnknown` COM interface** — It is a plain C++ class with process lifetime, owned by `Core::ProxyMapType`. It does not need COM ref-counting beyond what `Core::ProxyType` provides. Do not attempt to `QueryInterface` on it.

- **`LinkType<INTERFACE>` inheriting `IChannelClient`** — The four methods (`Opened`, `Closed`, `Inbound`, `Timed`) already exist on `LinkType`. Adding `virtual` and `: public IChannelClient` to them is binary-incompatible for callers that store `LinkType<INTERFACE>` objects by value on the stack. Confirm all callers use `LinkType<INTERFACE>` via heap allocation or pointer — they currently do (constructed with `new` or as members of `SmartLinkType`).

## Migration Plan

### Stage 1
1. Add `extern template class LinkType<Core::JSON::IElement>` after the `LinkType` class body in `JSONRPCLink.h`, with a comment documenting the registration requirement.
2. Add `template class LinkType<Core::JSON::IElement>` in `JSONRPCLink.cpp`.
3. Build the websocket library. Confirm with `nm -C libThunderWebSocket.so` that the `CommunicationChannel::channelMap` static and `ChannelImpl`/`HandlerType` vtable symbols have global (`T`/`V`) binding in the websocket lib only.
4. Build consumer plugins; verify they link cleanly with no duplicate-symbol warnings.
5. Run existing JSON-RPC integration tests; confirm channels are established and reused correctly.
6. Deactivate the plugin that opened a channel while a second plugin holds a shared reference; verify no SIGSEGV.

### Stage 2
1. Define `IFramingCallback` and `IChannelClient` in `JSONRPCLink.h`.
2. Define `WebSocketConnection` (non-template) in `JSONRPCLink.cpp` with `channelMap` static and `IResource` implementation.
3. Refactor `ChannelImpl<INTERFACE>` in the header to implement `IFramingCallback` rather than inheriting `StreamJSONType`. `ChannelImpl` holds a reference (or pointer) to `WebSocketConnection` for sending.
4. Make `CommunicationChannel` non-template in `JSONRPCLink.cpp`; change `_observers` to `std::list<IChannelClient*>`.
5. Make `FactoryImpl` non-template in `JSONRPCLink.cpp`; change `WatchDog` to store `IChannelClient*`.
6. Add `: public IChannelClient` and `virtual` to `Opened`, `Closed`, `Inbound`, `Timed` in `LinkType<INTERFACE>`.
7. Remove Stage 1 `extern template class` / `template class` lines — no longer needed.
8. Build, `nm -C` verification, full integration test suite, plugin deactivation SIGSEGV regression test.
