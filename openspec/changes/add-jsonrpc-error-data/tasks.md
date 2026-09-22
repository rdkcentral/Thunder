## 1. Core wire model: `Message::Info.Data`

- [ ] 1.1 Change `Core::JSONRPC::Message::Info::Data` in [Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h) from `Core::JSON::String` to `Core::JSON::Variant`, updating the copy/move constructors and assignment operators of `Info` accordingly
- [ ] 1.2 Verify (add/extend a unit test in `Tests/core`) that a default-constructed `Info` still omits `data` from `ToString()` output, matching today's behavior
- [ ] 1.3 Add unit tests asserting `Info.Data` round-trips each `Core::JSON::Variant` shape relevant to JSON-RPC (`object`, `array`, `string`, `number`, `boolean`, `null`) through `ToString()`/`FromString()`

## 2. JsonGenerator fix (`ThunderTools` repo) — primary generated-code path

- [ ] 2.1 In `ThunderTools/JsonGenerator/source/rpc_emitter.py` ("Emit result handling and serialization to JSON data" section, ~line 1454), remove the `if (errorCode_ == Core::ERROR_NONE)` gate around the per-field response-copy loop so populated output fields are always copied into the JSON response object, regardless of the returned error code
- [ ] 2.2 Regenerate a representative sample plugin's `J*.h` header (e.g. via `JsonGenerator.py` against one of the `.json` IDL files under `ThunderInterfaces`/`entservices-apis`) and diff the generated C++ to confirm only the intended gate removal changed, with no other generated-code drift
- [ ] 2.3 Add/update `ThunderTools` test coverage (its `tests/` tree or the existing `ProxyStubFunctionalTests` CI job) asserting a generated method that populates its response object and returns a non-`ERROR_NONE` code produces a response string with those fields present (today it produces an empty/cleared response)
- [ ] 2.4 Coordinate versioning/tagging between `Thunder` and `ThunderTools` so the `Source/core/JSONRPC.h` content-sniff rule (Task 3) and this generator fix ship together in release notes, since neither half is independently sufficient for generated plugins to gain the capability

## 3. Hand-written invocation path (secondary, `Source/core/JSONRPC.h`)

- [ ] 3.1 Update `InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD, ...>` so the OUTBOUND object is serialized into `result` whenever `outbound.IsSet() == true`, regardless of the returned `code` (stop the unconditional `result.clear()` on error) — for hand-written `Register<>` call sites that bypass JsonGenerator
- [ ] 3.2 Apply the same fix to the outbound-only `InternalRegisterImpl<OUTBOUND, METHOD, ...>` overload
- [ ] 3.3 Add unit tests covering: (a) OUTBOUND populated + success code → unchanged existing behavior, (b) OUTBOUND populated + error code → serialized into `result`, (c) OUTBOUND left unset + error code → `result` remains empty (no regression)

## 4. Content-sniff rule and response assembly

