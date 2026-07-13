## Why

Thunder plugin developers need to populate a typed `JSON::Container` or
`VariantContainer` from the content of another JSON object — for example,
updating a cached response from a freshly-populated source container, or
accumulating fields from multiple sub-system containers into one `VariantContainer`
response.

`IElement` already provides `FromString()` (populate from serialised text) and
`ToString()` (serialise to text). `FromObject()` closes the gap by accepting
another `IElement` directly — `container.FromObject(other)` is semantically
equivalent to `container.FromString(other.ToString())`, without the intermediate
string the caller would otherwise have to manage.

> **Container vs VariantContainer:** the two cases behave differently:
> - **Typed `Container`** — `FromObject` updates only pre-registered field slots.
>   It never adds new fields. It is a selective update, not a flat merge.
> - **`VariantContainer`** — `FromObject` updates existing entries AND creates new
>   `Variant` slots for keys not yet present (via `Request()` override). This is
>   the correct type for the "accumulate fields from multiple sources" use case.

## What Changes

A new method `FromObject(const IElement& source)` is added to `JSON::Container`
(defined in `Source/core/JSON.h`). The implementation is a two-line whole-object
round-trip — no field-by-field iteration, no access to source internals:

```cpp
bool FromObject(const IElement& source) {
    string json;
    source.ToString(json);
    return FromString(json);
}
```

The parameter type is `const IElement&` (not `const Container&`), making
`Container::FromObject(variantContainer)` and `VariantContainer::FromObject(container)`
work without overloading. No existing API is modified.

## Behavioral Specification

### Deep recursive update — not shallow replace
`FromObject` delegates entirely to `FromString`, which calls `Container::Deserialize`
recursively. `Deserialize` **only updates keys present in the incoming JSON**;
all other registered slots are left untouched. This gives deep-recursive-update
semantics at every level of nesting:

- If target has `"device": {"model":"X", "region":"EU"}` and source has
  `"device": {"model":"Y"}`, after `FromObject` the target has
  `"device": {"model":"Y", "region":"EU"}` — `region` is **preserved**.

### Typed `Container` — only registered fields updated; no new fields added
`Container::Deserialize` calls `Find(label)` per key. For a typed `Container`
subclass, `Find` returns `nullptr` for unregistered keys and `Request()` returns
`false` — unknown keys are silently skipped. **`FromObject` on a typed `Container`
never adds new fields.** The "accumulate from multiple sources into one flat
object" use case requires `VariantContainer`, not plain `Container`.

### `VariantContainer` — new fields inserted dynamically
`VariantContainer` overrides `Request()` to allocate a new `Variant` slot on
demand. `Find` succeeds for every incoming key, including previously absent ones.
`FromObject` on a `VariantContainer` both updates existing entries AND inserts
new ones.

### `FromObject` vs. copy assignment on `VariantContainer`
`VariantContainer::operator=(const VariantContainer&)` is an assign-and-replace
(all prior content overwritten). `FromObject` is a merge: existing entries are
updated, new ones inserted, but entries in `*this` absent from `source` are
**preserved**. These are distinct operations.

### Null and unset fields
`FromObject` delegates to `FromString`; null/unset handling follows `FromString`
semantics. Null-aware merging is deferred to US-1.5 / US-1.6 — see the null
model inconsistency finding in the design document.

### Thread safety
Not internally locked. Caller-holds-lock contract, same as `Add()` and `Remove()`.

## Capabilities

### New Capabilities
- `json-container-from-object`: `JSON::Container::FromObject(const IElement& source)` —
  populates this container from the JSON representation of `source` (equivalent
  to `this->FromString(source.ToString())`). On a typed `Container`: selectively
  updates pre-registered slots; never adds new fields. On `VariantContainer`:
  updates existing and inserts new slots. Deep recursive update at all nesting
  levels — absent fields in target are preserved.

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

### Example 1 — VariantContainer: accumulate fields from two sub-systems
```cpp
// The "accumulate fields from multiple sources" use case requires VariantContainer.
// A plain typed Container only updates pre-registered slots — it cannot add new fields.

Core::JSON::VariantContainer networkInfo;
networkInfo.Set(_T("interface"), Core::JSON::Variant(string("eth0")));
networkInfo.Set(_T("ip"),        Core::JSON::Variant(string("192.168.1.5")));

Core::JSON::VariantContainer deviceInfo;
deviceInfo.Set(_T("model"),    Core::JSON::Variant(string("ES1-A")));
deviceInfo.Set(_T("firmware"), Core::JSON::Variant(string("R4.4.7")));

Core::JSON::VariantContainer response;
response.FromObject(networkInfo);   // inserts interface + ip
response.FromObject(deviceInfo);    // inserts model + firmware

string json;
response.ToString(json);
// json == {"interface":"eth0","ip":"192.168.1.5","model":"ES1-A","firmware":"R4.4.7"}
```

### Example 2 — typed Container: selective update; unregistered keys skipped
```cpp
struct DeviceResponse : public Core::JSON::Container {
    DeviceResponse() : model(), firmware() {
        Add(_T("model"),    &model);
        Add(_T("firmware"), &firmware);
    }
    Core::JSON::String model;
    Core::JSON::String firmware;
};

DeviceResponse target;
target.model    = _T("ES1-A");
target.firmware = _T("R4.4.6");

// Source has updated firmware plus a key the target doesn't know about.
Core::JSON::VariantContainer patch;
patch.Set(_T("firmware"), Core::JSON::Variant(string("R4.4.7")));
patch.Set(_T("extra"),    Core::JSON::Variant(string("ignored")));

target.FromObject(patch);   // == target.FromString(patch.ToString())

string json;
target.ToString(json);
// json == {"model":"ES1-A","firmware":"R4.4.7"}
// "extra" silently skipped — no registered slot.
// "model" preserved — absent from source, untouched in target.
```

### Example 3 — nested typed Container: deep recursive update preserves absent fields
```cpp
struct DeviceInfo : public Core::JSON::Container {
    DeviceInfo() : model(), region() {
        Add(_T("model"),  &model);
        Add(_T("region"), &region);
    }
    Core::JSON::String model;
    Core::JSON::String region;
};
struct Response : public Core::JSON::Container {
    Response() { Add(_T("device"), &device); }
    DeviceInfo device;
};

Response target;
target.device.model  = _T("ES1-A");
target.device.region = _T("EU");

// Source only carries 'model' inside device.
Core::JSON::VariantContainer patch;
Core::JSON::VariantContainer patchDevice;
patchDevice.Set(_T("model"), Core::JSON::Variant(string("ES1-B")));
patch.Set(_T("device"), Core::JSON::Variant(patchDevice));

target.FromObject(patch);

string json;
target.ToString(json);
// json == {"device":{"model":"ES1-B","region":"EU"}}
// "region" is PRESERVED — FromString recurses into the nested Container
// and only updates keys present in the incoming JSON.
```
