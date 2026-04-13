# Implementation Tasks: NetworkInfo Unsupported Semantics Normalization

## Phase 1: Code Changes

### Task 1.1: Normalize unimplemented Windows branches
- [ ] Update `Source/core/NetworkInfo.cpp` (`__WINDOWS__` section).
- [ ] Change unimplemented `Up` to return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Change unimplemented `Gateway` to return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Change unimplemented `Broadcast` to return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Remove `ASSERT(false)` from these unsupported paths.

### Task 1.2: Normalize unimplemented Apple branches
- [ ] Update `Source/core/NetworkInfo.cpp` (`__APPLE__` section).
- [ ] Change unimplemented `Up` to return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Change unimplemented no-op `Add`/`Delete` to return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Change unimplemented `Gateway`/`Broadcast` to return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Remove `ASSERT(false)` from these unsupported paths.

## Phase 2: Validation

### Task 2.1: Build validation
- [ ] Build relevant target(s) successfully.
- [ ] Confirm no compile or lint regressions in modified file.

### Task 2.2: Semantics sanity
- [ ] Confirm unsupported branch methods consistently return `Core::ERROR_NOT_SUPPORTED`.
- [ ] Confirm implemented paths (for example Windows Add/Delete) are untouched.
- [ ] Reviewer checkpoint: verify unsupported operations are no longer reported as success (`Core::ERROR_NONE`) or caller fault (`Core::ERROR_BAD_REQUEST`).

## Delivery mode

- [x] Prepare and store review patch before applying source changes if patch-first workflow is requested.
	- Patch path: `openspec/changes/2026-04-13-networkinfo-not-supported-semantics/fix.patch`
	- Apply with: `git apply openspec/changes/2026-04-13-networkinfo-not-supported-semantics/fix.patch`

## Files Planned

- `Source/core/NetworkInfo.cpp`
