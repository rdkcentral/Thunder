## ADDED Requirements

### Requirement: Error object exposes a structured `data` member
The JSON-RPC error object (`Core::JSONRPC::Message::Info`) SHALL expose `data` as a JSON value capable of representing any JSON-RPC-legal type (object, array, string, number, boolean, or null), not only a plain string.

#### Scenario: Data field accepts a structured object
- **WHEN** a handler attaches a JSON object (e.g. `{"field": "value", "count": 3}`) as error data for a failed call
- **THEN** the serialized JSON-RPC response's `error.data` member SHALL contain that object with its nested structure preserved

### Requirement: `data` is omitted when not populated
The JSON-RPC error object SHALL omit the `data` member entirely from the serialized response when no handler, error-response path, or error assessor has populated it for that call, preserving current wire behavior for all calls that do not opt in.

#### Scenario: Unset data is not serialized
- **WHEN** a method invocation fails with an error code and neither the handler nor any error-response path assigns a value to error data
- **THEN** the serialized JSON-RPC error object SHALL contain only `code` and `message`, with no `data` key present

#### Scenario: Existing plain-text error behavior is unchanged
- **WHEN** a handler returns a non-success code and writes a plain, non-JSON-object string into its response output (the historical convention)
- **THEN** that string SHALL be assigned to `error.message` exactly as before, and `error.data` SHALL remain absent from the response

### Requirement: Typed response objects populated on error surface as `data`
A method registered through the typed `Handler::Register<INBOUND, OUTBOUND, METHOD>` mechanism SHALL be able to attach structured error detail by populating fields on the same OUTBOUND response object it already declares, without any change to its `Core::hresult`/`uint32_t` return-code signature.

#### Scenario: Populated response object on error becomes error data
- **WHEN** a typed method sets one or more fields on its declared response object and then returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC response SHALL contain an `error` object whose `data` member equals the JSON serialization of that populated response object, and SHALL NOT contain a `result` member

#### Scenario: Unpopulated response object on error still omits data
- **WHEN** a typed method returns a non-`Core::ERROR_NONE` result code without setting any field on its declared response object
- **THEN** the serialized JSON-RPC error object SHALL NOT contain a `data` member

#### Scenario: Out-of-process (COM-RPC-backed) plugin behaves identically to in-process
- **WHEN** a typed method whose real implementation runs out-of-process (invoked via a COM-RPC proxy/stub pair) sets one or more fields on its response object and returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC response SHALL contain `error.data` populated with that response object's JSON serialization, identically to an equivalent in-process plugin — the process boundary SHALL NOT affect whether `data` is preserved

#### Scenario: Non-object (scalar or array) response objects populate data using their own JSON shape
- **WHEN** a typed method's declared response is a single, non-struct value (e.g. an interface with exactly one `@out` scalar parameter, collapsed by JsonGenerator's default RPC format into a bare JSON value) and the method sets that value before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object's `data` member SHALL contain that value's own JSON representation (e.g. a bare number, string, or array), not forced into an object wrapper

### Requirement: Methods with no response object cannot surface structured data unless they opt in
A method with no `@out`/output parameter at all (a "void" response, e.g. `IAccount::SetLastCheckoutResetTime`) has no object for a plugin to populate, so it SHALL continue to produce only `code`/`message` on error, exactly as before this change, UNLESS its interface has explicitly opted into the dedicated error-detail parameter mechanism (see "Opt-in dedicated error-detail parameter" requirement below). This is a documented boundary of the default (Decisions 2/3) mechanism, not a defect.

#### Scenario: Void-response method error has no data member by default
- **WHEN** a method with no declared `@out`/response parameter, and no opted-in error-detail parameter, returns a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object SHALL contain only `code` and `message`, with no `data` key present, regardless of the JsonGenerator/framework changes introduced by this proposal

### Requirement: Opt-in dedicated error-detail parameter for methods without a reusable response object
An interface owner MAY explicitly add a dedicated, optional error-detail out-parameter (recognized by `JsonGenerator` structurally or via an explicit tag) to any interface method, including one with no other `@out` parameter. This parameter SHALL have two fixed fields, `Code` and `Message`, plus a `Data` member whose type is templated and defined per adopting interface. When present and populated on an error path, its `Data` value SHALL be surfaced as `error.data`, independent of whatever (if anything) the method returns on success; its `Code`/`Message` fields, when set, SHALL override `error.code`/`error.message` for that call only. Adopting this mechanism SHALL NOT be applied automatically by this proposal to any existing interface — it requires an explicit interface-owner change to that interface's method signature.

#### Scenario: Opted-in void-response method surfaces data via its dedicated error-detail parameter
- **WHEN** a method that has no other `@out` parameter, but declares an opted-in error-detail out-parameter, sets that parameter's `Data` member before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object's `data` member SHALL contain the JSON serialization of that error-detail parameter's `Data` value

#### Scenario: Unpopulated opt-in error-detail parameter is omitted
- **WHEN** a method with an opted-in error-detail out-parameter returns a non-`Core::ERROR_NONE` result code without setting that parameter
- **THEN** the serialized JSON-RPC error object SHALL NOT contain a `data` member

