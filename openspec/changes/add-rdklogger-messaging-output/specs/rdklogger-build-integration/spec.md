## ADDED Requirements

### Requirement: RDKLogger dependency is opt-in
The build SHALL expose an explicit RDKLogger integration option that is OFF by default. RDKLogger include paths and libraries SHALL be added only when that option is enabled. The CMake option and backend selection are part of this change; Buildroot, Lithosphere, SDK packaging, and target deployment integration are not.

#### Scenario: Default build has no RDKLogger dependency
- **WHEN** the default Thunder build is configured without the integration option
- **THEN** it does not require RDKLogger headers or libraries

#### Scenario: RDK-enabled build links the adapter
- **WHEN** the integration option is enabled and the target SDK is available
- **THEN** the Messaging implementation compiles and links its RDKLogger adapter

#### Scenario: SDK-independent mock build
- **WHEN** the mock backend option is enabled without the target RDKLogger SDK
- **THEN** the Messaging implementation compiles against the same public C external-output interface and the mock prints submitted messages to the console

### Requirement: Core remains independent
The RDKLogger integration SHALL NOT add an RDKLogger dependency to Thunder core primitives.

#### Scenario: Core-only consumer
- **WHEN** a consumer builds or links Thunder core without Messaging's optional integration
- **THEN** it remains free of RDKLogger headers, symbols, and link requirements
