## 1. Read & Understand Existing Code

- [ ] 1.1 Read `Container::Add`, `Container::Remove`, and `Container::IsSet` in
  `Source/core/JSON.h` — confirm `_data` layout and ownership rules
- [ ] 1.2 Confirm that `IElement` exposes `ToString(string&)` and `FromString(const string&)` —
  required for the Set round-trip implementation
- [ ] 1.3 Review existing Thunder plugin code (e.g. `rdkservices/`) for any existing
  patterns of "update a field by clearing + re-adding" to validate this US
  addresses a real pain point

## 2. API Declaration (JSON.h)

- [ ] 2.1 Add `Set(const TCHAR label[], const IElement& value)` to the public section
  of `Container` in `Source/core/JSON.h` with full API comment:
  ```cpp
  // Updates the value of an existing field identified by 'label'.
  // If 'label' is not found, the call is a no-op (TRACE_L1 warning emitted in debug builds).
  // Not thread-safe — caller must hold any necessary lock.
  void Set(const TCHAR label[], const IElement& value);
  ```
- [ ] 2.2 Add API comment blocks to the existing `Add(const TCHAR label[], IElement* element)`
  documenting: ownership (caller retains element pointer lifetime), thread-safety (caller-holds-lock)
- [ ] 2.3 Add API comment block to the existing `Remove(const TCHAR label[])` documenting:
  precondition (must not remove typed-member fields), thread-safety (caller-holds-lock)

## 3. Implementation (JSON.h)

- [ ] 3.1 Implement `Container::Set`:
  - Iterate `_data` to find entry with matching label
  - If found: serialise `value` to a temporary string with `value.ToString(buf)`;
    call `entry->second->FromString(buf)` to update in-place
  - If not found: emit `TRACE_L1` warning and return
- [ ] 3.2 Confirm the implementation compiles cleanly with `-Wall -Wextra` against the
  C++ standard in use by Thunder

## 4. Unit Tests

- [ ] 4.1 **Add then serialise** — add two fields, call `ToString()`, verify both appear
  in JSON output (regression: existing `Add` behaviour unchanged)
- [ ] 4.2 **Set existing field** — add a field with value A, call `Set` with value B,
  call `ToString()`, verify field holds value B
- [ ] 4.3 **Set typed field round-trip** — field is a typed member of an `ObjectType`
  subclass; `Set` round-trips through `ToString`/`FromString`; verify no data
  loss for string, integer, and boolean field types
- [ ] 4.4 **Set unknown label is no-op** — call `Set` on a label that was never added;
  verify container is unchanged and no crash
- [ ] 4.5 **Remove existing field** — add two fields, remove one, call `ToString()`,
  verify only the remaining field is present
- [ ] 4.6 **Remove unknown label is no-op** — regression test for pre-existing `Remove`
  behaviour
- [ ] 4.7 **Serialisation after each mutation** — verify `IsSet()` correctly reflects
  container state after `Add`, `Set`, and `Remove` sequences

## 5. Documentation

- [ ] 5.1 API comments on `Add`, `Set`, and `Remove` are the primary documentation
  (added in task 2.1–2.3); no separate doc file needed for R4.4.7
- [ ] 5.2 Update `CHANGELOG` / `ReleaseNotes` for R4.4.7 to note `Container::Set`
  addition and the documented thread-safety contract
