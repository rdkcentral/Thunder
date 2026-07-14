## Why

Thunder plugin developers need to populate one JSON container from the content
of another without going through an intermediate serialised string. `IElement`
already provides `FromString()` (from text) and `FromFile()` (from file).
`FromObject()` adds the third leg of this set: populate from another `IElement`
directly.

## What Changes

A new method `FromObject(const IElement& source)` is added to `JSON::Container`.

The semantics are **identical to `FromString()`** — the target is fully cleared
before import and then repopulated from `source`. This keeps one uniform mental
model across all three population methods.

## Behavioral Specification

### Rule 1 — Clear before import (same as FromString / FromFile)
The target container is fully cleared before any values are imported from
`source`. This applies to both typed `Container` and `VariantContainer`.

### Rule 2 — Only set values from source are imported
Fields in `source` where `IsSet() == false` are discarded — they do not appear
in `source.ToString()` and therefore are not imported.

### Rule 3 — Null values are imported as-is
Fields where `IsNull() == true` are serialised as `null` by `source.ToString()`
and are correctly round-tripped into the target.

> **Note — Container null defect:** `Container::Null(true)` currently does not
> set the `SET` flag, so a null-valued Container field has `IsSet() == false` and
> is invisible to `ToString()`. Such fields are silently lost during the
> round-trip. This is a **known defect** in the null model and will be fixed
> separately. The defect does not affect scalar types (String, Number, Boolean,
> etc.) which correctly set both `SET` and `UNDEFINED` on `Null(true)`.

### Rule 4 — Typed `Container` target: only registered fields updated
`Container::Deserialize` finds each incoming key via `Find(label)`. For a typed
subclass, only pre-registered slots are updated; unknown keys from `source` are
silently skipped. **No new fields are added to a typed Container.**

### Rule 5 — `VariantContainer` target: replace semantics (same as FromString)
`VariantContainer` overrides `Request()` to create new slots dynamically.
Combined with Rule 1 (Clear first), the result is a complete replacement: after
`FromObject`, the target holds exactly the keys from `source` and nothing else.
This is **not a merge** — the same behaviour as calling `FromString(source.ToString())`.

### Rule 6 — Parameter is `const IElement&`
Accepting any `IElement` means `Container::FromObject(variantContainer)` and
`VariantContainer::FromObject(typedContainer)` are both covered without
overloading.

## Capabilities

### New Capabilities
- `json-container-from-object`: `bool Container::FromObject(const IElement& source)`
  — populates this container from the JSON representation of `source`, following
  the same semantics as `FromString()` and `FromFile()`.

### Modified Capabilities
<!-- none -->

## Impact

- **`Source/core/JSON.h`** — one new method on `Container`.
- **No ABI break** — purely additive.
- **No CMake changes** required.

## Usage Examples

### Example 1 — Typed Container populated from another typed Container
```cpp
struct DeviceInfo : public Core::JSON::Container {
    DeviceInfo() : model(), firmware() {
        Add(_T("model"),    &model);
        Add(_T("firmware"), &firmware);
    }
    Core::JSON::String model;
    Core::JSON::String firmware;
};

DeviceInfo source;
source.model    = _T("ES1-A");
source.firmware = _T("R4.4.7");

DeviceInfo target;
target.model = _T("old-model");    // will be cleared and replaced

target.FromObject(source);

// target.model    == "ES1-A"   (updated from source)
// target.firmware == "R4.4.7"  (updated from source)
```

### Example 2 — Typed Container populated from VariantContainer
```cpp
JsonObject source;
source.Set(_T("model"),    JsonValue(string("ES1-A")));
source.Set(_T("firmware"), JsonValue(string("R4.4.7")));
source.Set(_T("extra"),    JsonValue(string("ignored")));  // not registered

DeviceInfo target;
target.FromObject(source);

// target.model    == "ES1-A"   (registered — updated)
// target.firmware == "R4.4.7"  (registered — updated)
// "extra" was not registered — silently skipped
```

### Example 3 — VariantContainer populated from typed Container (replace)
```cpp
DeviceInfo source;
source.model    = _T("ES1-B");
source.firmware = _T("R4.4.7");

JsonObject target;
target.Set(_T("model"),   JsonValue(string("old")));
target.Set(_T("extra"),   JsonValue(string("will-be-cleared")));  // not in source

target.FromObject(source);

// After FromObject: target is CLEARED first, then repopulated from source.
// target has: model="ES1-B", firmware="R4.4.7"
// "extra" is GONE — Clear() was called; only source fields survive.
```

### Example 4 — VariantContainer populated from VariantContainer (replace)
```cpp
JsonObject source;
source.Set(_T("a"), JsonValue(1));
source.Set(_T("b"), JsonValue(2));

JsonObject target;
target.Set(_T("b"), JsonValue(99));
target.Set(_T("c"), JsonValue(3));   // not in source — will be gone

target.FromObject(source);

// target: a=1, b=2   ("c" is gone — replace semantics)
```
