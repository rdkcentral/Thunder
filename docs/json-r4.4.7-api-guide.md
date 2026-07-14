# Thunder JSON — New APIs (R4.4.7)

This document describes the new APIs introduced in Thunder R4.4.7 for working
with JSON containers and arrays. It is intended for plugin developers who use
`Core::JSON` types directly.

---

## Thread Safety

None of the APIs described in this document perform internal locking.
**The caller is responsible for holding any necessary lock** before calling
`FromObject`, any mutation method on a Container, or `Remove`/`Insert` on an
array. This is consistent with all existing Thunder JSON mutation methods
(`Add`, `Remove`, `Set`, `Clear`).

Typically this means protecting access to a shared JSON object with a
`Core::CriticalSection` in the same way as any other shared data in a plugin.

---

## 1. `FromObject()` — Populate a Container from Another JSON Element

`FromObject()` follows the same rules as `FromString()` and `FromFile()`:
**the target is fully cleared before import**.

```cpp
bool Container::FromObject(const IElement& source);
```

| Behaviour | Detail |
|---|---|
| Target is cleared first | All existing fields are removed before any values are imported from `source` |
| Only set values imported | Fields in `source` where `IsSet() == false` are discarded |
| Null values round-tripped | `null` fields in `source` are preserved in the target |
| Parameter is `IElement&` | Accepts any JSON type: typed Container, VariantContainer, or ArrayType |

---

### 1.1 Typed Container target

Only fields that are **registered** in the target (declared in the constructor
with `Add()`) can receive values. Unknown keys from `source` are silently
skipped. No new fields are ever added to a typed Container.

```cpp
struct DeviceInfo : public Core::JSON::Container {
    DeviceInfo() : model(), firmware() {
        Add(_T("model"),    &model);
        Add(_T("firmware"), &firmware);
    }
    Core::JSON::String model;
    Core::JSON::String firmware;
};

// ── Populate from another typed Container ────────────────────────────────────
DeviceInfo source;
source.model    = _T("ES1-A");
source.firmware = _T("R4.4.7");

DeviceInfo target;
target.model = _T("old");   // will be cleared and replaced
target.FromObject(source);
// target.model == "ES1-A",  target.firmware == "R4.4.7"

// ── Populate from a VariantContainer ─────────────────────────────────────────
JsonObject patch;
patch.Set(_T("model"),   JsonValue(string("ES1-B")));
patch.Set(_T("extra"),   JsonValue(string("ignored")));  // not registered

target.FromObject(patch);
// target.model == "ES1-B"   (registered — updated)
// target.firmware unset     (not in patch — Clear() removed it)
// "extra" silently skipped  (not registered in DeviceInfo)
```

---

### 1.2 VariantContainer target

`VariantContainer` (aliased as `JsonObject`) dynamically creates slots for any
incoming key. Combined with the Clear-first rule, `FromObject` **replaces** the
entire contents of the target with those from `source`. This is not a merge.

```cpp
// ── Replace semantics ─────────────────────────────────────────────────────────
JsonObject source;
source.Set(_T("a"), JsonValue(1));
source.Set(_T("b"), JsonValue(2));

JsonObject target;
target.Set(_T("b"), JsonValue(99));
target.Set(_T("c"), JsonValue(3));   // will be gone after FromObject

target.FromObject(source);
// target: a=1, b=2   —   "c" is gone

// ── Populate from a typed Container ──────────────────────────────────────────
DeviceInfo info;
info.model    = _T("ES1-A");
info.firmware = _T("R4.4.7");

JsonObject result;
result.FromObject(info);
// result: model="ES1-A", firmware="R4.4.7"
```

> **Tip:** If you want to *merge* two `VariantContainer` objects (preserve keys
> absent from the source), use `VariantContainer::operator=` which performs a
> field-level reconciliation rather than a full replace.

---

## 2. Modifying JSON Containers

No new API is required. Thunder containers are already fully mutable through the
patterns below.

---

### 2.1 Typed Container — direct member access

Fields in a typed `Container` subclass are plain public member variables. Assign
to them directly; call `Clear()` on a member to remove it from serialised output.

