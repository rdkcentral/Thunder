## ADDED Requirements

### Requirement: JSON-RPC handlers may return structured error details

The JSON-RPC handler SHALL support independently optional code, message, and typed data fields on failed operations.

#### Scenario: No rich details are supplied

- **WHEN** a handler returns a Thunder error and its error container is not set
- **THEN** the response SHALL use Thunder's default JSON-RPC code and message
- **AND** the existing no-data behavior SHALL be preserved

#### Scenario: Typed data only is supplied

- **WHEN** a handler returns a Thunder error and sets only typed data
- **THEN** the response SHALL retain Thunder's default code and message
- **AND** the response SHALL include the serialized typed data

#### Scenario: Error type has no data

- **WHEN** a handler uses `Core::ErrorDetails<void>` and returns an error
- **THEN** code and message SHALL follow the same override and fallback rules
- **AND** the serialized JSON-RPC error SHALL NOT contain a `data` field

### Requirement: Explicit values override only corresponding defaults

The handler SHALL initialize the JSON-RPC error from the returned framework error before applying explicit detail values.

#### Scenario: Code-only override

- **WHEN** a failed handler sets only the detail code
- **THEN** the response SHALL use the explicit code
- **AND** the response SHALL retain Thunder's default message

#### Scenario: Message-only override

- **WHEN** a failed handler sets only the detail message
- **THEN** the response SHALL retain Thunder's default code
- **AND** the response SHALL use the explicit message

#### Scenario: Complete rich error

- **WHEN** a failed handler sets code, message, and typed data
- **THEN** the response SHALL contain all explicitly supplied values

### Requirement: Successful handlers ignore error details

Rich error details SHALL affect only failed handler invocation.

#### Scenario: Successful outbound response

- **WHEN** a handler returns `Core::ERROR_NONE`
- **THEN** its normal outbound value SHALL be serialized
- **AND** populated error details SHALL not produce an error response

#### Scenario: Failed handler populated an outbound value

- **WHEN** a handler populates an outbound value and returns an error
- **THEN** the outbound value SHALL NOT be returned as a successful result
- **AND** the applicable JSON-RPC error SHALL be returned

### Requirement: Error-aware registration supports typed handler shapes

`RegisterWithError` SHALL support no-payload, inbound-only, outbound-only, and inbound/outbound handlers.

Rich-error registration SHALL apply to methods only. Properties SHALL continue to use the existing property registration and error behavior.

#### Scenario: Inbound-only handler

- **WHEN** valid typed input is received
- **THEN** the input SHALL be passed to the handler
- **AND** rich details SHALL be serialized if the handler fails

#### Scenario: Outbound-only handler

- **WHEN** an outbound-only handler succeeds
- **THEN** its output SHALL be serialized
- **AND WHEN** it fails
- **THEN** its rich error SHALL be serialized instead

#### Scenario: Inbound/outbound handler

- **WHEN** an inbound/outbound handler succeeds
- **THEN** its parsed input SHALL reach the handler and its output SHALL be returned
- **AND WHEN** it fails
- **THEN** the applicable rich error SHALL be returned

### Requirement: Malformed inbound parameters prevent invocation

#### Scenario: Typed input parsing fails

- **WHEN** inbound JSON cannot be parsed into the registered type
- **THEN** the implementation SHALL NOT be invoked
- **AND** the handler SHALL return `Core::ERROR_PARSE_FAILURE`
- **AND** a parser diagnostic SHALL be available to the response path

### Requirement: Error-aware routing supports indexes

An indexed error-aware registration SHALL receive the index extracted from the JSON-RPC method designator.

#### Scenario: Indexed handler fails with partial details

- **WHEN** an indexed handler is invoked and returns an error with partial details
- **THEN** the requested index SHALL be passed to the handler
- **AND** omitted code or message fields SHALL retain Thunder defaults

#### Scenario: Instance ID without index

- **WHEN** a handler signature contains `instance_id_follows_t` and its instance-ID string but no subsequent index string
- **THEN** registration SHALL select the instance-ID-only dispatch path
- **AND** invocation SHALL NOT supply an index argument

#### Scenario: Direct lambda declares routing arguments

- **WHEN** a non-generic lambda directly passed to `RegisterWithError` declares context, index, or instance-ID arguments
- **THEN** callable traits SHALL derive the corresponding dispatch path from the lambda call operator

#### Scenario: Free function is passed by name

- **WHEN** a free function is passed directly to regular or error-aware registration
- **THEN** its deduced function type SHALL be normalized to a function pointer for callable-trait inspection
- **AND** registration SHALL select the dispatch path represented by its arguments

#### Scenario: Callable has no unambiguous call operator

- **WHEN** a callable wrapper such as `std::bind` has an overloaded or templated call operator
- **THEN** call-operator inspection SHALL fail through substitution rather than a compilation error
- **AND** registration SHALL retain default no-prefix dispatch unless an explicit callable signature is supplied

### Requirement: Plugin invocation preserves legacy errors

The plugin response path SHALL distinguish recognized rich errors from legacy error output.

#### Scenario: Recognized rich-error output

- **WHEN** handler output parses successfully and contains a recognized error field
- **THEN** each present field SHALL be overlaid on the Thunder-initialized response error

#### Scenario: Malformed output

- **WHEN** handler error output is malformed JSON
- **THEN** Thunder's default error code SHALL remain intact
- **AND** the handler output SHALL be used as legacy error text

#### Scenario: Unrecognized JSON output

- **WHEN** handler output is valid JSON but contains no recognized error field
- **THEN** the output SHALL be treated as legacy error text

### Requirement: Error-aware member registration remains C++11-compatible

The `REALOBJECT*` registration path SHALL bind and invoke member functions without requiring C++14 generic lambdas.

#### Scenario: Member-function handler

- **WHEN** a member function is registered with a valid object pointer
- **THEN** invocation SHALL be forwarded to that object
- **AND** it SHALL use the same rich-error serialization as callable registration
