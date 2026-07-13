## ADDED Requirements

### Requirement: JSON Container FromObject Merge
`JSON::Container` SHALL provide a `FromObject(const JSON::Container& source)`
method that copies all set top-level key-value pairs from `source` into the
calling container.

#### Scenario: Merge two non-overlapping containers
- **GIVEN** a target `Container` with fields `{a, b}` and a source `Container`
  with fields `{c, d}`
- **WHEN** `target.FromObject(source)` is called
- **THEN** the target contains `{a, b, c, d}` and `target.ToString()` serialises
  all four fields correctly

#### Scenario: Merge with duplicate keys (last-writer-wins)
- **GIVEN** a target `Container` with field `version = "4.4.6"` and a source
  `Container` with field `version = "4.4.7"`
- **WHEN** `target.FromObject(source)` is called
- **THEN** `target["version"]` equals `"4.4.7"`

#### Scenario: Chain multiple FromObject calls
- **GIVEN** three source containers A, B, C each with distinct fields
- **WHEN** `target.FromObject(A)`, `target.FromObject(B)`, `target.FromObject(C)`
  are called in sequence
- **THEN** the target accumulates all fields from A, B, and C

#### Scenario: Nested container — whole-replace, not deep-merge
- **GIVEN** a target with field `"device": {"model":"ES1-A", "region":"EU"}`
  AND a source with field `"device": {"model":"ES1-B"}`
- **WHEN** `target.FromObject(source)` is called
- **THEN** `target.ToString()` produces `"device":{"model":"ES1-B"}`
- **AND** `"region"` does NOT appear in the output (nested subtree is replaced)

#### Scenario: Nested array — whole-replace
- **GIVEN** a source `Container` whose field `"tags"` is an `ArrayType` with
  two elements
- **WHEN** `target.FromObject(source)` is called
- **THEN** the array is present in `target.ToString()` with both elements intact

#### Scenario: Null and unset source fields are both skipped
- **GIVEN** a source `Container` with one field explicitly set to null
  (`element.Null(true)`) and one field that is unset (`IsSet() == false`)
- **WHEN** `target.FromObject(source)` is called
- **THEN** neither field is copied to the target; the target's corresponding
  slots are left in their prior state
- **AND** no crash or exception occurs

> **Note:** Null-aware merging (propagating `null` source fields) is deferred to
> US-1.5 (Impact Analysis) and US-1.6 (Implementation). The R4.4.7 behaviour
> is conservative: treat null the same as unset.

#### Scenario: Unknown key in typed target is silently skipped
- **GIVEN** a typed target container that does not have a registered slot for
  key `"extra"`, and a source that has `"extra": 42`
- **WHEN** `target.FromObject(source)` is called
- **THEN** the target is unchanged for that key; no crash or exception occurs

#### Scenario: Type-mismatch leaves target slot unchanged
- **GIVEN** a target with a `JSON::String` slot for key `"info"`, and a source
  where `"info"` is a nested `Container` (serialises as `{...}`)
- **WHEN** `target.FromObject(source)` is called
- **THEN** the target's `"info"` slot is left in its prior state
- **AND** no exception is thrown (a debug-trace warning is emitted in debug builds)

#### Scenario: VariantContainer target accepts all source keys
- **GIVEN** a `VariantContainer` target with no pre-registered fields, and a
  source with three fields of mixed types (string, number, nested object)
- **WHEN** `target.FromObject(source)` is called
- **THEN** all three fields appear in `target.ToString()`

#### Scenario: IsComplete not set after FromObject
- **GIVEN** a target that has not been through `Deserialize()`
- **WHEN** `target.FromObject(source)` is called
- **THEN** `target.IsComplete()` remains `false`
- **AND** `target.IsSet()` returns `true` (fields have values)

#### Scenario: Existing FromString and Add unaffected
- **GIVEN** existing code that uses `FromString()` and `Add()`
- **WHEN** compiled against R4.4.7 headers
- **THEN** behaviour is identical to R4.4.6 — no regressions
