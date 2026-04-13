# Design: Hardening UNIX Socket Pathname Handling in PrivilegedRequest

## Context

Current behavior in `PrivilegedRequest` uses filesystem pathname sockets and stale-path cleanup before binding. This has operationally acceptable behavior but may not satisfy strict hardening targets around path ownership and race minimization.

## Design Goals

- Improve endpoint binding safety under strict local threat assumptions.
- Preserve current IPC behavior and compatibility where required.
- Keep implementation maintainable and observable.

## Evaluated Options

### Option A: Private runtime directory (preferred baseline)

- Create a dedicated runtime directory with `0700` permissions.
- Generate and bind socket path only within this directory.
- Restrict path namespace to owning process/user context.

Pros:
- Strong compatibility with pathname socket flows.
- Clear security boundary with minimal protocol impact.

Cons:
- Requires lifecycle management for runtime directory.

### Option B: Linux abstract namespace sockets

- Use abstract namespace addresses instead of filesystem paths.

Pros:
- Removes filesystem path races by design.

Cons:
- Linux-specific behavior; portability constraints.
- Requires compatibility review with existing consumers.

### Option C: Descriptor-only rendezvous redesign

- Avoid pathname rendezvous where feasible.

Pros:
- Strongest long-term model.

Cons:
- Larger redesign, higher migration risk.

## Proposed Direction

Phase 1 hardening should implement Option A (private 0700 runtime directory), with Option B as an optional Linux optimization subject to compatibility checks.

## Risks

- Medium rollout risk if deployment environments have path assumptions.
- Mitigated by controlled fallback and compatibility validation matrix.
