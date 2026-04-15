# Implementation Tasks: Gate Unit Test Coverage Instrumentation

## Single Phase: Configuration gating

### Task 1.1: Gate coverage flags in Tests/unit/CMakeLists.txt
- [x] Wrap `add_compile_options(--coverage ...)` in `if (ENABLE_CODE_COVERAGE)` guard
- [x] Wrap `add_link_options(-fprofile-arcs)` in the same guard
- [x] Verify variable is accessible from parent scope

## Phase 2: Validation

### Task 2.1: Build and test with ENABLE_CODE_COVERAGE=OFF
- [x] Clean rebuild with coverage disabled
- [x] Run unit tests and confirm no libgcov warnings
- [x] Verify test results unchanged

### Task 2.2: Build and test with ENABLE_CODE_COVERAGE=ON
- [x] Rebuild with coverage enabled
- [x] Verify `--coverage` flags present in compile commands
- [x] Confirm coverage instrumentation works as expected

## Files Modified

- `Tests/unit/CMakeLists.txt`
