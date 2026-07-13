## 1. Read & Understand Existing Code

- [ ] 1.1 Read `ArrayType<ELEMENT>` in `Source/core/JSON.h` — confirm `_data`
  (`std::list<ELEMENT>`), `Length()`, `Add()`, `operator[]`, and `Clear()`
- [ ] 1.2 Confirm `IsSet()` return value when `Length() == 0` — verify it returns
  `false` after all elements are removed (no `_state` manipulation needed)
- [ ] 1.3 Confirm `operator[]` walk pattern (`while(skip--)`) — `Remove` will reuse it

## 2. API Declaration (JSON.h)

- [ ] 2.1 Add `Remove(uint32_t index)` to the public section of `ArrayType<ELEMENT>`
  with full API comment:
  ```cpp
  // Removes the element at zero-based 'index'.
  // ASSERT(index < Length()) in debug builds; undefined behaviour for out-of-range
  // index in release builds (consistent with operator[]).
  // Indices of elements after 'index' shift down by one.
  // Not thread-safe — caller must hold any necessary lock.
  void Remove(const uint32_t index);
  ```

## 3. Implementation (JSON.h)

- [ ] 3.1 Implement `ArrayType<ELEMENT>::Remove(const uint32_t index)`:
  ```cpp
  void Remove(const uint32_t index) {
      uint32_t skip = index;
      ASSERT(index < Length());
      typename std::list<ELEMENT>::iterator locator = _data.begin();
      while (skip != 0) { locator++; skip--; }
      ASSERT(locator != _data.end());
      _data.erase(locator);
  }
  ```
- [ ] 3.2 Confirm the implementation compiles for multiple `ELEMENT` instantiations
  (at minimum `Core::JSON::String` and a numeric type)

## 4. Unit Tests

- [ ] 4.1 **Append and check length** — add three elements, verify `Length() == 3`
  (regression: existing `Add()` behaviour unchanged)
- [ ] 4.2 **Remove middle element** — add `{A, B, C}`, call `Remove(1)`, verify
  array is `{A, C}` and `Length() == 2`
- [ ] 4.3 **Remove first element** — add `{A, B, C}`, call `Remove(0)`, verify
  array is `{B, C}`
- [ ] 4.4 **Remove last element** — add `{A, B, C}`, call `Remove(2)`, verify
  array is `{A, B}`
- [ ] 4.5 **Remove until empty** — remove all elements one-by-one; verify `Length() == 0`
  and `IsSet() == false`
- [ ] 4.6 **Clear after Remove** — remove one element, then call `Clear()`; verify
  `Length() == 0` and no crash
- [ ] 4.7 **Serialisation after Remove** — add `{A, B, C}`, remove B, call `ToString()`,
  verify output matches `[A, C]`
- [ ] 4.8 **Add after Remove** — remove one element then append a new one; verify
  `Length()` and serialisation are correct
- [ ] 4.9 **Existing Add / operator[] / Get regression** — run existing ArrayType unit
  tests and confirm all still pass

## 5. Documentation

- [ ] 5.1 API comment on `Remove` added in task 2.1 is the primary documentation
- [ ] 5.2 Update `CHANGELOG` / `ReleaseNotes` for R4.4.7 to note `ArrayType::Remove`
  addition
