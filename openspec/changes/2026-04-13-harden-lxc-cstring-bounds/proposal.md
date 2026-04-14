# Proposal: Harden LXC C-String Bounds Handling

## Problem Statement

`Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp` contains two low-level C-string patterns that should be tightened:
- Unbounded `sprintf()` into fixed-size stack buffer `char buf[256]`.
- `sscanf()` with `%s` into fixed-size stack buffer `char name[128]`.

Even when current input sizes are expected to be small, these patterns can become unsafe under unexpected data growth and are avoidable with bounded forms.

## Scope

- Replace unbounded `sprintf()` with bounded `snprintf()` at current call sites.
- Constrain `sscanf()` string token parsing with explicit width (`%127s`) for `char name[128]`.
- Keep behavior and control flow otherwise unchanged.

## Non-goals

- No functional changes to container lifecycle logic.
- No changes to LXC API usage or tracing behavior.
- No broad migration of all C-string operations outside targeted call sites.

## Proposed Changes

- In `LXCNetworkInterfaceIterator` constructor:
  - `sprintf(buf, "lxc.net.%d.type", netnr)` -> `snprintf(buf, sizeof(buf), "lxc.net.%d.type", netnr)`
  - `sprintf(buf, "lxc.net.%d.name", netnr)` -> `snprintf(buf, sizeof(buf), "lxc.net.%d.name", netnr)`
  - `sprintf(buf, "lxc.net.%d.link", netnr)` -> `snprintf(buf, sizeof(buf), "lxc.net.%d.link", netnr)`
- In memory parser loop:
  - `sscanf(buffer + position, "%s %llu%n", name, &value, &charsRead)` -> `sscanf(buffer + position, "%127s %llu%n", name, &value, &charsRead)`

## Success Criteria

- [ ] All targeted `sprintf()` calls are replaced with bounded `snprintf()`.
- [ ] `%s` parse into `name[128]` uses explicit width `%127s`.
- [ ] No behavioral regression in normal execution paths.
- [ ] Build remains clean for affected target(s).
