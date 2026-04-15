# Design: IPTestAdministrator Wait/Timeout Hardening

## Architecture Notes

Consulted:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

This change is test-infrastructure scoped (`Tests/unit/**`) and does not alter runtime plugin/daemon contracts.

## Current Risk

### Timeout unit mismatch risk

`_waitTime` is defined as seconds in `IPTestAdministrator`, but destructor poll timeout path uses a value interpreted as milliseconds, shortening effective wait duration unexpectedly.

### Incomplete spurious wakeup handling

`Wait()` retries after spurious wakeup with a TODO for remaining-time accounting, which can violate intended timeout semantics.

## Desired Behavior

- A single clear interpretation of `_waitTime` across all waiting operations.
- Total wait budget is bounded and deterministic.
- Spurious wakeups or EINTR do not silently extend or invalidate timeout behavior.

## Design Decisions

1. Use explicit `std::chrono` conversions for poll timeout milliseconds.
2. In `Wait()`, track deadline/remaining budget and pass bounded timespec values to futex wait calls.
3. Preserve current error mapping where possible (`ERROR_NONE`, `ERROR_TIMEDOUT`, `ERROR_INVALID_RANGE`).
4. Strengthen tests around timeout and synchronization ordering in `test_iptestmanager`.

## Trade-offs

- Slightly more code complexity for timing bookkeeping.
- Better determinism and less flakiness in process-synchronization tests.
- No behavioral impact on production components.
