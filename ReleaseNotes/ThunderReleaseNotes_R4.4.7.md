# Thunder Release Notes R4.4.7

## Introduction

This document describes the changes introduced in Thunder R4.4.7 (compared to the R4.4.6 release).

The changes described here are present on the [R4_4 branch](https://github.com/rdkcentral/Thunder/tree/R4_4) and represent all commits since the [R4.4.6 tag](https://github.com/rdkcentral/Thunder/compare/R4.4.6...R4_4).

---

## Thunder

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

*Implemented in PR [#2176](https://github.com/rdkcentral/Thunder/pull/2176), ported to R4_4 in PR [#2193](https://github.com/rdkcentral/Thunder/pull/2193).*

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

*Implemented in PR [#2186](https://github.com/rdkcentral/Thunder/pull/2186), ported to R4_4 in PR [#2194](https://github.com/rdkcentral/Thunder/pull/2194).*

---

#### Feature: `std::vector<uint8_t>` and Delimiter Support in Serialization Helpers

The serialization helpers in `Source/core/Serialization.h` / `Serialization.cpp` are extended to:

1. **`std::vector<uint8_t>` overloads** — `ToHexString`, `FromHexString`, `ToString` (Base64),
   and `FromString` (Base64) now have overloads that accept or return `std::vector<uint8_t>`,
   removing the need for callers to manage raw buffer sizes manually.

2. **Delimiter support** — `ToHexString` and `FromHexString` now accept an optional
   `TCHAR delimiter` parameter (default `'\0'`, meaning no delimiter). When a delimiter is
   supplied the hex pairs are separated by that character on output and expected when parsing
   on input (e.g. `delimiter = ':'` produces / consumes `"de:ad:be:ef"`).

3. **Buffer-size type widened to `uint32_t`** — The length/size parameters for the raw-buffer
   overloads of `ToHexString`, `FromHexString`, `ToString`, and `FromString` are widened from
   `uint16_t` to `uint32_t`, lifting the 64 KB ceiling on single-call operations. Backward-
   compatible `uint16_t` and `uint8_t` bridge overloads are retained so existing callers
   continue to compile without modification.

*Implemented in PR [#2180](https://github.com/rdkcentral/Thunder/pull/2180).*

---

#### Feature: Telemetry Configuration in the Thunder Daemon Config Template

The Thunder daemon configuration template (`Source/Thunder/Thunder.conf.in`) is extended with two
new messaging settings that wire up the T2 Telemetry backend for RDK builds:

1. **`messaging.flush`** — a new boolean field in the generated `Thunder.conf` that controls
   whether the message engine flushes log output synchronously. It is driven by the new
   `FLUSH_LOGS` CMake build option added to `Source/Thunder/GenericConfig.cmake`.

2. **`messaging.telemetry` section** — a new block in the generated config that configures the
   telemetry output route for the Thunder message engine:
   ```json
   "telemetry": {
     "abbreviated": true,
     "output": "handler",
     "settings": [ { "enabled": false } ]
   }
   ```
   - `abbreviated` enables compact telemetry event encoding.
   - `output: "handler"` routes telemetry messages to the registered handler backend
     (i.e. `TelemetryBackendT2` inside `MessagingControl` when built with
     `PLUGIN_MESSAGECONTROL_TELEMETRY_T2=ON`).
   - `enabled` defaults to **`false`** in the upstream template. In RDKE MW Yocto builds the
     recipe overrides this to `true` and sets `PLUGIN_MESSAGECONTROL_TELEMETRY_T2=ON`,
     which links `libThunderTelemetryBackendT2` into `libThunderMessageControl.so` and
     forwards Thunder messaging events to the RDK T2 agent via `t2_event_s`,
     `t2_event_d`, and `t2_event_f`.

Thunder plugins do not call the T2 API directly; they emit through the existing Thunder
messaging framework, which routes to T2 via the `TelemetryBackendT2` backend that is part
of the `MessagingControl` plugin in [ThunderExtensions/R4_4](https://github.com/rdkcentral/ThunderExtensions/tree/R4_4/MessagingControl).

*Implemented in PR [#2200](https://github.com/rdkcentral/Thunder/pull/2200).*

---

### Changes and Bug Fixes

Thunder R4.4.7 does not introduce dedicated bug-fix-only commits beyond those present in
R4.4.6. The changes above are purely additive API extensions.

---

### Breaking Changes

Thunder R4.4.7 does not introduce intentional breaking changes relative to R4.4.6.

- All existing interface signatures remain unchanged.
- The new `FromObject()` and `Insert()`/`Remove()` APIs are purely additive.
- The serialization helper extensions are backward-compatible: existing call sites using
  `uint16_t` lengths continue to compile and behave identically.

---
