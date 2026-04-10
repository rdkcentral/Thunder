# Thunder Architecture Brief

## Document Role

This brief is the system-level source for architecture boundaries, runtime model, and change governance.
Implementation guardrails and recurring coding pitfalls are maintained in `docs/SME-Notes.md`.

## Purpose

Thunder is a plugin-centric runtime framework for embedded and platform software. The daemon is infrastructure-only: it hosts plugins, manages lifecycle, and provides communication, configuration, observability, and process supervision.

## Architectural Intent

- Keep runtime infrastructure deterministic, compact, and reusable.
- Keep domain logic in plugins, not in framework layers.
- Preserve binary compatibility of published COM contracts.
- Support both in-process and out-of-process plugin execution.
- Favor defensive behavior in foundational primitives (time, memory, sync, IPC).

## Layered Structure

- `Source/core`
	- Platform abstractions and primitives (threading, sync, containers, filesystem, timers, monitor loops).
	- Must remain independent of upper layers.
- `Source/com`
	- COM-RPC contract and transport machinery (IUnknown model, proxy/stub dispatch, communicator plumbing).
	- Depends on core only.
- `Source/plugins`
	- Plugin authoring API and framework-facing contracts (`IPlugin`, shell/context interfaces, JSON-RPC integration points).
	- Depends on core and com.
- `Source/Thunder`
	- Daemon runtime orchestration (plugin registry, activation/deactivation, config loading, channel and request routing, OOP hosting coordination).
- `Source/ThunderPlugin`
	- Child-process host path for out-of-process plugin execution.
- `Source/websocket`, `Source/messaging`, `Source/cryptalgo`
	- Protocol and auxiliary service layers used by runtime and plugins.

## Architectural Boundaries

- `core` must not include or depend on `com`, `plugins`, or `Thunder` runtime.
- `com` must not depend on plugin/runtime implementation details.
- `plugins` must consume public framework contracts only.
- Runtime ownership of plugin state transitions is centralized in the daemon lifecycle pipeline.
- Inter-plugin coupling should favor COM interface paths over ad-hoc transport shortcuts.

## Runtime Model

- Startup
	- Daemon loads configuration, initializes communication endpoints, and prepares plugin/service registry.
- Activation
	- Plugin activation performs controlled resource setup through framework-managed lifecycle entry points.
- Steady state
	- Requests flow through framework routing and worker infrastructure; plugin code handles domain behavior.
- Deactivation/shutdown
	- Lifecycle teardown must invert setup exactly to avoid leaks and stale registrations.
- OOP mode
	- Out-of-process plugins are launched/monitored through runtime hosting infrastructure with explicit communication and supervision paths.

## Contract Stability

- COM interfaces are treated as ABI contracts.
- Published interfaces should be extended via versioned additions, not in-place signature mutation.
- Ref-count ownership and lifetime boundaries remain explicit across API surfaces.

## Non-Functional Priorities

1. Correctness under boundary conditions (overflow/underflow, races, timeout paths).
2. Reliability under load and partial failure (bounded resources, predictable teardown).
3. Observability and diagnosability (traceability, assertions, postmortem-friendly behavior).
4. Reproducible builds and tests across Linux and Windows toolchains.

## Testing Strategy

- Unit tests
	- Validate primitive behavior and edge semantics in core facilities.
- Component/integration tests
	- Validate lifecycle, IPC, plugin activation/deactivation, and transport paths.
- Build-mode coverage
	- Confirm meaningful behavior in both debug and release profiles where semantics differ.

## Current Technical Risks

- Drift between lifecycle rules and plugin implementation patterns.
- Boundary assumptions at COM/IPC edges (ownership, casting, timeout behavior).
- Hidden coupling introduced across architecture layers.
- Documentation divergence between governance guidance and implementation reality.

## Change Governance

- Any change crossing module boundaries must document contract and lifecycle impact.
- Any semantic change in core/runtime behavior must include regression tests.
- Prefer additive, backward-compatible evolution unless a break is explicitly approved.
- Governance updates that imply behavioral changes must be reflected in architecture and SME documentation in the same change.
