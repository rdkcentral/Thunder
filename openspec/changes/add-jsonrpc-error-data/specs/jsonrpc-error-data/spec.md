## ADDED Requirements

### Requirement: Error object exposes a structured `data` member
The JSON-RPC error object (`Core::JSONRPC::Message::Info`) SHALL expose `data` as a JSON value capable of representing any JSON-RPC-legal type (object, array, string, number, boolean, or null), not only a plain string.

#### Scenario: Data field accepts a structured object
- **WHEN** a method opted into `Core::ErrorDetail<DATA>` (see below) attaches a JSON object (e.g. `{"field": "value", "count": 3}`) as its `Data` value for a failed call
- **THEN** the serialized JSON-RPC response's `error.data` member SHALL contain that object with its nested structure preserved

### Requirement: `data` is omitted when not populated
The JSON-RPC error object SHALL omit the `data` member entirely from the serialized response when no method has set `Data` on an opted-in `Core::ErrorDetail<DATA>` parameter for that call, preserving current wire behavior for the overwhelming majority of calls.

#### Scenario: Unset data is not serialized
- **WHEN** a method invocation fails with an error code and no `Core::ErrorDetail<DATA>::Data` value was set for that call
- **THEN** the serialized JSON-RPC error object SHALL contain only `code` and `message`, with no `data` key present

#### Scenario: Existing plain-text error behavior is unchanged
- **WHEN** a hand-written handler returns a non-success code and writes a plain, non-JSON-object string into its response output (the historical convention)
- **THEN** that string SHALL be assigned to `error.message` exactly as before, and `error.data` SHALL remain absent from the response

### Requirement: `Core::ErrorDetail<DATA>` is the only mechanism that can populate `data` or override `code`/`message`
An interface owner MAY explicitly add `Core::ErrorDetail<DATA>` (wrapped in `Core::OptionalType<>`) as an additional out-parameter to any interface method, whether or not that method already declares other `@out`/response parameters. This parameter SHALL have three independently optional fields — `Code` (`Core::OptionalType<int32_t>`), `Message` (`Core::OptionalType<string>`), and `Data` (`Core::OptionalType<DATA>`, where `DATA` is templated and defined per adopting interface). When populated on an error path: `Code`, if set, SHALL override `error.code` for that call only; `Message`, if set, SHALL override `error.message` for that call only; `Data`, if set, SHALL become `error.data` for that call only. This parameter SHALL NOT be recognized via any `@`-annotation tag — recognition SHALL be purely structural, by the parameter's C++ type. Adopting it SHALL NOT be applied automatically by this proposal to any existing interface.

#### Scenario: Only `Data` set
- **WHEN** a method with an opted-in `Core::ErrorDetail<DATA>` parameter sets only its `Data` field before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object SHALL contain that `Data` value as `error.data`, with `error.code`/`error.message` derived exactly as they would be for a method that has not opted in

#### Scenario: Only `Message` set, `Data` left unset
- **WHEN** a method with an opted-in `Core::ErrorDetail<DATA>` parameter sets only its `Message` field, leaving `Data` unset, before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object's `error.message` SHALL equal that call-specific message, `error.data` SHALL be absent, and `error.code` SHALL be derived exactly as it would be for a method that has not opted in

#### Scenario: Only `Code` set
- **WHEN** a method with an opted-in `Core::ErrorDetail<DATA>` parameter sets only its `Code` field before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object's `error.code` SHALL equal that call-specific code verbatim, taking precedence over the framework's default `ApplicationErrorCodeBase`-derived mapping for that call only, and SHALL NOT affect `error.code` on any other call to the same or any other method

#### Scenario: All three fields set
- **WHEN** a method with an opted-in `Core::ErrorDetail<DATA>` parameter sets `Code`, `Message`, and `Data` before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object SHALL contain all three as `error.code`, `error.message`, and `error.data` respectively

#### Scenario: Parameter left entirely unset
- **WHEN** a method with an opted-in `Core::ErrorDetail<DATA>` parameter returns a non-`Core::ERROR_NONE` result code without setting the parameter at all
- **THEN** the serialized JSON-RPC error object SHALL contain only `code`/`message`, both derived exactly as for a method that has not opted in, with no `data` member

#### Scenario: Opted-in parameter never appears in a successful response
- **WHEN** a method with an opted-in `Core::ErrorDetail<DATA>` parameter returns `Core::ERROR_NONE`
- **THEN** the serialized JSON-RPC `result` SHALL NOT contain the parameter's value, regardless of whether it was set

#### Scenario: Out-of-process (COM-RPC-backed) plugin behaves identically to in-process
- **WHEN** a method whose real implementation runs out-of-process (invoked via a COM-RPC proxy/stub pair) sets `Code`/`Message`/`Data` on its opted-in `Core::ErrorDetail<DATA>` parameter and returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC response SHALL contain the same `error.code`/`error.message`/`error.data` as an equivalent in-process plugin — the process boundary SHALL NOT affect whether these values are preserved

