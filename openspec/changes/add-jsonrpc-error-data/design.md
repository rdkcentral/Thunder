## Revision note (CCB review)

This design was revised in response to CCB review feedback on the original proposal. The prior version relied on an **implicit** mechanism: methods that already declared an `@out` response parameter got `error.data` "for free" by having JsonGenerator/`Register<>` stop discarding that same response object on the error path, while only methods with *no* `@out` parameter needed an explicit opt-in (`ErrorDetail<DATA>`).

CCB review correctly rejected that split as fundamentally wrong: whether a method happens to already have an `@out` parameter is an accident of its success-path shape, not a signal that its maintainer wants that shape reused for error diagnostics. **This revision replaces the implicit mechanism entirely.** There is now exactly one way for any method — regardless of whether it has other `@out` parameters — to surface a structured `error.data` payload or override `error.code`/`error.message`: it must explicitly declare a dedicated error-detail parameter. Existing `@out`/response parameters are never echoed into `error.data`, on any method, whether or not this proposal is adopted. This also resolves several smaller review comments that only made sense in the old, implicit design (see the “CCB comments addressed” table below).

| # | CCB comment | Resolution |
|---|---|---|
| 1 | "Free" reuse of existing `@out` params for error data is fundamentally wrong; rich error reporting must always be an explicit `ErrorDetail` parameter | Implicit mechanism removed. Explicit parameter is now the *only* mechanism, for every method (Decision 1, Decision 2). |
| 2 | The wrapper type shouldn't live in the `JSONRPC` namespace | Moved to `Core::ErrorDetail<DATA>` (Decision 1). |
| 3 | `@error` tag isn't needed | Dropped. Recognition is purely structural, the same way `Core::JSONRPC::Context` is recognized today (Decision 1). |
| 4 | `Data` must also be optional | `Data` is now `Core::OptionalType<DATA>`, so a method can set only `Message` (e.g. to override text) without providing a payload (Decision 1). |
| 5 | `JSONRPCErrorAssessor` was meant to be deprecated, not upgraded | The old "give it the same capability" decision is dropped. This proposal makes no change to it and flags it for deprecation instead (Decision 5). |
| 6 | Interface examples showed JsonGenerator-*generated* classes as if hand-authored | Worked examples below explicitly separate "interface header, hand-authored" from "generated glue", and no longer show a generated response class inline with the interface (Examples section). |
| 7 | Not sure `Message::Info.Data` should change from `string` to `Variant` | Downgraded from a firm Decision to an open implementation question; the proposal only commits to the wire *behavior* required (Decision 3, Open Questions). |
| 8 | `@errordata:omit` has no sense once (1) is corrected | Removed — there is no more automatic echo to opt out of (Open Questions). |

## Context

Thunder's JSON-RPC error object model lives in `Core::JSONRPC::Message::Info` ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L64-L230)). It defines `code`, `message` (`Text`), and `data` (`Data`), but `Data` is declared as a plain `Core::JSON::String` and is never assigned anywhere in the codebase today — it exists in the wire schema but carries no information.

Today, a handler's `uint32_t`/`Core::hresult` result is turned into `code`/`message` via `Info::SetError()`; whatever a method wrote to its own `@out`/response parameters before returning an error is **discarded** — JsonGenerator's generated glue (`ThunderTools/JsonGenerator/source/rpc_emitter.py`, "Emit result handling and serialization to JSON data", ~line 1454) only copies those parameters into the JSON response inside `if (errorCode_ == Core::ERROR_NONE) { ... }`, with no `else`; the hand-written `Register<INBOUND,OUTBOUND,METHOD>` path ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L1681-L1719)) does the same (`result.clear()` on the else branch). **This revision keeps that discard behavior exactly as-is, for every method, with or without this proposal.** It is not a bug to be fixed by reusing the success-shape object — see Decision 2.

### COM-RPC / out-of-process plugins: verified, no marshaling change needed

RDK/Thunder plugins are frequently out-of-process (OOP). When a JsonGenerator-emitted lambda calls through a COM-RPC proxy (`Core::IUnknown`-derived interface obtained via `_service->Root<>()`), the call is marshaled across the process boundary by code produced by the sibling `ProxyStubGenerator` tool (`ThunderTools/ProxyStubGenerator/StubGenerator.py`). This matters here because the new `Core::ErrorDetail<DATA>` parameter (Decision 1) is itself an ordinary reference `@out` parameter, and its OOP behavior depends on the same marshaling guarantee already verified for this proposal:

