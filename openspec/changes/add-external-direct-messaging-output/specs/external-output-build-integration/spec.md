## ADDED Requirements

### Requirement: External adapter dependency is opt-in
The build SHALL expose an explicit External Direct integration option that is OFF by default. External adapter include paths and libraries SHALL be added only when that option is enabled. The CMake option and backend selection are part of this change; Buildroot, Lithosphere, SDK packaging, and target deployment integration are not.

#### Scenario: Default build has no external adapter dependency
- **WHEN** the default Thunder build is configured without the integration option
- **THEN** it does not require external adapter headers or libraries

#### Scenario: Target-enabled build links the adapter
- **WHEN** the integration option is enabled and the target SDK is available
- **THEN** the Messaging implementation compiles and links its external adapter

#### Scenario: SDK-independent mock build
- **WHEN** the mock backend option is enabled without the target external SDK
- **THEN** the Messaging implementation compiles against the same public C external-output interface and the mock prints submitted messages to the console

### Requirement: Core remains independent
The External Direct integration SHALL NOT add an external adapter dependency to Thunder core primitives.

#### Scenario: Core-only consumer
- **WHEN** a consumer builds or links Thunder core without Messaging's optional integration
- **THEN** it remains free of external adapter headers, symbols, and link requirements
