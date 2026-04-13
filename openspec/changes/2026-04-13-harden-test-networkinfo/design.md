# Design: test_networkinfo Hardening

## Architecture Notes

Consulted:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

This change is test-only (`Tests/unit/core/test_networkinfo.cpp`) and does not alter production interfaces or runtime behavior.

## Current Issues

1. **Environment lock-in**
- Test expects adapter named `eth0`, which is not portable.

2. **Low-signal assertions**
- Self-comparisons (`x == x`) pass regardless of correctness.

3. **Unsafe unit-test behavior**
- Invoking `Add/Delete/Gateway/Broadcast` mutates host networking and can fail for permission/environment reasons unrelated to core logic.

## Proposed Approach

- Introduce adapter/address discovery based on available runtime data.
- Replace hard assertions on fixed adapter names with:
  - dynamic selection, and
  - `GTEST_SKIP()` when prerequisites are absent.
- Restrict unit tests to read-only validation (enumeration and object semantics), not host network mutation.

## Expected Test Semantics

- If no adapters or no suitable addresses exist in environment, tests skip with explicit reason.
- If adapters/addresses exist, assertions validate meaningful invariants (non-empty host address, valid iteration behavior, copy/reset path sanity).

## Risk Assessment

- Low risk: tests only.
- High benefit: reduces false negatives and improves CI portability.
