## ADDED Requirements

### Requirement: JSON Container Set Method
`JSON::Container` SHALL provide a `Set(const TCHAR label[], const IElement& value)`
method that updates the value of an existing field in-place.

#### Scenario: Update existing field value
- **GIVEN** a `Container` with field `state = "initialising"`
- **WHEN** `container.Set("state", newState)` is called with `newState = "active"`
- **THEN** `container.ToString()` serialises `state` as `"active"`
- **AND** all other fields are unchanged

#### Scenario: Set on unknown label is a no-op
- **GIVEN** a `Container` with field `state = "active"`
- **WHEN** `container.Set("nonexistent", value)` is called
- **THEN** the container is unchanged and no crash or exception occurs

#### Scenario: Serialisation reflects mutation sequence
- **GIVEN** a `Container` after a sequence of `Add`, `Set`, and `Remove` calls
- **WHEN** `ToString()` is called
- **THEN** the output matches the current in-memory state exactly

## MODIFIED Requirements

### Requirement: JSON Container Mutation Thread-Safety Contract
The `Add`, `Set`, and `Remove` operations on `JSON::Container` SHALL document
that they are not internally thread-safe and that callers MUST hold any
necessary lock before invoking them.
(Previously: undocumented — no thread-safety guidance existed)

#### Scenario: Caller-managed locking documented
- **GIVEN** a `Container` shared between threads
- **WHEN** a developer reads the `Add`/`Set`/`Remove` API comments
- **THEN** the comments explicitly state "caller-holds-lock" as the thread-safety contract
