## ADDED Requirements

### Requirement: RDKLogger interest is cached
Messaging SHALL cache RDKLogger enabled-state queries by the existing `MODULE_NAME` and mapped level. Thunder SHALL pass `MODULE_NAME` unchanged to the adapter.

#### Scenario: Duplicate query is suppressed
- **WHEN** the same module/level pair is requested more than once
- **THEN** the adapter reuses the cached result and performs only one RDKLogger enabled-state query

#### Scenario: Eager initialization populates known controls
- **WHEN** Messaging opens with RDKLogger support enabled
- **THEN** the adapter queries each known module/level pair once before normal message production

#### Scenario: Newly announced control is queried immediately
- **WHEN** a new module/level control is announced after initial population
- **THEN** the adapter queries that pair immediately and caches the result

### Requirement: External enablement does not replace local enablement
The cached external result SHALL control only RDKLogger admission and SHALL NOT mutate Thunder's local category state.

#### Scenario: External state changes local behavior
- **WHEN** RDKLogger reports a module/level as disabled while local Thunder output is enabled
- **THEN** local output remains governed by Thunder's existing enablement state

### Requirement: No-interest messages are short-circuited
The producer SHALL avoid constructing and submitting a message when neither local nor external destination is interested.

#### Scenario: Neither gate is enabled
- **WHEN** both local Thunder enablement and cached RDKLogger enablement are false
- **THEN** no message is constructed and no output adapter is called
