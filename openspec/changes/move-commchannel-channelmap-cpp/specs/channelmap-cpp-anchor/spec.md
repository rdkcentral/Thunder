## ADDED Requirements

### Requirement: Single channelMap instance per process
The websocket library SHALL own exactly one `Core::ProxyMapType` instance (the channel map) per process, regardless of how many shared libraries include `JSONRPCLink.h` or instantiate `LinkType<INTERFACE>`.

#### Scenario: Same endpoint accessed from two shared libraries
- **WHEN** two shared libraries in the same process each call `CommunicationChannel::Instance()` with identical `remoteNode` and `callsign` arguments
- **THEN** both calls return a `Core::ProxyType` wrapping the same underlying `CommunicationChannel` object (ref-count incremented, not a new allocation)

#### Scenario: Different endpoints remain independent
- **WHEN** `CommunicationChannel::Instance()` is called with distinct `remoteNode` or `callsign` values
- **THEN** each distinct key resolves to its own `CommunicationChannel` object in the shared map

### Requirement: channelMap static anchored in websocket library TU
The `Core::ProxyMapType<string, CommunicationChannel>` static variable SHALL be defined in a translation unit compiled into the websocket shared library (`JSONRPCLink.cpp` or equivalent), not inside a template method body in a header file.

#### Scenario: Link-time symbol uniqueness
- **WHEN** the websocket library and any number of consumer shared libraries are loaded into the same process
- **THEN** exactly one definition of the channel-map static symbol is present in the combined process image (verifiable with `nm -C` on the websocket library)

### Requirement: Instance() signature unchanged
The public static method `CommunicationChannel::Instance(const Core::NodeId&, const string&, const string&)` SHALL retain its existing parameter list and return type so that no callers require modification.

#### Scenario: Existing callers compile without change
- **WHEN** code that previously called `CommunicationChannel::Instance(remoteNode, callsign, query)` is recompiled against the updated header
- **THEN** it compiles and links without errors or warnings related to this change

### Requirement: channelMap accessor symbol exported from websocket library
The non-template accessor that owns the channel-map static SHALL be decorated with `EXTERNAL` (or equivalent visibility attribute) so that all shared libraries resolve to the single symbol in the websocket library.

#### Scenario: Out-of-process consumer resolves to websocket symbol
- **WHEN** a plugin built as a separate shared library calls into `CommunicationChannel::Instance()`
- **THEN** the dynamic linker resolves the channel-map accessor to the symbol exported by the websocket library, not a hidden local copy
