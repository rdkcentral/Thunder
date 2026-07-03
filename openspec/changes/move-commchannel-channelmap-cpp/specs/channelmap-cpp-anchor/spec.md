## ADDED Requirements

### Requirement: Single channelMap instance per process
The websocket library SHALL own exactly one `Core::ProxyMapType` instance (the channel map) per process, regardless of how many shared libraries include `JSONRPCLink.h` or instantiate `LinkType<INTERFACE>`.

#### Scenario: Same endpoint accessed from two shared libraries
- **WHEN** two shared libraries in the same process each call `CommunicationChannel::Instance()` with identical `remoteNode` and `callsign` arguments
- **THEN** both calls return a `Core::ProxyType` wrapping the same underlying `CommunicationChannel` object (ref-count incremented, not a new allocation)

#### Scenario: Different endpoints remain independent
- **WHEN** `CommunicationChannel::Instance()` is called with distinct `remoteNode` or `callsign` values
- **THEN** each distinct key resolves to its own `CommunicationChannel` object in the shared map

### Requirement: All INTERFACE-dependent vtables anchored in websocket library TU
The vtables of `CommunicationChannel`, `ChannelImpl<INTERFACE>`, and `HandlerType` (nested inside `StreamJSONType`) for each registered INTERFACE type SHALL be emitted into a translation unit compiled into the websocket shared library, not into plugin DSOs.

#### Scenario: ResourceMonitor vtable stability
- **WHEN** a plugin that opened a `CommunicationChannel` is deactivated and its shared library is unloaded
- **THEN** the `ResourceMonitor` callback via `IResource*` does not dereference a dangling vtable (no SIGSEGV)

### Requirement: Whole-class explicit instantiation in websocket library TU
`LinkType<Core::JSON::IElement>` SHALL be explicitly instantiated (via `template class LinkType<Core::JSON::IElement>` in `JSONRPCLink.cpp`), and its implicit instantiation in consumer TUs SHALL be suppressed via `extern template class LinkType<Core::JSON::IElement>` in the header. Both lines are required to anchor all symbols — including the `channelMap` static and all vtables — in the websocket library. `template<>` (explicit specialisation) SHALL NOT be used; this is a DSO placement mechanism, not a behavioural override.

#### Scenario: Link-time symbol uniqueness
- **WHEN** the websocket library and any number of consumer shared libraries are loaded into the same process
- **THEN** exactly one definition of each `LinkType<Core::JSON::IElement>` member symbol (vtables, statics) is present in the combined process image (verifiable with `nm -C` on the websocket library)

### Requirement: Instance() signature unchanged
The public static method `CommunicationChannel::Instance(const Core::NodeId&, const string&, const string&)` SHALL retain its existing parameter list and return type so that no callers require modification.

#### Scenario: Existing callers compile without change
- **WHEN** code that previously called `CommunicationChannel::Instance(remoteNode, callsign, query)` is recompiled against the updated header
- **THEN** it compiles and links without errors or warnings related to this change

### Requirement: Registration comment for new INTERFACE types
The `extern template class` declaration in `JSONRPCLink.h` SHALL be accompanied by a comment stating that any new INTERFACE type requiring cross-library use must receive a matching `extern template class` in the header and `template class` in `JSONRPCLink.cpp`.

#### Scenario: Developer adds a new INTERFACE type
- **WHEN** a developer adds `extern template class LinkType<Core::JSON::IMessagePack>` + `template class LinkType<Core::JSON::IMessagePack>` in the designated locations
- **THEN** the new INTERFACE type's vtables and statics are anchored in the websocket library with no other code changes required
