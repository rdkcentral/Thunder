## Why

Thunder plugin developers frequently need to collect data from multiple distinct
`JSON::Container` objects and combine them into a single JSON response. Today
this requires manually iterating every field of each source container and calling
`Add(label, element)` for each one — boilerplate that is error-prone and breaks
as fields are added to source containers over time.

`JSON::Container` already supports `FromString()` to populate from a serialised
string and `Add(label, element)` to insert individual fields. `FromObject()`
closes the gap by accepting another `Container` directly, mirroring the
relationship between `FromString` (string → container) and `FromObject`
(container → container).

## What Changes

A new method `FromObject(const JSON::Container& source)` is added to
`JSON::Container` (defined in `Source/core/JSON.h`). It iterates all top-level
key-value pairs in `source` that are in a set state and copies their values
into the calling container via a per-field `ToString`/`FromString` round-trip.

No existing API is removed or modified; `FromString()` and `Add()` continue to
work exactly as before.

## Behavioral Specification

### Merge semantics
`FromObject` performs a **shallow, top-level merge**. Each field in the source
is treated as an opaque value. If a field value is itself a nested `Container`
or an `ArrayType`, it is transferred whole — not field-by-field or element-by-element.

### Nested containers — field-replace, not field-merge
When a field in the source is a nested `Container` and the target has a matching
field with the same label:
- The **target's nested container is replaced in its entirety** with the source value.
- No recursive merge of the inner fields is performed.
- Example: if target has `"device": {"model":"X", "region":"EU"}` and source
  has `"device": {"model":"Y"}`, after `FromObject` the target has
  `"device": {"model":"Y"}` — `region` is lost.

Deep/recursive merge (merging nested objects key-by-key) is **explicitly out of
scope for R4.4.7** and is a candidate follow-on story.

### Nested arrays — same rule
A top-level field whose value is an `ArrayType` is replaced wholesale in the same
way as a nested container.

### Null-valued and unset fields — both skipped
Fields where `IsSet() == false` OR `IsNull() == true` in the source are **not
copied** into the target. This is the conservative safe default for R4.4.7.

> **Rationale:** The Thunder JSON null model is inconsistent across the type
> hierarchy: scalar types (`NumberType`, `String`, `Boolean`, …) have `Null(true)`
> set both the `UNDEFINED` and `SET` flags, so `IsSet() == true` for a null
> scalar. `Container::Null(true)`, however, sets only the `UNDEFINED` flag and
> leaves `SET` unset — meaning a null `Container` field has `IsSet() == false`
> and is already invisible to `FindNext()` during serialisation. Propagating null
> fields in `FromObject()` would silently behave differently for scalar vs.
> container-typed fields without a cross-framework audit.
>
> US-1.5 (Impact Analysis) and US-1.6 (Implementation) exist for exactly this
> purpose. Null-aware merging in `FromObject()` is a follow-on task **blocked on
> US-1.5** and MUST NOT be implemented in R4.4.7.

### Typed (`ObjectType`) targets — field registration mismatch
In a typed target (a struct-like subclass of `Container` with pre-registered
member fields), any source key that does not correspond to a registered field is
**silently skipped**. This is the same behaviour as `FromString()` with an
unknown key. No error is raised.

If a source field key exists in the target but the types are incompatible (e.g.,
source serialises the field as `{...}` but target slot is a `String`),
`FromString` on the target slot will fail and the field is left in its prior
state. The failure is recorded via `TRACE_L1` in debug builds; no exception is
thrown.

### Generic (`VariantContainer`) targets
`VariantContainer` overrides `Request()` to dynamically create field slots for
unknown keys. When the target is a `VariantContainer`, **all** set fields from
the source are accepted regardless of whether they were pre-registered, including
nested objects and arrays.

### Duplicate keys — last-writer-wins
If both source and target contain the same key, the source value overwrites the
target value. This matches `FromString()` semantics.

### Chaining
Multiple `FromObject()` calls accumulate fields. Later calls override fields set
by earlier calls for duplicate keys.

### Thread safety
Not internally locked. Caller must hold any necessary lock — identical contract
to `Add()` and `Remove()`.

## Capabilities

### New Capabilities
- `json-container-from-object`: `JSON::Container::FromObject(const Container&)` — merges all
  set top-level fields from a source container into the target container. Supports
  chaining multiple calls to accumulate fields from several sources. Behaviour for
  duplicate keys is **last-writer-wins** (source overwrites target), and this
  contract is documented in the API reference.

### Modified Capabilities
<!-- none -->

## Impact

- **`Source/core/JSON.h`** — `Container` class gains one new public method.
- **`Tests/unit/`** (or equivalent) — new unit test file covering the scenarios
  in the acceptance criteria.
- **No ABI break** — the method is an addition; all existing binaries continue
  to link and run unchanged.
- **No CMake changes** required; the method lives entirely in the existing
  `Thunder::Core` library translation unit / header.

## Usage Examples

### Example 1 — merge two flat containers
```cpp
// Sub-system A fills its own container.
Core::JSON::Container networkInfo;
networkInfo.Add(_T("interface"),  Core::JSON::String("eth0"));
networkInfo.Add(_T("ip"),         Core::JSON::String("192.168.1.5"));

// Sub-system B fills a separate container.
Core::JSON::Container deviceInfo;
deviceInfo.Add(_T("model"),       Core::JSON::String("ES1-A"));
deviceInfo.Add(_T("firmware"),    Core::JSON::String("R4.4.7"));

// Merge both into the final response object.
Core::JSON::Container response;
response.FromObject(networkInfo);   // pulls in interface + ip
response.FromObject(deviceInfo);    // accumulates model + firmware

string json;
response.ToString(json);
// json == {"interface":"eth0","ip":"192.168.1.5","model":"ES1-A","firmware":"R4.4.7"}
```

### Example 2 — nested container: whole-object replace semantics
```cpp
// Target already has device info with two sub-fields.
Core::JSON::Container target;
Core::JSON::Container existingDevice;
existingDevice.Add(_T("model"),  Core::JSON::String("ES1-A"));
existingDevice.Add(_T("region"), Core::JSON::String("EU"));
target.Add(_T("device"), &existingDevice);

// Source has an updated device block — only 'model', no 'region'.
Core::JSON::Container patch;
Core::JSON::Container updatedDevice;
updatedDevice.Add(_T("model"), Core::JSON::String("ES1-B"));
patch.Add(_T("device"), &updatedDevice);

target.FromObject(patch);

string json;
target.ToString(json);
// json == {"device":{"model":"ES1-B"}}
//
// NOTE: "region" is GONE — nested containers are replaced, not merged.
```

### Example 3 — null and unset source fields are both skipped
```cpp
Core::JSON::Container source;
Core::JSON::String nullableField;
nullableField.Null(true);                     // IsNull()==true, IsSet()==true on String
Core::JSON::String unsetField;                // IsSet()==false
source.Add(_T("token"),   &nullableField);
source.Add(_T("unused"),  &unsetField);

Core::JSON::Container target;
Core::JSON::String tokenSlot(_T("old-value"));
Core::JSON::String unusedSlot(_T("old-unused"));
target.Add(_T("token"),  &tokenSlot);
target.Add(_T("unused"), &unusedSlot);

target.FromObject(source);

// Both source fields are skipped:
//   - nullableField: IsNull()==true  -> skipped in R4.4.7
//   - unsetField:    IsSet()==false  -> skipped always
// Target is UNCHANGED.
string json;
target.ToString(json);
// json == {"token":"old-value","unused":"old-unused"}
//
// NOTE: Null-aware merging is deferred to US-1.5 / US-1.6.
```
