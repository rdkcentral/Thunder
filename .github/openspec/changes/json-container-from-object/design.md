## Context

`IElement` (the common base of all Thunder JSON types) provides two virtual
methods that are the foundation of `FromObject`:
- `ToString(string& result)` — serialises any element to its JSON text form
- `FromString(const string& text)` — populates any element from JSON text

`FromString` on `IElement` is a **static** helper (`IElement::FromString(text, object)`)
that drives the streaming `Deserialize` state machine; `Container::FromString`
is an instance convenience wrapper that calls it on `*this`.

`Container::Deserialize` finds each incoming key via `Find(label)`, which searches
`_data` and falls back to `Request(label)`. The default `Request` returns `false`;
`VariantContainer` overrides it to allocate a new `Variant` slot on demand. Keys
not found by `Find` are **silently skipped** — identical to the behaviour of
`FromString` on unknown keys. Registered slots NOT present in the incoming JSON
are **left untouched** — `Deserialize` never clears them. This is the foundation
of the deep-recursive-update semantics.

### Whole-object round-trip via IElement

`FromObject(const IElement& source)` is implemented as:
```cpp
string json;
source.ToString(json);   // IElement::ToString — works for Container, VariantContainer, any type
return FromString(json); // Container::FromString → IElement::FromString(json, *this)
```

No field-by-field iteration. No access to `source._data`. No `Find`/`Request`
calls needed in `FromObject` itself — all of that is handled inside `Deserialize`.
The implementation is two lines and is fully type-agnostic on the source side.

### Why `const IElement&` not `const Container&`

`ToString` and `FromString` live on `IElement`, not on `Container`. The
round-trip works for any `IElement` — `Container`, `VariantContainer`, a
`JSON::String` (though merging a scalar into a container is an unusual use).
Using `const IElement& source` means `Container::FromObject(variantContainer)`
and `VariantContainer::FromObject(typedContainer)` are covered by a single
method.

### Container vs. VariantContainer — different outcomes

| Target type | `FromObject` behaviour |
|---|---|
| Typed `Container` subclass | Updates only pre-registered slots; unknown keys silently skipped; never adds fields |
| `VariantContainer` | Updates existing entries; creates new `Variant` slots via `Request()` |

The "accumulate new fields from multiple sources into one object" use case is
only possible with `VariantContainer`.

### `FromObject` vs. copy assignment on `VariantContainer`

`VariantContainer::operator=` replaces all content. `FromObject` merges: absent
entries in `source` are preserved in `*this`. These are distinct operations.

### Null model — finding from code review

`IElement::IsNull()` is a virtual on the interface. However, `Container::Null(true)`
sets only `UNDEFINED` (not `SET`), while all scalar types set both `SET|UNDEFINED`.
Because `FromObject` delegates entirely to `FromString`, null handling follows
`FromString` semantics. Null-aware merging is deferred to US-1.5/US-1.6.

The inconsistency in `Container::Null()` is a **finding for the US-1.5 impact
analysis**: scalar null fields ARE included in `ToString()` output (since they
are "set"), while a null `Container` is only included if its sub-fields are set.

## Goals / Non-Goals

**Goals:**
- Add `FromObject(const IElement& source)` to `JSON::Container`.
- Implementation: `source.ToString(json); return this->FromString(json)`.
- Typed `Container`: selective update of registered slots; never adds new fields.
- `VariantContainer`: updates existing and inserts new slots via `Request()`.
- Deep recursive update at all nesting levels (absent target fields preserved).
- Null/unset handling delegated to `FromString` — no special-casing in `FromObject`.
- Zero impact on existing `FromString()`, `Add()`, `Remove()` paths.
- Full unit-test coverage.

**Non-Goals:**
- Null-aware merging — deferred to US-1.5 / US-1.6.
- Adding a virtual `FromObject` to `IElement` — would affect every concrete type.
- Thread-safety inside `FromObject()` — caller-holds-lock, same as `Add`/`Remove`.
- Error return value — `FromObject` returns `bool` (the return of `FromString`).

## Decisions

### Decision 1 — Whole-object `ToString`/`FromString` round-trip, not field iteration

The implementation is `source.ToString(json); return this->FromString(json)`.

This is simpler, correct, and leverages the existing `IElement` machinery fully:
- `ToString` serialises the entire source including nested objects recursively
- `FromString` / `Deserialize` handles all cases: registered slots, unknown keys,
  `VariantContainer` dynamic slots, nested recursion, null, type mismatch

**Rejected:** Field-by-field iteration with per-field `ToString`/`FromString` —
more code, same result, and would require duplicating `Find`/`Request` logic that
`Deserialize` already implements correctly.

**Rejected:** Virtual `Clone()` on `IElement` — ABI change to a widely-implemented
interface, not justified for R4.4.7.

### Decision 2 — Parameter `const IElement&`, not `const Container&`

`ToString` and `FromString` are on `IElement`. Using `const IElement&` requires
no downcasting, enables cross-type calls (`VariantContainer` ↔ typed `Container`),
and is consistent with how `FromString` itself accepts any `IElement`.

### Decision 3 — `FromObject` returns `bool` (return of `FromString`)

`FromString` returns `bool` (success/failure). `FromObject` propagates this return.
This is more useful than `void` and costs nothing.

### Decision 4 — Null/unset handling delegated to `FromString`

No special `IsNull()`/`IsSet()` guards in `FromObject`. `ToString` naturally
omits unset fields (they don't serialise). Null handling follows `FromString`
semantics — consistent, no extra code, and the null model audit belongs to US-1.5.

### Decision 5 — `IsComplete` handled by `FromString`

`FromString` → `Deserialize` correctly sets `COMPLETE` when the `}` is parsed.
No manual `_state` manipulation needed in `FromObject`.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| `source.ToString()` allocates a string for the full JSON text | Only on construction/response-building path, not JSON-RPC read path. Acceptable. |
| Typed `Container` silently ignores source keys with no registered slot | Documented behaviour; identical to `FromString`. Caller must use `VariantContainer` for dynamic accumulation. |
| `Container::Null()` inconsistency affects round-trip of null Container fields | Finding raised for US-1.5 impact analysis; no code change in this US. |
| `VariantContainer` target entries absent from source are preserved, not deleted | This IS the desired merge semantics; distinguished from `operator=` (replace). Documented. |
