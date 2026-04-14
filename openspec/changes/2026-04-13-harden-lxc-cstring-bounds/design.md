# Design: LXC C-String Bounds Hardening

## Architecture Notes

Consulted architecture and SME constraints before proposing:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

This change is local to `LXCImplementation.cpp` and does not alter interfaces, ABI, or lifecycle ownership.

## Current Risk

### Unbounded formatting

`char buf[256]` is populated using `sprintf()`. While current format strings are short, unbounded formatting is fragile and unsafe by construction.

### Unbounded token parse

`sscanf(..., "%s %llu%n", name, ...)` writes into `char name[128]` without width bound. A long token can overflow `name`.

## Desired Behavior

- Use bounded writes for stack buffers.
- Keep data parsing within declared buffer capacity.
- Preserve existing logic and return behavior.

## Design Decisions

1. Use `snprintf(buf, sizeof(buf), ...)` at the three current formatting sites.
2. Use `%127s` when parsing into `name[128]`.
3. Keep control flow unchanged (`scanned != 2` remains the parse failure gate).

## Trade-offs

- Very low risk and minimal code churn.
- No semantic feature changes; purely defensive hardening.
- Does not attempt broad refactor to C++ string utilities to keep scope focused.
