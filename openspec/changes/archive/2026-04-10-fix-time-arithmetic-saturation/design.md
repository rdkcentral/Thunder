# Design: Time Arithmetic Overflow/Underflow Saturation

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│ Public API: Time::Add(ms), Time::Sub(ms)                │
│ Behavior: Saturation (clamp to min/max)                 │
└────────────┬────────────────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────────────┐
│ Internal: In-method saturating arithmetic               │
│ - Overflow guard in Time::Add()                         │
│ - Underflow guard in Time::Sub()                        │
└────────────┬────────────────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────────────┐
│ Debug Assertions (Thunder ASSERT_VERBOSE)              │
│ - Pre-saturation overflow assert in Add                │
│ - Pre-saturation underflow assert in Sub               │
└─────────────────────────────────────────────────────────┘
```

## Implementation Details

### 1. In-method Saturation Logic

Implement saturation checks directly inside `Time::Add()` and `Time::Sub()`:

```cpp
Time& Time::Add(const uint32_t timeInMilliseconds)
{
    const uint64_t currentTicks = Ticks();
    const uint64_t microSecsToAdd = static_cast<uint64_t>(timeInMilliseconds) * static_cast<uint64_t>(MicroSecondsPerMilliSecond);
    uint64_t newTime = currentTicks;

    const uint64_t maxTicks = NumberType<uint64_t>::Max();

    ASSERT_VERBOSE((currentTicks <= (maxTicks - microSecsToAdd)),
        "Time::Add overflow detected (ticks=%" PRIu64 ", delta=%" PRIu64 ", max=%" PRIu64 ")",
        currentTicks,
        microSecsToAdd,
        maxTicks);

    if (currentTicks > (maxTicks - microSecsToAdd)) {
        newTime = maxTicks;
    } else {
        newTime += microSecsToAdd;
    }

    return (operator=(Time(newTime)));
}

Time& Time::Sub(const uint32_t timeInMilliseconds)
{
    const uint64_t currentTicks = Ticks();
    const uint64_t microSecsToSub = static_cast<uint64_t>(timeInMilliseconds) * static_cast<uint64_t>(MicroSecondsPerMilliSecond);
    uint64_t newTime = currentTicks;

    ASSERT_VERBOSE((currentTicks >= microSecsToSub),
        "Time::Sub underflow detected (ticks=%" PRIu64 ", delta=%" PRIu64 ")",
        currentTicks,
        microSecsToSub);

    if (currentTicks < microSecsToSub) {
        newTime = 0;
    } else {
        newTime -= microSecsToSub;
    }

    return (operator=(Time(newTime)));
}
```

**Rationale for checks:**
- Overflow check: MAX - delta > ticks ⟹ overflow
- Underflow check: ticks < delta ⟹ underflow (since ticks is unsigned)

### 2. Assertion and Diagnostic Strategy

Use Thunder-native assertion macros:

- `ASSERT_VERBOSE` for explicit diagnostics
- `PRIu64` for portable formatting of `uint64_t` values
- Pre-saturation assertions in debug/assert-enabled configurations

**Design Decisions:**
- Saturation is transparent in Release builds (no API changes)
- Debug/assert-enabled builds assert before saturation
- Assertions wrapped in #ifndef NDEBUG for dual-mode behavior
- Simple comparison checks keep runtime impact negligible

### 3. Documentation Updates

Add to Time.h public documentation for Add/Sub methods:

```cpp
/**
 * Add milliseconds to this time (with saturation).
 * 
 * Uses saturation arithmetic: overflow clamps to max, underflow clamps to 0.
 * Assertions trigger in debug/assert-enabled builds before saturation.
 * 
 * @param timeInMilliseconds Time in milliseconds to add (0 to UINT32_MAX)
 * @return Reference to this Time object (modified)
 * 
 * @note Saturation ensures predictable behavior at boundaries:
 *       - Adding to near-max timestamp returns max timestamp
 *       - No silent wrapping to small values
 */
Time& Add(const uint32_t timeInMilliseconds);
```

### 4. Test Strategy

**Boundary Cases to Test**:
- Add to 0: Should add normally
- Add to near max uint64_t: Should clamp to max uint64_t
- Sub from 0: Should clamp to 0
- Sub from small value: Should clamp to 0
- Valid arithmetic near boundaries: Should work correctly

**Test Organization**:
- Extend test_time.cpp with saturation tests
- Keep existing happy-path tests unchanged
- Use nested brace initialization: `TimeAsLocal{Time{value}}`
- All tests pass in Release mode (exit code 0)
- All tests abort in Debug mode (ASSERT_VERBOSE = exit 134)

## Backward Compatibility

✅ **Fully compatible** — Method signatures unchanged, return type unchanged
- Existing code continues to work
- Behavior improves (saturation vs. wrap-around)
- No caller changes needed

## Thread Safety

✅ **Thread-safe** — No shared state introduced
- In-method checks are local computations
- No new static/global state
- Existing Time object thread-safety properties preserved

## Performance Impact

✅ **Negligible** — Simple comparisons
- Add path: One subtraction + one comparison + one branch
- Sub path: One comparison + one branch
- Assertions are compiled out when asserts are disabled (#ifndef NDEBUG)
