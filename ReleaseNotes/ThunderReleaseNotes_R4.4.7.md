# Thunder Release Notes R4.4.7

## Introduction

This document describes the changes introduced in Thunder R4.4.7 (compared to the R4.4.6 release) and in ThunderTools.

The changes described here are present on the [R4_4 branch](https://github.com/rdkcentral/Thunder/tree/R4_4) and represent all commits since the [R4.4.6 tag](https://github.com/rdkcentral/Thunder/compare/R4.4.6...R4_4).

---

## Thunder and ThunderTools

### Features

#### Feature: `JSON::Container::FromObject()`

`JSON::Container` (and therefore `VariantContainer` / `JsonObject`, which inherits from it) gains a new method:

```cpp
bool FromObject(const IElement& source);
bool FromObject(const IElement& source, Core::OptionalType<Error>& error);
```

This completes the trio of populate operations alongside the existing `FromString()` and `FromFile()`:
it populates the target container from the JSON representation of any `IElement` without requiring
the caller to manually serialise the source to a string first.

**Semantics (identical to `FromString()`):**
- The target is fully **cleared** before any values are imported from `source`.
- Fields in `source` where `IsSet() == false` are **discarded** (not imported).
- `null`-valued fields in `source` are imported as-is.
- Passing a scalar type (`JSON::String`, `JSON::Number`, etc.) or an `ArrayType` as `source`
  returns `false` because the serialised form is not a JSON object.

**Behaviour per container type:**

| Target type | Result |
|---|---|
| Typed `Container` sub-class | Only pre-registered fields (added via `Add()`) are updated; unknown keys from `source` are silently skipped; no new fields are added. |
| `VariantContainer` (`JsonObject`) | Target is cleared, then all keys from `source` are inserted — the target holds exactly the keys present in `source` (replace semantics). |

`FromObject` is not thread-safe; the caller is responsible for holding any necessary lock,
consistent with all other `Container` mutation methods. The two-argument overload mirrors
the error-reporting overload available on `FromString()` and `FromFile()`.

More information and worked examples can be found in the [JSON documentation](https://rdkcentral.github.io/Thunder/utils/json/).

---

#### Feature: `JSON::ArrayType::Insert()` and `Remove()`

`JSON::ArrayType<T>` gains two new index-based mutation methods, provided as a symmetric pair:

```cpp
// Insert a default-constructed element before index, return reference to it
ELEMENT& Insert(uint32_t index);

// Insert a copy before index, return reference to the inserted copy
ELEMENT& Insert(uint32_t index, const ELEMENT& element);

// Insert by move before index, return reference to the inserted element
ELEMENT& Insert(uint32_t index, ELEMENT&& element);

// Remove element at index; returns pointer to the next element, or nullptr if
// the removed element was the last one
ELEMENT* Remove(uint32_t index);
```

**Key properties:**
- `Insert(index, …)` inserts *before* the element currently at `index`. Inserting at
  `index == Length()` is equivalent to `Add(element)`.
- `Remove(index)` removes the element at `index` and returns a pointer to the successor.
- Both operations are **O(n)** due to the `std::list<ELEMENT>` backing store; this cost is
  documented in the API so callers avoid their use in tight loops over large arrays.
- The existing `Add()` (append, O(1)), `operator[]`, `Get()`, `Length()`, and `Clear()`
  interfaces are unchanged.
- `Length()` correctly reflects the element count after any sequence of `Insert`, `Remove`,
  and `Add` calls.
- Serialised output matches the current in-memory state after any sequence of mutations.
- `Insert` and `Remove` are not thread-safe; the caller is responsible for holding any
  necessary lock.


More information and worked examples can be found in the [JSON documentation](https://rdkcentral.github.io/Thunder/utils/json/).

---

#### Feature: array encode support

It is now possible to use encode:hex, encode:mac and encode:base64 for buffer, array and std::vector types (encode:base64 already worked for the first two).

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#encode) for more information. 

---

#### Feature: (t2) Telemetry Turned on By default (RDK-e MW builds)

For RDK-E MW builds the Telemetry output service (for T2) is now enabled by default, meaning it will initialize the output engine (calling T2 initialization) and that Telemetry can be used from plugins (see [here](https://rdkcentral.github.io/Thunder/plugin/messaging/#using-the-real-t2-library-or-the-mock) for more info)

---

### General bug fixes

Some small issues were fixed in Thunder R4.4.7 on the ThunderTools code generator.

---

### Breaking Changes

Thunder R4.4.7 does not introduce breaking changes relative to R4.4.6.

---
