## 1. Preparation

- [ ] 1.1 Read `Source/websocket/JSONRPCLink.h` in full — confirm exact location of `channelMap` static inside `CommunicationChannel::Instance()` and note the `INTERFACE` template parameter dependency
- [ ] 1.2 Verify that `CommunicationChannel` is `private` inside `LinkType<INTERFACE>` and identify any access-control barriers the helper function must work around (friend declaration vs. type erasure)
- [ ] 1.3 Review `Source/websocket/JSONRPCLink.cpp` — confirm it already includes `JSONRPCLink.h` and compiles into the websocket library

## 2. Header Changes (JSONRPCLink.h)

- [ ] 2.1 Add `extern template` declarations for `LinkType<Core::JSON::IElement>::CommunicationChannel::Instance(const Core::NodeId&, const string&, const string&)` (and for `IMessagePack` if required) after the `LinkType` class body to suppress implicit instantiation in every consumer TU
- [ ] 2.2 Add a comment above the `extern template` block stating that any new INTERFACE type intended for cross-library use must receive a matching explicit specialization in `JSONRPCLink.cpp`
- [ ] 2.3 Confirm that the generic template definition of `Instance()` in the header is left intact as the fallback for unspecialized INTERFACE types

## 3. Source Changes (JSONRPCLink.cpp)

- [ ] 3.1 Add an explicit full specialization `template <> EXTERNAL Core::ProxyType<LinkType<Core::JSON::IElement>::CommunicationChannel> LinkType<Core::JSON::IElement>::CommunicationChannel::Instance(const Core::NodeId&, const string&, const string&)` containing a `static Core::ProxyMapType<string, LinkType<Core::JSON::IElement>::CommunicationChannel> channelMap`
- [ ] 3.2 Repeat 3.1 for `Core::JSON::IMessagePack` if that INTERFACE type needs covering
- [ ] 3.3 Verify the `searchLine = remoteNode.HostAddress() + '@' + callsign` key construction is preserved verbatim in each specialization

## 4. Verification

- [ ] 4.1 Build the websocket library; resolve any compile or linker errors introduced by the access-control or type changes
- [ ] 4.2 Run `nm -C libThunderWebSocket.so` (or equivalent) and confirm exactly one definition of the channel-map static/accessor symbol is present
- [ ] 4.3 Build a consumer plugin or test binary that links against the websocket library; verify it links cleanly without duplicate-symbol warnings
- [ ] 4.4 Run existing JSON-RPC integration tests (e.g., `Tests/unit/` or manual WPEFramework startup) and confirm channels are established and reused correctly
- [ ] 4.5 Confirm that two callers using the same endpoint within the same process share a single `CommunicationChannel` (ref-count > 1 on the returned proxy)
