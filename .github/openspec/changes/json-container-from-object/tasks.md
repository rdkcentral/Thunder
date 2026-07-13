## 1. Read & Understand Existing Code

- [ ] 1.1 Read `Source/core/JSON.h` — `Container` class body: confirm `_data` layout
  (`JSONElementList`), existing `Add()`, `Remove()`, `FromString()`, `Clear()`,
  `Find()`, `Request()`, `IsComplete()`, `FindNext()`, and the public `Iterator`
- [ ] 1.2 Confirm `IElement` interface methods available for per-field round-tripping
  (`ToString`, `FromString`, `IsSet`, `IsNull`, `Clear`)
- [ ] 1.3 Read `VariantContainer::Request()` override to confirm it dynamically
  allocates slots — ensures `Find(label)` is used (not direct `_data` scan)
- [ ] 1.4 Verify `Container::Serialize` recurses into nested `Container` fields
  (lossless round-trip for any nesting depth)
- [ ] 1.5 **Null model finding — document for US-1.5:** Confirm the inconsistency:
  scalar types (`NumberType`, `String`, etc.) set both `SET|UNDEFINED` on
  `Null(true)` so `IsSet()==true`; `Container::Null(true)` sets only `UNDEFINED`
  so `IsSet()==false`. Record this as a finding in the US-1.5 impact analysis
  backlog item — no code change in this US

## 2. API Declaration (JSON.h)

- [ ] 2.1 Add the `FromObject` public method declaration to `Container` in
  `Source/core/JSON.h` with the full API-comment documenting:
  - Shallow top-level merge semantics
  - Nested containers/arrays: whole-replace (not deep-merge)
  - Null-valued source fields: propagated
  - Unset source fields: skipped
  - Duplicate-key behaviour: last-writer-wins
  - Unknown key in typed target: silently skipped
  - Type-mismatch: target slot left unchanged, TRACE_L1 in debug builds
  - `VariantContainer` target: all source keys accepted via `Request()` override
  - `IsComplete` not set
  - Thread-safety contract: caller-holds-lock
  ```cpp
  // Merges all set top-level fields from 'source' into this container.
  // Semantics:
  //   - Nested containers/arrays are whole-replaced, not deep-merged.
  //   - Duplicate keys: last-writer-wins.
  //   - Null source fields: propagated. Unset source fields: skipped.
  //   - Unknown key in typed target: silently skipped.
  //   - Type-mismatch: target slot unchanged; TRACE_L1 warning in debug builds.
  //   - IsComplete is NOT set by this method.
  // Not thread-safe — caller must hold any necessary lock.
  void FromObject(const Container& source);
  ```

## 3. Implementation (JSON.h or JSON.cpp)

- [ ] 3.1 Implement `Container::FromObject(const Container& source)` using
  the source's `_data` list (direct member access, since `FromObject` is a
  `Container` member):
  - Skip entries where `source_element->IsSet() == false` **OR** `source_element->IsNull() == true`
  - For set entries: call `source_element->ToString(buf)`
  - Call `Find(label)` on `*this` (NOT direct `_data` scan — respects `Request()`)
  - If `Find` returns non-null: call `slot->FromString(buf)`; on failure emit
    `TRACE_L1` and leave slot unchanged
  - If `Find` returns null (unregistered key in typed target): skip silently
- [ ] 3.2 Verify the implementation does NOT set `_state |= COMPLETE`
- [ ] 3.3 Verify the implementation compiles cleanly against C++11/14/17

## 4. Unit Tests

- [ ] 4.1 **Merge two non-overlapping containers** — verify all fields appear and
  serialisation is correct
- [ ] 4.2 **Merge with overlapping keys** — last-writer-wins: source value
  overwrites pre-existing target value
- [ ] 4.3 **Nested container — whole-replace semantics** — target has nested object
  with fields `{model, region}`; source has same key with `{model}` only;
  verify `region` is absent after merge
- [ ] 4.4 **Nested array** — source field is an `ArrayType`; verify array appears
  in merged output with all elements intact
- [ ] 4.5 **Null and unset source fields both skipped** — source has one null
  field (`Null(true)`) and one unset field; verify neither is copied to target;
  target slots are left unchanged

  > Not a null-propagation test — null-aware merge is deferred to US-1.5/US-1.6.
- [ ] 4.6 **Unset source field skipped** — source has a set and an unset field;
  verify only the set field is copied
- [ ] 4.8 **Type-mismatch — target slot left unchanged** — source field serialises
  as `{...}` but target slot is `JSON::String`; verify target slot unchanged
- [ ] 4.9 **VariantContainer target — all source keys accepted** — source has three
  fields of mixed types; verify all appear in `VariantContainer` output
- [ ] 4.10 **IsComplete not set** — verify `IsComplete()` remains `false` after
  `FromObject`; `IsSet()` returns `true`
- [ ] 4.11 **Chain multiple `FromObject()` calls** — three sources merged in
  sequence; verify accumulated field count and values
- [ ] 4.12 **Empty source container** — `FromObject` on default-constructed source;
  verify target is unchanged
- [ ] 4.13 **Existing `FromString()` and `Add()` regression** — run existing
  Container unit tests and confirm all still pass

## 5. Documentation

- [ ] 5.1 API-comment block in `JSON.h` at the `FromObject` declaration (task 2.1)
  is the primary documentation; covered further by US-1.4
- [ ] 5.2 Update `CHANGELOG` / `ReleaseNotes` for R4.4.7 to mention the new method
  and note the shallow-merge / nested-replace behaviour explicitly
