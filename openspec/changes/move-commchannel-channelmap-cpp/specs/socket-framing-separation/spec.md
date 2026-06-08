## ADDED Requirements

### Requirement: Non-template WebSocketConnection owns the socket
A non-template class `WebSocketConnection` SHALL be defined in `JSONRPCLink.cpp` (not in the header). It SHALL own the `Web::WebSocketClientType<Core::SocketStream>` object and SHALL implement `Core::IResource` via `SocketStream`. The `channelMap` static (`Core::ProxyMapType<string, WebSocketConnection>`) SHALL reside as a function-local static inside `WebSocketConnection::Instance()` in `JSONRPCLink.cpp`.

#### Scenario: ResourceMonitor holds a pointer into WebSocketConnection
- **WHEN** `ResourceMonitor` stores an `IResource*` obtained from a channel
- **THEN** the pointer points into `WebSocketConnection`, whose vtable is permanently in the websocket library, not into any plugin DSO

#### Scenario: Plugin unload does not affect ResourceMonitor
- **WHEN** a plugin that registered a `LinkType<INTERFACE>` observer is deactivated and its shared library is unloaded
- **THEN** `WebSocketConnection` (and its `IResource*`) remain valid; no SIGSEGV occurs on subsequent `ResourceMonitor` callbacks

### Requirement: IFramingCallback interface decouples framing from socket management
A narrow interface `IFramingCallback` SHALL be defined in `JSONRPCLink.h` (or the websocket module's public header). It SHALL declare exactly two pure virtual methods: `OnReceived(const Core::ProxyType<Core::JSONRPC::Message>&)` and `OnStateChange(bool open)`. `IFramingCallback` SHALL be marked `EXTERNAL`.

#### Scenario: ChannelImpl implements IFramingCallback
- **WHEN** `WebSocketConnection` receives a raw frame from the socket
- **THEN** it deserialises to a `Core::ProxyType<Core::JSONRPC::Message>` and calls `IFramingCallback::OnReceived()` on the registered framing adapter, without knowledge of the INTERFACE type

### Requirement: ChannelImpl<INTERFACE> is a framing adapter only
`ChannelImpl<INTERFACE>` SHALL implement `IFramingCallback`. It SHALL NOT own a socket and SHALL NOT inherit from `StreamJSONType` or any type that implements `IResource`. Its vtable MAY reside in a plugin DSO; `ResourceMonitor` SHALL never hold a pointer into it.

#### Scenario: Adding a new INTERFACE type
- **WHEN** a new INTERFACE type is instantiated in a plugin
- **THEN** no changes to `WebSocketConnection`, `CommunicationChannel`, or `FactoryImpl` are required; only a new `ChannelImpl<INewProtocol>` framing adapter is needed

### Requirement: IChannelClient interface decouples observer list from INTERFACE
A narrow interface `IChannelClient` SHALL be defined in the header with pure virtual methods: `Opened()`, `Closed()`, `Inbound(const Core::ProxyType<Core::JSONRPC::Message>&)`, and `Timed()`. `IChannelClient` SHALL be marked `EXTERNAL`. `CommunicationChannel`'s `_observers` list SHALL be `std::list<IChannelClient*>`.

#### Scenario: CommunicationChannel is non-template
- **WHEN** `CommunicationChannel` is compiled into `JSONRPCLink.cpp`
- **THEN** it has no INTERFACE template parameter and no INTERFACE-dependent member types

### Requirement: LinkType<INTERFACE> implements IChannelClient
`LinkType<INTERFACE>` SHALL inherit `IChannelClient` and declare `Opened()`, `Closed()`, `Inbound()`, and `Timed()` as `virtual`. No change to the external signatures of these methods is permitted.

#### Scenario: Register/Unregister via IChannelClient
- **WHEN** `CommunicationChannel::Register()` and `Unregister()` are called with a `LinkType<INTERFACE>&`
- **THEN** they accept it as `IChannelClient&` without requiring knowledge of INTERFACE

### Requirement: FactoryImpl is non-template
`FactoryImpl` SHALL be defined in `JSONRPCLink.cpp` without INTERFACE template parameter. Its `WatchDog` type SHALL store `IChannelClient*`. There SHALL be exactly one `Core::TimerType<WatchDog>` instance per process.

#### Scenario: Single timer thread regardless of INTERFACE count
- **WHEN** both `LinkType<Core::JSON::IElement>` and `LinkType<Core::JSON::IMessagePack>` observers are active in the same process
- **THEN** they share a single `FactoryImpl` timer thread (not one per INTERFACE type)

### Requirement: Stage 1 extern template lines removed
After Stage 2 is complete, the `extern template class LinkType<Core::JSON::IElement>` declaration in the header and the corresponding `template class LinkType<Core::JSON::IElement>` in `JSONRPCLink.cpp` SHALL be removed. They are made redundant by `WebSocketConnection` being non-template.

#### Scenario: New INTERFACE type requires zero channel-layer changes
- **WHEN** a new INTERFACE type is used in a plugin without adding any `extern template` or `template class` lines
- **THEN** the channel ownership, `channelMap` static, and `ResourceMonitor` registration all behave correctly because `WebSocketConnection` is INTERFACE-agnostic
