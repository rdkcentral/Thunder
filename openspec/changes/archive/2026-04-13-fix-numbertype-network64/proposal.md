# Proposal: Fix NumberType 64-bit Host/Network Conversion

## Problem Statement

`NumberType<uint64_t>::ToNetwork/FromNetwork` and signed 64-bit equivalents in `Source/core/Number.h` returned input unchanged on little-endian builds and asserted on the alternate branch. This diverged from 16/32-bit conversion behavior and produced incorrect wire-order handling for 64-bit values.

## Scope

- Correct 64-bit conversion behavior in `Source/core/Number.h`.
- Keep public API unchanged.
- Update unit tests in `Tests/unit/core/test_numbertype.cpp` to validate 64-bit expectations.
- Add a narrow CTest entry for `Core_NumberType` to support targeted validation.

## Non-goals

- No interface or ABI changes.
- No broader refactor of NumberType parsing/serialization logic.
- No behavior changes for 8/16/32-bit conversion paths.

## Solution

- Implement 64-bit conversion by splitting into high/low 32-bit parts and applying `htonl/ntohl`, then recombining.
- Reuse unsigned conversion in signed 64-bit wrappers.
- Update test expectations:
  - little-endian: `90 -> 0x5A00000000000000`
  - big-endian: unchanged `90`
- Add round-trip assertions for 64-bit:
  - `FromNetwork(ToNetwork(x)) == x`

## Success Criteria

- [x] 64-bit `ToNetwork/FromNetwork` behave consistently with wire-order expectations.
- [x] Unit tests verify the corrected behavior for signed/unsigned 64-bit.
- [x] Targeted CTest entry exists and passes for NumberType suite.
