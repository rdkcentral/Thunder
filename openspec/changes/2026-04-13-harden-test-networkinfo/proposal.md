# Proposal: Harden test_networkinfo for Environment-Independent Validation

## Problem Statement

`Tests/unit/core/test_networkinfo.cpp` currently contains fragile and weak checks:
- Hardcoded adapter assumption (`"eth0"`) causing failures on systems where this interface does not exist.
- Self-comparison assertions (for example `EXPECT_EQ(x, x)` / `EXPECT_STREQ(s, s)`) that do not validate behavior.
- Calls to mutating network methods (`Add/Delete/Gateway/Broadcast`) in a unit test context, which can be environment-dependent and potentially disruptive.

This causes flaky failures and low signal from test outcomes.

## Scope

- Harden enabled tests in `test_networkinfo.cpp` to be robust across host environments.
- Replace hardcoded adapter selection with dynamic discovery.
- Convert weak assertions into meaningful invariants.
- Avoid host-network mutating actions in unit tests.

## Non-goals

- No behavioral changes to production NetworkInfo logic.
- No platform-specific integration test harness creation in this change.

## Proposed Changes

1. In `test_ipv4addressiterator`:
- Remove `AdapterIterator("eth0")` dependency.
- Iterate available adapters dynamically.
- If no valid adapter/address context is available, use `GTEST_SKIP()` with clear reason.
- Remove mutating calls (`Add/Gateway/Delete/Broadcast`) from this unit test.

2. In `test_ipv6addressiterator`:
- Replace self-comparison assertions with meaningful checks (for example non-empty host address for enumerated entries).
- Skip gracefully when environment lacks suitable IPv6 addresses.

## Success Criteria

- [ ] No hard dependency on interface name `eth0` in enabled tests.
- [ ] No self-comparison assertions remain in enabled tests.
- [ ] Enabled tests avoid mutating host network configuration.
- [ ] Filtered NetworkInfo test run is stable across environments with reasonable skip behavior.
