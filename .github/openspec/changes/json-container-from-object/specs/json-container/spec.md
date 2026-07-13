## ADDED Requirements

### Requirement: JSON Container FromObject
`JSON::Container` SHALL provide a `bool FromObject(const IElement& source)` method
equivalent to `this->FromString(source.ToString())`.

#### Scenario: VariantContainer accumulates new fields from multiple sources
- **GIVEN** a `VariantContainer` target with no entries
  AND two source containers with distinct keys
- **WHEN** `target.FromObject(sourceA)` then `target.FromObject(sourceB)` are called
- **THEN** the target contains all keys from both sources
- **AND** `target.ToString()` serialises all accumulated fields correctly

#### Scenario: Typed Container updates only registered slots
- **GIVEN** a typed `Container` with registered fields `{model, firmware}`
  AND a source with fields `{firmware, extra}` where `extra` is not registered
- **WHEN** `target.FromObject(source)` is called
- **THEN** `firmware` is updated to the source value
- **AND** `extra` does NOT appear in `target.ToString()`
- **AND** `model` retains its prior value (absent from source, not cleared)

#### Scenario: Typed Container preserves fields absent from source
- **GIVEN** a typed `Container` with registered fields `{model, firmware}`, both set
  AND a source that carries only `{firmware}`
- **WHEN** `target.FromObject(source)` is called
- **THEN** `firmware` is updated
- **AND** `model` is unchanged — absent source keys do NOT clear target fields

#### Scenario: Deep recursive update of nested typed Container
- **GIVEN** a target with a nested typed `Container` field `"device"` holding
  `{model:"ES1-A", region:"EU"}`
  AND a source with `"device": {"model":"ES1-B"}` (no `region`)
- **WHEN** `target.FromObject(source)` is called
- **THEN** `target.device.model` equals `"ES1-B"`
- **AND** `target.device.region` equals `"EU"` — **preserved, not cleared**

#### Scenario: Cross-type — VariantContainer source into typed Container target
- **GIVEN** a typed `Container` with registered fields
  AND a `VariantContainer` source carrying matching keys
- **WHEN** `target.FromObject(variantSource)` is called
- **THEN** the registered fields are updated from the VariantContainer's values

#### Scenario: Cross-type — typed Container source into VariantContainer target
- **GIVEN** a `VariantContainer` target
  AND a typed `Container` source with set fields
- **WHEN** `target.FromObject(typedSource)` is called
- **THEN** all set fields from the typed source appear in the VariantContainer target

#### Scenario: FromObject vs operator= on VariantContainer
- **GIVEN** a `VariantContainer` target with entries `{a, b}`
  AND a source with entry `{b}` (carrying a new value for b, no a)
- **WHEN** `target.FromObject(source)` is called (NOT `target = source`)
- **THEN** `b` is updated to the source value
- **AND** `a` is **preserved** (merge semantics)
- **WHEN** `target = source` is called instead
- **THEN** only `b` exists in target — `a` is **gone** (replace semantics)

#### Scenario: Returns false on malformed source
- **GIVEN** a source whose `ToString()` produces invalid JSON
- **WHEN** `target.FromObject(source)` is called
- **THEN** `FromObject` returns `false`

#### Scenario: Existing FromString and Add unaffected
- **GIVEN** existing code that uses `FromString()` and `Add()`
- **WHEN** compiled against R4.4.7 headers
- **THEN** behaviour is identical to R4.4.6 — no regressions
