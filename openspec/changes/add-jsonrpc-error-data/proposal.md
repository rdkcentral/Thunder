## Why

Thunder's JSON-RPC error object (`Core::JSONRPC::Message::Info` in [Source/core/JSONRPC.h](../../../Source/core/JSONRPC.h#L64)) implements only `code` and `message` from the [JSON-RPC 2.0 error object spec](https://www.jsonrpc.org/specification#error_object). The optional `data` member — meant to carry structured, application-defined error detail — is declared as a plain `Core::JSON::String` and is never populated anywhere in the codebase. As a result, plugins have no spec-compliant way to return machine-readable error detail (validation failures, offending parameter names, nested COM-RPC error chains, etc.) alongside a JSON-RPC error.

In the absence of a real `data` channel, plugin authors and RDK integrators have improvised workarounds that the Thunder team wants to eliminate:
- Overloading `Error.Text` (the `message` field) with ad-hoc detail strings (`response->Error.Text = output` in [Source/Thunder/PluginServer.h](../../../Source/Thunder/PluginServer.h#L1112)), mixing human-readable summary and machine-readable detail in one untyped string.
- Returning `Core::ERROR_NONE` (a JSON-RPC success) with error detail smuggled into the `result` object instead, which violates the spec and breaks generic JSON-RPC client error handling.
- The existing `CustomErrorCode` mechanism ([Source/core/ICustomErrorCode.h](../../../Source/core/ICustomErrorCode.h), [Source/core/Errors.h](../../../Source/core/Errors.h#L33-L46)) only maps a single global, process-wide custom code to a replacement *string* for `message` — it cannot express structured `data`, cannot carry per-call context, and only supports one registered handler at a time ("can only set one, not multithreaded safe", per its own source comment).

RDK has requested first-class, spec-compliant error `data` support so plugins can surface structured diagnostic information to clients without abusing `message` or `result`. The team wants **one** coherent, explicit, call-specific mechanism as the single source of truth for custom error detail — including removing `CustomErrorCode` entirely — rather than maintaining it alongside a new, parallel feature. (Verification note: no in-tree caller of `Core::CustomCode()`/`Core::IsCustomCode()`/`Core::SetCustomCodeToStringHandler` was found in `Thunder`/`entservices*`/`ThunderTools`; the one closely related real-world need found in-tree, `entservices-cpc/entservices-contentprotection`'s `IErrorToString`, does not use this mechanism at all and independently reinvented its own per-plugin equivalent — see design.md for the full evidence trail.)

**CCB revision**: an earlier version of this proposal made structured `data` "free" for any method that already declared an `@out` response parameter, by having the JSON-RPC glue stop discarding that parameter's value on the error path, and reserved an explicit opt-in parameter only for methods with no `@out` parameter at all. CCB review rejected that split: whether a method happens to already have an `@out` parameter is unrelated to whether its maintainer wants that parameter's contents exposed as error detail, and doing so implicitly would silently leak response-object contents into `error.data` the moment a plugin's glue was regenerated. This revision replaces that mechanism entirely — see "What Changes" below and design.md's revision note for the full list of CCB comments addressed.

## What Changes

- Introduce a structured, spec-compliant `data` member on the JSON-RPC error object. It is populated **only** when a method explicitly opts in via the new `Core::ErrorDetail<DATA>` parameter described below — an existing `@out`/response parameter is never inspected or echoed into `error.data`, for any method, under any circumstance.
- Introduce `Core::ErrorDetail<DATA>` (plain `Core::` namespace — not `Core::JSONRPC`), a reusable wrapper an interface owner can add as an extra out-parameter to **any** method, regardless of whether that method already declares other `@out` parameters:
  ```cpp
  namespace Core {
      template<typename DATA>
      struct ErrorDetail {
          Core::OptionalType<int32_t> Code;
          Core::OptionalType<string> Message;
          Core::OptionalType<DATA> Data;
      };
  }
  ```
  All three members are independently optional, so a method can set only `Message` (e.g. to override the default text) without providing a `Data` payload. `JsonGenerator` recognizes the parameter purely structurally, by its C++ type name — the same way it already recognizes `Core::JSONRPC::Context` — with no new `@`-annotation tag required.
- When a method sets `Code`/`Message`/`Data` on this parameter before returning an error: `Code` (if set) overrides `error.code`, taking precedence over `SetError()`'s default-derived value for that call only; `Message` (if set) overrides `error.message`; `Data` (if set) becomes `error.data`. Any left unset behave exactly as they do for a method that hasn't opted in (derived from `SetError()`, or omitted).
- This is **not** applied to any existing interface by this proposal. Adopting `Core::ErrorDetail<DATA>` on a method is an explicit, per-method, ABI-affecting interface change (an added parameter) made deliberately by that method's own interface owner — uniformly, whether or not the method already has other `@out` parameters.
- **Remove `CustomErrorCode` entirely** as the single source of truth for call-specific custom error detail: delete `Core::CustomCode()`/`Core::IsCustomCode()`/`CustomCodeToStringHandler`/`SetCustomCodeToStringHandler` from `Source/core/Errors.h`/`Errors.cpp`, delete `Source/core/ICustomErrorCode.h`, delete the `CustomCodeLibrary` dynamic-library-loading class from `Source/Thunder/PluginHost.cpp`, and simplify `Info::SetError()` to drop its custom-code branch. Its two legitimate capabilities — a call-specific replacement `message`, and an app-chosen value appearing verbatim as `error.code` — are replaced (not dropped) by `Core::ErrorDetail<DATA>`'s `Message`/`Code` fields, now scoped per-method/per-call rather than a global, process-wide handler.
- `JSONRPCErrorAssessor` ([Source/common/JSONRPC.h](../../../Source/common/JSONRPC.h#L1918-L1971)) is **not modified** by this proposal. Per CCB review, this hook is intended for future deprecation, not extension; it is annotated as deprecated (documentation only) as a small courtesy within this change.
- The exact C++ representation needed for `Message::Info::Data` to carry an arbitrary JSON value (as opposed to a plain string) is left open for implementation (see design.md Decision 3 / Open Questions) — this proposal commits to the required wire behavior (a real JSON value when set, omitted entirely when not) without prematurely fixing the representation.

## Capabilities

### New Capabilities
- `jsonrpc-error-data`: The explicit, opt-in `Core::ErrorDetail<DATA>` mechanism — its shape, structural recognition by `JsonGenerator`, wire behavior for `code`/`message`/`data`, and the guarantee that no other `@out` parameter is ever used as a source of error detail.

### Modified Capabilities
- (none — `CustomErrorCode` and the JSON-RPC dispatch path are existing implementation, not previously specified capabilities; this proposal specifies `jsonrpc-error-data` as the first formal spec covering this area. `CustomErrorCode`'s removal is documented as an implementation change in design.md/tasks.md rather than an openspec capability delta.)

## Impact

- **Affected code**:
  - `Source/core/JSONRPC.h` — `Message::Info` (error object model), `Message::Info::SetError()`.
  - `Source/core/Errors.h` / `Errors.cpp`, `Source/core/ICustomErrorCode.h` — **deleted**: `Core::CustomCode()`, `Core::IsCustomCode()`, `CustomCodeToStringHandler`/`SetCustomCodeToStringHandler`, and `ICustomErrorCode.h` in full.
  - `Source/Thunder/PluginHost.cpp` — **deleted**: the `CustomCodeLibrary` dynamic-library-loading class and its associated config wiring (~lines 152–206).
  - `Source/common/JSONRPC.h` — `JSONRPCErrorAssessor`: documentation-only deprecation annotation; no capability change.
  - `docs/utils/customcodes.md` — replaced with documentation for `Core::ErrorDetail<DATA>`, including a migration note for the removed API.
  - **`ThunderTools/JsonGenerator/source/header_loader.py` / `rpc_emitter.py`** (sibling repository) — structural recognition of the new `Core::ErrorDetail<DATA>` parameter type: exclude it from the generated success-result schema, and emit error-path assignment of `Code`/`Message`/`Data` into the wire `error` object when set. No change to how existing `@out`/response parameters are handled on the error path (they remain untouched/discarded exactly as today).
  - `ThunderTools/ProxyStubGenerator/` — inspected, **no change required** (COM-RPC stub/proxy code already marshals output parameters unconditionally, regardless of the returned `hresult`); the new parameter marshals like any other `@out` parameter.
- **API impact**: New, purely additive public API surface (`Core::ErrorDetail<DATA>`) for interface owners who choose to adopt it, method by method — no change to any method that doesn't. **Intentional breaking change**: removal of the public `EXTERNAL` Core symbols `Core::CustomCode`, `Core::IsCustomCode`, `Core::SetCustomCodeToStringHandler`, and the `ICustomErrorCode.h` externally-loadable library contract — call out explicitly and prominently in release notes, since this contract is designed to be implementable outside this workspace and cannot be proven unused purely by in-tree search.
- **Wire format impact**: JSON-RPC error responses may now include a `data` member, and/or a `code`/`message` that diverges from the framework default, but **only** on calls to a method whose interface owner explicitly adopted `Core::ErrorDetail<DATA>` — additive per spec; downstream consumers doing strict schema validation on error objects should be noted in release notes.
- **Dependencies**: No new external dependencies within `Thunder` itself. Introduces a **cross-repository dependency on `ThunderTools`**: this change cannot be considered complete until `JsonGenerator` recognizes `Core::ErrorDetail<DATA>` and interfaces that choose to adopt it regenerate their JSON-RPC glue.
