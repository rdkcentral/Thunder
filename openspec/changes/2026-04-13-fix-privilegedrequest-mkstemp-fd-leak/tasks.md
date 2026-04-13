# Implementation Tasks: PrivilegedRequest mkstemp Descriptor Leak Fix

## Phase 1: Code Change

### Task 1.1: Close mkstemp descriptor in UniqueDomainName
- [ ] Apply `fix.patch` to `Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h`.
  - Patch stored at `openspec/changes/2026-04-13-fix-privilegedrequest-mkstemp-fd-leak/fix.patch`.
  - Apply with: `git apply openspec/changes/2026-04-13-fix-privilegedrequest-mkstemp-fd-leak/fix.patch`

## Phase 2: Validation

### Task 2.1: Build validation
- [ ] Build relevant target(s) successfully.
- [ ] Confirm no compile or lint regressions in modified file.

### Task 2.2: Behavioral sanity
- [ ] Confirm request/offer path still opens and cleans domain socket paths.
- [ ] Confirm no API behavior changes were introduced.

> Note: No unit tests exist for PrivilegedRequest. Validation requires integration/manual test of request/offer flow via UNIX domain socket roundtrip.

## Files Planned

- `Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h`
