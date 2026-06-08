## Stage 1 — Short-term fix: whole-class explicit instantiation

### 1. Preparation

- [ ] 1.1 Read `Source/websocket/JSONRPCLink.h` in full — confirm exact location of `channelMap` static inside `CommunicationChannel::Instance()`, confirm the full vtable inheritance chain (`ChannelImpl` → `StreamJSONType` → `HandlerType` → `SocketStream` → `IResource`), and note the INTERFACE template parameter dependency
- [ ] 1.2 Review `Source/websocket/JSONRPCLink.cpp` — confirm it already includes `JSONRPCLink.h` and compiles into the websocket library
- [ ] 1.3 Confirm that `Core::JSON::IElement` is the only active `LinkType<INTERFACE>` instantiation in the codebase (`git grep 'LinkType<'` across all repos using the websocket library)

### 2. Header Changes (JSONRPCLink.h)

- [ ] 2.1 After the `LinkType` class body (before `typedef LinkType<Core::JSON::IElement> DEPRECATED Client`), add:
  ```cpp
  // Suppress implicit instantiation of LinkType<IElement> in all consumer TUs.
  // The websocket library provides the single authoritative instantiation (see JSONRPCLink.cpp).
  // Add a matching `template class LinkType<...>` line in JSONRPCLink.cpp for each new INTERFACE type.
  extern template class LinkType<Core::JSON::IElement>;
  ```
- [ ] 2.2 Confirm that `SmartLinkType<INTERFACE>` internally uses `LinkType<INTERFACE>` and is covered by the same `extern template` line — no separate declaration needed for `SmartLinkType`

### 3. Source Changes (JSONRPCLink.cpp)

- [ ] 3.1 Add `template class LinkType<Core::JSON::IElement>;` to `JSONRPCLink.cpp` to force complete instantiation of all `LinkType<IElement>` members (vtables, statics) into the websocket library TU
- [ ] 3.2 Confirm `channelMap`, `ChannelImpl` vtable, and `HandlerType` vtable symbols appear with global binding in `nm -C libThunderWebSocket.so` and are absent from consumer plugin DSOs

### 4. Verification (Stage 1)

- [ ] 4.1 Build the websocket library; resolve any compile or linker errors
- [ ] 4.2 Run `nm -C libThunderWebSocket.so` (or equivalent) and confirm exactly one definition of `LinkType<IElement>::CommunicationChannel::Instance()` and the `channelMap` static symbol
- [ ] 4.3 Build a consumer plugin; verify it links cleanly without duplicate-symbol warnings
- [ ] 4.4 Run existing JSON-RPC integration tests (e.g., `Tests/unit/` or manual WPEFramework startup) and confirm channels are established and reused correctly
- [ ] 4.5 Deactivate a plugin that uses `LinkType<IElement>` while a second plugin holds a shared channel reference; verify no SIGSEGV

---

## Stage 2 — Long-term fix: socket/framing separation

### 5. Define Narrow Interfaces (JSONRPCLink.h)

- [ ] 5.1 Add `IFramingCallback` interface after the module-level `using` declarations and before `LinkType`:
  ```cpp
  class EXTERNAL IFramingCallback {
  public:
      virtual ~IFramingCallback() = default;
      virtual void OnReceived(const Core::ProxyType<Core::JSONRPC::Message>&) = 0;
      virtual void OnStateChange(bool open) = 0;
  };
  ```
- [ ] 5.2 Add `IChannelClient` interface in the same location:
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

### 6. Implement WebSocketConnection (JSONRPCLink.cpp)

- [ ] 6.1 Define non-template class `WebSocketConnection` in `JSONRPCLink.cpp`:
  - Owns `Web::WebSocketClientType<Core::SocketStream>` by value
  - Provides `static Core::ProxyType<WebSocketConnection> Instance(const Core::NodeId&, const string&, const string&)` with function-local `static Core::ProxyMapType<string, WebSocketConnection> channelMap`
  - Implements `Core::IResource` (via `SocketStream`) — `ResourceMonitor` will hold `IResource*` here
  - Stores one `IFramingCallback*` per registered framing adapter (or delegates via `CommunicationChannel`)
