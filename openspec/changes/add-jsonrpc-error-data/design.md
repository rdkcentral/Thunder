## Context

Thunder's JSON-RPC error object model lives in `Core::JSONRPC::Message::Info` ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L64-L230)). It defines `code`, `message` (`Text`), and `data` (`Data`), but `Data` is declared as a plain `Core::JSON::String` and is never assigned anywhere in the codebase — it exists in the wire schema but carries no information today.

Three independent invocation paths turn a handler's `uint32_t`/`Core::hresult` result into this wire object:

1. **Generated path (the vast majority of real plugin methods)** — for modern plugins the IDL source of truth is **not** a hand-authored `.json` file; it is the plugin's annotated COM-RPC interface header itself (e.g. [entservices-apis/apis/Account/IAccount.h](../../../../entservices-apis/apis/Account/IAccount.h), a `struct EXTERNAL IAccount : virtual public Core::IUnknown` with `@json`/`@text`/`@brief`/`@param`/`@retval`/`@out` doc-comment annotations). `JsonGenerator` (`ThunderTools/JsonGenerator/source/header_loader.py`, sharing the same `CppParser.py` that `ProxyStubGenerator` uses to parse interface headers for COM-RPC marshaling) parses this header directly and emits, per interface, a `J<Interface>.h` glue header (e.g. `JAccount.h`) — the legacy hand-authored `.json` IDL path still exists for older plugins but is not how new interfaces are defined. Either way, the emitted glue in `rpc_emitter.py` has the same shape: for every method it emits its own bespoke lambda registered via `Register<std::function<uint32_t(...)>>` (not the generic `Handler::Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>` template), whose body (a) calls the real interface method directly — `errorCode_ = implementation->Method(...)` — capturing the plugin's real out-parameters into local C++ temporaries, then (b) copies those temporaries field-by-field into the JSON response container **only inside `if (errorCode_ == Core::ERROR_NONE) { ... }`** (`rpc_emitter.py`, "Emit result handling and serialization to JSON data", ~line 1454). There is no `else` branch — on error, none of the response fields are copied, so the JSON response object is left completely unset, and whatever the plugin implementation wrote to its output parameters is silently thrown away **at codegen level**, not inside `Source/core/JSONRPC.h`. This is the actual, dominant discard point for real Thunder plugins, whether in-process or out-of-process (OOP).

2. **Hand-written generic path** — the small set of call sites that register directly via `Handler::Register<INBOUND, OUTBOUND, METHOD>(...)` without going through JsonGenerator. Its glue, `InternalRegisterImplIO` / `InternalRegisterImpl<OUTBOUND,...>` ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L1681-L1719)), has the same shape of bug:
   ```cpp
   if ((code == Core::ERROR_NONE) && (outbound.IsSet() == true)) {
       outbound.ToString(result);
   } else {
       result.clear();   // <-- any data the plugin put on its response object is lost
   }
   ```
   This affects a much smaller surface than path 1, but should be fixed for consistency (a hand-written `Register<>` call site should behave the same as a generated one).

3. **Raw / hand-written path** — plugins (or Thunder itself, for `exists`/`register`/`unregister`) that register a bare `InvokeFunction` lambda and write directly to the `response` string. Here, [Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L1108-L1114) already has a convention: if `output` (the `response` string) is non-empty on error, it is dumped verbatim into `Error.Text`, overwriting the default text `SetError()` produced from the code:
   ```cpp
   response->Error.SetError(result);
   if (output.empty() == false) {
       response->Error.Text = output;   // ad-hoc "extra detail" channel, but only text
   }
   ```

### Where the JSON-RPC endpoint runs vs. where an OOP implementation runs (verified with a real plugin)

Answering directly: *how can an out-of-process implementation return error `data` through COM-RPC, given the JSON-RPC endpoint is implemented in the part of plugin code that runs in Thunder's process context?* — Inspecting the real `Account`/`IAccount` plugin ([entservices/entservices-account/plugin/](../../../../entservices/entservices-account/plugin/)) confirms the mechanism precisely:

