# Implementation Tasks: Harden test_networkinfo

## Phase 1: Test Refactor

### Task 1.1: Remove hardcoded adapter assumptions
- [ ] Update `Tests/unit/core/test_networkinfo.cpp` enabled tests to avoid `AdapterIterator("eth0")` dependency.
- [ ] Select adapters dynamically from `AdapterIterator` enumeration.
- [ ] Add `GTEST_SKIP()` for missing environment prerequisites.

### Task 1.2: Replace weak assertions
- [ ] Remove self-comparison assertions (`EXPECT_EQ(x, x)`, `EXPECT_STREQ(s, s)`) in enabled tests.
- [ ] Add meaningful assertions for iteration/address invariants.

### Task 1.3: Avoid host-network mutation in unit tests
- [ ] Remove or isolate `Add/Delete/Gateway/Broadcast` calls from enabled unit tests.

## Phase 2: Validation

### Task 2.1: Filtered test validation
- [ ] Run filtered NetworkInfo tests (`test_ipv4addressiterator.*:test_ipv6addressiterator.*`).
- [ ] Confirm stable pass/skip behavior in current environment.

## Delivery mode

- [x] Prepare and store review patch before applying source changes (patch-first workflow).
	- Patch path: `openspec/changes/2026-04-13-harden-test-networkinfo/fix.patch`
	- Apply with: `git apply openspec/changes/2026-04-13-harden-test-networkinfo/fix.patch`

## Files Planned

- `Tests/unit/core/test_networkinfo.cpp`
