# Design: NetworkInfo Unsupported Semantics Normalization

## Architecture Notes

Consulted architecture and SME constraints before proposing:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

Change remains local to `Source/core/NetworkInfo.cpp` and does not alter interfaces, ABI, or Linux runtime behavior.

## Current Inconsistency

In `AdapterIterator` non-Linux implementations:
- Some unimplemented methods indicate success (`ERROR_NONE`) despite doing nothing.
- Some unimplemented methods assert and map to `ERROR_BAD_REQUEST`.
- Result: callers cannot reliably distinguish unsupported from successful operation.

## Desired Behavior

Unsupported operations should be explicit and uniform:
- Return `Core::ERROR_NOT_SUPPORTED` for unimplemented mutating actions.
- Avoid `ASSERT(false)` in expected unsupported branches.

## Targeted Branches

- `#if defined(__WINDOWS__)`
  - `Up`, `Gateway`, `Broadcast` (TODO paths)
- `#elif defined(__APPLE__)`
  - `Up`, `Add`, `Delete`, `Gateway`, `Broadcast` (TODO/no-op paths)

## Design Decisions

1. Keep success semantics unchanged where implementation actually exists (for example, Windows `Add`/`Delete` current implementation path).
2. Normalize only clearly unimplemented paths to avoid accidental regressions.
3. Preserve method signatures and surrounding control flow.

## Decision Rationale

The chosen behavior aligns return codes with the real runtime condition:
- If an operation is recognized but not implemented on a platform, return `Core::ERROR_NOT_SUPPORTED`.

This avoids two misleading outcomes present today:
- False success (`Core::ERROR_NONE`) from no-op stubs.
- Caller-blame semantics (`Core::ERROR_BAD_REQUEST`) for a platform-capability gap.

Removing `ASSERT(false)` from these expected unsupported branches also prevents debug-only process aborts and keeps failure handling in normal control flow.

## Trade-offs

- Compatibility trade-off: callers that previously treated no-op success as valid behavior will now receive explicit unsupported status.
- Mitigation: this is a correctness improvement and enables deterministic fallback logic.
- Scope trade-off: this change does not implement missing platform functionality; it only makes current behavior truthful and consistent.

## Risks

- Low: semantic tightening in currently unimplemented branches.
- Medium compatibility risk for callers that incorrectly relied on false success (`ERROR_NONE`) from no-op paths.
- Risk is acceptable because explicit unsupported signaling is more correct and stable.