- `Account` (`Account.h`/`Account.cpp`) is the **plugin shell**: `class Account : public PluginHost::IPlugin, public PluginHost::JSONRPC`. It is what `PluginHost` loads and what implements `IDispatcher` — i.e. it *is* the JSON-RPC endpoint, and it runs wherever `PluginHost` instantiates the `Account` object (ordinarily Thunder's main process context).
- In `Account::Initialize()`:
  ```cpp
  _account = _service->Root<Exchange::IAccount>(_connectionId, 5000, _T("AccountImplementation"));
  // ...
  Exchange::JAccount::Register(*this, _account);
  ```
  `_service->Root<Exchange::IAccount>(...)` is what may place the real implementation (`AccountImplementation`, in `AccountImplementation.h`/`.cpp`) **out-of-process** (per plugin config) and hands back either a direct pointer (in-process) or a COM-RPC **proxy** pointer (OOP) — the plugin shell code is identical either way. `Exchange::JAccount::Register(*this, _account)` is the JsonGenerator-emitted `Register()` function (Decision 3a's target): it registers the per-method lambdas — which capture `_account` — on `*this`, the `Account` plugin shell/`IDispatcher`. **These lambdas therefore always execute in the plugin shell's process (Thunder's process context)**, exactly as observed; they are never relocated into the OOP implementation's process.
- When `_account` is a COM-RPC proxy, calling e.g. `_account->GetLastCheckoutResetTime(resetTime)` from inside that lambda is what performs the actual COM-RPC round trip: the call is marshaled to the `AccountImplementation` process, `AccountImplementation::GetLastCheckoutResetTime` runs there and populates `resetTime` (its `@out` parameter) exactly as it would if returning success, then returns a `Core::hresult`. The generated **stub**, running in the `AccountImplementation` process, writes `resetTime` back over the wire **unconditionally** (verified below); the generated **proxy**, running back in the `Account` plugin shell's process, reads it back into the local `resetTime` variable **unconditionally** too. By the time `_account->GetLastCheckoutResetTime(resetTime)` returns inside the JsonGenerator-emitted lambda, `resetTime` is fully populated *regardless of the returned hresult* — the OOP implementation "returns the error object through COM-RPC" the same way it returns anything else: via its already-existing `@out` parameter, which COM-RPC always marshals back. The only place that then discards it is the lambda's own `if (errorCode_ == Core::ERROR_NONE)` gate (Decision 3a), immediately after the call returns, in the plugin shell's process — never inside COM-RPC and never inside the OOP implementation's process.

### COM-RPC / out-of-process plugins: verified, no marshaling change needed

RDK/Thunder plugins are frequently out-of-process (OOP); when they are, the `implementation` pointer captured by the JsonGenerator-emitted lambda (path 1 above) is a COM-RPC proxy (`Core::IUnknown`-derived interface, e.g. `Exchange::IAccount*` obtained via `_service->Root<>()`), and calling a method on it marshals the call across the process boundary via code produced by the sibling `ProxyStubGenerator` tool (`ThunderTools/ProxyStubGenerator/StubGenerator.py`). This raised the question of whether COM-RPC marshaling *itself* discards "out" parameters when the remote call returns a failure `Core::hresult`, which would require a second fix at the ProxyStub layer. Direct inspection of `StubGenerator.py` answers this:

- **Stub (server) side** (`EmitStubMethodImplementation`, ~line 2021-2049): after invoking the real implementation (`CallImplementation(...)`), the stub writes **every** output parameter back to the response writer unconditionally — `for p in output_params: if p: WriteParameter(p)` — with no `hresult`/`ERROR_NONE` guard at all.
- **Proxy (client) side** (`EmitProxyMethodImplementation`, ~line 2440-2480): after the transport-level `Invoke()` succeeds (i.e. the message was delivered and a response received — a different, lower-level notion of "success" than the application's returned `Core::hresult`), the proxy reads back the application-level `hresult` first, then reads **every remaining output parameter unconditionally** — `for p in output_params[_first_param:]: ReadParameter(p)` — again with no gate on the application-level `hresult` value.

**Conclusion: COM-RPC already faithfully transports "out" parameters across the process boundary regardless of the application-level error code.** No change is needed in `ProxyStubGenerator`. The entire gap is in JsonGenerator's generated glue (path 1), which has its own, separate, unconditional discard behavior downstream of a COM-RPC call that already delivered the data correctly. This significantly de-risks the design: it confirms the fix is confined to two known code-generation/glue layers rather than the lower-level IPC transport.

Separately, `CustomErrorCode` ([Source/core/ICustomErrorCode.h](../../../Source/core/ICustomErrorCode.h), [Source/core/Errors.h](../../../Source/core/Errors.h#L33-L46), [Source/core/Errors.cpp](../../../Source/core/Errors.cpp), [Source/Thunder/PluginHost.cpp](../../../Source/Thunder/PluginHost.cpp#L152-L206)) lets a process register **one global** `int32_t -> const TCHAR*` handler (dynamically loaded from a separately configured shared library, via `PluginHost.cpp`'s `CustomCodeLibrary`), invoked from `Info::SetError()` to fill in `Text` for custom (non-framework) codes. It is code-level (one string per code, process-wide, "can only set one, not multithreaded safe" per its own header comment), not call-level, and cannot express structured `data`. The Thunder team has confirmed this specific mechanism is not known to be wired up anywhere in RDK today, and considers this a window to consolidate it away entirely rather than keep it alongside the new mechanism. **This proposal now removes it — see "CustomErrorCode is removed" (Decision 5) below.**

### Real-world precedent: teams already needed this and built their own version

Before removing `CustomErrorCode`, it's worth confirming *why* it went unused rather than assuming indifference. Searching `entservices`/`entservices-cpc` for any of `CustomCode`, `IsCustomCode`, `SetCustomCodeToStringHandler` turns up exactly one real hit, and it is telling: [entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h](../../../../entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h#L271-L295) defines its own free function `CustomCodeToString(int32_t code)` — annotated `// Replace with Thunder Custom (Error) Codes`, i.e. clearly *aware* of the official mechanism — but instead of wiring it up via `Core::SetCustomCodeToStringHandler`, the plugin exposes it through a bespoke, plugin-local, `@json`-annotated COM-RPC interface it invented itself: `Exchange::IErrorToString` ([entservices-cpc-apis/apis/ContentProtection/IContentProtection.h](../../../../entservices-cpc-apis/apis/ContentProtection/IContentProtection.h#L184-L192), `virtual Core::hresult ErrorToString(const int32_t code, string& result) const`). A separate process, [entservices-cpc/app-gateway-cpc/FbEntos/delegate/CPSDelegate.h](../../../../entservices-cpc/app-gateway-cpc/FbEntos/delegate/CPSDelegate.h#L116-L135), then queries this interface cross-process via `QueryInterfaceByCallsign<Exchange::IErrorToString>(...)` whenever it needs to translate one of ContentProtection's ~60 DRM/watermarking error codes into text. This confirms real demand for "call a plugin to resolve a numeric code into text" — but it also confirms the *official* global-singleton-function-pointer mechanism was inadequate enough that a real team quietly reinvented it rather than adopt it (plausibly because it doesn't scale across multiple plugins each wanting their own code space, and can't be queried cross-process the way a COM-RPC interface can). Two conclusions follow: (1) removing the official mechanism is very unlikely to break anything in this workspace — nothing here calls `Core::SetCustomCodeToStringHandler`, confirmed by the same search; (2) `IErrorToString`'s use case — resolving a **bare code, on demand, independent of any specific in-flight call** (e.g. a code that arrived via an event/callback, not a method's own error response) — is a genuinely different shape of problem than "attach data to *this* call's own error" (Decisions 2/3/7), is already fully solvable today with zero framework changes (as `ContentProtection` proves), and is deliberately left out of this proposal's scope rather than folded in.

`Core::JSON::Variant` ([Source/core/JSON.h](../../../Source/core/JSON.h#L4395)) already exists in the JSON framework and can represent any JSON-RPC-legal value (`EMPTY`, `BOOLEAN`, `NUMBER`, `STRING`, `ARRAY`, `OBJECT`, `FLOAT`, `DOUBLE`) — it is the natural wire type for a spec-compliant `data` member, and is already used elsewhere for "any JSON value" fields (e.g. `Core::JSON::VariantContainer`).

## Goals / Non-Goals

**Goals:**
- Make `Message::Info.Data` a real, spec-compliant JSON value (any type), and have it serialize/deserialize correctly, while remaining **omitted from the wire** when a call never sets it (matching today's behavior for the vast majority of error responses).
- Let a plugin method attach structured `data` **using the response object it already declares**, with no change to its `Core::hresult`/`uint32_t` return-code signature and no new registration API for the common (typed/generated) case.
- Preserve, byte-for-byte, the existing behavior of hand-written handlers that put plain text in `output` on error (it must keep landing in `Error.Text`, not silently start showing up in `Error.Data`).
- Give the `JSONRPCErrorAssessor` hook the same ability to attach `data` that regular handlers get, so error-enrichment glue code doesn't need a second, different mechanism.
- **Consolidate to a single, call-specific source of truth for custom error text/detail**: remove `CustomErrorCode`'s global-handler mechanism entirely (Decision 5) rather than document a boundary between two parallel systems, and replace its two legitimate capabilities (an app-chosen value appearing verbatim as `error.code`, and a replacement `message` for an app-specific error) with call-specific equivalents on Decision 7's opt-in `ErrorDetail<DATA>` (its `Code` and `Message` fields, respectively).
- Provide an **explicit, opt-in** mechanism (Decision 7) for interface owners who want structured `data` (and/or a call-specific `message` override) on a method that has no existing response object to reuse (e.g. a void-response method), or who want an error-detail schema independent of the method's success-result schema — without requiring it, and without pretending it's free (it is an interface change, unlike the rest of this proposal).

**Non-Goals:**
- Changing the synchronous `uint32_t`/`Core::hresult` return-code contract of `Handler::Register` methods (this is Thunder's fundamental plugin ABI; not up for renegotiation here) — `error.code` remains derived exclusively from that return value via `SetError()`'s existing (now sole) mapping for every method that has not explicitly opted into Decision 7's `ErrorDetail<DATA>::Code` override for that one call (see Decision 7).
- Building a general "translate a bare error code to text, independent of any specific call" lookup service (the `IErrorToString`/`ContentProtection` pattern above) — already achievable today per-plugin with zero framework changes (an ordinary `@json`-annotated method), and out of scope here.
- Adding new `@`-annotation syntax to COM-RPC interface headers (or to the legacy `.json` IDL) to declare a typed `data` schema per error (tracked as an open question / follow-up).
- Changing how COM-RPC (`Core::hresult`, `COM_ERROR` bit) errors that cross into JSON-RPC carry detail, beyond what `SetError()` already does — **verified unnecessary**: `ProxyStubGenerator`-emitted stub/proxy code already marshals "out" parameters unconditionally, regardless of the application-level `hresult` (see Context above), so COM-RPC needs no change.
- Changing the wire format or behavior of successful (`Core::ERROR_NONE`) responses.
- Changing `ProxyStubGenerator` (`ThunderTools/ProxyStubGenerator/`) — confirmed out of scope; it already preserves output parameters on error.
- **Automatically or silently** adding an `@out` parameter to an existing interface method that doesn't already declare one, or retrofitting it without the interface owner's explicit action — that remains out of scope; this proposal never modifies an interface on a plugin's behalf. Decision 7 below *does* define an explicit, opt-in parameter shape an interface owner can choose to add — but adopting it is a per-method decision made (and paid for, in interface-version/ABI terms) by that interface's own maintainer, not something this change does automatically. Methods that don't adopt it keep exactly today's `code`/`message`-only error behavior.

## Decisions

### 1. `Message::Info.Data`: `Core::JSON::String` → `Core::JSON::Variant`
`Core::JSON::Variant` already models every JSON-RPC-legal `data` shape and already participates in the standard `Core::JSON::Container` `IsSet()`-driven (de)serialization used throughout Thunder's JSON layer — an unset `Variant` is omitted from `ToString()` output exactly like every other unset `Container` member today, so backward compatibility ("no `data` in the wire unless something set it") falls out of the existing framework behavior rather than needing new logic.

*Alternative considered*: introduce a brand-new `AnyType`/`Core::JSON::Value` wrapper specifically for this. Rejected — `Variant` already exists, is already public API, and is already used for "arbitrary JSON" elsewhere (`VariantContainer`); adding a second "any JSON value" type would fragment the framework for no benefit.

### 2. Population mechanism: two different mechanisms for two different levels of type information

Not every invocation path has the same information available, so this uses two complementary mechanisms rather than one:

- **Typed paths (1 generated, 2 hand-written `Register<INBOUND,OUTBOUND,METHOD,...>`)**: the framework already holds the OUTBOUND/response value as a real `Core::JSON::IElement`-derived object (`Core::JSON::Container`, or a bare `Core::JSON::Variant`-compatible scalar/array when JsonGenerator's default "collapsed" RPC format reduces a single-field result to a bare value — see the "Non-object and void responses" note below). No string content-sniffing is needed or used here: whenever that object `IsSet() == true` on the error path, it is assigned **directly** into `Error.Data` as a typed value (Decision 3), regardless of whether it happens to serialize as an object (`{...}`), an array (`[...]`), or a bare scalar (`42`, `"text"`, `true`) — `Core::JSON::Variant` (Decision 1) represents all of these equally well, so there is nothing to disambiguate.
- **Raw/hand-written path (3, bare `InvokeFunction` lambda)**: by the time [Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L1108-L1114) sees this path's result, all it has is a plain `string& output` with no type information at all — this is the one place a runtime heuristic is unavoidable. On the error path, if `output` is non-empty and parses as a JSON **object** (`{...}`), the parsed value is assigned to `Error.Data`; if it is non-empty but does **not** parse as a JSON object (the historical convention — a plain human-readable sentence), it is assigned to `Error.Text` exactly as today; if it is empty, neither field is touched. This heuristic is intentionally narrower than "any valid JSON value" (i.e. a bare `42` or `"quoted string"` written to `output` is still treated as plain text, not `data`) specifically because this path has no static type to lean on, and legacy plain-text messages are more likely to collide with bare scalars/strings than with a full `{...}` object.

This means path 1/2 never needs to guess, and path 3 keeps exactly today's narrow, safe heuristic.

*Alternative considered*: reserve reserved keys inside a wrapper envelope, e.g. `{"message": "...", "data": {...}}`, and have the framework pull `message`/`data` back out. Rejected — it collides with real interface-declared field names (a plugin's response struct might legitimately have a field called `data` or `message`), and it re-couples `code`/`text` derivation (already owned by `SetError()`) with the new mechanism. Keeping `Error.Text` sourced only from `SetError()`/the plain-text legacy path/Decision 7's opt-in per-call override, and `Error.Data` sourced only from the typed value (paths 1/2) or a parsed JSON object (path 3), keeps the two concerns orthogonal.

*Alternative considered*: add an explicit `Core::JSON::Variant* data` (or `Core::OptionalType<Core::JSON::Variant>&`) out-parameter to `Handler::Invoke` / `InvokeFunction`. Rejected as the primary mechanism — `InvokeFunction` is a long-standing public typedef; widening it breaks source compatibility for every hand-registered raw handler in the field, for a benefit (an explicit out-param) that content-sniffing already achieves without an ABI change. Recorded as an open question in case content-sniffing proves insufficient in practice.

### 3. Stop discarding populated response fields on error — primary fix in JsonGenerator (`ThunderTools`), secondary fix for hand-written `Register<>`

**3a. Primary fix — JsonGenerator's generated glue (`ThunderTools/JsonGenerator/source/rpc_emitter.py`)**: this is where the actual discard happens for essentially all real plugin methods (Context, path 1). The "Emit result handling and serialization to JSON data" section currently only copies the interface call's real output values into the JSON response container inside `if (errorCode_ == Core::ERROR_NONE) { ... }`, with no `else`. Since Decision/Context above confirms COM-RPC already delivers those output values intact to this point regardless of the returned `hresult` (whether the plugin is in-process or OOP), the generator is changed to run the same field-copy logic on the error path too, so the JSON response object generated code produces is populated whenever the plugin populated its output parameters — whether it returned success or an error code. Concretely, the emitted C++ changes from:
```cpp
if (errorCode_ == Core::ERROR_NONE) {
    // ... copy each output param into the response object ...
}
```
to:
```cpp
if (errorCode_ == Core::ERROR_NONE || /* response object has any output fields copied */ true) {
    // ... copy each output param into the response object (unconditionally) ...
}
```
(exact emitted form to be finalized during implementation — the essential change is removing the `errorCode_ == Core::ERROR_NONE` gate around the existing field-copy loop, not adding new logic). This is a **cross-repository change**: it lives in `ThunderTools`, a sibling repository to `Thunder`, and every plugin needs its generated `J*.h` header regenerated (via the updated `JsonGenerator.py`) to pick up the new behavior — existing, not-yet-regenerated generated headers keep today's behavior unchanged (fully backward compatible, opt-in via regeneration).

**3b. Secondary fix — hand-written `Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>` call sites (`Source/core/JSONRPC.h`)**: `InternalRegisterImplIO` and `InternalRegisterImpl<OUTBOUND, METHOD, ...>` ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L1681-L1719)) change from:
```cpp
if ((code == Core::ERROR_NONE) && (outbound.IsSet() == true)) {
    outbound.ToString(result);
} else {
    result.clear();
}
```
to serializing `outbound` whenever it `IsSet()`, regardless of `code`:
```cpp
if (outbound.IsSet() == true) {
    outbound.ToString(result);
} else {
    result.clear();
}
```
This affects a much smaller surface (hand-written registrations that bypass JsonGenerator) but keeps the two paths behaviorally consistent — a developer shouldn't get different error-`data` behavior depending on whether their `Register<>` call was generated or hand-written.

Combined, 3a (dominant case) and 3b (remaining case) are what actually unlocks structured error data for the whole plugin population: any existing plugin method **that already has at least one `@out` parameter** can start returning error detail just by setting fields on the response object it already has, before returning a non-`ERROR_NONE` code — once its generated header is regenerated with the updated JsonGenerator. Both fixes assign the already-typed OUTBOUND/response value directly (Decision 2) — they do not care whether that value's JSON shape is an object, an array, or (after JsonGenerator's default single-field "collapsed" format, see below) a bare scalar; `IsSet()`/direct assignment works the same way regardless of shape.

### Non-object, collapsed, and void responses (methods without a usable data vessel)

Two real shapes need calling out explicitly, both visible on the real `IAccount` interface ([entservices-apis/apis/Account/IAccount.h](../../../../entservices-apis/apis/Account/IAccount.h)):

- **Single-field results collapse to a bare scalar, but still carry data.** `JsonGenerator`'s default "collapsed" RPC format (`header_loader.py`'s `BuildResult`, unless the interface is tagged `@wrapped`) reduces a result with exactly one output property to that property's own schema — for `GetLastCheckoutResetTime`, the single `@out` parameter is itself a struct (`GetLastCheckoutResetTimeResult { uint64_t resetTime; }`), so collapsing here is a no-op observably (`"result": {"resetTime": ...}`, matching the worked example above). But a method with a **bare scalar** `@out` parameter directly — e.g. a hypothetical `virtual Core::hresult GetCount(uint32_t& count /* @out */) const = 0;` with no wrapping struct — collapses to a bare number: `"result": 42` on success. Decision 2/3 still apply unchanged: the OUTBOUND value is still a real, typed `Core::JSON::IElement` (just a scalar one, e.g. `Core::JSON::DecUInt32` instead of a `Container`), so on an error path it is still assigned directly into `Error.Data` — the wire shape becomes `"data": 42`, a bare JSON number rather than an object. Per the JSON-RPC spec, `data` is legal as any JSON value, not only an object, so this is fully compliant; it does mean a consumer must not assume `error.data` is always an object.
- **Methods with no `@out` parameter at all have no vessel from Decisions 2/3 alone.** `IAccount::SetLastCheckoutResetTime(const uint64_t resetTime)` takes only an *input* and returns `Core::hresult` with no `@out` parameter whatsoever. `BuildResult` returns `{"type": "null", "description": "Always null"}` for such methods, and the JsonGenerator-emitted lambda for a void response never even declares a `response` parameter (Context, path 1) — there is no C++ object of any kind for `AccountImplementation::SetLastCheckoutResetTime` to populate. **Decisions 2/3 alone cannot add a `data` channel to such a method** — there is nothing for the plugin author to set fields on, and those decisions deliberately don't add a new out-of-band parameter/signature to manufacture one. Decision 7 below defines an **opt-in** path for interface owners who want one anyway; until/unless a method's interface owner adopts it, that method keeps exactly today's behavior: `code`/`message` only, never `data`.

### 4. Consolidate error-response assembly in `PluginServer.h`
[Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L1108-L1114) changes from unconditionally writing `output` into `Error.Text`, to applying the Decision 2 rule for the raw path (try parse as JSON object → `Error.Data`; else → `Error.Text`). The same rule is applied at the other `output`-into-error sites the exploration identified (parse-failure paths around lines 4776/4924, and the `JSONRPCErrorAssessor` response string), so there is one rule, applied consistently, rather than one-off special cases per call site.

### 5. `CustomErrorCode` is removed; consolidated into Decision 7's opt-in mechanism
Rather than leave two parallel, disconnected "custom error detail" systems in place, `CustomErrorCode` is deleted outright, giving the codebase a **single source of truth** for call-specific error detail:
- **Removed**: `Core::CustomCode()`/`Core::IsCustomCode()` and the `CustomCodeToStringHandler`/`SetCustomCodeToStringHandler` pair from [Source/core/Errors.h](../../../Source/core/Errors.h#L33-L46)/[Errors.cpp](../../../Source/core/Errors.cpp); [Source/core/ICustomErrorCode.h](../../../Source/core/ICustomErrorCode.h) deleted entirely; the `CustomCodeLibrary` dynamic-library-loading class and its config wiring removed from [Source/Thunder/PluginHost.cpp](../../../Source/Thunder/PluginHost.cpp#L152-L206); [docs/utils/customcodes.md](../../../docs/utils/customcodes.md) replaced with documentation for Decision 7's mechanism.
- **`SetError()` simplified**: the `IsCustomCode()` branch in `Info::SetError()` ([Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L100-L215)) is deleted. Every hresult not matching one of the fixed framework codes now falls through unconditionally to the existing deterministic default: `Code = ApplicationErrorCodeBase - static_cast<int32_t>(frameworkError)`. This is not a functional regression for "gets a distinct, collision-free code" — that already worked for arbitrary hresults before `CustomCode()` was involved. The one capability actually lost by removing the *global* mechanism — an app's ability to make an arbitrary chosen integer appear **verbatim, unmodified** as `error.code` on the wire — is not dropped, it is reinstated in a narrower, opt-in, per-call form: Decision 7's `ErrorDetail<DATA>` gains a `Code` field (see below) that, when set, overrides `SetError()`'s default-derived value for that call only. Unlike `CustomCode()`, this is per-method (an explicit interface/ABI opt-in an interface owner makes deliberately, not a global process-wide handler any process could register), so there is exactly one place — the opted-in `ErrorDetail<DATA>` value itself — a caller needs to look to understand why `error.code` differs from the default mapping for that call.
- **Its two legitimate capabilities — an app-chosen value appearing verbatim as `error.code`, and a call-specific replacement `message` — are replaced, not dropped**: Decision 7's `ErrorDetail<DATA>` gains `Code` and `Message` fields (see below) so a method that opts in can set `Error.Code`/`Error.Text` to arbitrary values *for that specific call*, which is strictly more capable than `CustomErrorCode`'s one-value-per-code, process-wide, single-registered-handler model (the source comment on `SetCustomCodeToStringHandler` literally says "can only set one, not multithreaded safe").
- **This is an intentional breaking change to public, `EXTERNAL`-exported Core symbols and a documented external-library contract** (`ICustomErrorCode.h`'s `CustomCodeToString`, loadable from an arbitrary out-of-tree shared library by path). It cannot be proven zero-risk purely by grepping this workspace, precisely because that contract is designed to be implemented by code outside it. The recommended mitigation is a clear breaking-change release note (not a silent removal) calling out the removed symbols by name, so any out-of-tree consumer has a chance to react — see tasks.md.
- **Real-world precedent supports removing the mechanism, but also validates the underlying need**: the `ContentProtection`/`IErrorToString` pattern described above shows a real RDK plugin needed exactly this class of capability and built its own parallel mechanism rather than adopt `CustomErrorCode` — reinforcing that the *official* global-handler mechanism specifically was the wrong shape (unused for a concrete reason, not because nobody needed the capability), while its narrow "per-call text override" niche is fully covered by Decision 7.
- **`ErrorToString()`/`ErrorToStringExtended()` lose their custom-code branch too, not just `SetError()`** ([Source/core/Errors.cpp](../../../Source/core/Errors.cpp#L104-L129)): both currently call `IsCustomCode()` themselves and, if set, delegate to `HandleCustomErrorCodeToString(Extended)()`, which in turn calls the registered handler. Once `IsCustomCode()` is deleted, these two functions keep only their existing fallback path — recursive lookup against the fixed `ERROR_CODES` enum table (`_bogus_ErrorToString<N>`), and, for `ErrorToStringExtended()`, `"Undefined Thunder error code: <N>"` when no fixed code matches. This fallback path is exercised directly in the worked example below.

#### Worked example: what a client actually sees for an app-specific error code, before and after this proposal

To make Decision 5's abstract "falls through to the default mapping" claim concrete, trace a single plugin call that returns an app-specific numeric error outside Thunder's fixed `ERROR_CODES` table — reusing `ContentProtection`'s own DRM error-code range (21001+, see Context above) as a realistic value:

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

**Today, with `CustomErrorCode` (before this proposal)** — if the plugin instead returns `Core::CustomCode(21001)` and some process has called `Core::SetCustomCodeToStringHandler(...)` with a handler that maps `21001` to `"DRM session invalid"`, `SetError()`'s custom-code branch sets `Code` to the value **verbatim**, and `ErrorToStringExtended()`'s own `IsCustomCode()` check resolves the registered handler's text:

```json
{
    "jsonrpc": "2.0",
    "id": 7,
    "error": {
        "code": 21001,
        "message": "DRM session invalid"
    }
}
```

(In practice, no in-tree plugin does this — see the verification note above — but this is the mechanism's intended contract.)

**After this proposal (Decision 5 — `CustomErrorCode` removed), the same `return static_cast<Core::hresult>(21001);` with no other change** falls through `SetError()`'s `default:` branch unconditionally:

```cpp
// Source/core/JSONRPC.h — Info::SetError(), default branch, after Decision 5
Code = ApplicationErrorCodeBase - static_cast<int32_t>(frameworkError);   // -31000 - 21001 = -52001
// Text.IsSet() == false, so:
Text = Core::ErrorToStringExtended(frameworkError);                      // "Undefined Thunder error code: 21001"
```

```json
{
    "jsonrpc": "2.0",
    "id": 7,
    "error": {
        "code": -52001,
        "message": "Undefined Thunder error code: 21001"
    }
}
```

Two concrete, observable differences from removing `CustomErrorCode`, both already called out in Decision 5 but shown here with real numbers: **`code` is no longer the app's literal `21001`** (it's the offset-transformed `-52001` — still deterministic and collision-free, just not the value the plugin author chose), and **`message` is a generic, unhelpful placeholder** rather than "DRM session invalid" (nothing populated `Text`, so the default fallback string wins). Neither of these is a functional regression in the sense of losing distinctness/collision-safety — but a plugin author who wants a *meaningful* message back must now opt into Decision 7:

```cpp
// Illustrative opt-in (Decision 7) — NOT applied to any existing interface by this proposal.
Core::hresult SomePlugin::DoSomething(Core::OptionalType<Core::JSONRPC::ErrorDetail<SomeErrorData>>& error /* @out @error */)
{
    if (_drmSessionInvalid) {
        // Code left unset here: error.code keeps the default ApplicationErrorCodeBase-derived mapping.
        error = Core::JSONRPC::ErrorDetail<SomeErrorData>{ {}, string(_T("DRM session invalid")), { 21001 } };
        return static_cast<Core::hresult>(21001);
    }
    return Core::ERROR_NONE;
}
```

```json
{
    "jsonrpc": "2.0",
    "id": 7,
    "error": {
        "code": -52001,
        "message": "DRM session invalid",
        "data": { "reason": 21001 }
    }
}
```

`message` is now call-specific and meaningful, and the app's own `21001` scheme travels intact inside `data` as an ordinary field of the plugin's own `SomeErrorData` struct. `code` is still `-52001` here only because this particular call left `Code` unset — had it instead set `error = Core::JSONRPC::ErrorDetail<SomeErrorData>{ 21001, string(_T("DRM session invalid")), { 21001 } };`, `error.code` would be `21001` verbatim instead, taking precedence over the default mapping for this call only (see the `Code` field added to `ErrorDetail<DATA>` in Decision 7 below).

### 6. `JSONRPCErrorAssessor` gets the same capability, no signature change
The assessor callback ([Source/common/JSONRPC.h](../../../Source/common/JSONRPC.h#L1918-L1971)) already receives `errorcode` and a mutable `string& response`. It follows the same Decision 2 raw-path rule: if it leaves a JSON-object string in `response`, that becomes `Error.Data`. This gives error-enrichment glue code parity with regular handlers without widening `JSONRPCErrorAssessorTypes::FunctionCallbackType`/`StdFunctionCallbackType`.

### 7. Opt-in dedicated error-detail parameter, for methods with no reusable response object (interface-owner decision, not automatic)

Decisions 2/3 give `data` "for free" only to methods that already declare at least one `@out` parameter, by reusing whatever object they already populate. That leaves two real gaps, both visible on `IAccount`: methods with no `@out` parameter at all (`SetLastCheckoutResetTime`), and methods where the success-result shape isn't a good fit for error diagnostics (e.g. an interface owner wants richer error detail than the success payload naturally carries). Decision 7 closes both gaps, but **only when an interface owner explicitly opts in** — it is not applied automatically to any existing method.

**Mechanism**: introduce a reusable wrapper, structurally recognized by `JsonGenerator` the same way it already special-cases `Core::JSONRPC::Context` ([header_loader.py](../../../../ThunderTools/JsonGenerator/source/header_loader.py) — `if "Core::JSONRPC::Context" in cppType.full_name: result = ["@context", {}]`):

```cpp
// New, in Source/core/ (name illustrative)
namespace Core { namespace JSONRPC {
    template<typename DATA>
    struct ErrorDetail {
        Core::OptionalType<int32_t> Code;     // per-call Error.Code override — takes precedence over SetError()'s default mapping when set
        Core::OptionalType<string> Message;   // per-call Error.Text override; replaces CustomErrorCode's one legitimate use (Decision 5)
        DATA Data;                            // interface owner's own struct/scalar type — the error-detail schema
    };
} }
```

**Decision (Thunder team)**: `Code` is included, alongside `Message`, as an explicit per-call override of the wire `error.code` — reinstating `CustomErrorCode`'s one real capability (an app-chosen value appearing verbatim on the wire) in a narrower, opt-in, per-method form rather than a global, process-wide handler. When an opted-in method's `ErrorDetail<DATA>::Code` `IsSet()`, it takes precedence over `SetError()`'s `ApplicationErrorCodeBase`-derived default for that call only; when unset, `error.code` is derived exactly as it is for any method that hasn't opted in. An earlier iteration of this design rejected a `Code` field on the grounds that it reintroduces a second source of truth for `error.code` (see "Prior art" below) — the Thunder team's decision supersedes that recommendation: plugin teams (`ContentProtection` chief among them, see the real-world worked example below) have a concrete, recurring need to hand a JSON-RPC client the exact numeric code their own subsystem produced (e.g. SecManager's `21002`), not a re-derived offset, and requiring that value to travel only inside `Data` (an ordinary field with no framework-enforced meaning to a generic client) was judged an unnecessary indirection for that specific, common case. This stays well-defined despite there now being two candidate sources for `error.code`, because only a method that has itself opted into `ErrorDetail<DATA>` and itself set `Code` can produce the override, and it only ever affects that one call.

An interface owner who wants this for a given method appends an optional out-parameter of this shape:

```cpp
// Illustrative opt-in for SetLastCheckoutResetTime — NOT part of this proposal's default behavior;
// an interface owner would make this change explicitly, in their own interface header.
virtual Core::hresult SetLastCheckoutResetTime(
    const uint64_t resetTime,
    Core::OptionalType<Core::JSONRPC::ErrorDetail<SetLastCheckoutResetTimeErrorData>>& error /* @out @error */) = 0;
```

`header_loader.py` recognizes the `Core::JSONRPC::ErrorDetail<...>` type (structurally, like `Context`) or an explicit `@error` tag on the parameter (simpler to implement, easier to grep for — exact choice left to implementation), and: (a) **excludes it from `BuildResult`'s success-result schema entirely** — it never appears in a successful response, so adopting this is invisible to existing successful-call consumers; (b) records it separately so `rpc_emitter.py` can emit an extra local `Core::OptionalType<...>` temporary, pass it by reference into the real interface call alongside the normal parameters, and — on the error path only — if it `IsSet()`: assign its `Code` field into `Error.Code` when set, overriding `SetError()`'s default-derived value for that call only; assign its `Message` field into `Error.Text` when set (replacing `CustomErrorCode`'s one legitimate use, Decision 5); and assign its `Data` member into `Error.Data` (Decision 1) — instead of (or alongside) whatever Decision 3 already does with the method's normal response object. Because this is an ordinary reference out-parameter, `ProxyStubGenerator` marshals it with the same already-verified unconditional read/write behavior as any other `@out` parameter (Context above) — no COM-RPC change needed here either.

**This is deliberately not free**: appending a parameter to an existing `virtual` interface method changes its C++ ABI (vtable slot/signature) — every consumer of that interface (the plugin shell's generated glue, the OOP implementation, any direct COM-RPC client) must be rebuilt against the new signature together, and the interface's `@json` version should be bumped to reflect the contract change, following Thunder's normal interface-evolution conventions. This is a per-method, per-interface-owner decision with a real cost, not a JsonGenerator-regeneration-only change like Decisions 2/3 — so it is offered as an **explicit, opt-in extension**, not applied to any existing method by this proposal.

*Alternative considered*: making `DATA` a true, generator-erased template parameter on the interface method itself (rather than each method instantiating its own concrete `ErrorDetail<ConcreteType>`). Rejected — COM-RPC virtual interfaces require a fixed, concrete ABI per method; there is no type erasure boundary that would let a genuinely generic/unbounded `DATA` cross a COM-RPC call. `ErrorDetail<DATA>` is a reusable *wrapper pattern*, but each adopting method still fixes a concrete `DATA` at the interface-header level, exactly like any other COM-RPC parameter type.

## Example: plugin usage, end to end (real interface: `IAccount`)

`GetLastCheckoutResetTimeResult` is not new API — it is the existing `@out` struct already declared today in the plugin's COM-RPC interface header, which `JsonGenerator` parses directly (Context above) to produce the generated response class:

```cpp
// entservices-apis/apis/Account/IAccount.h — existing interface, unchanged by this proposal
struct EXTERNAL IAccount : virtual public Core::IUnknown {
    enum { ID = ID_ACCOUNT };

    struct GetLastCheckoutResetTimeResult {
        uint64_t resetTime; // @text resetTime
    };

    // @text getLastCheckoutResetTime
    // @retval Core::ERROR_NONE Last Checkout reset time is successfully retrieved
    virtual Core::hresult GetLastCheckoutResetTime(GetLastCheckoutResetTimeResult& resetTime /* @out */) const = 0;
};
```

`JsonGenerator` emits a `GetLastCheckoutResetTimeResultData` (naming illustrative) `Core::JSON::Container`-derived class from that `@out` struct exactly as it does today (this proposal does not change class/field generation, only whether the already-generated glue *uses* a populated instance on the error path):

```cpp
// Generated by JsonGenerator into interfaces/json/JAccount.h — unchanged shape
class GetLastCheckoutResetTimeResultData : public Core::JSON::Container {
public:
    GetLastCheckoutResetTimeResultData()
        : Core::JSON::Container()
    {
        Add(_T("resetTime"), &ResetTime);
    }

public:
    Core::JSON::DecUInt64 ResetTime;
};
```

The real implementation, `AccountImplementation` ([entservices/entservices-account/plugin/AccountImplementation.h](../../../../entservices/entservices-account/plugin/AccountImplementation.h)), keeps its signature exactly as-is — it simply populates `resetTime` before returning a non-`ERROR_NONE` code on failure, just as it would on success:

```cpp
// AccountImplementation.cpp — signature and out-parameter are unchanged.
Core::hresult AccountImplementation::GetLastCheckoutResetTime(
    GetLastCheckoutResetTimeResult& resetTime /* @out */) const
{
    if (_store == nullptr) {
        // Populate the @out struct exactly like the success path would,
        // then return an error code instead of Core::ERROR_NONE.
        resetTime.resetTime = 0;

        return (Core::ERROR_UNAVAILABLE);
    }

    resetTime.resetTime = _store->LastCheckoutResetTime();
    return (Core::ERROR_NONE);
}
```

`AccountImplementation` may be running in-process or, per the "Where the JSON-RPC endpoint runs" subsection above, out-of-process behind the COM-RPC proxy that `Account::Initialize()` obtains via `_service->Root<Exchange::IAccount>(...)` — the implementation code above is identical either way, and COM-RPC marshals `resetTime` back unconditionally in both cases (verified above).

Today (before this change), the JsonGenerator-emitted lambda inside `JAccount.h` (invoked from the `Account` plugin shell's `IDispatcher::Invoke`) discards the populated `resetTime` once a non-`ERROR_NONE` code is returned (Decision 3a), producing:

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "error": {
        "code": -32603,
        "message": "Unavailable"
    }
}
```

After this change (once `JAccount.h` is regenerated with the updated `JsonGenerator`, per Decision 3a), the same `AccountImplementation` code — unmodified — produces:

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "error": {
        "code": -32603,
        "message": "Unavailable",
        "data": {
            "resetTime": 0
        }
    }
}
```

`code`/`message` are unchanged (Decision 5/6 — still derived from `SetError()`, unless an interface owner adopts Decision 7's opt-in `Code`/`Message` override), and `data` is simply the JSON serialization of the `@out` struct the implementation already populated — whether `AccountImplementation` runs in-process or out-of-process over COM-RPC (verified above). No changes are required in `IAccount.h`, `AccountImplementation`, `Account` (the plugin shell), or `ProxyStubGenerator`.

### Counter-example: `SetLastCheckoutResetTime` has no vessel for `data`

```cpp
// entservices-apis/apis/Account/IAccount.h — no @out parameter at all
virtual Core::hresult SetLastCheckoutResetTime(const uint64_t resetTime) = 0;
```

```cpp
// AccountImplementation.cpp — the only thing this method can ever return is a bare hresult.
Core::hresult AccountImplementation::SetLastCheckoutResetTime(const uint64_t resetTime)
{
    if (_store == nullptr) {
        return Core::ERROR_UNAVAILABLE;   // nothing to attach data to — there is no @out parameter
    }
    // ...
}
```

Because this method declares no `@out` parameter, `JsonGenerator` treats its result as `void` (Context above) and never generates a response object for its lambda to populate. Before and after this change, the error response is identical:

```json
{
    "jsonrpc": "2.0",
    "id": 43,
    "error": {
        "code": -32603,
        "message": "Unavailable"
    }
}
```

This is not a gap Decisions 2/3 fix by themselves — it is a real boundary of that (free, zero-migration) mechanism: structured `data` can only reflect a response object the plugin's interface already declares. Closing this specific gap requires `IAccount`'s maintainer to explicitly opt into Decision 7 — e.g.:

```cpp
// Illustrative opt-in, NOT applied by this proposal — IAccount's own maintainer would make this change.
virtual Core::hresult SetLastCheckoutResetTime(
    const uint64_t resetTime,
    Core::OptionalType<Core::JSONRPC::ErrorDetail<SetLastCheckoutResetTimeErrorData>>& error /* @out @error */) = 0;
```

after which `AccountImplementation::SetLastCheckoutResetTime` could set `error.Value().Data.reason = ...` (and, optionally, `error.Value().Code = ...`/`error.Value().Message = ...` to override the wire `code`/`message` for this one call) before returning an error code, and the response would gain a `data` member — plus a call-specific `code`/`message` if those were also set — at the cost of an `IAccount` interface/ABI version bump (Decision 7), not something this proposal does to `IAccount` on its own.

### Prior art: no dedicated inner "code" field in comparable JSON-RPC-based protocols

Two widely-deployed, independently-implemented JSON-RPC 2.0-based protocols were checked for how they split "broad, framework-level code" from "rich, caller-specific detail" — both validate the `ErrorDetail<DATA>` shape (`Message` + free-form `Data`, no dedicated `Code`) adopted above:

- **Language Server Protocol** (verified against the official 3.17 specification, `ResponseError`/`ErrorCodes`): `ResponseError { code: integer; message: string; data?: LSPAny }` — `data` is `LSPAny`, a fully free-form value with no framework-standardized inner "code" field. `ErrorCodes` is a single, framework-owned, closed enum (the JSON-RPC-reserved range `-32700..-32603`, a reserved block `-32099..-32000`, and an LSP-specific block `-32800..-32803` such as `ContentModified`, `RequestCancelled`) — every request type that wants extra typed error context (e.g. `InitializeError.retry: boolean`) puts it inside `data`, never as a second "code" sibling.
- **Ethereum JSON-RPC** (verified against EIP-1474): `error { code, message }` uses a similarly small, spec-owned code table (`-32700..-32006`), explicitly including a `-32000`-range block reserved for "non-standard"/implementation-specific errors — again a single owned code space, not a per-plugin one. This spec is implemented independently by multiple unrelated client codebases (Geth, the then-Parity/OpenEthereum client, Aleth), the closest analogue here to Thunder's many independent RDK plugin authors, and none of them mint their own competing "code" namespace inside the error object — app/implementation-specific detail is carried in `data` instead (e.g. revert reasons on `eth_call` failures).

Neither protocol gives every domain/plugin its own coordinated numeric "code" range at the wire level — the pattern in both is: one small, centrally-owned `code` enum, plus an open `data` payload where a caller-specific discriminator (if wanted) is just an ordinary field the caller defines, not a framework-blessed slot.

**Update — Thunder team decision**: despite this precedent, the team decided to add a dedicated, opt-in `Code` field to `ErrorDetail<DATA>` anyway (see Decision 7 above), specifically so a plugin's exact subsystem-native code (e.g. `ContentProtection`'s SecManager-derived `21002`) can appear verbatim on the wire without requiring clients to separately extract it from `Data`. This is a deliberate, documented divergence from the LSP/Ethereum pattern above, deliberately scoped narrowly — per-method opt-in, only takes effect when the interface owner both adopts `ErrorDetail<DATA>` and explicitly sets `Code` for a given call — to limit the collision/ambiguity risk that originally motivated the recommendation against it.

### Real-world worked example: `IContentProtection` (the plugin that actually motivates this proposal)

`Exchange::IContentProtection` ([entservices-cpc-apis/apis/ContentProtection/IContentProtection.h](../../../../entservices-cpc-apis/apis/ContentProtection/IContentProtection.h)) and its implementation ([entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h](../../../../entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h)/`.cpp`) is a real, in-tree instance of exactly the problem this proposal solves — not a hypothetical. Verified facts about its current behavior:

- `OpenDrmSession`/`UpdateDrmSession` already declare `@retval` doc-comments listing 20+ app-specific DRM/watermarking failure codes (`21002` Invalid aspect dimension, `22001` DRM general failure, `23001` Watermark general failure, etc.). These are not Thunder framework codes — they come from the SecManager subsystem the plugin talks to.
- The real failure path, `Implementation::OpenDrmSession` ([ContentProtection.h](../../../../entservices-cpc/entservices-contentprotection/plugin/ContentProtection.h)):
  ```cpp
  result = _parent._secManager->Invoke<JsonObject, JsonObject>(
      OpenSessionTimeout, _T("openPlaybackSession"), out, in);
  if (result == Core::ERROR_NONE) {
      if (!in["success"].Boolean()) {
          auto context = in["secManagerResultContext"].Object();
          result = SecManagerStatus(context["class"].Number(), context["reason"].Number());
      } else {
          sessionId = in["sessionId"].String();
          in.ToString(response);
          // ...
      }
  }
  return result;
  ```
  `SecManagerStatus()` computes and returns the raw numeric code (e.g. `21002`) directly as the method's `Core::hresult` — it is **not** wrapped in `Core::CustomCode()` (confirmed: `Core::CustomCode`/`Core::SetCustomCodeToStringHandler` do not appear anywhere in this plugin). On this failure branch, neither `@out` parameter (`sessionId`, `response`) is populated — they stay empty, exactly like the generic `SomePlugin`/`21001` example above.
- The plugin ships its own hand-rolled `CustomCodeToString()` free function (`ContentProtection.cpp`) with correct human text for every one of these codes (e.g. `21002` → `"Invalid content aspect dimension parameters"`), but — confirmed by inspection — it is never passed to `Core::SetCustomCodeToStringHandler()`. It is used exactly once, inside a second, bespoke COM-RPC interface the plugin also implements, `Exchange::IErrorToString { ErrorToString(int32_t code, string& result) }`, registered as its own `CodeToString` object. A caller that already has the raw numeric code (typically a COM-RPC client calling `IContentProtection` directly, in-process with the code as a plain return value) must make a **second, separate round trip** to `IErrorToString::ErrorToString` to resolve it to text — the code and its text are never attached to the original call's own response.
- Consequently, today, over the JSON-RPC surface actually exposed by this plugin (`org.rdk.ContentProtection.1.openDrmSession`), a real `21002` failure produces (`ApplicationErrorCodeBase = -31000`):
  ```json
  {
      "jsonrpc": "2.0",
      "id": 7,
      "error": { "code": -52002, "message": "Undefined Thunder error code: 21002" }
  }
  ```
  The JSON-RPC client gets neither the original `21002` nor its meaning — both are only reachable via the separate `IErrorToString` COM-RPC interface, which a plain JSON-RPC client has no direct path to invoke with the right argument, since it never received `21002` in the first place.

**Decisions 2/3 alone already fix this for `OpenDrmSession`/`UpdateDrmSession`/`CloseDrmSession` — no interface/ABI change, because these methods already declare a `response`/`response`/`response` `@out` parameter.** The plugin's own maintainers could change only `ContentProtection.cpp`, reusing the `CustomCodeToString()` table it already has, to populate that parameter on the error path instead of leaving it empty:
```cpp
} else {
    auto context = in["secManagerResultContext"].Object();
    result = SecManagerStatus(context["class"].Number(), context["reason"].Number());
    JsonObject err;
    err["code"] = static_cast<int32_t>(result);
    err["reason"] = ::CustomCodeToString(static_cast<int32_t>(result));
    err.ToString(response);   // now non-empty on error too — Decision 2/3 does the rest
}
```
producing, with zero interface/ABI change:
```json
{
    "jsonrpc": "2.0",
    "id": 7,
    "error": {
        "code": -52002,
        "message": "Undefined Thunder error code: 21002",
        "data": { "code": 21002, "reason": "Invalid content aspect dimension parameters" }
    }
}
```

**Decision 7 is what closes the remaining gap — methods with no `@out` parameter at all**, e.g. `SetDrmSessionState`:
```cpp
// entservices-cpc-apis/apis/ContentProtection/IContentProtection.h — real signature today, no @out at all:
virtual Core::hresult SetDrmSessionState(const string& sessionId, State sessionState) = 0;
```
Its real implementation today only ever returns a bare code (`NoSuchSession = 21009`, or `Core::ERROR_GENERAL` from `SetPlaybackSessionState`'s own failure) with no way to attach `data`, since there is no response object to reuse. An interface owner opting into Decision 7 would change this to:
```cpp
// Illustrative opt-in — NOT applied by this proposal; IContentProtection's own maintainer would make this
// change explicitly, bumping the interface's @json version, since it changes the method's ABI.
virtual Core::hresult SetDrmSessionState(
    const string& sessionId, State sessionState,
    Core::OptionalType<Core::JSONRPC::ErrorDetail<DrmSessionErrorData>>& error /* @out @error */) = 0;
```
letting `Implementation::SetDrmSessionState` set, on the `NoSuchSession` path:
```cpp
error = Core::JSONRPC::ErrorDetail<DrmSessionErrorData>{
    NoSuchSession, string(::CustomCodeToString(NoSuchSession)), { NoSuchSession } };
return NoSuchSession;
```
which now produces, for this method only after the ABI bump — note `error.code` is now the plugin's own `21009` verbatim, not the framework's `-52009` offset, because `Code` was set:
```json
{
    "jsonrpc": "2.0",
    "id": 9,
    "error": {
        "code": 21009,
        "message": "Invalid session identifier",
        "data": { "code": 21009 }
    }
}
```
Note this reuses the plugin's own existing `CustomCodeToString()` table verbatim for `Message`, and its own SecManager-native numeric scheme verbatim for `Code` — adopting Decision 7 requires no new lookup table or code range, only wiring what already exists directly into each call's own response instead of exposing it solely through the separate `IErrorToString` round trip. `IErrorToString` itself is not made redundant by this: `INotification::Status.failureReason` is delivered on a fire-and-forget event (`WatermarkStatusChanged`), which has no JSON-RPC `error` object to attach `data` to at all, so an out-of-band code-to-text lookup remains the only option for that specific path. It does become redundant for ordinary request/response failures on methods that adopt Decisions 2/3/7 directly.

## Risks / Trade-offs

- **[Risk]** A legacy hand-written handler's plain-text error message happens to be valid, brace-delimited JSON (e.g. it literally returns `"{}"` or a JSON-looking string) and gets misclassified as `data` instead of `text`.
  **Mitigation**: Requiring a full, valid JSON **object** parse (not just "looks stringy") makes this vanishingly rare for natural-language text; call out as a documented, low-probability behavior change in release notes rather than a hard migration requirement.
- **[Risk]** Plugins put sensitive/internal diagnostic information (stack traces, file paths, tokens) into `data` since it's now easy to populate, increasing information disclosure to JSON-RPC clients (OWASP A01/A09-relevant).
  **Mitigation**: Document that `data`, like `message`, is part of the response returned to whatever token/session made the call, and that plugin authors are responsible for not including secrets; add this to code-review guidance and the plugin authoring doc produced by this change.
- **[Risk]** Existing external tooling that does strict schema validation on JSON-RPC error objects (expects only `code`/`message`) starts seeing an unexpected `data` member on calls that opt in.
  **Mitigation**: `data` is additive per JSON-RPC 2.0 and only appears when a plugin explicitly populates its response object on an error path — it never appears on calls that don't opt in; called out in release notes.
- **[Trade-off]** The "JSON object → data, else → text" content-sniffing rule (raw/hand-written path only, Decision 2) is implicit rather than an explicit typed API. Simpler and fully backward compatible, but relies on a convention rather than the compiler enforcing intent. Recorded as an open question below in case a future explicit-API iteration is warranted.
- **[Risk/Limitation]** Methods with no `@out` parameter at all (e.g. `IAccount::SetLastCheckoutResetTime`) gain no `data` capability from Decisions 2/3 alone — there is no response object to populate, by construction. **Mitigation**: Decision 7 provides an explicit, opt-in path (a dedicated error-detail out-parameter) for interface owners who want one, but adopting it is their own decision and carries a real interface/ABI-version cost — it is not automatic. Documented in Non-Goals, the worked counter-example, and the plugin-authoring docs so neither the gap nor the opt-in cost is mistaken for an oversight.
- **[Risk/Trade-off, Decision 7]** The opt-in error-detail parameter changes an existing interface method's C++ signature — an ABI-affecting change requiring every consumer of that interface (plugin shell glue, OOP implementation, direct COM-RPC clients) to be rebuilt together, and the interface's `@json` version to be bumped per Thunder's normal interface-evolution rules. **Mitigation**: kept strictly opt-in and per-method; this proposal does not apply it to any existing interface, and documentation will state the ABI cost plainly so interface owners adopt it deliberately, not by accident.
- **[Risk, Decision 7]** Letting an opted-in method override `error.code` verbatim via `ErrorDetail<DATA>::Code` reintroduces `CustomErrorCode`'s original collision risk: a plugin-chosen value could numerically collide with one of Thunder's own fixed framework codes, or with another plugin's chosen value, with no central registry enforcing distinctness. **Mitigation**: scoped narrowly — the override only takes effect for a call to a method whose interface owner both opted into `ErrorDetail<DATA>` and explicitly set `Code`; plugin-authoring documentation (Task 7) will recommend interface owners pick values outside Thunder's own reserved `ERROR_CODES` range and document their chosen range, the same guidance that would have applied to `CustomErrorCode` callers.
- **[Risk]** Removing `CustomErrorCode` (Decision 5) touches public, `EXTERNAL`-exported Core symbols (`Core::CustomCode`, `Core::IsCustomCode`, `Core::SetCustomCodeToStringHandler`) and a documented, externally-loadable library contract (`ICustomErrorCode.h`'s `CustomCodeToString`) whose out-of-tree usage cannot be fully ruled out by searching this workspace. **Mitigation**: a clear, explicit breaking-change release note naming the removed symbols (not a silent removal); confirmed zero in-tree callers in Thunder/entservices*/ThunderTools before removal; the one real related pattern found in-tree (`ContentProtection`'s `IErrorToString`, see Context) does not use this mechanism at all and is unaffected by its removal.

## Migration Plan

- Purely additive, server-side change; no client-facing opt-in mechanism or feature flag is needed since population is per-call and per-plugin.
- **Cross-repository coordination required**: this change spans two repositories — `Thunder` (wire model, `PluginServer.h` consolidation, hand-written-path fix) and `ThunderTools` (JsonGenerator fix, Decision 3a). The `ThunderTools` change should land and tag/version first (or together), since `Thunder`'s content-sniff rule (Decision 2) is what turns a regenerated plugin's now-preserved response object into `Error.Data` — the two halves are complementary but independently buildable/testable.
- Existing plugins gain the capability only after their generated `J*.h` header is regenerated with the updated `JsonGenerator.py`; until regenerated, they keep exactly today's behavior (no `data`, response cleared on error) — this makes rollout gradual and per-plugin rather than a flag day.
- Implementation order (mirrors `tasks.md`): (1) `Message::Info.Data` type change, (2) JsonGenerator fix in `ThunderTools` (Decision 3a) + regenerate a representative sample plugin to validate, (3) hand-written-path fix in `Source/core/JSONRPC.h` (Decision 3b), (4) `PluginServer.h` consolidation of the content-sniff rule, (5) apply the same rule at the remaining `output`-into-error call sites and the `JSONRPCErrorAssessor` hook, (6) tests (including an OOP functional test), (7) plugin-authoring documentation update.
- No data migration needed (nothing persists error responses). No rollback complexity beyond reverting the commit(s) — the change does not alter any on-disk format or IPC/COM-RPC wire ABI (verified: `ProxyStubGenerator`'s frame layout is unchanged, only which fields the generated JSON-RPC glue chooses to copy into a string is changing).
- Add regression tests (Thunder's `Tests/` tree) asserting: `data` is absent from the wire when unset (non-regression on every existing passing JSON-RPC test), a typed method that sets response fields and returns an error surfaces them under `error.data`, a legacy plain-text `output` on error still lands in `error.message`, and — to lock in the OOP invariant this design depends on — an **out-of-process** plugin method that populates its output parameters and returns an error surfaces identical `error.data` to an equivalent in-process plugin (regression-proofing the "COM-RPC already preserves out params on error" assumption this design relies on).

## Open Questions

- Should COM-RPC interface headers (or the legacy `.json` IDL) gain an explicit way to declare an error `data` shape per method/error (stronger typing, generated docs), as a follow-up to this content-sniffing approach?
- Should interface headers support an explicit per-method opt-out annotation (e.g. an `@errordata:omit`-style tag, mirroring the existing `@json:omit` convention) for methods whose response struct should never be echoed back as `error.data`, in case some existing plugins consider their output fields sensitive when returned via an error path?
- Would `JSONRPCErrorAssessorTypes` benefit from an explicit `Core::JSON::Variant&` out-parameter in a future, additive overload, instead of relying solely on content-sniffing the response string?
- ~~Is there a parallel need to carry structured detail across COM-RPC (`Core::hresult`/`COM_ERROR`) boundaries before they are translated into JSON-RPC errors, and if so, is that a separate proposal?~~ **Resolved during design**: no. Direct inspection of `ThunderTools/ProxyStubGenerator/StubGenerator.py` confirms COM-RPC stub/proxy code already marshals all "out" parameters unconditionally, regardless of the returned `hresult` — the only discard happens downstream, in JsonGenerator's generated glue (Decision 3a) and the hand-written `Register<>` path (Decision 3b), both addressed by this design.
- Does exposing `data` on any capability warrant a JSON-RPC interface/version bump, or is this treated as transport-level and version-agnostic (current assessment: version-agnostic, since it's purely additive per spec)?
