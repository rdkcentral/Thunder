## ADDED Requirements

### Requirement: RDKLogger lifecycle belongs to each Messaging process
Every process that opens Thunder Messaging SHALL initialize and deinitialize its own RDKLogger integration when the optional feature is enabled.

#### Scenario: Independent process initialization
- **WHEN** two processes open Messaging with RDKLogger support enabled
- **THEN** each process initializes its own adapter and does not depend on a daemon-only forwarding path

#### Scenario: Teardown deinitializes the adapter
- **WHEN** Messaging is deinitialized
- **THEN** the adapter releases its RDKLogger resources before the process-local Messaging state is destroyed

### Requirement: RDKLogger failure is isolated
Failure to initialize RDKLogger or query its enabled state SHALL disable only external output and SHALL preserve local Thunder output.

#### Scenario: Initialization failure
- **WHEN** RDKLogger initialization fails
- **THEN** `HANDLER`, `DIRECT`, and the local portions of `ALL` continue according to their existing behavior

#### Scenario: Query failure
- **WHEN** an RDKLogger enabled-state query fails
- **THEN** that external module/level pair is treated as unavailable and local output is unaffected

### Requirement: RDKLogger support is optional
Builds without the RDKLogger feature SHALL compile without RDKLogger headers or libraries and SHALL preserve current Messaging behavior.

#### Scenario: Feature disabled
- **WHEN** Messaging is built without RDKLogger support
- **THEN** existing routes work as before and `ALL` has no external RDKLogger side effect
