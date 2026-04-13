# Design: LXC get_ips Null Pointer Guard

## Architecture Notes

Consulted architecture brief and SME guidance before proposing:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

Change is strictly local to `LXCNetworkInterfaceIterator` constructor. No API boundaries or lifecycle contracts are affected.

## Current Behavior

```
get_ips() returns nullptr (interface has no IPs or LXC call fails)
    ↓
null check fires → warning logged
    ↓                          ← falls through, no skip
numAddresses = 0
for (i = 0; addresses[i] != nullptr; i++)   ← DEREF nullptr 💥
```

## Proposed Behavior

```
get_ips() returns nullptr
    ↓
null check fires → warning logged → loop skipped
    ↓
interface pushed with numAddresses = 0 ✓

get_ips() returns valid pointer
    ↓
loop counts addresses normally ✓
interface pushed with correct numAddresses ✓
```

## Design Decisions

1. **Restructure rather than add `continue`** — Restructuring the null check into an `if/else` is cleaner and more readable than `if (null) { trace; continue; }` because it keeps the interface push unconditional outside the branch, making the intent explicit that an interface with zero addresses is still a valid, recorded interface.

2. **Move `numAddresses = 0` before the branch** — Initializing before the conditional makes both paths consistent and avoids accidentally leaving it uninitialized on the null path.

3. **No change to `_interfaces.push_back`** — The push remains unconditional so interfaces without IPs are preserved. Callers can already handle `numAddresses == 0`.

## Risk Assessment

- Low: single-method, isolated change.
- Low: no protocol, API, or lifecycle impact.
- Low: the null case was already partially handled (warning trace existed); this completes the guard.
