# Design: NumberType 64-bit Network Conversion

## Architecture Notes

Consulted architecture and SME guidance before implementation:
- `docs/thunder-architecture-brief.md`
- `docs/SME-Notes.md`

Change stays within `Source/core` and preserves public contracts.

## Design Decisions

1. Use explicit 64-bit conversion logic in `NumberType`.
- `ToNetwork(uint64_t)`:
  - convert high 32 and low 32 using `htonl`
  - recombine in network order
- `FromNetwork(uint64_t)`:
  - convert high 32 and low 32 using `ntohl`
  - recombine in host order

2. Keep signed 64-bit wrappers simple.
- `ToNetwork(int64_t)` delegates through `uint64_t` conversion via casts.
- `FromNetwork(int64_t)` delegates through `uint64_t` conversion via casts.

3. Strengthen tests around 64-bit paths.
- Update expected values per endianness in `Core_NumberType.generic`.
- Add explicit signed/unsigned round-trip checks.

4. Add a focused CTest target for reliability.
- New test registration:
  - `Thunder_test_core_numbertype`
  - command: `Thunder_test_core --gtest_filter=Core_NumberType*`

## Risk Assessment

- Low risk: conversion behavior aligns with established 16/32-bit pattern.
- Low risk: no API changes.
- Medium operational risk mitigated by filtered test target due unrelated full-suite instability in environment.
