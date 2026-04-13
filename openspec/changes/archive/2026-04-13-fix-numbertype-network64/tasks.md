# Implementation Tasks: NumberType 64-bit Network Conversion

## Status

Archived on 2026-04-13. All planned tasks completed.

## Phase 1: Implementation

### Task 1.1: Fix 64-bit conversion logic in NumberType
- [x] Updated `Source/core/Number.h` for `uint64_t` `ToNetwork/FromNetwork` using 32-bit split+recombine conversion.
- [x] Updated signed 64-bit wrappers to delegate through unsigned conversion.

## Phase 2: Tests

### Task 2.1: Update NumberType unit assertions
- [x] Updated `Tests/unit/core/test_numbertype.cpp` 64-bit expectations in `Core_NumberType.generic`.
- [x] Added signed/unsigned round-trip assertions.

### Task 2.2: Add targeted filtered test registration
- [x] Updated `Tests/unit/core/CMakeLists.txt` with `Thunder_test_core_numbertype` using `--gtest_filter=Core_NumberType*`.

## Phase 3: Validation

### Task 3.1: Run targeted test
- [x] Executed `Thunder_test_core_numbertype` via CMake Tools.
- [x] Result: passed.

## Files Changed

- `Source/core/Number.h`
- `Tests/unit/core/test_numbertype.cpp`
- `Tests/unit/core/CMakeLists.txt`

## Notes

- Full `Thunder_test_core` runs in this environment can abort due unrelated gcov profile merge/checksum issues; targeted NumberType run provides deterministic verification for this change.
