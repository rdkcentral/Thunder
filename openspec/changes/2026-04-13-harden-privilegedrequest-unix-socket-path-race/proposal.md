# Proposal: Harden PrivilegedRequest UNIX Socket Pathname Race Handling

## Problem Statement

`PrivilegedRequest` currently uses pathname-based UNIX domain sockets with a stale-path cleanup pattern (`unlink` before `bind`). While functionally correct, strict hardening standards may require minimizing or eliminating pathname race exposure around path lifecycle.

## Scope

- Evaluate hardening strategies for UNIX socket endpoint naming and binding in `PrivilegedRequest`.
- Select and specify a hardened approach that preserves compatibility requirements.
- Implement and validate the selected hardening approach in a dedicated change.

## Non-goals

- No changes to descriptor payload protocol semantics.
- No unrelated refactors in `PrivilegedRequest`.

## Candidate Approaches

1. Use a private runtime directory with mode `0700` and bind only within it.
2. Use Linux abstract namespace sockets where platform policy allows.
3. Replace pathname rendezvous where feasible with descriptor-only flow.

## Success Criteria

- [ ] Selected approach documented with trade-offs.
- [ ] Race exposure materially reduced for supported environments.
- [ ] Existing request/offer behavior remains functionally intact.
- [ ] Validation covers startup, request/offer exchange, and cleanup paths.
