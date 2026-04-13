# Proposal: Fix Null Pointer Dereference in LXC get_ips Handling

## Problem Statement

In `LXCNetworkInterfaceIterator::LXCNetworkInterfaceIterator` (`Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp`), `lxcContainer->get_ips()` can return `nullptr` when an interface has no IP addresses or the call fails. A null check exists and logs a warning, but does **not** guard the immediately following loop:

```cpp
interface.addresses = lxcContainer->get_ips(lxcContainer, interface.name, NULL, 0);
if (interface.addresses == 0) {
    TRACE(Trace::Warning, (_T("Failed to get IP addresses...")));
    // ← no skip — falls through to:
}

interface.numAddresses = 0;
for (int i = 0; interface.addresses[i] != nullptr; i++) {  // ← null deref if addresses == nullptr
    interface.numAddresses++;
}
```

This causes undefined behaviour (typically a crash) whenever an interface has no IPs or `get_ips` fails.

## Scope

- Fix the null guard in `LXCNetworkInterfaceIterator` constructor to skip the address counting loop when `addresses` is null.
- Preserve existing warning trace.
- Still push the interface to `_interfaces` with `numAddresses = 0` so the interface entry is not silently dropped.

## Non-goals

- No changes to the LXC API interaction pattern.
- No broader refactor of `LXCNetworkInterfaceIterator`.

## Solution

Add a skip guard so that when `addresses` is null, `numAddresses` stays `0` and the dereference loop is bypassed. The interface is still recorded.

```cpp
interface.addresses = lxcContainer->get_ips(lxcContainer, interface.name, NULL, 0);
interface.numAddresses = 0;
if (interface.addresses == nullptr) {
    TRACE(Trace::Warning, (_T("Failed to get IP addresses inside container. Interface: %s"), interface.name));
} else {
    for (int i = 0; interface.addresses[i] != nullptr; i++) {
        interface.numAddresses++;
    }
}
```

## Success Criteria

- [ ] No null dereference when `get_ips` returns null.
- [ ] Interface with no IP addresses is still recorded with `numAddresses = 0`.
- [ ] Warning trace is preserved.
- [ ] Build remains clean for affected targets.
