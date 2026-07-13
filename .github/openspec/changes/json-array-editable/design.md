## Context

`JSON::ArrayType<ELEMENT>` stores elements in a `std::list<ELEMENT>` named `_data`
(confirmed in `Source/core/JSON.h`). `Length()` returns `_data.size()` cast to
`uint16_t`. `operator[](uint32_t index)` walks the list with a `skip` counter —
an O(n) linear scan already accepted as correct for JSON array sizes in this domain.

`std::list::erase(iterator)` is O(1) after the iterator is obtained; obtaining the
iterator for a given index requires the same O(n) scan as `operator[]`. There is
therefore no performance regression from implementing `Remove(index)` using the
same walk pattern.

The existing `Clear()` sets `_state = 0` and calls `_data.clear()` — this is the
reference pattern for list mutation. `Remove` follows the same approach: walk to
position, call `_data.erase(iterator)`.

## Goals / Non-Goals

**Goals:**
- Add `Remove(uint32_t index)` to `ArrayType<ELEMENT>`.
- `Length()` accurately reflects the element count after removal.
- `ToString()` serialises only the remaining elements.
- `operator[]` and `Get()` continue to work correctly after removal (indices shift
  down, consistent with list semantics).
- Unit tests covering append, remove-middle, remove-first, remove-last, clear, and
  serialisation after each operation.

**Non-Goals:**
- `Remove(const ELEMENT& value)` (remove by value / predicate) — out of scope for
  R4.4.7; index-based removal is sufficient for the identified use-cases.
- `Insert(uint32_t index, element)` — not requested in this US; `Add` (append) is
  sufficient for incremental construction.
- Thread-safety: same caller-holds-lock contract as `Add` and `Clear`.
- Bounds-checking in release builds: consistent with existing `operator[]`
  which uses `ASSERT` only.

## Decisions

### Decision 1 — Walk pattern identical to `operator[]`

`_data` is a `std::list`, so a random-access iterator is not available.
`operator[]` already uses a `while(skip--)` walk. `Remove` reuses the same
pattern and then calls `_data.erase(locator)`. Code duplication is minimal
(4 lines); extracting a shared `_iteratorAt(index)` helper would be over-
engineering for a two-call-site pattern.

### Decision 2 — ASSERT for out-of-range, not exception

Consistent with the existing `operator[]` contract (`ASSERT(index < Length())`).
Thunder core avoids C++ exceptions throughout; an out-of-range `Remove` in debug
builds asserts and in release builds has undefined behaviour (list iterator
invalidation) — same as `operator[]`. This matches developer expectations.

### Decision 3 — No `_state` mutation on Remove

`_state` tracks `SET` and `UNDEFINED` flags. `IsSet()` returns true if
`Length() > 0 || (_state & SET)`. After `Remove`, if the list becomes empty,
`IsSet()` naturally returns `false` (because `Length() == 0` and `_state` was
never explicitly SET). This is correct behaviour without additional `_state`
manipulation.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| O(n) walk per `Remove` call | Consistent with `operator[]`; JSON array sizes in Thunder are small (rarely >100 elements). |
| Index shift after remove may confuse callers iterating with stored indices | Documented in API comment; caller must not cache indices across mutations — same as any `std::list` erase. |