```cpp
struct PluginStatus : public Core::JSON::Container {
    PluginStatus() : state(), version(), clients(0) {
        Add(_T("state"),   &state);
        Add(_T("version"), &version);
        Add(_T("clients"), &clients);
    }
    Core::JSON::EnumType<PluginState> state;
    Core::JSON::String                version;
    Core::JSON::DecUInt32             clients;
};

PluginStatus s;

// Add / update
s.state   = PluginState::ACTIVATED;
s.version = _T("R4.4.7");
s.clients = 3;

// Remove from output — clear the member
s.version.Clear();          // IsSet()==false → omitted from ToString()

string json;
s.ToString(json);
// → {"state":"ACTIVATED","clients":3}   ("version" omitted)
```

---

### 2.2 VariantContainer — `Set()`, `operator[]`, `Remove()`

`VariantContainer` (aliased as `JsonObject`) supports a full dynamic mutation API.

```cpp
JsonObject obj;

// Add a field
obj.Set(_T("model"),   JsonValue(string("ES1-A")));
obj.Set(_T("clients"), JsonValue(3));

// Update — Set() overwrites if the key already exists
obj.Set(_T("model"), JsonValue(string("ES1-B")));

// Read
string model      = obj.Get(_T("model")).String();  // "ES1-B"
JsonValue& ref    = obj[_T("clients")];              // reference to live entry
ref               = JsonValue(5);                    // update in place

// Remove a field
obj.Remove(_T("clients"));

string json;
obj.ToString(json);
// → {"model":"ES1-B"}
```

---

## 3. `ArrayType<T>` — `Remove()` and `Insert()`

Two new methods on `JSON::ArrayType<ELEMENT>` allow elements to be removed from
or inserted at any position.

---

### 3.1 `Remove(uint32_t index)`

```cpp
void ArrayType<ELEMENT>::Remove(uint32_t index);
```

Erases the element at zero-based `index`. Elements after `index` shift down.

**Complexity:** O(n) — the internal storage is a `std::list`; reaching `index`
requires a linear walk. The erase itself is O(1) once the position is found.
`Add()` (append) remains O(1) and is unaffected.

```cpp
Core::JSON::ArrayType<Core::JSON::String> caps;
caps.Add() = _T("HDMI");   // 0
caps.Add() = _T("4K");     // 1
caps.Add() = _T("HDR");    // 2

caps.Remove(2);             // removes "HDR"

// caps.Length() == 2
// Serialises as: ["HDMI","4K"]
```

```cpp
// Remove middle element — remaining elements shift down
Core::JSON::ArrayType<Core::JSON::String> tags;
tags.Add() = _T("alpha");   // 0
tags.Add() = _T("beta");    // 1
tags.Add() = _T("gamma");   // 2

tags.Remove(1);  // removes "beta"
// Serialises as: ["alpha","gamma"]
```

---

### 3.2 `Insert(uint32_t index, const ELEMENT& element)`

```cpp
void ArrayType<ELEMENT>::Insert(uint32_t index, const ELEMENT& element);
```

Inserts a copy of `element` before position `index`. Valid range is
`0 <= index <= Length()`. Inserting at `index == Length()` is equivalent to
`Add(element)`. Elements at `index` and beyond shift up.

**Complexity:** O(n) — same linear walk as `Remove` to reach `index`; the
list insertion itself is O(1). For pure append use `Add()` (O(1)) instead.
Avoid repeated `Insert(0, …)` in a loop on large arrays.

```cpp
// Insert between existing elements
Core::JSON::ArrayType<Core::JSON::String> pipeline;
pipeline.Add() = _T("decode");   // 0
pipeline.Add() = _T("render");   // 1

Core::JSON::String scale(_T("scale"));
pipeline.Insert(1, scale);

// Serialises as: ["decode","scale","render"]
```

```cpp
// Insert at start
Core::JSON::ArrayType<Core::JSON::DecUInt32> ids;
ids.Add() = 20;
ids.Add() = 30;

Core::JSON::DecUInt32 first;
first = 10;
ids.Insert(0, first);

// Serialises as: [10,20,30]
```

```cpp
// Move an element — combine Remove and Insert
Core::JSON::ArrayType<Core::JSON::String> steps;
steps.Add() = _T("A");   // 0
steps.Add() = _T("B");   // 1
steps.Add() = _T("C");   // 2

Core::JSON::String a = steps[0];   // copy "A"
steps.Remove(0);                    // ["B","C"]
steps.Insert(1, a);                 // ["B","A","C"]
```
