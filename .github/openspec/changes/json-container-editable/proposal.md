## Why

Thunder middleware plugins need to build and mutate `JSON::Container` objects
dynamically at runtime — for example, constructing a telemetry payload
incrementally as sub-systems report their state, or patching a cached response
object when a single field changes. Today `Container` supports `Add()` to
append a new field and `Remove()` to remove one by label, but there is no
first-class `Set(key, value)` operation to update the value of an already-registered
field in a typed or generic container. Developers work around this with
`Clear()` + re-add, which is fragile and loses unrelated fields.

## What Changes

`JSON::Container` gains a documented, explicitly-supported `Set(label, value)`
mutation path. The thread-safety contract for `Add`, `Set`, and `Remove` is
made explicit in the API comments (caller-holds-lock). `Remove` already exists
but is underdocumented; this change improves its contract as well.

Existing `Add(const TCHAR label[], IElement* element)` and
`Remove(const TCHAR label[])` signatures are **not changed** — they already
exist (lines 3862 / 3867 of `Source/core/JSON.h`). The work is:

1. Add `Set(const TCHAR label[], const IElement& value)` — update the value of
   an existing field in-place via `FromString` round-trip.
2. Add doxygen-style API comments to `Add`, `Set`, and `Remove` documenting the
   thread-safety contract.
3. Unit-test all three mutations and serialisation after each.

## Capabilities

### New Capabilities
- `json-container-set`: `JSON::Container::Set(const TCHAR label[], const IElement& value)` —
  updates the value of an already-registered field by label. If the label does
  not exist in the container, the call is a no-op (with a debug-trace warning),
  consistent with how `Remove` silently skips unknown labels.

### Modified Capabilities
- `json-container-mutations`: `Add` and `Remove` on `JSON::Container` — no
  behaviour change, but thread-safety contract and semantics are now formally
  documented in API comments.

## Impact

- **`Source/core/JSON.h`** — one new `Set` method on `Container`; API-comment
  additions to `Add` and `Remove`.
- **No ABI break** — purely additive.
- **No CMake changes** required.

## Usage Example

```cpp
// Scenario: update a single cached field without rebuilding the whole container.

Core::JSON::Container status;
Core::JSON::String state;
Core::JSON::String build;

state = _T("initialising");
build = _T("R4.4.7");
status.Add(_T("state"), &state);
status.Add(_T("build"), &build);

// ... later, after activation completes ...
Core::JSON::String newState(_T("active"));
status.Set(_T("state"), newState);   // overwrites only "state"

string json;
status.ToString(json);
// json == {"state":"active","build":"R4.4.7"}

// Remove a field entirely.
status.Remove(_T("build"));
status.ToString(json);
// json == {"state":"active"}
```
