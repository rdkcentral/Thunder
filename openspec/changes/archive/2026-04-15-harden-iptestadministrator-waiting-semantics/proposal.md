# Proposal: Harden IPTestAdministrator Waiting Semantics

## Problem Statement

`Tests/unit/IPTestAdministrator.cpp` contains timing and synchronization behavior that can make tests flaky or semantically inconsistent:
- `_waitTime` is documented as seconds, but one code path uses it directly where milliseconds are expected.
- `Wait()` has a TODO for remaining-time handling after spurious wakeups.
- Current behavior can produce non-deterministic wait windows under scheduler jitter.

## Scope

- Update `Tests/unit/IPTestAdministrator.cpp` and, if needed, `Tests/unit/IPTestAdministrator.h`.
- Make timeout units explicit and consistent across poll/futex paths.
- Replace TODO behavior in `Wait()` with deterministic bounded wait semantics.
- Add/adjust tests in `Tests/unit/tests/test_iptestmanager.cpp` for the hardened behavior.

## Non-goals

- No production-runtime IPC behavior changes outside unit-test helper code.
- No redesign of the parent/child sync protocol itself.
- No cross-platform support expansion (Linux-only constraint remains).

## Proposed Changes

1. Convert destructor `poll()` timeout to use explicit seconds-to-milliseconds conversion.
2. Rework `Wait()` to account for elapsed time and enforce the intended total timeout bound under spurious wakeups/interruption.
3. Keep return-code contract stable (`ERROR_NONE`, `ERROR_TIMEDOUT`, etc.).
4. Add/adjust `test_iptestmanager` cases to validate timeout behavior and wakeup robustness.

## Success Criteria

- [ ] Timeout units are consistent and explicit in all waiting paths.
- [ ] `Wait()` no longer contains TODO and behaves deterministically under spurious wakeups.
- [ ] Existing tests remain green and targeted new/updated tests cover the hardened paths.
- [ ] No functional regression in basic parent/child handshake tests.
