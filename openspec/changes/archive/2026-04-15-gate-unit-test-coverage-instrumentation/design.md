# Design: Gate Unit Test Coverage Instrumentation

## Root Cause Analysis

The root CMakeLists.txt defines `ENABLE_CODE_COVERAGE` as a toggleable option (default OFF), but `Tests/unit/CMakeLists.txt` applies coverage flags unconditionally via:

```cmake
add_compile_options(--coverage -fno-inline -fno-inline-small-functions -fno-default-inline)
add_link_options(-fprofile-arcs)
```

These commands run regardless of the toggle value. Result: unit tests are always instrumented, even when coverage collection is disabled globally.

## Solution Strategy

Wrap the coverage instrumentation in a conditional block that reads the `ENABLE_CODE_COVERAGE` variable:

```cmake
if (ENABLE_CODE_COVERAGE)
    add_compile_options(--coverage -fno-inline -fno-inline-small-functions -fno-default-inline)
    add_link_options(-fprofile-arcs)
endif()
```

This ensures:
- When `ENABLE_CODE_COVERAGE` is OFF: no coverage flags, no libgcov instrumentation, no spurious profiling I/O
- When `ENABLE_CODE_COVERAGE` is ON: coverage flags applied as before, full instrumentation

## Changes Required

**File:** `Tests/unit/CMakeLists.txt`

- Locate the unconditional coverage instrumentation block
- Wrap entire block in `if (ENABLE_CODE_COVERAGE)` ... `endif()` guard
- No other changes needed; variable is already available from parent scope

## Testing

1. Build with `ENABLE_CODE_COVERAGE=OFF` (default):
   - Verify no `--coverage` flags in compile commands
   - Run tests and confirm no libgcov warnings
   
2. Build with `ENABLE_CODE_COVERAGE=ON`:
   - Verify `--coverage` flags present in compile commands
   - Run tests and confirm coverage data is generated
