## Context

`JSON::Container` already has `Add(const TCHAR label[], IElement* element)` and
`Remove(const TCHAR label[])` defined in `Source/core/JSON.h` (confirmed at lines
3862 and 3867). `_data` is a `std::list<JSONLabelValue>` where `JSONLabelValue`
is `std::pair<const TCHAR*, IElement*>`.

`Set(label, value)` must locate the existing `IElement*` for the given label and
update its content. Because `IElement` is a heterogeneous type hierarchy (String,
Number, Boolean, Container, ArrayType…), the update cannot be a direct pointer
swap without ownership complications. The cleanest in-tree approach is the same
serialise-then-deserialise round-trip used by `FromObject()`:
`existing->FromString(value.ToString())`.

Thread-safety: `Container` has no internal lock. The pattern used everywhere in
Thunder for shared `Container` objects is caller-managed locking (a
`Core::CriticalSection` around the mutation block). This contract will be documented
in API comments on all three mutation methods, making it explicit rather than
implicit.

## Goals / Non-Goals

**Goals:**
- Add `Set(const TCHAR label[], const IElement& value)` to `Container`.
- Document thread-safety contract on `Add`, `Set`, and `Remove`.
- Ensure `IsSet()` returns `true` after any mutation that stores a value, and
  `false` (or as expected) after `Remove`.
- Unit tests covering add → serialise, set → serialise, remove → serialise.

**Non-Goals:**
- Adding an internal lock to `Container` — would change existing single-threaded
  usage overhead and is not warranted for R4.4.7.
- `Set` creating a new field if the label is absent — this is `Add`'s job; `Set`
  is strictly an update operation. Mixing them would hide developer errors.
- Typed `Set<T>(label, T value)` template overloads — the `IElement&` overload is
  sufficient for R4.4.7; typed overloads can follow.

## Decisions

### Decision 1 — `Set` is a no-op (with trace) for unknown labels, not an error

Returning an error code (e.g. `bool`) would change the function signature relative
to the void `Add` / `Remove` pattern and force callers to handle it. A
`TRACE_L1("Set: label not found")` debug warning preserves the "fire-and-forget"
mutation style used throughout Thunder plugins while flagging developer mistakes
during development.

### Decision 2 — Value update via IElement round-trip (ToString + FromString)

Same rationale as `FromObject` Decision 1. Avoids adding a `Clone()` virtual to
`IElement`.

### Decision 3 — Document, not change, `Add` and `Remove` thread-safety

Both are already implemented correctly for single-threaded use. Adding locks would
change performance characteristics of existing code. Documentation is the correct
R4.4.7 action; an internal-lock variant can be a separate US if the need is proven.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| `Set` on typed (`ObjectType`) subclass calls `FromString` on a field that was registered as a typed member — round-trip must be lossless | Covered by unit test 4.3 (typed-field round-trip). |
| Developer confusion between `Add` (append) and `Set` (update) | Clear API comments and naming convention; `Set` no-ops on unknown labels to surface the mistake at test time. |
| `Remove` on a typed subclass removes a field whose pointer is still held by the subclass member — dangling pointer in `_data` after re-add | This is an existing pre-condition of `Remove`; documented as "caller must not remove fields whose pointers are owned by the declaring type" in the API comment. |
