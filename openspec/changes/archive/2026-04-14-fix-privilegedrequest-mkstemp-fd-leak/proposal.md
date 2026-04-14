# Proposal: Fix mkstemp File Descriptor Leak in PrivilegedRequest

## Problem Statement

`PrivilegedRequest::Connection::UniqueDomainName` calls `::mkstemp` to produce a unique pathname but discards the returned file descriptor. This leaks one descriptor per call path and can accumulate over time.

## Scope

- Update `Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h`.
- Capture and close the descriptor returned by `::mkstemp` in `UniqueDomainName`.
- Preserve existing socket-path behavior and public API.

## Non-goals

- No redesign of `PrivilegedRequest` IPC protocol.
- No change to current path-based UNIX socket addressing model.
- No architectural migration in this change.

## Solution

- Replace the ignored `::mkstemp(binder)` call with:
  - `int fd = ::mkstemp(binder);`
  - close when `fd >= 0`
- Keep returning the generated pathname string as before.
- Keep `OpenDomainSocket` flow intact (`unlink` stale path, then `bind`).

## Related Work

Pathname race-window hardening is intentionally out of scope for this leak fix and is tracked as a separate OpenSpec change.

## Success Criteria

- [ ] No leaked descriptor from `UniqueDomainName`.
- [ ] Existing behavior remains unchanged for request/offer/listen flows.
- [ ] Build remains clean for affected targets.
