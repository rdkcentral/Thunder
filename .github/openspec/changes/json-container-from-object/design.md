## Context

`JSON::Container` (in `Source/core/JSON.h`) stores its fields as a
`std::list<std::pair<const TCHAR*, IElement*>>` aliased as `JSONElementList`.
Each entry is a raw pointer pair — the label is a `const TCHAR*` (string literal
owned by the declaring typed subclass) and the element is an `IElement*` (owned
by the same class).

The existing `Add(const TCHAR label[], IElement* element)` pushes to the back of
`_data`. `FromString()` deserialises a JSON text string by calling the `IElement`
parser machinery; it populates pre-registered field slots in-place through label
matching via the `Find(label)` protected method.

`Container` already exposes a public `Iterator` class (line 3708 of `JSON.h`)
that walks `_data` and exposes `Label()` / `Element()` per entry. `FromObject`
will use this iterator to visit source fields without relying on private member
access from a non-member context — though since `FromObject` is a member of
`Container` it has direct access to `_data` as well.

`FromObject()` cannot simply copy raw `IElement*` pointers from the source into
the target — the source elements are owned by their declaring class and their
lifetime is tied to it. The only stable, type-agnostic way to transfer a value
between two `IElement` slots is the `ToString(buf)` / `FromString(buf)` round-trip
already used throughout Thunder for config cloning.

### Nested container round-trip correctness

When a source field is a nested `Container`, its `Serialize()` implementation
recurses into its own `_data` and emits the full JSON object subtree (confirmed
in `Container::Serialize(char[],...)`, lines ~3884–3945). `FromString` on the
target's matching field (also a `Container`) will fully deserialise the nested
JSON object — the round-trip is **lossless for any depth** of nesting. The
behaviour is therefore **shallow merge at the top level, full-replace of nested
subtrees** — not a deep merge.

### Unset AND null fields — both skipped; null deferred to US-1.5

`Container::FindNext()` (called by `Serialize`) already skips fields where
`IsSet() == false`. `FromObject` follows the same convention: it skips any
source field where `IsSet() == false`.

For null-valued fields (`IsNull() == true`): these are **also skipped** in
R4.4.7. The reason is an observed inconsistency in the null model:

| Type | `Null(true)` sets | `IsSet()` when null |
|---|---|---|
| `NumberType`, `FloatType`, `Boolean`, `String`, `Buffer`, `EnumType`, `ArrayType` | `SET \| UNDEFINED` | `true` |
| `Container` | `UNDEFINED` only | `false` (unless sub-fields are set) |

Because `Container::Null(true)` does NOT set `SET`, a null `Container` field
cannot be reliably detected as a "set null value" vs. "unset field" in a
uniform way across field types. Propagating null fields in `FromObject()` without
a framework-wide audit risks inconsistent behaviour that is hard to reason about.

**This is a finding to be raised with the Thunder architecture team.** Null-aware
merging in `FromObject()` is explicitly deferred to US-1.5 (Impact Analysis) and
US-1.6 (Implementation). The R4.4.7 behaviour is documented: `FromObject` skips
both unset (`IsSet()==false`) and null (`IsNull()==true`) source fields.

### Typed target — `Find()` and `Request()` mechanics

For a typed (`ObjectType`-style) target, `Find(label)` searches `_data` for a
pre-registered slot with a matching label. If none is found and `Request(label)`
returns `false` (default), `Find` returns `nullptr`. `FromObject` treats a
`nullptr` result as "field not registered in target" and skips it — consistent
with `FromString()` silent-skip of unknown keys.

Type-mismatch failures (e.g. target slot is `JSON::String`, source serialises
as `{...}`) surface as `IElement::FromString` returning `false`. `FromObject`
shall emit a `TRACE_L1` warning and leave the target slot unchanged.

### Generic / `VariantContainer` target

`VariantContainer` overrides `Request(const TCHAR label[])` to dynamically
allocate a `Variant` slot. When `Find(label)` returns `nullptr` on a
`VariantContainer`, calling `Request(label)` creates the slot, so a second `Find`
returns the new entry. `FromObject` must call `Find` — which internally calls
`Request` — rather than scanning `_data` directly, so that `VariantContainer`
semantics are respected.

### `IsComplete` state

`Container` has an `IsComplete()` flag (`(_state & COMPLETE) != 0`) that
`Deserialize` sets when a closing `}` is parsed. `FromObject` sets individual
field values but does not parse a complete JSON document; it **shall not set
`COMPLETE`** on the target — the flag semantics belong to the deserialiser
state machine, not to programmatic mutation.

## Goals / Non-Goals

**Goals:**
- Add `FromObject(const JSON::Container& source)` to `JSON::Container`.
- Shallow merge: last-writer-wins for top-level duplicate keys.
- Nested containers and arrays transferred whole (not deep-merged).
- Skip both unset (`IsSet()==false`) AND null (`IsNull()==true`) source fields.
- Field-registration mismatch silently skipped (unknown key) or traced (type mismatch).
- `VariantContainer` target: all non-null, set source keys accepted via `Request()` override.
- `IsComplete` NOT set by `FromObject`.
- Zero impact on existing `FromString()` and `Add()` paths.
- Full unit-test coverage.

