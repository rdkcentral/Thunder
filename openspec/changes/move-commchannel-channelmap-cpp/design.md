## Context

`CommunicationChannel` is a `private` inner class of the `LinkType<INTERFACE>` class template, defined entirely in `JSONRPCLink.h`. Its `Instance()` static method holds `channelMap` as a function-local static:

```cpp
static Core::ProxyType<CommunicationChannel> Instance(...)
{
    static Core::ProxyMapType<string, CommunicationChannel> channelMap;
    ...
}
```

In RDK, platform-specific linker flags (e.g. `--export-dynamic` / symbol-merging behaviour) ensure that only **one** copy of the static exists per process — channel sharing does work. The problem is not duplication; it is **ownership and lifetime**.

The single `channelMap` instance ends up owned by whichever plugin shared library first triggers the template instantiation. Subsequent plugins that call `Instance()` for the same endpoint receive a `Core::ProxyType` wrapping a channel whose vtable pointer (`_vptr.CommunicationChannel`) points into that first library. When the owning plugin is **deactivated and its shared library is unloaded**, the vtable pointer becomes dangling. Any later use of the shared channel — including the `ResourceMonitor` callback that still holds a reference — dereferences the unmapped vtable address and produces a SIGSEGV (see [rdkcentral/Thunder#2040](https://github.com/rdkcentral/Thunder/issues/2040)).

The fix is to move `channelMap` into `JSONRPCLink.cpp` so it is owned by the websocket shared library. The websocket library's lifetime is tied to the Thunder daemon process and is never unloaded while Thunder is running, guaranteeing that all channels in the map — and their vtables — remain valid for the process lifetime.

## Goals / Non-Goals

**Goals:**
- Anchor `channelMap` ownership in the websocket shared library so its lifetime is tied to the Thunder process, not to any individual plugin.
- Prevent use-after-free of shared `CommunicationChannel` objects (and their vtables) when the plugin that originally instantiated the template is deactivated and its library unloaded.
- Preserve the existing `CommunicationChannel::Instance()` public signature unchanged.
- Require no changes to callers or CMakeLists.

**Non-Goals:**
- Changing the channel sharing policy (identity key remains `hostAddress@callsign`).
- Adding thread-safety above what `Core::ProxyMapType` already provides.
- Refactoring unrelated parts of `JSONRPCLink.h`.

## Decisions

### Decision 1 — Explicit template specialization in `.cpp` + `extern template` in header

**Constraint:** `CommunicationChannel` is a private nested dependent type of `LinkType<INTERFACE>`. Its concrete type — and therefore the type of `Core::ProxyMapType<string, CommunicationChannel>` — is distinct for every `INTERFACE`. A single non-template function in `.cpp` cannot name or own that map; the fix must be per-instantiation.

**Chosen:** Explicit full specialization of `CommunicationChannel::Instance()` in `JSONRPCLink.cpp` for each INTERFACE type used in practice, paired with `extern template` declarations in the header.

In `JSONRPCLink.h`:
```cpp
extern template
Core::ProxyType<LinkType<Core::JSON::IElement>::CommunicationChannel>
LinkType<Core::JSON::IElement>::CommunicationChannel::Instance(
    const Core::NodeId&, const string&, const string&);
```

In `JSONRPCLink.cpp`:
```cpp
template <>
EXTERNAL Core::ProxyType<LinkType<Core::JSON::IElement>::CommunicationChannel>
LinkType<Core::JSON::IElement>::CommunicationChannel::Instance(
    const Core::NodeId& remoteNode, const string& callsign, const string& query)
{
    static Core::ProxyMapType<string,
        LinkType<Core::JSON::IElement>::CommunicationChannel> channelMap;
    string searchLine = remoteNode.HostAddress() + '@' + callsign;
    return (channelMap.template Instance<
        LinkType<Core::JSON::IElement>::CommunicationChannel>(
            searchLine, remoteNode, callsign, query));
}
```

The `static channelMap` inside the explicit specialization is now a local static of a function defined in a `.cpp` TU compiled once into the websocket library. The `extern template` in the header suppresses implicit instantiation in every consumer TU — all callers link to the websocket library's symbol.

**INTERFACE types to specialize:** Source search confirms only `Core::JSON::IElement` is instantiated in the current codebase. `Core::JSON::IMessagePack` exists as a protocol alternative but has no callers at this time; a matching specialization MAY be added alongside `IElement` for completeness.

**Custom/third-party INTERFACE types** not covered by explicit specializations fall back to the generic header template. Since those types are always instantiated within a single library (no cross-library channel sharing), the original crash scenario cannot apply to them.

**Alternative considered: non-template free function with type-erased registry**  
A single free function in `.cpp` using a `std::unordered_map<std::type_index, void*>` registry can serve all INTERFACE types, but introduces type-erasure complexity, requires a factory lambda, and has RTTI/visibility risks across shared libraries. Not warranted given the finite set of INTERFACE types in use.

**Alternative considered: moving `CommunicationChannel` out of the template**  
Extracting `CommunicationChannel` to a non-template class at namespace scope would remove the dependency entirely but requires significant refactoring of `ChannelImpl`, `FactoryImpl`, and all observer lists. Out of scope for this fix.

### Decision 2 — Keep `Instance()` signature in the header unchanged

The generic template definition of `Instance()` in the header remains as-is (for unspecialized INTERFACE types). The `extern template` declarations are additive. No change to the method's parameter list, return type, or the call sites inside `LinkType`.

### Decision 3 — Map key scheme unchanged

The `ProxyMapType` key remains `remoteNode.HostAddress() + '@' + callsign`. No change to the channel identity or sharing policy.

## Risks / Trade-offs

- **`EXTERNAL` visibility on explicit specializations** — Without `EXTERNAL`, the explicit specialization symbol may have default hidden visibility in some build configurations, causing each consumer library to produce its own copy via an inline fallback, recreating the original problem.  
  → *Mitigation*: Annotate each explicit specialization in `JSONRPCLink.cpp` with `EXTERNAL`. Verify with `nm -C libThunderWebSocket.so` that the symbol has global (`T`) binding and that no other loaded library exports the same mangled name.

- **`extern template` must appear in the header before any implicit instantiation** — If a consumer TU includes the header and instantiates `LinkType<IElement>` before the `extern template` declaration is parsed, the compiler may silently generate an inline version of `Instance()` with its own `channelMap`.  
  → *Mitigation*: Place the `extern template` declarations unconditionally at the bottom of the `LinkType` class body (or immediately after the class) so they are always visible. This is standard practice for `extern template` on class templates.

- **Access to private nested type in explicit specialization** — Explicit specializations of a member function of a class template specialization are defined at namespace scope but the return type (`Core::ProxyType<LinkType<IElement>::CommunicationChannel>`) references a `private` nested class. Most compilers allow this for explicit specializations but it is technically access-controlled.  
  → *Mitigation*: If a compiler rejects access, add a `friend` declaration for the specialization inside `CommunicationChannel`, or make `CommunicationChannel` `protected` instead of `private`.

- **New INTERFACE types** — Future INTERFACE types added to the framework will use the unspecialized header template and be subject to the original per-library-static behaviour.  
  → *Mitigation*: Document in the header comment above the `extern template` block that any new INTERFACE type intended for cross-library use MUST receive a corresponding explicit specialization and `extern template` pair.

- **ODR for `FactoryImpl`** — `FactoryImpl` is a function-local-static singleton accessed via `Core::SingletonType<FactoryImpl>::Instance()`, which provides its own ODR-safe guarantee through the Singleton machinery. No change needed.

## Migration Plan

1. Add `extern template` declarations for `CommunicationChannel::Instance()` (one per INTERFACE type) at the end of `JSONRPCLink.h` to suppress implicit instantiation in consumer TUs.
2. Add explicit full specializations of `CommunicationChannel::Instance()` in `JSONRPCLink.cpp`, each owning its own `static channelMap`; annotate with `EXTERNAL`.
3. Rebuild the websocket library and any consumer. Confirm with `nm -C` that only the websocket library exports the `Instance()` symbol and no other loaded library duplicates it.
4. Run existing unit/integration tests for JSON-RPC channel establishment.
5. Deactivate a plugin that uses `LinkType<IElement>` while another plugin shares the same channel; verify no SIGSEGV.
6. No rollback complexity — the change is confined to two files with no API surface change.
