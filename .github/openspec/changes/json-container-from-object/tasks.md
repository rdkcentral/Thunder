## 1. Read & Understand Existing Code

- [ ] 1.1 Read `IElement` in `Source/core/JSON.h` — confirm `ToString(string&)`,
  `static FromString(text, object)`, and instance `FromString(text)` signatures
- [ ] 1.2 Read `Container::Deserialize` — confirm it calls `Find(label)` per key,
  silently skips unknown keys, and does NOT clear registered slots absent from
  the incoming JSON
- [ ] 1.3 Read `VariantContainer::Request()` override — confirm it allocates a new
  `Variant` slot, making `Find` succeed for any key
- [ ] 1.4 **Null model finding for US-1.5:** confirm `Container::Null(true)` sets
  only `UNDEFINED` while scalar types set `SET|UNDEFINED`; record as finding

## 2. API Declaration (JSON.h)

- [ ] 2.1 Add `FromObject` to `Container` with full API comment:
  ```cpp
  // Populates this container from the JSON representation of 'source'.
  // Equivalent to: this->FromString(source.ToString()).
  //
  // On typed Container: updates only pre-registered field slots; unknown keys
  // in source are silently skipped; registered slots absent from source are
  // preserved (not cleared). Never adds new fields.
  //
  // On VariantContainer: additionally inserts new Variant slots for keys not
  // yet present (via Request() override).
  //
  // Deep recursive: nested Container fields updated key-by-key, not replaced.
  //
  // Returns false if FromString fails (e.g. malformed JSON).
  // Not thread-safe — caller must hold any necessary lock.
  bool FromObject(const IElement& source);
  ```

## 3. Implementation (JSON.h)

- [ ] 3.1 Implement `Container::FromObject` — two lines only:
  ```cpp
  bool Container::FromObject(const IElement& source) {
      string json;
      source.ToString(json);
      return FromString(json);
  }
  ```
- [ ] 3.2 Verify no field-by-field iteration, no `_data` access in `FromObject` —
  all behaviour is inside `FromString`/`Deserialize`

## 4. Unit Tests

- [ ] 4.1 **VariantContainer: accumulate new fields** — two VariantContainer sources
  with distinct keys; verify all keys appear in VariantContainer target after two
  `FromObject` calls
- [ ] 4.2 **Typed Container: only registered slots updated** — source has registered
  + unregistered keys; verify only registered slots updated; unregistered absent
- [ ] 4.3 **Typed Container: absent source keys preserve target values** — source
  carries only one of two registered fields; verify the other is unchanged
- [ ] 4.4 **Deep recursive update of nested typed Container** — target has nested
  Container with `{model, region}`; source has nested object with `{model}` only;
  verify `region` is **preserved** after `FromObject`
- [ ] 4.5 **Cross-type: Container::FromObject(VariantContainer)** — typed Container
  updates registered slots from a VariantContainer source
- [ ] 4.6 **Cross-type: VariantContainer::FromObject(typed Container)** — VariantContainer
  receives all set fields from a typed Container source
- [ ] 4.7 **FromObject vs operator= on VariantContainer** — `operator=` replaces all
  content; `FromObject` preserves entries absent from source — verify distinction
- [ ] 4.8 **Chaining on VariantContainer** — three sources with distinct keys;
  verify all accumulated in target
- [ ] 4.9 **Returns bool** — `true` on success; `false` when `FromString` fails
- [ ] 4.10 **Empty source** — default-constructed source; target unchanged; returns `true`
- [ ] 4.11 **Existing `FromString()` and `Add()` regression** — run existing Container
  unit tests; confirm all still pass

## 5. Documentation

- [ ] 5.1 API comment on `FromObject` (task 2.1) is the primary documentation
- [ ] 5.2 Update `CHANGELOG` / `ReleaseNotes` for R4.4.7 noting `FromObject`,
  Container-vs-VariantContainer distinction, and the null model finding for US-1.5
