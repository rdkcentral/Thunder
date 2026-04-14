# Implementation Tasks: Harden PrivilegedRequest UNIX Socket Pathname Handling

## Phase 1: Decision and Constraints

### Task 1.1: Confirm target hardening mode
- [ ] Confirm baseline approach (private `0700` runtime directory).
- [ ] Document portability and compatibility constraints.

## Phase 2: Implementation

### Task 2.1: Introduce hardened socket path root
- [ ] Add runtime directory creation/validation with secure permissions.
- [ ] Ensure generated socket names are confined to hardened directory.

### Task 2.2: Integrate with existing lifecycle
- [ ] Keep request/offer/listen semantics unchanged.
- [ ] Ensure cleanup logic handles socket file and runtime path lifecycle safely.

### Task 2.3: Incremental retry hardening follow-up (post-resync)
- [x] Document landed client-side retry behavior and open gaps.
- [x] Prepare review patch for remaining retry gaps:
	- Add attempt-indexed retry diagnostics for unique bind collisions.
	- Add bounded retry behavior for server-side fixed connector bind (`OpenDomainSocket`).
	- Keep existing protocol and lifecycle semantics unchanged.
	- Review patch: `openspec/changes/2026-04-13-harden-privilegedrequest-unix-socket-path-race/fix.patch`

## Phase 3: Validation

### Task 3.1: Functional validation
- [ ] Validate request/offer flows under hardened path handling.
- [ ] Validate startup/shutdown cleanup paths.

### Task 3.2: Robustness checks
- [ ] Validate behavior with stale paths and restart scenarios.
- [ ] Validate permission boundary expectations.

## Files Expected (to be finalized during apply)

- `Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h`
- Additional source/config files as required by selected approach
