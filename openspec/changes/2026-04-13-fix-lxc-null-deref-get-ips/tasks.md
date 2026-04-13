# Implementation Tasks: LXC get_ips Null Pointer Guard

## Phase 1: Code Change

### Task 1.1: Fix null guard in LXCNetworkInterfaceIterator constructor
- [ ] Apply patch `fix.patch` to `Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp`.
	- Patch path: `openspec/changes/2026-04-13-fix-lxc-null-deref-get-ips/fix.patch`
	- Apply with: `git apply openspec/changes/2026-04-13-fix-lxc-null-deref-get-ips/fix.patch`

## Phase 2: Validation

### Task 2.1: Build validation
- [ ] Build relevant target(s) successfully.
- [ ] Confirm no compile or lint regressions in modified file.

### Task 2.2: Behavioral sanity
- [ ] Verify interface with no IPs is still recorded with `numAddresses = 0`.
- [ ] Verify interface with IPs still records correct `numAddresses`.

## Files Planned

- `Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp`
