# Design: PrivilegedRequest mkstemp Descriptor Leak Fix

## Architecture Notes

Consulted architecture and SME guidance before proposing implementation:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

The change is localized to `PrivilegedRequest::Connection::UniqueDomainName` and keeps service boundaries and API contracts unchanged.

## Current Behavior

`UniqueDomainName`:
1. builds a template pathname ending with `.XXXXXX`
2. calls `::mkstemp` to atomically materialize a unique file path
3. returns the path for later socket `bind`

The descriptor returned by `::mkstemp` is currently ignored, which leaks an fd.

## Proposed Behavior

- Capture `::mkstemp` result in a local `int fd`.
- If `fd >= 0`, close it immediately using `::close(fd)`.
- Preserve the generated pathname and existing control flow.

## Why This Is Safe

- `unlink(path)` after `close(fd)` remains valid and is expected on UNIX.
- `bind` behavior remains unchanged: stale path is removed before binding.
- No changes to message format, callback API, or lifecycle logic.

## Risks

- Low functional risk: narrow, local resource cleanup fix.
- Low compatibility risk: UNIX-only block already guarded in existing code.

## Deferred Hardening Enhancement

Pathname race-window hardening is tracked in a separate OpenSpec change. This design remains strictly focused on mkstemp descriptor leak remediation.