#### Scenario: Opt-in error-detail parameter can also override `message` for that call
- **WHEN** a method with an opted-in error-detail out-parameter sets that parameter's `Message` field before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object's `error.message` SHALL equal that call-specific message, taking precedence over the framework's default `SetError()`-derived text for that call only, and SHALL NOT affect `error.message` on any other call to the same or any other method

#### Scenario: Opt-in error-detail parameter can also override `code` for that call
- **WHEN** a method with an opted-in error-detail out-parameter sets that parameter's `Code` field before returning a non-`Core::ERROR_NONE` result code
- **THEN** the serialized JSON-RPC error object's `error.code` SHALL equal that call-specific code verbatim, taking precedence over the framework's default `ApplicationErrorCodeBase`-derived mapping for that call only, and SHALL NOT affect `error.code` on any other call to the same or any other method

#### Scenario: Opt-in error-detail parameter never appears in a successful response
- **WHEN** a method with an opted-in error-detail out-parameter returns `Core::ERROR_NONE`
- **THEN** the serialized JSON-RPC `result` SHALL NOT contain the error-detail parameter's value, regardless of whether that parameter was set

### Requirement: `code` and `message` derivation is unaffected by `data`
Populating `error.data` SHALL NOT change how `error.code` or `error.message` are derived: `code`/`message` continue to be produced by the existing framework error mapping (`Message::Info::SetError`), optionally overridden per-call only via the opt-in error-detail parameter's `Code`/`Message` fields (see above), independent of whatever value is assigned to `data`.

#### Scenario: Structured data and a default-derived message coexist
- **WHEN** a handler returns a non-`Core::ERROR_NONE` result code, does not use the opt-in error-detail parameter's message override, and also populates structured error data for the same call
- **THEN** the response SHALL contain the framework's default-derived `message` text (unchanged from today's behavior) together with the populated `data` value

### Requirement: Legacy `CustomErrorCode` mechanism is removed
The legacy global, process-wide custom-error-code mechanism (`Core::CustomCode`, `Core::IsCustomCode`, `CustomCodeToStringHandler`/`SetCustomCodeToStringHandler`, `ICustomErrorCode.h`) SHALL be removed. `error.code` derivation SHALL rely solely on the framework's existing default mapping for any `Core::hresult` not matching one of the fixed, well-known framework error codes, unless the responding method has explicitly opted into the dedicated error-detail parameter (see "Opt-in dedicated error-detail parameter" requirement above) and set its `Code` field for that call.

#### Scenario: Unrecognized hresult still produces a deterministic, collision-free code
- **WHEN** a method returns a `Core::hresult` that is not one of the framework's fixed, well-known error codes, and the method has not opted into the dedicated error-detail parameter's `Code` override
- **THEN** the serialized JSON-RPC error object's `code` SHALL be derived via the framework's existing default mapping (`ApplicationErrorCodeBase` offset), exactly as it already was for any such value before this change

#### Scenario: Unrecognized hresult without an opt-in message override gets a generic fallback message
- **WHEN** a method returns a `Core::hresult` that is not one of the framework's fixed, well-known error codes, and does not use the opt-in error-detail parameter's message override
- **THEN** the serialized JSON-RPC error object's `message` SHALL be the framework's generic fallback text (`"Undefined Thunder error code: <value>"`), since no plugin-supplied text is available for a value the framework does not otherwise recognize

#### Scenario: Only the opt-in error-detail parameter's own `Code` field can override the wire-level error code
- **WHEN** a method uses the opt-in error-detail parameter (with its `Code`/`Message`/`Data` fields) on an error path
- **THEN** the serialized `error.code` SHALL equal that parameter's `Code` field verbatim if it was set, or otherwise be derived exclusively from the method's own `Core::hresult` return value — no other field of the error-detail parameter, and no other plugin-declared value outside this explicit opt-in mechanism, SHALL be able to influence `error.code`

### Requirement: Error assessor hook can attach structured data
The `JSONRPCErrorAssessor` error-handling hook SHALL be able to attach structured `data` to the error response it produces, using the same population rule applied to regular handlers, without changing the hook's existing callback signature.

#### Scenario: Error assessor supplies structured data
- **WHEN** a registered error assessor callback is invoked after a handler error and writes a JSON object into the response it controls
- **THEN** the final JSON-RPC error response's `data` member SHALL contain that JSON object

### Requirement: Framework-generated errors remain data-less by default
Errors generated directly by the JSON-RPC transport/dispatch framework itself (parse failures, unknown method, privileged request, async abort, timeout) SHALL continue to populate only `code` and `message`, and SHALL NOT synthesize a `data` value on the caller's behalf.

#### Scenario: Timeout error has no data
- **WHEN** a pending asynchronous JSON-RPC call times out before a handler-level result is available
- **THEN** the resulting error response SHALL contain `code` and `message` describing the timeout, and SHALL NOT contain a `data` member
