## ADDED Requirements

### Requirement: JSON Array Remove By Index
`JSON::ArrayType<ELEMENT>` SHALL provide a `Remove(uint32_t index)` method that
erases the element at the given zero-based position.

#### Scenario: Remove middle element
- **GIVEN** an `ArrayType` containing elements `[A, B, C]`
- **WHEN** `array.Remove(1)` is called
- **THEN** the array contains `[A, C]`
- **AND** `array.Length()` equals `2`

#### Scenario: Remove first element
- **GIVEN** an `ArrayType` containing `[A, B, C]`
- **WHEN** `array.Remove(0)` is called
- **THEN** the array contains `[B, C]`

#### Scenario: Remove last element
- **GIVEN** an `ArrayType` containing `[A, B, C]`
- **WHEN** `array.Remove(2)` is called
- **THEN** the array contains `[A, B]`

#### Scenario: Remove all elements
- **GIVEN** an `ArrayType` with `Length() > 0`
- **WHEN** all elements are removed one-by-one via `Remove(0)`
- **THEN** `Length() == 0` and `IsSet() == false`

#### Scenario: Serialisation after Remove
- **GIVEN** an `ArrayType` after one or more `Remove` calls
- **WHEN** `ToString()` is called
- **THEN** the output matches the current in-memory elements only — removed
  elements do not appear

#### Scenario: Append after Remove
- **GIVEN** an `ArrayType` from which one element has been removed
- **WHEN** `Add()` is called to append a new element
- **THEN** `Length()` increments correctly and the new element appears last in
  `ToString()` output

#### Scenario: Existing Add and operator[] unaffected
- **GIVEN** existing code that uses `Add()`, `operator[]`, `Get()`, and `Clear()`
  on `ArrayType`
- **WHEN** compiled against R4.4.7 headers
- **THEN** behaviour is identical to R4.4.6 — no regressions
