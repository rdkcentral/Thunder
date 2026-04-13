# Proposal: Normalize NetworkInfo Unsupported-Operation Semantics (Non-Linux)

## Problem Statement

`Source/core/NetworkInfo.cpp` has inconsistent behavior on non-Linux platforms for unimplemented adapter operations:
- Some TODO methods return `Core::ERROR_NONE` (false success).
- Some TODO methods call `ASSERT(false)` and return `Core::ERROR_BAD_REQUEST`.
- Similar unsupported operations use mixed semantics across `__WINDOWS__` and `__APPLE__` blocks.

This inconsistency makes caller behavior unpredictable and can trigger avoidable debug aborts for expected unsupported operations.

## Rationale

This proposal is a correctness and contract-clarity change for non-Linux unsupported paths.

Today, callers can receive three different signals for the same runtime condition (operation not implemented on platform):
- Success (`Core::ERROR_NONE`) from no-op TODO stubs.
- Bad request (`Core::ERROR_BAD_REQUEST`) from assert-backed TODO stubs.
- Process abort in Debug builds due to `ASSERT(false)`.

These outcomes are misleading for integrators:
- `ERROR_NONE` implies state changed when nothing happened.
- `ERROR_BAD_REQUEST` implies caller misuse, not missing platform support.
- Debug aborts make expected unsupported calls fail catastrophically instead of returning a handled status.

Standardizing these branches to `Core::ERROR_NOT_SUPPORTED` improves behavior without changing platform capabilities:
- Callers get a truthful, deterministic result and can implement fallback logic.
- Error handling becomes consistent across methods and non-Linux platforms.
- Linux functional paths remain untouched, so runtime behavior there is unchanged.

Net effect: a more reliable API contract now, with clean semantics that also make future platform implementations easier to reason about.

## Scope

- Normalize unimplemented adapter operation semantics in non-Linux `NetworkInfo` paths.
- Target methods in `AdapterIterator` platform blocks (`__WINDOWS__`, `__APPLE__`) that are currently TODO/no-op/assert paths.
- Return `Core::ERROR_NOT_SUPPORTED` consistently for unsupported operations.

## Non-goals

- No Linux netlink behavior changes.
- No implementation of currently TODO network configuration operations.
- No behavioral hardening of `test_networkinfo` in this change (tracked separately as item 5).

## Proposed Semantics

For non-Linux, unimplemented mutating adapter operations should consistently return `Core::ERROR_NOT_SUPPORTED`:
- `Up(const bool enabled)` when not implemented
- `Add(const IPNode&)` when not implemented
- `Delete(const IPNode&)` when not implemented
- `Gateway(const IPNode&, const NodeId&)` when not implemented
- `Broadcast(const NodeId&)` when not implemented

Additionally, debug-only aborts (`ASSERT(false)`) should be removed from these unsupported operation paths.

## Success Criteria

- [ ] All targeted unimplemented non-Linux operations return `Core::ERROR_NOT_SUPPORTED`.
- [ ] No `ASSERT(false)` remains in those unsupported operation branches.
- [ ] Build remains clean for affected targets.
- [ ] Behavior aligns with documented API contract expectations for unsupported operations.
