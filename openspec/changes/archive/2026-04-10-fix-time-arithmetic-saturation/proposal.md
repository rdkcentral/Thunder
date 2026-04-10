# Proposal: Fix Time Arithmetic Underflow/Overflow

## Problem Statement

The `Thunder::Core::Time::Add()` and `Time::Sub()` methods perform unsigned 64-bit arithmetic without overflow/underflow guards. Silent wrapping causes:

- **Sub() Underflow**: Subtracting more than current ticks wraps to `uint64_t::max - delta` (huge future timestamp)
- **Add() Overflow**: Exceeding `uint64_t::max` wraps to a small value
- **Silent Data Corruption**: No error reporting, assertions, or diagnostics — developers won't detect the failure

### Impact

Time arithmetic affects every timeout, deadline, and scheduling operation in Thunder:
- Timeouts become infinite waits (underflow wraps to year 2262)
- Missed deadlines silently pass (wrap-around)
- Race conditions in workflow coordination
- Silent corruption is **worse than crashes** — fails in production without visibility

### Current Behavior

```cpp
// Source/core/Time.cpp lines 1151-1165
Time& Time::Add(const uint32_t timeInMilliseconds) {
    uint64_t newTime = Ticks() + (static_cast<uint64_t>(timeInMilliseconds) * MicroSecondsPerMilliSecond);
    return (operator=(Time(newTime)));  // Silent wrap if overflow
}

Time& Time::Sub(const uint32_t timeInMilliseconds) {
    uint64_t newTime = Ticks() - (static_cast<uint64_t>(timeInMilliseconds) * MicroSecondsPerMilliSecond);
    return (operator=(Time(newTime)));  // Silent wrap if underflow
}
```

Existing tests (test_time_arithmetic.cpp) intentionally avoid underflow by starting from `Time::Now()` which is a huge epoch value.

## Solution

Implement **dual-mode arithmetic**:

1. **Production (Release Builds)**: Saturation behavior
   - Underflow: Clamp to 0 (minimum timestamp)
   - Overflow: Clamp to `uint64_t::max` (maximum timestamp)
   - Reason: Predictable, self-healing, no API changes

2. **Debug Builds**: Assertions
   - Assert on underflow/overflow before saturation
   - Forces developers to catch edge cases during testing
   - Reason: Fail loudly during development, safely degrade in production

### Benefits

- **No API Changes**: Existing code continues to work
- **Explicit**: Behavior is documented and predictable
- **Safe**: Silent corruption replaced with explicit clamping
- **Debuggable**: Developers alerted to boundary conditions early
- **Production-Safe**: Systems won't crash from edge cases

## Success Criteria

- [x] Time::Add() clamps on overflow (debug: asserts first)
- [x] Time::Sub() clamps on underflow (debug: asserts first)
- [x] All existing tests pass
- [x] New tests cover boundary cases (max values, 0, near-limits)
- [x] No changes to method signatures or return types
- [x] Documentation updated with saturation behavior

## Affected Components

- **Source/core/Time.cpp**: Add() and Sub() implementations
- **Source/core/Time.h**: Public API (documentation only, no signature changes)
- **Tests/unit/core/test_time_arithmetic.cpp**: Add boundary case tests
- **Tests/unit/core/test_timer.cpp**: Verify no regressions

## Estimate

- **Implementation**: 2-3 hours
- **Testing**: 1-2 hours
- **Review**: 30 minutes
- **Total**: ~4 hours

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Existing code relies on wrap-around | Medium | Unlikely (no wrap-around is documented); saturation is more predictable |
| Debug asserts catch legitimate edge cases | Low | Assert only on true boundary; document expected limits |
| Performance impact of saturation checks | Very Low | Simple comparisons; negligible CPU cost |

## Related Issues

- User memory: Time arithmetic can underflow/overflow (documented with examples)
- No existing GitHub issues; discovered via code review
