## ADDED Requirements

### Requirement: External adapter lifecycle belongs to each Messaging process
Every process that opens Thunder Messaging SHALL initialize and deinitialize its own external adapter when the optional feature is enabled.

#### Scenario: Independent process initialization
- **WHEN** two processes open Messaging with External Direct support enabled
- **THEN** each process initializes its own adapter and does not depend on a daemon-only forwarding path

#### Scenario: Teardown deinitializes the adapter
- **WHEN** Messaging is deinitialized
- **THEN** the adapter releases its external resources before the process-local Messaging state is destroyed

### Requirement: External adapter failure is isolated
Failure to initialize the external adapter or query its enabled state SHALL disable only external output and SHALL preserve local Thunder output.

#### Scenario: Initialization failure
- **WHEN** external adapter initialization fails
- **THEN** `HANDLER`, `DIRECT`, and the local portions of `ALL` continue according to their existing behavior

#### Scenario: Query failure
- **WHEN** an external enabled-state query fails
- **THEN** that external module/level pair is treated as unavailable and local output is unaffected

### Requirement: External Direct support is optional
Builds without the External Direct feature SHALL compile without external adapter headers or libraries and SHALL preserve current Messaging behavior.

#### Scenario: Feature disabled
- **WHEN** Messaging is built without External Direct support
- **THEN** existing routes work as before and `ALL` has no external side effect
