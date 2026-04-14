# Implementation Tasks: Harden LXC C-String Bounds Handling

## Phase 1: Code Changes

### Task 1.1: Bound key formatting writes
- [ ] Update `Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp`.
- [ ] Replace three `sprintf()` calls writing into `buf[256]` with `snprintf(buf, sizeof(buf), ...)`.

### Task 1.2: Bound parser token width
- [ ] Replace `%s` with `%127s` in the `sscanf()` format string parsing into `name[128]`.
- [ ] Keep existing parse success condition and loop control unchanged.

## Phase 2: Validation

### Task 2.1: Build validation
- [ ] Build relevant target(s) successfully.
- [ ] Confirm no compile or lint regressions in modified file.

### Task 2.2: Semantics sanity
- [ ] Confirm no functional behavior changes beyond bounds hardening.
- [ ] Reviewer checkpoint: ensure every fixed-size destination has matching bounded formatting/parsing.

## Delivery mode

- [x] Prepare and store review patch before applying source changes (patch-first workflow).
	- Patch path: `openspec/changes/2026-04-13-harden-lxc-cstring-bounds/fix.patch`
	- Apply with: `git apply openspec/changes/2026-04-13-harden-lxc-cstring-bounds/fix.patch`
- [x] Keep as patch-only for colleague review (not applied in source at this stage).

## Files Planned

- `Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp`
