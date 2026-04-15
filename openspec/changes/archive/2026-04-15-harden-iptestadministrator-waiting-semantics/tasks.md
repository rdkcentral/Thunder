# Implementation Tasks: Harden IPTestAdministrator Waiting Semantics

## Phase 1: Timing semantics hardening

### Task 1.1: Normalize timeout units
- [x] Ensure `_waitTime` interpretation is consistent (seconds) across destructor poll and futex wait paths.
- [x] Replace implicit timeout arithmetic with explicit `std::chrono` conversions.

### Task 1.2: Remove TODO in Wait()
- [x] Implement deterministic remaining-time handling for spurious wakeups/interrupted waits.
- [x] Keep timeout bounded to configured wait budget.

## Phase 2: Test coverage

### Task 2.1: Update/add sync timing tests
- [x] Extend `Tests/unit/tests/test_iptestmanager.cpp` with targeted checks for timeout-bound behavior.
- [x] Validate no regression in existing handshake scenarios.

## Phase 3: Validation

### Task 3.1: Build and run
- [x] Build affected unit test target(s).
- [x] Run `test_iptestmanager` and confirm stable behavior.

## Files Planned

- `Tests/unit/IPTestAdministrator.h`
- `Tests/unit/IPTestAdministrator.cpp`
- `Tests/unit/tests/test_iptestmanager.cpp`