### Requirement: Existing `@out`/response parameters are never a source of `error.data`
A method's existing `@out`/response parameter(s) — declared for its success-result shape — SHALL NEVER be inspected, serialized, or echoed into `error.data`, regardless of whether the method populated them before returning an error, and regardless of whether the method has adopted `Core::ErrorDetail<DATA>`. This holds uniformly whether the method has zero, one, or many other `@out` parameters.

#### Scenario: Populated response object on error is not surfaced as data, without opt-in
- **WHEN** a method that has an existing `@out`/response parameter and has NOT adopted `Core::ErrorDetail<DATA>` sets one or more fields on that response parameter and then returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object SHALL contain only `code`/`message`, with no `data` member and no trace of the populated response parameter's contents

#### Scenario: Populated response object on error is not surfaced as data, even with opt-in present
- **WHEN** a method that has both an existing `@out`/response parameter and an opted-in `Core::ErrorDetail<DATA>` parameter populates the former (but not the latter's `Data` field) and returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object SHALL NOT contain `error.data` sourced from the response parameter — only an explicitly set `Core::ErrorDetail<DATA>::Data` value can ever populate `error.data`

### Requirement: Methods with no opted-in error-detail parameter cannot surface `data`
A method that has not adopted `Core::ErrorDetail<DATA>` — whether or not it declares other `@out` parameters — SHALL continue to produce only `code`/`message` on error, exactly as before this change.

#### Scenario: Non-opted-in method error has no data member
- **WHEN** a method with no opted-in `Core::ErrorDetail<DATA>` parameter returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object SHALL contain only `code` and `message`, with no `data` key present, regardless of the JsonGenerator/framework changes introduced by this proposal

### Requirement: `code` and `message` derivation is unaffected by `data`
Populating `error.data` SHALL NOT change how `error.code` or `error.message` are derived: `code`/`message` continue to be produced by the existing framework error mapping (`Message::Info::SetError`), optionally overridden per-call only via an opted-in `Core::ErrorDetail<DATA>`'s `Code`/`Message` fields, independent of whatever value is assigned to `Data`.

#### Scenario: Structured data and a default-derived message coexist
- **WHEN** a method sets only `Data` on its opted-in `Core::ErrorDetail<DATA>` parameter (leaving `Message` unset) and returns a non-`Core::ERROR_NONE` result code
- **THEN** the response SHALL contain the framework's default-derived `message` text (unchanged from today's behavior) together with the populated `data` value

### Requirement: Legacy `CustomErrorCode` mechanism is removed
The legacy global, process-wide custom-error-code mechanism (`Core::CustomCode`, `Core::IsCustomCode`, `CustomCodeToStringHandler`/`SetCustomCodeToStringHandler`, `ICustomErrorCode.h`) SHALL be removed. `error.code` derivation SHALL rely solely on the framework's existing default mapping for any `Core::hresult` not matching one of the fixed, well-known framework error codes, unless the responding method has explicitly opted into `Core::ErrorDetail<DATA>` and set its `Code` field for that call.

#### Scenario: Unrecognized hresult still produces a deterministic, collision-free code
- **WHEN** a method returns a `Core::hresult` that is not one of the framework's fixed, well-known error codes, and has not opted into `Core::ErrorDetail<DATA>`'s `Code` override
- **THEN** the serialized JSON-RPC error object's `code` SHALL be derived via the framework's existing default mapping (`ApplicationErrorCodeBase` offset), exactly as it already was for any such value before this change

#### Scenario: Unrecognized hresult without an opt-in message override gets a generic fallback message
- **WHEN** a method returns a `Core::hresult` that is not one of the framework's fixed, well-known error codes, and has not opted into `Core::ErrorDetail<DATA>`'s `Message` override
- **THEN** the serialized JSON-RPC error object's `message` SHALL be the framework's generic fallback text (`"Undefined Thunder error code: <value>"`), since no plugin-supplied text is available for a value the framework does not otherwise recognize

### Requirement: `JSONRPCErrorAssessor` is unaffected by this proposal
The `JSONRPCErrorAssessor` error-handling hook SHALL retain its existing signature and capability, unmodified by this proposal. It SHALL NOT gain the ability to attach structured `data` or override `code`/`message`. It is documented as deprecated; extending it further is explicitly out of scope.

#### Scenario: Error assessor output is unaffected
- **WHEN** a registered `JSONRPCErrorAssessor` callback runs after a handler error, exactly as it does today
- **THEN** the final JSON-RPC error response SHALL be assembled exactly as it was before this proposal, with no new `data` member attributable to the assessor

### Requirement: Framework-generated errors remain data-less by default
Errors generated directly by the JSON-RPC transport/dispatch framework itself (parse failures, unknown method, privileged request, async abort, timeout) SHALL continue to populate only `code` and `message`, and SHALL NOT synthesize a `data` value on the caller's behalf.

#### Scenario: Timeout error has no data
- **WHEN** a pending asynchronous JSON-RPC call times out before a handler-level result is available
- **THEN** the resulting error response SHALL contain `code` and `message` describing the timeout, and SHALL NOT contain a `data` member
