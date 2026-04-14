# Implementation Tasks: PrivilegedRequest mkstemp Descriptor Leak Fix

## Phase 1: Code Change

### Task 1.1: Close mkstemp descriptor in UniqueDomainName
- [x] Apply `fix.patch` to `Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h`.
  - Patch stored at `openspec/changes/2026-04-13-fix-privilegedrequest-mkstemp-fd-leak/fix.patch`.
  - Apply with: `git apply openspec/changes/2026-04-13-fix-privilegedrequest-mkstemp-fd-leak/fix.patch`
  - Final state: landed via branch resync (equivalent fd-close behavior present in source).

## Phase 2: Validation

### Task 2.1: Build validation
- [x] Build relevant target(s) successfully.
- [x] Confirm no compile or lint regressions in modified file.

### Task 2.2: Behavioral sanity
- [x] Confirm request/offer path still opens and cleans domain socket paths.
- [x] Confirm no API behavior changes were introduced.

> Note: No unit tests exist for PrivilegedRequest. Validation requires integration/manual test of request/offer flow via UNIX domain socket roundtrip.

## Files Planned

- `Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h`
