## Why

Tracked in [rdkcentral/Thunder#2040](https://github.com/rdkcentral/Thunder/issues/2040).

`CommunicationChannel::Instance()` holds `channelMap` as a function-local static inside a class template method defined entirely in `JSONRPCLink.h`:

```cpp
static Core::ProxyType<CommunicationChannel> Instance(const Core::NodeId& remoteNode, const string& callsign, const string& query)
{
    static Core::ProxyMapType<string, CommunicationChannel> channelMap;
    string searchLine = remoteNode.HostAddress() + '@' + callsign;
    return (channelMap.template Instance<CommunicationChannel>(searchLine, remoteNode, callsign, query));
}
```

Because `CommunicationChannel` is a nested class of the `LinkType<INTERFACE>` template, the entire class — including its vtable, the vtable of its nested `ChannelImpl<INTERFACE>` member, and the `channelMap` static — is instantiated in whichever plugin shared library first triggers the template instantiation. In RDK, platform linker flags ensure only one copy of this static exists per process via COMDAT merging, so channel sharing works: subsequent plugins calling `Instance()` for the same endpoint receive a proxy to the existing channel. However, the `channelMap` object itself and the vtable of every `CommunicationChannel` and `ChannelImpl<INTERFACE>` instance are owned by the originating plugin's library.

The inheritance chain that carries the problem is:

```
ChannelImpl<INTERFACE>
  → StreamJSONType<WebSocketClientType<SocketStream>, FactoryImpl&, INTERFACE>
      → HandlerType<WebSocketClientType<SocketStream>>   ← nested inside StreamJSONType
          → WebSocketClientType<SocketStream>
              → SocketStream  →  IResource
```

`ResourceMonitor` holds an `IResource*` pointer into the `SocketStream` deep inside `ChannelImpl`. Because `HandlerType` is nested inside the `StreamJSONType` template, its vtable is instantiated in the same plugin DSO as `CommunicationChannel`.

When the originating plugin is **deactivated and its shared library is unloaded**, both the `channelMap` static and the vtable for `CommunicationChannel` / `HandlerType` / `ChannelImpl` become dangling. Any later operation on the shared channel — in particular the `ResourceMonitor` callback that still holds a live `IResource*` — dereferences the unmapped vtable address and produces a **SIGSEGV**. The gdb evidence from the issue confirms this pattern exactly.

### Root architectural tension

`StreamJSONType<SOURCE, ALLOCATOR, INTERFACE>` fuses two unrelated concerns into one template class:

- **Socket ownership** — the `HandlerType<SOURCE>` member manages the underlying TCP socket and implements `IResource`. This has **process lifetime**: `ResourceMonitor` holds a reference to it as long as the connection is open.
- **Protocol framing** — the `Serializer` / `Deserializer` members handle JSON text (for `IElement`) or binary MessagePack (for `IMessagePack`). This is **INTERFACE-dependent**.

Because both concerns are fused inside a template parameterised on `INTERFACE`, the socket-related vtables are tied to the instantiating DSO, which may be a plugin library with a shorter lifetime than the process.

## What Changes

This change is implemented in **two stages**. Stage 1 is the immediate bug fix; Stage 2 is the correct long-term architectural resolution.

---

### Stage 1 — Short-term fix: whole-class explicit instantiation (Option A)

`CommunicationChannel` and all of its nested types (`ChannelImpl`, `FactoryImpl`, `HandlerType`) are INTERFACE-dependent. The correct mechanism to anchor their code — including `channelMap`, all vtables, and all statics — in a specific DSO without changing any behaviour is **explicit template instantiation** (not explicit specialisation, which would imply behavioural difference):

**`JSONRPCLink.h`** — after the `LinkType` class body, suppress implicit instantiation in all consumer TUs:

```cpp
// Suppress implicit instantiation of LinkType<IElement> in all consumer TUs.
// The websocket library provides the single authoritative instantiation.
// Add a matching `template class LinkType<...>` line in JSONRPCLink.cpp for each new INTERFACE type.
extern template class LinkType<Core::JSON::IElement>;
```

**`JSONRPCLink.cpp`** — force the complete instantiation into the websocket library TU:

```cpp
template class LinkType<Core::JSON::IElement>;
```

This causes the compiler to emit every symbol for `LinkType<IElement>` — including `CommunicationChannel`'s vtable, `ChannelImpl`'s vtable, `HandlerType`'s vtable, and the `channelMap` function-local static — exactly once, into the websocket library's object file. Plugin libraries that include the header see `extern template` and do not emit their own copies; they link to the websocket library's symbols instead.

**Properties of this approach:**
- `extern template` / `template class` is the C++ standardised mechanism for controlling which TU owns template instantiations. It expresses DSO placement, not behavioural divergence — semantically correct for this problem.
- No new function bodies, no specialisations, no ABI changes to `LinkType<INTERFACE>` or `CommunicationChannel`.
- Fixes both the `channelMap` static and all affected vtables in a single operation.
- Adding a new INTERFACE type requires two lines: one `extern template` in the header and one `template class` in the `.cpp`. A comment at the `extern template` site documents this contract. The INTERFACE value space is bounded: only `Core::JSON::IElement` (active) and `Core::JSON::IMessagePack` (protocol-supported but currently unused) are valid because `Core::JSONRPC::Message` must implement the INTERFACE, and only these two are implemented.

---

### Stage 2 — Long-term fix: separate socket ownership from protocol framing

The two-line-per-type requirement in Stage 1, while minimal, exists because the socket's vtable is inside a template class. The correct architectural fix eliminates the requirement entirely by separating the two concerns that `StreamJSONType` conflates.

**New non-template class `WebSocketConnection`** (defined in `JSONRPCLink.cpp`):
- Owns the `WebSocketClientType<SocketStream>` by value — non-template, vtable permanently in the websocket library.
- Owns the `channelMap` static — the map stores `Core::ProxyType<WebSocketConnection>`, not a template type.
- Implements `IResource` (via `SocketStream` inheritance) — `ResourceMonitor` holds `IResource*` into this object, which is always valid.
- Calls back via a narrow non-template `IFramingCallback` interface when a message arrives or the connection state changes.

```cpp
class EXTERNAL IFramingCallback {
public:
    virtual ~IFramingCallback() = default;
    virtual void OnReceived(const Core::ProxyType<Core::JSONRPC::Message>&) = 0;
    virtual void OnStateChange(bool open) = 0;
};
```

**`ChannelImpl<INTERFACE>` becomes a thin framing adapter** (remains in the header):
- No longer inherits from `StreamJSONType` — no longer owns the socket.
- Implements `IFramingCallback` — handles JSON text or binary MessagePack deserialisation for the specific INTERFACE.
- Created per-plugin or per-client, destroyed when the library unloads. Its vtable lives in the plugin DSO, but `ResourceMonitor` never touches it — `ResourceMonitor` only touches `WebSocketConnection`.

**`FactoryImpl` becomes non-template** (moves to `JSONRPCLink.cpp`):
- `WatchDog` stores `IChannelClient*` instead of `LinkType<INTERFACE>*`.
- One shared message pool and one timer thread for the entire process, regardless of how many INTERFACE types are used.

**`IChannelClient` interface** (new, in header, ~8 lines):
- `Opened()`, `Closed()`, `Inbound(Core::ProxyType<Core::JSONRPC::Message>)`, `Timed()` as pure virtuals.
- `LinkType<INTERFACE>` implements it — four methods already exist, add `virtual` and inherit.
- `CommunicationChannel`'s `_observers` list becomes `std::list<IChannelClient*>`, removing its INTERFACE dependency.

**`CommunicationChannel` becomes non-template** (moves to `JSONRPCLink.cpp`):
- Owns `WebSocketConnection` by value (or as a composed member).
- All channel management logic (register/unregister/sequence/inbound routing) is unchanged in behaviour.
- `Instance()` is a plain non-template `EXTERNAL` function — no per-type registration needed.

**Properties of this approach:**
- Adding a new INTERFACE type (e.g., a future binary protocol) requires zero changes to the channel ownership layer. `ChannelImpl<INewProtocol>` is a thin adapter that compiles into the new plugin without any registration.
- Adding a new transport (e.g., TLS socket) requires a new non-template `WebSocketConnection` subclass in `.cpp` — semantically correct because it is genuinely a new transport implementation.
- `FactoryImpl`'s timer thread is created once per process instead of once per INTERFACE type, reducing resource consumption.
- `ResourceMonitor` always holds a pointer into a non-template class with a permanently stable vtable.

### Complete usage matrix evaluated against both stages

| Instantiation | Status | Stage 1 | Stage 2 |
|---|---|---|---|
| `LinkType<Core::JSON::IElement>` | Active — only real usage | Fixed: 2 lines in header + .cpp | Fixed: zero per-type changes |
| `LinkType<Core::JSON::IMessagePack>` | Dead code — protocol supported, no callers | Fixed: 2 lines if activated | Fixed: zero per-type changes |
| `SmartLinkType<Core::JSON::IElement>` | Active — wraps two `LinkType<IElement>` instances | Fixed by same 2 lines | Fixed: zero per-type changes |
| `SmartLinkType<Core::JSON::IMessagePack>` | Dead code | Fixed: 2 lines if activated | Fixed: zero per-type changes |
| `Client` (deprecated alias for `LinkType<IElement>`) | Backward compat only | Fixed by same 2 lines | Fixed: zero per-type changes |
| Hypothetical new INTERFACE type | Not currently possible without modifying `JSONRPC::Message` | Requires 2 new lines | Zero changes to channel layer |
| Hypothetical new transport (TLS etc.) | Not in current design | Not addressed | One new non-template class in `.cpp` |

## Capabilities

### New Capabilities
- `channelmap-cpp-anchor` (Stage 1): `channelMap` and all INTERFACE-dependent vtables are anchored in the websocket library translation unit, ensuring that shared channels and their `ResourceMonitor` references remain valid for the lifetime of the process.
- `socket-framing-separation` (Stage 2): `WebSocketConnection` separates socket ownership (process lifetime, non-template) from protocol framing (INTERFACE-dependent, per-plugin). Adding a new INTERFACE type requires zero changes to the channel ownership layer.

### Modified Capabilities
- (Stage 2) `FactoryImpl` — becomes non-template; one shared message pool and timer thread per process.

## Impact

**Stage 1:**
- **Source/websocket/JSONRPCLink.h** — `extern template class LinkType<Core::JSON::IElement>` added after the `LinkType` class body.
- **Source/websocket/JSONRPCLink.cpp** — `template class LinkType<Core::JSON::IElement>` added.
- **Bug fixed**: SIGSEGV on plugin deactivation caused by dangling vtables after the owning shared library is unloaded (rdkcentral/Thunder#2040).
- No API, ABI, or CMakeLists changes. Consumers of `LinkType<INTERFACE>` require no source changes.

**Stage 2:**
- **Source/websocket/JSONRPCLink.h** — `IFramingCallback`, `IChannelClient` interfaces added (~16 lines); `CommunicationChannel` reduced to a forward declaration or thin shell; `ChannelImpl<INTERFACE>` refactored to implement `IFramingCallback`; `LinkType<INTERFACE>` gains `: public IChannelClient` and four `virtual` methods; `FactoryImpl` moved out of the template.
- **Source/websocket/JSONRPCLink.cpp** — `WebSocketConnection`, non-template `CommunicationChannel`, non-template `FactoryImpl` added (~250 lines); `channelMap` static moves here permanently.
- **Source/core/StreamJSON.h** — no changes required; `ChannelImpl` no longer inherits from `StreamJSONType` directly (it wraps it or uses it differently).
- Stage 1 `extern template` / `template class` lines are removed as they become unnecessary.
- No API changes visible to `LinkType<INTERFACE>` callers.
