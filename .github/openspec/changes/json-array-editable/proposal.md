## Why

Thunder middleware plugins build `JSON::ArrayType<T>` responses incrementally.
The existing API covers append (`Add`) and random read access (`operator[]`,
`Get`), but provides no way to remove an element by position or insert one at an
arbitrary position. Both operations are needed to support runtime-built
capability lists and response arrays where elements must be removed or
re-ordered without rebuilding the whole array.

## What Changes

Two new methods are added to `JSON::ArrayType<ELEMENT>`:

| Method | Description |
|---|---|
| `void Remove(uint32_t index)` | Erases the element at zero-based `index` |
| `void Insert(uint32_t index, const ELEMENT& element)` | Inserts a copy of `element` before position `index` |

All existing methods (`Add`, `operator[]`, `Get`, `Length`, `Clear`,
`Elements`) are unchanged.

## Capabilities

### New Capabilities
- `json-array-remove`: `void ArrayType<ELEMENT>::Remove(uint32_t index)` —
  erases the element at zero-based `index`. `index` must be less than `Length()`;
  behaviour for out-of-range index matches `operator[]` (ASSERT in debug builds).
  Elements after `index` shift down by one.

- `json-array-insert`: `void ArrayType<ELEMENT>::Insert(uint32_t index, const ELEMENT& element)` —
  inserts a copy of `element` before position `index`. Valid range is
  `0 <= index <= Length()`. Inserting at `index == Length()` is equivalent to
  `Add(element)`. Elements at `index` and beyond shift up by one.

### Modified Capabilities
<!-- none -->

## Impact

- **`Source/core/JSON.h`** — two new template methods on `ArrayType<ELEMENT>`.
- **No ABI break** — purely additive.
- **No CMake changes** required.

## Usage Examples

### Example 1 — Remove an element by index
```cpp
Core::JSON::ArrayType<Core::JSON::String> caps;
caps.Add() = _T("HDMI");    // index 0
caps.Add() = _T("4K");      // index 1
caps.Add() = _T("HDR");     // index 2

ASSERT(caps.Length() == 3);

// Runtime check — HDR not available on this SKU.
caps.Remove(2);

ASSERT(caps.Length() == 2);
// Serialises as: ["HDMI","4K"]
```

### Example 2 — Remove middle element; remaining elements shift
```cpp
Core::JSON::ArrayType<Core::JSON::String> tags;
tags.Add() = _T("alpha");   // 0
tags.Add() = _T("beta");    // 1
tags.Add() = _T("gamma");   // 2

tags.Remove(1);  // removes "beta"

// tags[0] == "alpha",  tags[1] == "gamma"
// Serialises as: ["alpha","gamma"]
```

### Example 3 — Insert at a position
```cpp
Core::JSON::ArrayType<Core::JSON::String> pipeline;
pipeline.Add() = _T("decode");    // 0
pipeline.Add() = _T("render");    // 1

// Insert "scale" between "decode" and "render".
Core::JSON::String scale(_T("scale"));
pipeline.Insert(1, scale);

// pipeline[0]=="decode", pipeline[1]=="scale", pipeline[2]=="render"
// Serialises as: ["decode","scale","render"]
```

### Example 4 — Insert at end (equivalent to Add)
```cpp
Core::JSON::ArrayType<Core::JSON::DecUInt32> ids;
ids.Add() = 10;
ids.Add() = 20;

Core::JSON::DecUInt32 newId;
newId = 30;
ids.Insert(ids.Length(), newId);  // append — same as Add(newId)

// Serialises as: [10,20,30]
```

### Example 5 — Combined Remove and Insert (move an element)
```cpp
Core::JSON::ArrayType<Core::JSON::String> steps;
steps.Add() = _T("A");   // 0
steps.Add() = _T("B");   // 1
steps.Add() = _T("C");   // 2

// Move "A" to position 2.
Core::JSON::String moved = steps[0];   // copy value
steps.Remove(0);                        // ["B","C"]
steps.Insert(1, moved);                 // ["B","A","C"]
```
