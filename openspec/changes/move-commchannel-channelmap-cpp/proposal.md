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

Because `CommunicationChannel` is a nested class of the `LinkType<INTERFACE>` template, the `channelMap` static is instantiated in whichever plugin shared library first triggers the template instantiation. In RDK, platform linker flags ensure only one copy of this static exists per process, so channel sharing does work — subsequent plugins calling `Instance()` for the same endpoint correctly receive a proxy to the existing channel. However, that channel's vtable pointer (`_vptr.CommunicationChannel`) points into the library that originally instantiated it.

When that originating plugin is **deactivated and its shared library is unloaded**, the vtable pointer becomes a dangling pointer. Any later operation on the shared channel — including the `ResourceMonitor` callback that still holds a reference — dereferences the unmapped vtable address and produces a **SIGSEGV**. The gdb evidence from the issue confirms exactly this pattern: `channelMap` is in library Y, the returned channel's vtable is in library Z, and when Z unloads the crash follows.

The fix is to move `channelMap` into `JSONRPCLink.cpp` so it is owned by the websocket shared library. Because the websocket library's lifetime is tied to the Thunder daemon process, the map — and the channels it keeps alive — can never be unloaded while Thunder is running.

## What Changes

`CommunicationChannel` is a nested dependent type of `LinkType<INTERFACE>`, so its type — and the type of `channelMap` — varies per `INTERFACE`. A single non-template function in `.cpp` cannot directly own the map. The correct mechanism is **explicit template specialization** of `CommunicationChannel::Instance()` combined with `extern template`:

- In `JSONRPCLink.h`, add `extern template` declarations for the concrete INTERFACE types used in practice (currently only `Core::JSON::IElement`). This suppresses implicit instantiation in every consumer TU and forces all callers to link to the websocket library's definition.
- In `JSONRPCLink.cpp`, add explicit full specializations of `CommunicationChannel::Instance()` for those same types. The `static channelMap` local variable inside these specializations is compiled once into the websocket library TU — never into plugin libraries.
- Annotate each explicit specialization with `EXTERNAL` so the symbol is exported from the websocket library.
- No public API or ABI changes to `LinkType<INTERFACE>` or `CommunicationChannel`; the change is purely an implementation fix.
- Custom/third-party INTERFACE types not listed in the `extern template` set fall back to the old header template behaviour, but those are always instantiated in a single library (no cross-library channel sharing), so the original crash cannot affect them.

## Capabilities

### New Capabilities
- `channelmap-cpp-anchor`: `channelMap` is anchored in the websocket library translation unit, ensuring that shared channels outlive any individual plugin library and their vtable pointers remain valid for the lifetime of the process.

### Modified Capabilities

## Impact

- **Source/websocket/JSONRPCLink.h** — `CommunicationChannel::Instance()` body refactored; `channelMap` static removed.
- **Source/websocket/JSONRPCLink.cpp** — non-template accessor added with the `channelMap` static, annotated `EXTERNAL`.
- **Bug fixed**: SIGSEGV on plugin deactivation caused by dangling `_vptr.CommunicationChannel` after the owning shared library is unloaded (rdkcentral/Thunder#2040).
- **ResourceMonitor safety**: channels held by the `ResourceMonitor` remain valid because they are now kept alive by the websocket library static, not by a plugin library.
- Build: no CMakeLists changes required; `JSONRPCLink.cpp` is already compiled into the websocket library.
- Consumers of `LinkType<INTERFACE>` outside the websocket library: no source or binary changes required.
