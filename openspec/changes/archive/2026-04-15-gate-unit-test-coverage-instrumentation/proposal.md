# Proposal: Gate Unit Test Coverage Instrumentation

## Problem Statement

`Tests/unit/CMakeLists.txt` unconditionally applies coverage instrumentation flags (`--coverage -fprofile-arcs`) to all unit test targets, regardless of the `ENABLE_CODE_COVERAGE` CMake toggle set at root level.

This causes:
- libgcov profiling warnings during test runs when coverage is disabled
- Spurious merge/checksum errors in fork-heavy test environments (esp. IPTestAdministrator spawn tests)
- Inconsistent behavior between `ENABLE_CODE_COVERAGE=OFF` (intended) and actual instrumentation

## Scope

- Update `Tests/unit/CMakeLists.txt` to respect the upstream `ENABLE_CODE_COVERAGE` toggle
- Gate all coverage-specific compiler/linker flags behind a conditional block
- Ensure coverage instrumentation is only applied when explicitly enabled

## Non-goals

- No changes to coverage configuration or instrumentation quality when enabled
- No impact on test behavior or semantics when coverage is disabled
- No changes to root-level CMakeLists.txt or other configuration layers

## Proposed Changes

1. Wrap unconditional `add_compile_options(--coverage -fno-inline ...)` block in `if (ENABLE_CODE_COVERAGE)` guard
2. Wrap unconditional `add_link_options(-fprofile-arcs)` block in the same guard
3. Verify coverage flags are still applied when `ENABLE_CODE_COVERAGE=ON`

## Success Criteria

- [ ] `ENABLE_CODE_COVERAGE=OFF` prevents any libgcov instrumentation in unit tests
- [ ] No libgcov warnings appear during unit test runs with coverage disabled
- [ ] Coverage still works correctly when `ENABLE_CODE_COVERAGE=ON`
- [ ] Existing tests continue to pass without regression
