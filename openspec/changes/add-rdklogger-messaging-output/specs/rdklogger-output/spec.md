## ADDED Requirements

### Requirement: External output is a Messaging destination
Thunder Messaging SHALL provide an optional RDKLogger destination without changing MessageControl or DirectOutput implementations.

#### Scenario: External route submits directly to RDKLogger
- **WHEN** the selected output route is `EXTERNAL_DIRECT` and the `MODULE_NAME`/level pair is externally enabled
- **THEN** Messaging submits the message payload to RDKLogger and does not enqueue it for MessageControl solely because of the external submission

#### Scenario: Existing local routes remain independent
- **WHEN** the selected output route is `HANDLER` or `DIRECT`
- **THEN** Messaging preserves the existing handler or DirectOutput behavior and does not require RDKLogger

### Requirement: ALL includes every destination
The `ALL` output route SHALL include handler delivery, DirectOutput, and RDKLogger delivery when each destination is available and enabled.

#### Scenario: ALL delivers to all interested destinations
- **WHEN** `ALL` is selected and local and external destinations are enabled
- **THEN** the message is submitted once to the handler route, once to DirectOutput, and once to RDKLogger

#### Scenario: External failure does not remove local delivery
- **WHEN** `ALL` is selected and RDKLogger initialization or submission is unavailable
- **THEN** handler and DirectOutput delivery continue according to their existing rules

### Requirement: RDKLogger receives the payload
Messaging SHALL submit the message payload to RDKLogger. Message metadata SHALL select the module and level. For logging and tracing messages mapped to the default external level, Messaging SHALL prefix a non-empty category as `<CATEGORY>: <PAYLOAD>`; explicitly classified messages SHALL preserve the original payload.

#### Scenario: Payload is preserved
- **WHEN** an externally enabled Thunder message is submitted
- **THEN** the RDKLogger call receives the original payload content, unless it is a default-level logging or tracing message with a non-empty category, in which case it receives `<CATEGORY>: <PAYLOAD>`

### Requirement: Telemetry is not duplicated to the T2 path
Messaging SHALL not submit telemetry to RDKLogger when the same message is also routed to the handler destination, because that path is forwarded to the T2 backend by MessageControl.

#### Scenario: Handler-routed telemetry is not duplicated
- **WHEN** telemetry uses `ALL` or another route selecting the handler and RDKLogger is externally enabled
- **THEN** Messaging delivers telemetry to the handler and does not submit it to RDKLogger

#### Scenario: External-only telemetry remains available
- **WHEN** telemetry uses `EXTERNAL_DIRECT` and RDKLogger is externally enabled
- **THEN** Messaging submits telemetry to RDKLogger

### Requirement: Tracing severity mapping is category-aware
Messaging SHALL map tracing categories `Fatal`/`Crash`, `Error`, `Warning`/`Warn`, and `Notice` to `FATAL`, `ERROR`, `WARN`, and `NOTICE` respectively. Other tracing categories SHALL map to `TRACE`.

### Requirement: Thunder module mapping is explicit
Messaging SHALL identify an external message using the category `LOG.RDK.THUNDER.<MODULE_NAME>`, where `MODULE_NAME` is the existing Thunder module identifier passed unchanged. Thunder SHALL document this mapping, while RDKLogger deployment configuration SHALL own category registration and enablement.

#### Scenario: RDKLogger team configures the category
- **WHEN** the RDKLogger deployment registers and enables `LOG.RDK.THUNDER.<MODULE_NAME>`
- **THEN** Messaging's enabled-state query and submission use that category without any additional Thunder-side normalization

### Requirement: External submission is synchronous in the first phase
The first implementation SHALL treat the RDKLogger call as synchronous from the producer's perspective.

#### Scenario: Producer timing includes external submission
- **WHEN** an externally enabled message is emitted
- **THEN** the producer call does not return until the adapter's RDKLogger submission returns