- [ ] 6.2 Annotate `WebSocketConnection::Instance()` with `EXTERNAL`
- [ ] 6.3 Confirm `ResourceMonitor` registration/deregistration uses `WebSocketConnection`'s `IResource*`, not `ChannelImpl`'s

### 7. Refactor FactoryImpl to non-template (JSONRPCLink.cpp)

- [ ] 7.1 Move `FactoryImpl` out of the `LinkType<INTERFACE>` class body into `JSONRPCLink.cpp` as a standalone (non-template) class
- [ ] 7.2 Change `WatchDog` to store `IChannelClient*` instead of `LinkType<INTERFACE>*`; update `Trigger()` and `Revoke()` accordingly
- [ ] 7.3 Verify that `Core::SingletonType<FactoryImpl>::Instance()` still provides one shared instance — confirm only one `TimerType<WatchDog>` thread is created per process regardless of INTERFACE type count

### 8. Refactor CommunicationChannel to non-template (JSONRPCLink.cpp)

- [ ] 8.1 Move `CommunicationChannel` out of the `LinkType<INTERFACE>` class body into `JSONRPCLink.cpp` as a standalone (non-template) class
- [ ] 8.2 Change `_observers` from `std::list<LinkType<INTERFACE>*>` to `std::list<IChannelClient*>`
- [ ] 8.3 Change `Register()` and `Unregister()` to accept `IChannelClient&`
- [ ] 8.4 Change the `channelMap` type to use `WebSocketConnection` (or keep it on `WebSocketConnection::Instance()` directly)
- [ ] 8.5 Remove `ChannelImpl` from `CommunicationChannel` — `WebSocketConnection` now owns the socket

### 9. Refactor ChannelImpl<INTERFACE> to framing adapter (JSONRPCLink.h)

- [ ] 9.1 Change `ChannelImpl<INTERFACE>` to implement `IFramingCallback` instead of inheriting from `StreamJSONType<WebSocketClientType<SocketStream>, FactoryImpl&, INTERFACE>`
- [ ] 9.2 Add a reference/pointer to `WebSocketConnection` in `ChannelImpl` for use in sending frames
- [ ] 9.3 Implement `OnReceived()` to cast `Core::ProxyType<Core::JSONRPC::Message>` to `Core::ProxyType<INTERFACE>` and dispatch to `CommunicationChannel::Inbound()`
- [ ] 9.4 Implement `OnStateChange()` to call `CommunicationChannel::StateChange()`
- [ ] 9.5 Confirm `ChannelImpl` no longer implements `IResource` and its vtable in a plugin DSO does not affect `ResourceMonitor`

### 10. Update LinkType<INTERFACE> (JSONRPCLink.h)

- [ ] 10.1 Add `: public IChannelClient` to `LinkType<INTERFACE>`
- [ ] 10.2 Declare `Opened()`, `Closed()`, `Inbound()`, `Timed()` as `virtual` — their existing bodies remain unchanged
- [ ] 10.3 Update `_channel` member from `Core::ProxyType<CommunicationChannel>` (template type) to the non-template `CommunicationChannel` equivalent
- [ ] 10.4 Remove the Stage 1 `extern template class LinkType<Core::JSON::IElement>` line — no longer needed

### 11. Remove Stage 1 registration lines (JSONRPCLink.cpp)

- [ ] 11.1 Remove `template class LinkType<Core::JSON::IElement>` from `JSONRPCLink.cpp` — `WebSocketConnection` being non-template makes per-INTERFACE registration unnecessary

### 12. Verification (Stage 2)

- [ ] 12.1 Build the websocket library; resolve compile errors from the refactoring
- [ ] 12.2 Confirm `WebSocketConnection` vtable and `IResource*` symbols are in the websocket library (`nm -C`); confirm no `ChannelImpl` vtable symbol in the websocket library (it lives in plugin DSOs, which is correct)
- [ ] 12.3 Build consumer plugins; verify no source changes required
- [ ] 12.4 Run all JSON-RPC integration tests; confirm channel establishment, reuse, and teardown are correct
- [ ] 12.5 Instantiate `LinkType<Core::JSON::IMessagePack>` in a test plugin without adding any registration lines; confirm the channel ownership layer works correctly (zero registration changes)
- [ ] 12.6 Deactivate the originating plugin while a second plugin holds the channel; confirm no SIGSEGV (full regression of the original issue)