**Non-Goals:**
- Null-aware merging (propagating null source fields) — deferred to US-1.5 / US-1.6.
- Deep/recursive merge of nested objects — follow-on US.
- Thread-safety inside `FromObject()` — caller-holds-lock, same as `Add`/`Remove`.
- `FromObject` on `ArrayType` — separate type hierarchy.
- Error return value — void method with TRACE_L1 on field-level failures.

## Decisions

### Decision 1 — Serialise-then-deserialise per field (IElement round-trip)

`IElement` does not expose a deep-copy virtual method. Rather than adding one
(ABI change to a widely-implemented interface), `FromObject` iterates source
fields and for each set `IElement*` calls `element->ToString(buf)`, then calls
`Find(label)` on `*this` (which respects `Request()` overrides) and calls
`slot->FromString(buf)` on the found slot.

Nested containers serialise their full subtree in `buf` — the round-trip is
lossless at any nesting depth because `Container::Serialize` recurses. This gives
shallow-top-level, whole-replace-of-nested semantics without any special-casing.

**Trade-off:** Minor per-field string buffer allocation. Acceptable for a merge
operation not on the hot JSON-RPC read path.

**Alternative rejected:** Virtual `Clone()` on `IElement` — requires every
concrete type to implement it, a far larger change not justified for R4.4.7.

### Decision 2 — Nested containers are whole-replaced, not deep-merged

Because the round-trip treats a nested `Container`'s serialised form as a single
string token and deserialises it into the target slot whole, the result is always
a complete replace of the nested subtree. This is the correct minimal behaviour:
deep merge requires knowing the schema of inner fields, which `FromObject` does
not have. Documented as a known limitation and explicit non-goal.

### Decision 3 — Skip both unset AND null source fields; null deferred to US-1.5

Mirrors `Container::FindNext()` for unset fields. Null fields are additionally
skipped because the null model is inconsistent across the type hierarchy
(see Context section above): scalar types set `SET|UNDEFINED` on `Null(true)`,
but `Container::Null(true)` sets only `UNDEFINED`. A uniform `IsNull()` check
cannot be used to reliably detect "set null" across all field types without the
broader audit that US-1.5 prescribes.

Documented as a **finding**: the Thunder architecture team should review the
`Container::Null()` inconsistency as part of the US-1.5 impact analysis.

### Decision 4 — Use `Find()` not `_data` scan for target slot lookup

`Find()` is protected and calls `Request()` as a fallback, which is the extension
point overridden by `VariantContainer`. Scanning `_data` directly would bypass
this and break `VariantContainer` targets. `FromObject` must call `Find(label)`
to respect the full class hierarchy.

### Decision 5 — Type-mismatch is TRACE_L1, not error return

Consistent with how `Container::Deserialize` handles mismatches (it notes the
error in the `error` out-parameter but continues parsing, and `Find` returns
`nullptr` for unrecognised keys without throwing). `FromObject` is a void method
(matching `Add`/`Remove` style); a `TRACE_L1` at the failure site surfaces the
problem in debug builds without forcing callers to handle an error code.

### Decision 6 — `IsComplete` not set by `FromObject`

`COMPLETE` is a deserialiser state-machine flag, not a "container has data" flag.
Setting it from outside the parser would confuse `Deserialize` if the same
container is later re-used as an input buffer. `IsSet()` is the correct predicate
for "container has values" and is unaffected.

### Decision 7 — Last-writer-wins for duplicate keys

Matches `FromString()` semantics (which also overwrites when the same key appears
more than once in a JSON document). Documented in API comment.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| Nested container replace silently drops inner fields of target | Documented behaviour (non-goal: deep merge); explicit in proposal, API comment, and test scenario. |
| Per-field string allocation on merge path | Only on construction / response-building path, not JSON-RPC read path. Acceptable. |
| Type-mismatch between source and target slot: `FromString` fails | TRACE_L1 emitted; target slot left unchanged; consistent with `FromString` unknown-key handling. |
| Unset AND null fields skipped — caller expecting null to be copied will be surprised | Documented non-goal in R4.4.7; deferred to US-1.5/1.6; clearly stated in API comment. |
| `Container::Null()` inconsistency (finding) | Raised to architecture team via US-1.5 impact analysis scope; no code change in this US. |
| `VariantContainer` target: keys added dynamically may outlive the source's lifetime | Value is deserialised into a new owned `Variant` slot; source element lifetime is irrelevant after the round-trip. |
| `IsComplete` not set after `FromObject` | Correct: `IsSet()` is the data predicate; `IsComplete` is a parser state flag only. Documented in design. |
| No recursive merge | Documented as non-goal; follow-on US. |
