## Why

Thunder middleware plugins build JSON array responses incrementally — for example,
collecting active plugin callsigns, error codes, or capability entries one-by-one
as each sub-system responds. `JSON::ArrayType<ELEMENT>` already supports `Add()`
(append) and `operator[]` (random access), but has no `Remove(index)` operation.
Without it, developers must rebuild the entire array from scratch to drop a single
element, or work around it with a second temporary array — both patterns are
wasteful and hard to maintain.

## What Changes

`JSON::ArrayType<ELEMENT>` gains a `Remove(uint32_t index)` method that erases
the element at the given position and decrements `Length()` accordingly.

The existing `Add()`, `Add(element)`, `operator[]`, `Get()`, `Length()`, and
`Clear()` already exist (confirmed in `Source/core/JSON.h` lines 3330–3380) and
are **not changed**. The work is purely additive.

## Capabilities

### New Capabilities
- `json-array-remove`: `JSON::ArrayType<ELEMENT>::Remove(uint32_t index)` — erases
  the element at `index` (0-based). `ASSERT`s that `index < Length()` in debug
  builds; undefined behaviour for out-of-range index in release builds, consistent
  with the existing `operator[]` contract.

### Modified Capabilities
<!-- none -->

## Impact

- **`Source/core/JSON.h`** — one new `Remove` method on `ArrayType<ELEMENT>`.
- **No ABI break** — purely additive template method.
- **No CMake changes** required.

## Usage Example

```cpp
// Scenario: build a capability list and remove one entry based on a runtime flag.

Core::JSON::ArrayType<Core::JSON::String> caps;
caps.Add() = _T("network");
caps.Add() = _T("display");
caps.Add() = _T("audio");     // index 2

ASSERT(caps.Length() == 3);

// Runtime check — audio capability not available on this SKU.
if (!AudioAvailable()) {
    caps.Remove(2);           // erases "audio"
}

string json;
caps.ToString(json);
// json == ["network","display"]    (if audio removed)

// Append more entries after removal.
caps.Add() = _T("hdmi");
ASSERT(caps.Length() == 3);   // network, display, hdmi
```