- **Stub (server) side** (`EmitStubMethodImplementation`, ~line 2021-2049): after invoking the real implementation, the stub writes **every** output parameter back to the response writer unconditionally — `for p in output_params: if p: WriteParameter(p)` — with no `hresult`/`ERROR_NONE` guard.
- **Proxy (client) side** (`EmitProxyMethodImplementation`, ~line 2440-2480): after the transport-level `Invoke()` succeeds, the proxy reads back the application-level `hresult` first, then reads **every remaining output parameter unconditionally** — `for p in output_params[_first_param:]: ReadParameter(p)` — again with no gate on the application-level `hresult` value.

**Conclusion: COM-RPC already faithfully transports "out" parameters across the process boundary regardless of the application-level error code**, whether that parameter is an ordinary response struct or the new `Core::ErrorDetail<DATA>` parameter. No change is needed in `ProxyStubGenerator`. This means an out-of-process plugin implementation can populate `Core::ErrorDetail<DATA>` before returning a failing `Core::hresult`, exactly like any other `@out` parameter, and the JsonGenerator-emitted lambda running in the plugin shell's process (Thunder's process context) will see it populated correctly.

### `CustomErrorCode`: background and why it's being removed, not extended

Separately, `CustomErrorCode` ([Source/core/ICustomErrorCode.h](../../../Source/core/ICustomErrorCode.h), [Source/core/Errors.h](../../../Source/core/Errors.h#L33-L46), [Source/core/Errors.cpp](../../../Source/core/Errors.cpp), [Source/Thunder/PluginHost.cpp](../../../Source/Thunder/PluginHost.cpp#L152-L206)) lets a process register **one global** `int32_t -> const TCHAR*` handler (dynamically loaded from a separately configured shared library, via `PluginHost.cpp`'s `CustomCodeLibrary`), invoked from `Info::SetError()` to fill in `Text` for custom (non-framework) codes. It is code-level (one string per code, process-wide, "can only set one, not multithreaded safe" per its own header comment), not call-level, and cannot express structured `data`. This mechanism is not known to be wired up anywhere in RDK today (see the `ContentProtection` precedent below), and this proposal removes it entirely — see Decision 4.

### Real-world precedent: teams already needed this and built their own version

Before removing `CustomErrorCode`, it's worth confirming *why* it went unused rather than assuming indifference. Searching `entservices`/`entservices-cpc` for any of `CustomCode`, `IsCustomCode`, `SetCustomCodeToStringHandler` turns up exactly one real hit, and it is telling: [entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h](../../../../entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h#L271-L295) defines its own free function `CustomCodeToString(int32_t code)` — annotated `// Replace with Thunder Custom (Error) Codes`, i.e. clearly *aware* of the official mechanism — but instead of wiring it up via `Core::SetCustomCodeToStringHandler`, the plugin exposes it through a bespoke, plugin-local, `@json`-annotated COM-RPC interface it invented itself: `Exchange::IErrorToString` ([entservices-cpc-apis/apis/ContentProtection/IContentProtection.h](../../../../entservices-cpc-apis/apis/ContentProtection/IContentProtection.h#L184-L192), `virtual Core::hresult ErrorToString(const int32_t code, string& result) const`). A separate process, [entservices-cpc/app-gateway-cpc/FbEntos/delegate/CPSDelegate.h](../../../../entservices-cpc/app-gateway-cpc/FbEntos/delegate/CPSDelegate.h#L116-L135), then queries this interface cross-process via `QueryInterfaceByCallsign<Exchange::IErrorToString>(...)` whenever it needs to translate one of ContentProtection's ~60 DRM/watermarking error codes into text. This confirms real demand for "attach detail to a numeric failure code" — but it also confirms the *official* global-singleton-function-pointer mechanism was inadequate enough that a real team quietly reinvented it rather than adopt it. Two conclusions follow: (1) removing the official mechanism is very unlikely to break anything in this workspace, and (2) the real, demonstrated need is exactly the call-specific, per-method shape `Core::ErrorDetail<DATA>` provides (Decision 1), not a global handler.

## Goals / Non-Goals

**Goals:**
- Provide **one** explicit, opt-in mechanism — `Core::ErrorDetail<DATA>` (Decision 1) — for an interface owner to let a method surface a structured `data` payload and/or override `error.code`/`error.message` for that call, applicable uniformly to **any** method, whether or not it already declares other `@out` parameters.
- Make that mechanism cost nothing for methods that don't adopt it: no wire, schema, or behavior change to any method that doesn't add the parameter.
- **Consolidate to a single, call-specific source of truth for custom error text/detail**: remove `CustomErrorCode`'s global-handler mechanism entirely (Decision 4) rather than document a boundary between two parallel systems, replacing its two legitimate capabilities (an app-chosen value appearing verbatim as `error.code`, and a replacement `message`) with `Core::ErrorDetail<DATA>`'s `Code`/`Message` fields.
- Define the minimum wire *behavior* required of `error.data` (a real JSON value, never a JSON-encoded string) without prematurely committing to a specific `Message::Info` C++ representation — see Decision 3.

**Non-Goals:**
- Changing the synchronous `uint32_t`/`Core::hresult` return-code contract of `Handler::Register` methods — `error.code` remains derived exclusively from that return value via `SetError()`'s existing mapping for every method that has not explicitly opted into `Core::ErrorDetail<DATA>::Code` for that one call.
- **Reusing or echoing an existing `@out`/response parameter as `error.data`, for any method, under any circumstance** — this was the mechanism in the pre-review version of this proposal and CCB review correctly rejected it (comment 1). A method's success-result shape is never treated as an implicit source of error detail; adopting `Core::ErrorDetail<DATA>` is the only path, and it is always an explicit, separate parameter.
- Changing the raw/hand-written `InvokeFunction` path's existing behavior ([Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L1108-L1114): non-empty `output` on error is dumped verbatim into `Error.Text`). That ad hoc, string-only convention is unrelated to `Core::ErrorDetail<DATA>` (bare `InvokeFunction` lambdas have no typed parameter list to add it to) and is left exactly as-is.
- Extending `JSONRPCErrorAssessor` ([Source/common/JSONRPC.h](../../../Source/common/JSONRPC.h#L1918-L1971)) with any new capability. Per CCB review (comment 5), this hook is intended for deprecation, not enhancement — see Decision 5.
- Building a general "translate a bare error code to text, independent of any specific call" lookup service (the `IErrorToString`/`ContentProtection` pattern above) — already achievable today per-plugin with zero framework changes, and out of scope here.
- Adding new `@`-annotation syntax to COM-RPC interface headers to declare `Core::ErrorDetail<DATA>` — recognition is purely structural (Decision 1), mirroring `Core::JSONRPC::Context`.
- Changing how COM-RPC (`Core::hresult`, `COM_ERROR` bit) errors carry detail, beyond what `SetError()` already does and what `Core::ErrorDetail<DATA>` adds — `ProxyStubGenerator` already marshals "out" parameters (including this new one) unconditionally, regardless of the application-level `hresult` (Context above), so COM-RPC needs no change.
- Changing the wire format or behavior of successful (`Core::ERROR_NONE`) responses, or of any method that does not adopt `Core::ErrorDetail<DATA>`.
- Changing `ProxyStubGenerator` (`ThunderTools/ProxyStubGenerator/`) — confirmed out of scope.
- **Automatically or silently** adding a parameter to an existing interface method, or retrofitting one without the interface owner's explicit action. Adopting `Core::ErrorDetail<DATA>` is a per-method decision made (and paid for, in interface-version/ABI terms) by that interface's own maintainer.

## Decisions

### 1. `Core::ErrorDetail<DATA>`: the single, explicit mechanism for rich errors

**Mechanism**: a reusable wrapper, in the plain `Core::` namespace (not `Core::JSONRPC`, per CCB comment 2), structurally recognized by `JsonGenerator` the same way it already special-cases `Core::JSONRPC::Context` ([header_loader.py](../../../../ThunderTools/JsonGenerator/source/header_loader.py) — `if "Core::JSONRPC::Context" in cppType.full_name: result = ["@context", {}]`) — no `@error`-style annotation tag is needed or used (per CCB comment 3):

```cpp
// New, e.g. Source/core/Errors.h (name/location illustrative)
namespace Core {
    template<typename DATA>
    struct ErrorDetail {
        Core::OptionalType<int32_t> Code;     // per-call Error.Code override — takes precedence over SetError()'s default mapping when set
        Core::OptionalType<string> Message;   // per-call Error.Text override; replaces CustomErrorCode's one legitimate use (Decision 4)
        Core::OptionalType<DATA> Data;         // per-call structured payload; optional on its own (per CCB comment 4) so a method can set only Code/Message without a payload
    };
}
```

All three members are independently optional: a method can set only `Message` (e.g. to give a call-specific replacement for a generic framework message, leaving `code` and `data` untouched), only `Code` (e.g. to make its own numeric scheme appear verbatim), only `Data`, or any combination.

An interface owner who wants this for a given method appends an out-parameter of this shape, **regardless of whether the method already declares other `@out` parameters**:

```cpp
// Illustrative opt-in — NOT part of this proposal's default behavior;
// an interface owner would make this change explicitly, in their own interface header.
virtual Core::hresult SetLastCheckoutResetTime(
    const uint64_t resetTime,
    Core::OptionalType<Core::ErrorDetail<SetLastCheckoutResetTimeErrorData>>& error /* @out */) = 0;
```

`header_loader.py` recognizes the `Core::ErrorDetail<...>` type structurally, purely from the parameter's C++ type name, and: (a) **excludes it from `BuildResult`'s success-result schema entirely** — it never appears in a successful response, so adopting this is invisible to existing successful-call consumers; (b) records it separately so `rpc_emitter.py` can emit an extra local `Core::OptionalType<Core::ErrorDetail<DATA>>` temporary, pass it by reference into the real interface call alongside the normal parameters, and — on the error path only — if it `IsSet()`: assign `Code` into `Error.Code` when set (overriding `SetError()`'s default-derived value for that call only), assign `Message` into `Error.Text` when set, and assign `Data` into `Error.Data` when set. Because this is an ordinary reference out-parameter, `ProxyStubGenerator` marshals it with the same already-verified unconditional read/write behavior as any other `@out` parameter (Context above) — no COM-RPC change needed.

**Decision (Thunder team, unaffected by this review): `Code` is included.** This reinstates `CustomErrorCode`'s one real capability (an app-chosen value appearing verbatim on the wire) in a narrower, opt-in, per-method form. See "Prior art" below for the LSP/Ethereum precedent this diverges from, and why the team chose to diverge anyway.

*Alternative considered*: making `DATA` a true, generator-erased template parameter on the interface method itself. Rejected — COM-RPC virtual interfaces require a fixed, concrete ABI per method; there is no type erasure boundary that would let a genuinely generic/unbounded `DATA` cross a COM-RPC call. `Core::ErrorDetail<DATA>` is a reusable *wrapper pattern*, but each adopting method still fixes a concrete `DATA` at the interface-header level.

**This is not free**: appending a parameter to an existing `virtual` interface method changes its C++ ABI (vtable slot/signature) — every consumer of that interface (the plugin shell's generated glue, the OOP implementation, any direct COM-RPC client) must be rebuilt together, and the interface's `@json` version should be bumped. This is a per-method, per-interface-owner decision with a real, uniform cost for *every* adopting method — including ones that already have other `@out` parameters (see Decision 2) — not a JsonGenerator-regeneration-only change.

### 2. Existing `@out`/response parameters are never a source of `error.data`

This is the corrected core principle CCB review established (comment 1), stated as its own decision because it reverses the pre-review design outright: **a method's existing `@out`/response parameter(s) are never inspected, serialized, or echoed into `error.data`, regardless of whether the method populated them before returning an error, and regardless of whether this proposal is adopted.** This applies uniformly, whether the method has zero, one, or many other `@out` parameters. The only way any method gets `error.data` (or a `code`/`message` override) is the explicit `Core::ErrorDetail<DATA>` parameter (Decision 1) — added deliberately, as its own parameter, by that method's interface owner.

The reasoning: whether a method happens to already have an `@out` parameter is an artifact of its *success*-result shape, chosen for what a caller needs on success. Treating that same object as a de facto error-detail schema conflates two independent design decisions (what does a caller need back on success vs. what does a caller need back on failure) that don't generally coincide, and does so implicitly — a plugin maintainer populating their response struct on an error path (perhaps for their own internal reasons, e.g. partial results) would silently and invisibly start leaking that struct's contents into `error.data` the moment JsonGenerator regenerated their glue, with no explicit signal in their interface header that this was intended. Requiring `Core::ErrorDetail<DATA>` as a separate, explicit parameter makes the intent to expose error detail visible directly in the interface header, at the cost of requiring an ABI bump even for methods that already have an unrelated `@out` parameter.

### 3. `error.data` wire representation: required behavior, C++ representation left open

The wire requirement is fixed: when a method's `Core::ErrorDetail<DATA>::Data` is set, `error.data` must contain the actual JSON serialization of `DATA` (an object, array, or scalar, as `DATA` dictates) — never a JSON string containing an escaped/quoted serialization of it. When `Data` is unset, `error.data` must be omitted entirely (no regression for the overwhelming majority of error responses that don't opt in).

CCB review (comment 7) questioned whether this requires changing `Message::Info::Data` from `Core::JSON::String` to `Core::JSON::Variant`, as the pre-review design proposed. That change is no longer asserted as a settled decision. Two options remain open for implementation:
- **(a) `Core::JSON::Variant`**: already exists, already participates in `Core::JSON::Container`'s `IsSet()`-driven (de)serialization, and is already used elsewhere in the framework as an "any JSON value" field (`Core::JSON::VariantContainer`). Simple, but introduces a fully generic runtime container for a value whose shape is actually known and fixed at code-generation time per adopting method.
- **(b) Generator-level serialization**: instead of widening `Message::Info::Data`'s C++ type, have the JsonGenerator-emitted glue serialize the concrete, per-method `DATA` value directly into the outgoing response text for the `data` key, without `Message::Info` needing a strongly-typed member for it at all. This avoids introducing a generic runtime type into `Info` but requires `Info`/its `ToString()` composition to support a "raw, pre-serialized JSON fragment" concept it does not have today.

This proposal commits only to the required wire *behavior* above; the choice between (a), (b), or another representation achieving the same behavior is deferred to implementation (see Open Questions).

### 4. `CustomErrorCode` is removed; consolidated into Decision 1's mechanism

Rather than leave two parallel, disconnected "custom error detail" systems in place, `CustomErrorCode` is deleted outright, giving the codebase a **single source of truth** for call-specific error detail:
- **Removed**: `Core::CustomCode()`/`Core::IsCustomCode()` and the `CustomCodeToStringHandler`/`SetCustomCodeToStringHandler` pair from [Source/core/Errors.h](../../../Source/core/Errors.h#L33-L46)/[Errors.cpp](../../../Source/core/Errors.cpp); [Source/core/ICustomErrorCode.h](../../../Source/core/ICustomErrorCode.h) deleted entirely; the `CustomCodeLibrary` dynamic-library-loading class and its config wiring removed from [Source/Thunder/PluginHost.cpp](../../../Source/Thunder/PluginHost.cpp#L152-L206); [docs/utils/customcodes.md](../../../docs/utils/customcodes.md) replaced with documentation for Decision 1's mechanism.
- **`SetError()` simplified**: the `IsCustomCode()` branch in `Info::SetError()` ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L100-L215)) is deleted. Every hresult not matching one of the fixed framework codes now falls through unconditionally to the existing deterministic default: `Code = ApplicationErrorCodeBase - static_cast<int32_t>(frameworkError)`. The one capability actually lost by removing the *global* mechanism — an app's ability to make an arbitrary chosen integer appear **verbatim, unmodified** as `error.code` — is not dropped, it is reinstated in a narrower, opt-in, per-call form via `Core::ErrorDetail<DATA>::Code`.
- **`ErrorToString()`/`ErrorToStringExtended()` lose their custom-code branch too** ([Source/core/Errors.cpp](../../../Source/core/Errors.cpp#L104-L129)): both currently call `IsCustomCode()` and, if set, delegate to the registered handler. Once `IsCustomCode()` is deleted, these keep only their fallback path — lookup against the fixed `ERROR_CODES` table, and, for `ErrorToStringExtended()`, `"Undefined Thunder error code: <N>"` when no fixed code matches.
- **This is an intentional breaking change to public, `EXTERNAL`-exported Core symbols and a documented external-library contract**, whose out-of-tree usage cannot be fully ruled out by searching this workspace. Mitigation: a clear breaking-change release note naming the removed symbols, rather than a silent removal — see tasks.md.

#### Worked example: what a client actually sees for an app-specific error code, before and after this proposal

```cpp
// Illustrative plugin method, out-of-scope interface — shows the mechanism, not a real Thunder interface change.
Core::hresult SomePlugin::DoSomething()
{
    if (_drmSessionInvalid) {
        return static_cast<Core::hresult>(21001);   // an app-specific, non-framework error value
    }
    return Core::ERROR_NONE;
}
```

**After this proposal, with no other change**, `21001` falls through `SetError()`'s `default:` branch unconditionally:

```json
{ "jsonrpc": "2.0", "id": 7, "error": { "code": -52001, "message": "Undefined Thunder error code: 21001" } }
```

A plugin author who wants a *meaningful* message and the original numeric code back must opt into Decision 1 explicitly:

```cpp
// Illustrative opt-in (Decision 1) — NOT applied to any existing interface by this proposal.
Core::hresult SomePlugin::DoSomething(Core::OptionalType<Core::ErrorDetail<SomeErrorData>>& error /* @out */)
{
    if (_drmSessionInvalid) {
        error = Core::ErrorDetail<SomeErrorData>{ { 21001 }, string(_T("DRM session invalid")), {} };
        return static_cast<Core::hresult>(21001);
    }
    return Core::ERROR_NONE;
}
```

```json
{ "jsonrpc": "2.0", "id": 7, "error": { "code": 21001, "message": "DRM session invalid" } }
```

`code` is now the app's literal `21001` (via `Code`), `message` is call-specific and meaningful (via `Message`), and `data` is simply omitted here since `Data` was left unset (per CCB comment 4, this is a valid, well-formed use of `Core::ErrorDetail<DATA>`).

### 5. `JSONRPCErrorAssessor`: unaffected by this proposal, flagged for deprecation

The pre-review design (formerly "Decision 6") proposed giving the assessor callback ([Source/common/JSONRPC.h](../../../Source/common/JSONRPC.h#L1918-L1971)) the same data-attaching capability as regular handlers. CCB review (comment 5) states this hook was intended for deprecation, not extension. This proposal makes **no change** to `JSONRPCErrorAssessor`'s capability or signature. As a small, low-risk documentation courtesy within this same change, its declaration is annotated to note it is deprecated and should not be extended further (see tasks.md); an actual removal/migration-away plan is out of scope here and should be tracked as a separate, follow-up change.

## Examples

The examples below keep the plugin's hand-authored interface header and JsonGenerator's generated glue clearly separate (per CCB comment 6) — no generated class is presented as if it were part of the interface.

### `IAccount`: adopting `Core::ErrorDetail<DATA>` on two different methods

`IAccount` ([entservices-apis/apis/Account/IAccount.h](../../../../entservices-apis/apis/Account/IAccount.h)) has two illustrative cases: `GetLastCheckoutResetTime` already declares an `@out` result struct for its success shape; `SetLastCheckoutResetTime` declares none. Under the corrected design (Decision 2), **both need the same explicit change** if their maintainer wants rich errors — having an existing `@out` parameter grants `GetLastCheckoutResetTime` no shortcut:

```cpp
// entservices-apis/apis/Account/IAccount.h — interface header, hand-authored.
// Illustrative opt-in for BOTH methods — NOT applied to the interface by this proposal;
// IAccount's own maintainer would make this change explicitly, bumping the interface's @json version.
struct EXTERNAL IAccount : virtual public Core::IUnknown {
    enum { ID = ID_ACCOUNT };

    struct GetLastCheckoutResetTimeResult {
        uint64_t resetTime; // @text resetTime
    };

    // @text getLastCheckoutResetTime
    virtual Core::hresult GetLastCheckoutResetTime(
        GetLastCheckoutResetTimeResult& resetTime /* @out */,
        Core::OptionalType<Core::ErrorDetail<AccountErrorData>>& error /* @out */) const = 0;

    // @text setLastCheckoutResetTime
    virtual Core::hresult SetLastCheckoutResetTime(
        const uint64_t resetTime,
        Core::OptionalType<Core::ErrorDetail<AccountErrorData>>& error /* @out */) = 0;
};
```

`JsonGenerator` parses this header directly (as it does today) and — unchanged from today for the success path — continues to emit a response class for `GetLastCheckoutResetTimeResult` from the existing `@out` struct; the new `error` parameter never appears in that generated success schema at all (Decision 1). On the error path, whichever of `error.Value().Code`/`.Message`/`.Data` the implementation set (if any) populate the wire `error` object; `resetTime` is not inspected on the error path, exactly as it is not today.

```cpp
// AccountImplementation.cpp — illustrative, after IAccount adopts the opt-in above.
Core::hresult AccountImplementation::GetLastCheckoutResetTime(
    GetLastCheckoutResetTimeResult& resetTime /* @out */,
    Core::OptionalType<Core::ErrorDetail<AccountErrorData>>& error /* @out */) const
{
    if (_store == nullptr) {
        error = Core::ErrorDetail<AccountErrorData>{ {}, {}, AccountErrorData{ /* ... */ } };
        return Core::ERROR_UNAVAILABLE;
    }
    resetTime.resetTime = _store->LastCheckoutResetTime();
    return Core::ERROR_NONE;
}
```

```json
{ "jsonrpc": "2.0", "id": 42, "error": { "code": -32603, "message": "Unavailable", "data": { /* AccountErrorData fields */ } } }
```

`AccountImplementation` may run in-process or out-of-process behind the COM-RPC proxy `Account::Initialize()` obtains via `_service->Root<Exchange::IAccount>(...)` — the code above is identical either way, and COM-RPC marshals both `resetTime` and `error` back unconditionally in both cases (verified above).

### `IContentProtection`: the same rule applies whether or not the method already has an `@out` parameter

`Exchange::IContentProtection` ([entservices-cpc-apis/apis/ContentProtection/IContentProtection.h](../../../../entservices-cpc-apis/apis/ContentProtection/IContentProtection.h)) is a real, in-tree instance of exactly the problem this proposal solves. `OpenDrmSession`/`UpdateDrmSession` already declare a `response` `@out` parameter and `@retval` doc-comments listing 20+ app-specific DRM/watermarking failure codes (`21002` Invalid aspect dimension, `22001` DRM general failure, etc.) that come from the SecManager subsystem the plugin talks to; `SetDrmSessionState` declares no `@out` parameter at all:

```cpp
// entservices-cpc-apis/apis/ContentProtection/IContentProtection.h — real signatures today.
virtual Core::hresult OpenDrmSession(/* ...inputs... */, string& sessionId /* @out */, string& response /* @out */) = 0;
virtual Core::hresult SetDrmSessionState(const string& sessionId, State sessionState) = 0;
```

Per Decision 2, `OpenDrmSession`'s existing `response` parameter is **never** treated as a source of `error.data` — the fact that it already has somewhere to write on the success path does not exempt it from needing the same explicit change `SetDrmSessionState` needs. Both require the identical opt-in:

```cpp
// Illustrative opt-in for BOTH methods — NOT applied by this proposal; IContentProtection's own
// maintainer would make this change explicitly, bumping the interface's @json version.
virtual Core::hresult OpenDrmSession(
    /* ...inputs... */, string& sessionId /* @out */, string& response /* @out */,
    Core::OptionalType<Core::ErrorDetail<DrmSessionErrorData>>& error /* @out */) = 0;

virtual Core::hresult SetDrmSessionState(
    const string& sessionId, State sessionState,
    Core::OptionalType<Core::ErrorDetail<DrmSessionErrorData>>& error /* @out */) = 0;
```

Both implementations can then reuse the plugin's existing `CustomCodeToString()` table (already present in `ContentProtection.cpp`, currently only reachable via the separate `IErrorToString` COM-RPC interface) to populate `Message`, and their own SecManager-native numeric scheme to populate `Code`, e.g. on `SetDrmSessionState`'s `NoSuchSession` path:

```cpp
error = Core::ErrorDetail<DrmSessionErrorData>{
    { NoSuchSession }, string(::CustomCodeToString(NoSuchSession)), {} };
return NoSuchSession;
```

```json
{ "jsonrpc": "2.0", "id": 9, "error": { "code": 21009, "message": "Invalid session identifier" } }
```

`IErrorToString` is not made redundant by this: `INotification::Status.failureReason` is delivered on a fire-and-forget event (`WatermarkStatusChanged`), which has no JSON-RPC `error` object to attach detail to at all, so an out-of-band code-to-text lookup remains the only option for that path. It becomes redundant only for ordinary request/response failures on methods that adopt Decision 1 directly.

### Prior art: no dedicated inner "code" field in comparable JSON-RPC-based protocols

Two widely-deployed, independently-implemented JSON-RPC 2.0-based protocols were checked for how they split "broad, framework-level code" from "rich, caller-specific detail" — both would suggest `Message` + free-form `Data` with no dedicated `Code`:

- **Language Server Protocol** (3.17 spec, `ResponseError`/`ErrorCodes`): `ResponseError { code: integer; message: string; data?: LSPAny }` — `data` is fully free-form, with no framework-standardized inner "code" field; `ErrorCodes` is a single, framework-owned, closed enum.
- **Ethereum JSON-RPC** (EIP-1474): `error { code, message }` uses a similarly small, spec-owned code table, with app/implementation-specific detail carried in `data`, never as a second "code" sibling.

**Thunder team decision (unaffected by this review)**: despite this precedent, the team decided to add a dedicated, opt-in `Code` field to `Core::ErrorDetail<DATA>` anyway (Decision 1), specifically so a plugin's exact subsystem-native code (e.g. `ContentProtection`'s SecManager-derived `21002`) can appear verbatim on the wire without requiring clients to separately extract it from `Data`. This is a deliberate, documented divergence from the LSP/Ethereum pattern, scoped narrowly — per-method opt-in, only takes effect when the interface owner both adopts `Core::ErrorDetail<DATA>` and explicitly sets `Code` for a given call.

## Risks / Trade-offs

- **[Trade-off, was a Risk in the pre-review design]** Requiring an explicit `Core::ErrorDetail<DATA>` parameter on *every* adopting method — even ones that already have an unrelated `@out` parameter — means there is no "free" adoption path; every method wanting rich errors pays the same ABI-bump cost (Decision 1). This is a deliberate result of CCB review (comment 1): uniformity and explicitness over convenience. Mitigation: this is a one-time, per-method cost, and the parameter shape is identical regardless of the method's other parameters, so it is at least a small, mechanical, well-documented change to make.
- **[Risk]** Plugins put sensitive/internal diagnostic information (stack traces, file paths, tokens) into `Data` since it's now easy to populate, increasing information disclosure to JSON-RPC clients (OWASP A01/A09-relevant). **Mitigation**: document that `Data`, like `Message`, is part of the response returned to whatever token/session made the call, and that plugin authors are responsible for not including secrets; add this to code-review guidance and the plugin authoring doc produced by this change.
- **[Risk, Decision 1]** Letting an opted-in method override `error.code` verbatim via `Core::ErrorDetail<DATA>::Code` reintroduces `CustomErrorCode`'s original collision risk: a plugin-chosen value could numerically collide with one of Thunder's own fixed framework codes, or with another plugin's chosen value, with no central registry enforcing distinctness. **Mitigation**: scoped narrowly — the override only takes effect for a call to a method whose interface owner both opted in and explicitly set `Code`; plugin-authoring documentation will recommend interface owners pick values outside Thunder's own reserved `ERROR_CODES` range and document their chosen range.
- **[Risk, Decision 4]** Removing `CustomErrorCode` touches public, `EXTERNAL`-exported Core symbols and a documented, externally-loadable library contract whose out-of-tree usage cannot be fully ruled out by searching this workspace. **Mitigation**: a clear, explicit breaking-change release note naming the removed symbols; confirmed zero in-tree callers before removal; the one real related pattern found in-tree (`ContentProtection`'s `IErrorToString`) does not use this mechanism at all and is unaffected by its removal.
- **[Open point, Decision 3]** The exact C++ representation for `Message::Info::Data` (`Core::JSON::Variant` vs. generator-level serialization) is not yet settled, per CCB comment 7 — see Open Questions. Whichever is chosen must satisfy the wire-behavior requirement in Decision 3 without introducing a regression for methods that don't opt in.

## Migration Plan

- Purely additive; no client-facing opt-in mechanism or feature flag is needed since adoption is per-method and per-interface-owner.
- **Cross-repository coordination required**: this change spans `Thunder` (wire model per Decision 3, `CustomErrorCode` removal) and `ThunderTools` (JsonGenerator recognition of `Core::ErrorDetail<DATA>`, Decision 1). No existing plugin's generated glue changes behavior unless that plugin's interface owner explicitly adds the new parameter and regenerates.
- No data migration needed (nothing persists error responses). No rollback complexity beyond reverting the commit(s) — the change does not alter any on-disk format or IPC/COM-RPC wire ABI for methods that don't adopt `Core::ErrorDetail<DATA>` (verified: `ProxyStubGenerator`'s frame layout is unchanged).
- Add regression tests (Thunder's `Tests/` tree) asserting: `data`/`code`-override/`message`-override are absent from the wire for any method that hasn't adopted `Core::ErrorDetail<DATA>` (non-regression on every existing passing JSON-RPC test); a method that adopts it and sets some/all of `Code`/`Message`/`Data` surfaces exactly those fields; an existing `@out`/response parameter populated on an error path is still never echoed into `error.data` (locks in Decision 2); and — to lock in the OOP invariant this design depends on — an **out-of-process** plugin method that populates `Core::ErrorDetail<DATA>` and returns an error surfaces identical `error` fields to an equivalent in-process plugin.

## Open Questions

- **`Message::Info::Data`'s C++ representation** (Decision 3, raised by CCB comment 7): `Core::JSON::Variant`, generator-level raw-fragment serialization, or another approach — to be resolved during implementation, constrained only by the wire-behavior requirement in Decision 3.
- Should COM-RPC interface headers gain additional generated documentation for a method's `Core::ErrorDetail<DATA>` schema (e.g. surfaced in generated docs the way success-result schemas are), as a follow-up to this proposal?
- Should `JSONRPCErrorAssessor` (Decision 5) be scheduled for actual removal in a follow-up change, and if so, what is the migration path for any existing registered assessor?
- ~~Is there a parallel need to carry structured detail across COM-RPC (`Core::hresult`/`COM_ERROR`) boundaries before they are translated into JSON-RPC errors, and if so, is that a separate proposal?~~ **Resolved during design**: no. Direct inspection of `ThunderTools/ProxyStubGenerator/StubGenerator.py` confirms COM-RPC stub/proxy code already marshals all "out" parameters unconditionally, regardless of the returned `hresult`, including the new `Core::ErrorDetail<DATA>` parameter (Context above).
- Does exposing `Core::ErrorDetail<DATA>` on any capability warrant a JSON-RPC interface/version bump, or is this treated as transport-level and version-agnostic? (Current assessment: this is an ABI/interface-version bump for the adopting interface's `@json` version, since it changes the method's C++ signature — see Decision 1 — but is version-agnostic from the JSON-RPC wire-schema perspective, since `data` is purely additive per spec.)