- [ ] 4.1 Implement a shared helper (e.g. in `Source/core/JSONRPC.h` or `Source/common/JSONRPC.h`) that, given a non-empty `output` string on an error path, attempts a strict JSON-object parse and returns either a populated `Core::JSON::Variant` (object) or an indication that it should be treated as plain text
- [ ] 4.2 Update [Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L1108-L1114) (synchronous invoke error path) to use the helper: JSON object → `response->Error.Data`; otherwise → `response->Error.Text` (existing behavior)
- [ ] 4.3 Apply the same helper at the parse-failure call sites in [Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L4776) and [Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L4924) for consistency
- [ ] 4.4 Confirm [Source/websocket/JSONRPCLink.h](../../../Source/websocket/JSONRPCLink.h#L395-L414) (async abort/timeout paths) are left untouched — these remain `code`+`message` only, no `data`

## 5. `JSONRPCErrorAssessor` parity

- [ ] 5.1 Update the call site(s) that finalize the response after `JSONRPCErrorAssessor`'s errorhandler runs ([Source/common/JSONRPC.h](../../../Source/common/JSONRPC.h#L1918-L1971)) to apply the same content-sniff helper from Task 4.1 to the assessor's resulting response string
- [ ] 5.2 Add a test with a mock error assessor callback that emits a JSON object and verify it surfaces as `error.data` in the final response

## 6. Remove legacy `CustomErrorCode` mechanism (consolidation, Decision 5)

- [ ] 6.1 Confirm zero in-tree callers before removal: re-run a search for `Core::CustomCode`, `Core::IsCustomCode`, `Core::SetCustomCodeToStringHandler`, `CustomCodeToString`, `ICustomErrorCode` across `Thunder`, `entservices*`, and `ThunderTools` at the time of removal (baseline already found zero official-mechanism callers outside `Source/core` itself; `entservices-cpc/entservices-contentprotection`'s `IErrorToString` is a separate, self-built mechanism that does not call into this API and is unaffected)
- [ ] 6.2 Delete `Core::CustomCode()`, `Core::IsCustomCode()`, `CustomCodeToStringHandler`, and `SetCustomCodeToStringHandler` from [Source/core/Errors.h](../../../Source/core/Errors.h#L33-L46) / [Source/core/Errors.cpp](../../../Source/core/Errors.cpp)
- [ ] 6.3 Delete [Source/core/ICustomErrorCode.h](../../../Source/core/ICustomErrorCode.h) in full, and remove it from `Source/core/CMakeLists.txt`/`core.vcxproj` header lists
- [ ] 6.4 Delete the `CustomCodeLibrary` class and its associated config/wiring from [Source/Thunder/PluginHost.cpp](../../../Source/Thunder/PluginHost.cpp#L152-L206)
- [ ] 6.5 Remove the `IsCustomCode()` branch from `Info::SetError()` in [Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L194) so every non-well-known `Core::hresult` falls through to the existing deterministic default (`ApplicationErrorCodeBase` offset) mapping unconditionally
- [ ] 6.6 Remove the `IsCustomCode()`-checking branches inside `Core::ErrorToString()` and `Core::ErrorToStringExtended()` ([Source/core/Errors.cpp](../../../Source/core/Errors.cpp#L104-L129)) — after Task 6.2 these branches are dead code; the functions keep only their existing `_bogus_ErrorToString`/`"Undefined Thunder error code: <N>"` fallback path
- [ ] 6.7 Add a regression test (traced in design.md's worked example) asserting that an arbitrary, non-well-known `Core::hresult` (e.g. `21001`) produces `error.code == ApplicationErrorCodeBase - 21001` and `error.message == "Undefined Thunder error code: 21001"` after removal — confirms no functional regression for "gets a collision-free code", only for "gets that exact value verbatim / a meaningful default message", and documents the new fallback text a plugin author would need to override via Task 9's `Message` field if they want something better
- [ ] 6.8 Replace [docs/utils/customcodes.md](../../../docs/utils/customcodes.md) with documentation for the new opt-in `ErrorDetail<DATA>`/`Message` mechanism (Task 9), including a short "if you used `CustomErrorCode` for per-call text, use this instead" migration note
- [ ] 6.9 Add a prominent breaking-change release note naming the removed public `EXTERNAL` symbols (`Core::CustomCode`, `Core::IsCustomCode`, `Core::SetCustomCodeToStringHandler`, `ICustomErrorCode.h`'s `CustomCodeToString` contract), since this was a documented, externally-loadable library contract whose out-of-tree usage cannot be ruled out by in-tree search alone

## 7. Documentation

- [ ] 7.1 Add/update JSON-RPC plugin-authoring documentation (e.g. under `docs/`) explaining: how to populate `error.data` from a typed method, that call-specific `code`/`message` overrides and structured error detail for methods without a reusable response object are available via Task 9's opt-in `ErrorDetail<DATA>` parameter (the replacement for the removed `CustomErrorCode`, Task 6), that `Code`/`Message` are fixed fields on `ErrorDetail<DATA>` while `Data` is templated and defined per adopting interface, the security note about not putting sensitive information in `data`, the default limitation that methods with no `@out` parameter (e.g. `IAccount::SetLastCheckoutResetTime`) cannot surface `data` without opting into Task 9's dedicated error-detail parameter, and the ABI/interface-version cost of doing so
- [ ] 7.2 Add a short migration/release note describing the (rare) behavior change for handlers whose plain-text error output happens to parse as a JSON object, and the fact that generated plugins need a `JsonGenerator` regeneration (Task 2) to gain the capability
- [ ] 7.3 Document, in the plugin-authoring notes, that out-of-process (COM-RPC-backed) plugins behave identically to in-process ones for this feature with no extra steps required by the plugin author (verified: COM-RPC already preserves output parameters on error)

## 8. Validation

- [ ] 8.1 Run the full existing JSON-RPC test suite in `Tests/` to confirm no regressions in error responses that don't populate `data`
- [ ] 8.2 Add end-to-end coverage (client + server) exercising a real plugin method that returns structured `data` on error, confirming the serialized response matches the `jsonrpc-error-data` spec scenarios
- [ ] 8.3 Add end-to-end coverage for an **out-of-process** plugin method (COM-RPC-backed) that returns structured `data` on error, confirming parity with the in-process case (locks in the "COM-RPC preserves out params on error" invariant this design depends on)
- [ ] 8.4 Add coverage for a method whose single `@out` parameter is a bare scalar (not a struct), confirming JsonGenerator's default "collapsed" format still surfaces it as `error.data` using its own JSON shape (e.g. a bare number), not forced into an object
- [ ] 8.5 Add a regression test using a **void-response** method with no opted-in error-detail parameter (no `@out` parameter at all, e.g. modeled on `IAccount::SetLastCheckoutResetTime`) confirming its error response still contains only `code`/`message` with no `data` member after this change

## 9. Opt-in dedicated error-detail parameter (Decision 7) — separately adoptable, not applied to any existing interface by this proposal

- [ ] 9.1 Add `Core::JSONRPC::ErrorDetail<DATA>` (name illustrative) to `Source/core/` — a minimal wrapper type with `Core::OptionalType<int32_t> Code` (a per-call override of `error.code`, the replacement for `CustomErrorCode`'s verbatim-code capability), `Core::OptionalType<string> Message` (the replacement for `CustomErrorCode`'s per-call text override), and a `DATA Data` member, for interfaces to opt into. `DATA` is templated and defined per adopting interface (e.g. its own `SomeMethodErrorData` struct); `Code`/`Message` are fixed, non-templated fields shared by every instantiation
- [ ] 9.2 In `ThunderTools/JsonGenerator/source/header_loader.py`, recognize a parameter of this shape (structurally, mirroring the existing `Core::JSONRPC::Context` special-case) or an explicit `@error` tag, and exclude it from `BuildResult`'s success-result schema while recording it separately for `rpc_emitter.py`
- [ ] 9.3 In `ThunderTools/JsonGenerator/source/rpc_emitter.py`, emit an extra local `Core::OptionalType<...>` temporary for the recognized parameter, pass it by reference into the real interface call, and on the error path, if set: assign `Code` into `Error.Code` when present (overriding `SetError()`'s default-derived value for that call only), assign `Message` into `Error.Text` when present, and assign `Data` into `Error.Data`
- [ ] 9.4 Confirm (via a `ProxyStubGenerator` test, not a change) that this parameter marshals with the same already-verified unconditional read/write behavior as any other `@out` parameter
- [ ] 9.5 Add an isolated sample/test interface (not `IAccount` or any real production interface) exercising the full opt-in flow end to end, since this proposal does not modify any existing interface to use it — include cases where only `Data` is set, where `Message` is also set, and where `Code` is also set (asserting `error.code` equals the plugin-chosen value verbatim in the last case, and the framework's default-derived value in the first two)
- [ ] 9.6 Document, alongside Task 7's plugin-authoring docs, the ABI/interface-version cost of adopting this (existing consumers of the interface must be rebuilt together; the interface's `@json` version should be bumped) so adoption is a deliberate, informed choice per interface owner
- [ ] 9.7 Add plugin-authoring guidance (Task 7) recommending interface owners who set `Code` pick values outside Thunder's own reserved `ERROR_CODES` range and document their own chosen range, since overriding `error.code` verbatim reintroduces the same collision risk `CustomErrorCode` carried, now scoped per opted-in method instead of process-wide
