# Thunder code improvement analysis (2026-04-10)

Priority findings from code review:
- High: 64-bit host/network conversion logic in Source/core/Number.h appears incorrect on little-endian (ToNetwork/FromNetwork return unchanged value for 64-bit).
- High: Potential file descriptor leak in privileged request domain name generation (mkstemp result not closed) in Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h.
- High: Potential null dereference in LXC interface iterator when get_ips returns null but addresses[i] is still accessed in Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp.
- Medium: Non-Linux network adapter operations have inconsistent semantics (assert/no-op/success for unimplemented behavior) in Source/core/NetworkInfo.cpp.
- Medium: test_networkinfo has weak assertions (self-comparisons) and mutates host networking in tests; should be hardened and isolated.
- Low: Replace unbounded sprintf calls with bounded snprintf in LXC implementation.

Suggested fix order:
1) Number.h 64-bit endianness + tests
2) mkstemp fd leak fix
3) LXC null guard
4) NetworkInfo explicit ERROR_NOT_SUPPORTED for unimplemented paths
5) test_networkinfo cleanup/hardening

Status update (2026-04-13):
- Completed: Number.h 64-bit endianness + tests.
	- Code updated in Source/core/Number.h for uint64_t/int64_t ToNetwork/FromNetwork conversions.
	- Tests updated in Tests/unit/core/test_numbertype.cpp with 64-bit expectations and round-trip checks.
	- Added targeted ctest entry Thunder_test_core_numbertype in Tests/unit/core/CMakeLists.txt using --gtest_filter=Core_NumberType*.
	- Validation: Thunder_test_core_numbertype passed.
- Item 2 (mkstemp fd leak): proposal prepared and split from hardening.
	- Archived change: openspec/changes/archive/2026-04-14-fix-privilegedrequest-mkstemp-fd-leak/
	- Review patch (historical): openspec/changes/archive/2026-04-14-fix-privilegedrequest-mkstemp-fd-leak/fix.patch
- Item 3 (LXC null guard): proposal prepared and stored as review patch.
	- Proposal: openspec/changes/2026-04-13-fix-lxc-null-deref-get-ips/
	- Review patch (not applied): openspec/changes/2026-04-13-fix-lxc-null-deref-get-ips/fix.patch
- Item 4 (NetworkInfo unsupported semantics): proposal prepared and stored as review patch.
	- Proposal: openspec/changes/2026-04-13-networkinfo-not-supported-semantics/
	- Review patch (not applied): openspec/changes/2026-04-13-networkinfo-not-supported-semantics/fix.patch
	- Proposal and design rationale clarified for review (return-value contract and trade-offs).
- Item 5 (test_networkinfo hardening): proposal prepared and stored as review patch.
	- Proposal: openspec/changes/2026-04-13-harden-test-networkinfo/
	- Review patch (not applied): openspec/changes/2026-04-13-harden-test-networkinfo/fix.patch
- Item 6 (LXC C-string bounds hardening): proposal prepared and stored as review patch.
	- Proposal: openspec/changes/2026-04-13-harden-lxc-cstring-bounds/
	- Review patch (not applied): openspec/changes/2026-04-13-harden-lxc-cstring-bounds/fix.patch

Administration update (2026-04-14):
- LXC Item 3 (null guard) remains patch-only and is delegated for colleague review.
- LXC Item 6 (C-string bounds hardening) remains patch-only and is delegated for colleague review.
- Source file `Source/extensions/processcontainers/implementations/LXCImplementation/LXCImplementation.cpp` is kept unchanged for both LXC items pending external review.

PrivilegedRequest race-hardening review update (2026-04-14):
- Landed from colleague as part of hardening suggestions:
	- `OpenUniqueDomainSocket` now has bounded retry behavior (`MaxRetries = 5`) for client-side unique bind collisions.
	- Retry loop is limited to `EADDRINUSE`; non-collision bind failures break immediately.
	- `mkstemp`-based unique path generation is rebuilt on each attempt.
- Confirmed gaps (still open):
	- No equivalent retry behavior on server-side fixed connector bind path (`OpenDomainSocket`).
	- No attempt-indexed tracing for retry diagnostics (collision count / exhaustion).
	- No explicit bounded backoff between retries.
- Follow-up prepared (not applied):
	- Review patch to close retry gaps: `openspec/changes/2026-04-13-harden-privilegedrequest-unix-socket-path-race/fix.patch`
	- Scope: server-side bind retries + attempt-indexed retry diagnostics only.

IPTestAdministrator hardening (2026-04-14):
- Change: openspec/changes/2026-04-14-harden-iptestadministrator-waiting-semantics/
- Task 1.1 applied (Tests/unit/IPTestAdministrator.cpp):
	- Bug: `_waitTime` (unit: seconds) was passed raw to `poll()` which expects milliseconds, resulting in 1000x shorter timeout. Child processes were SIGKILL'd after ~4ms instead of ~4s.
	- Fix: explicit seconds-to-milliseconds conversion using `std::chrono`, with overflow clamp to `INT_MAX` before narrowing to `int`.
- Task 1.2 applied (Tests/unit/IPTestAdministrator.cpp):
	- Bug: `Wait()` used a fixed `struct timespec` for all FUTEX_WAIT calls; spurious wakeups always restarted with the original full timeout instead of remaining time, violating the stated wait contract.
	- Fix: replaced fixed timeout with deadline-based accounting via `CLOCK_MONOTONIC`. Spurious wakeups recalculate remaining time against the deadline and retry. Deadline expiry returns `ERROR_TIMEDOUT` directly.
- Task 2.1 applied (Tests/unit/tests/test_iptestmanager.cpp):
	- Added `TimeoutEnforcement` test: validates `ERROR_TIMEDOUT` is returned when no signal arrives before the deadline.
	- Added `DeadlineAccountingWithRetries` test: stresses deadline accounting across multiple signal-wait round trips with retry counts.
	- Tests designed for low-end embedded devices: generous timeouts (10–15 s), conservative iterations (2 rounds). Iteration count is bounded by the destructor's child-wait timeout (also maxWaitTime). Fixed-width integers (`uint32_t`, `int32_t`) used throughout.

Methodology note — test design for embedded targets:
- Use `uint32_t` / `int32_t` for all counter and timing variables in test code; avoid plain `int`.
- Timeout values must account for destructor child-wait budget (same `_waitTime`). Total test duration must stay well within one `_waitTime` period per synchronization step.
- Iteration counts in stress loops should be validated against worst-case retry math: `iterations × (maxWaitTime + maxRetries × (maxWaitTime / waitTimeDivisor))`.

Gate unit test coverage instrumentation (2026-04-14):
- Change: openspec/changes/archive/2026-04-15-gate-unit-test-coverage-instrumentation/
- Problem: `Tests/unit/CMakeLists.txt` unconditionally applied coverage instrumentation flags (`--coverage -fprofile-arcs`) even when `ENABLE_CODE_COVERAGE=OFF`. Caused libgcov profiling warnings in fork-heavy test environments (esp. IPTestAdministrator).
- Task 1.1 applied (Tests/unit/CMakeLists.txt):
	- Wrapped `add_compile_options(--coverage -fno-inline -fno-inline-small-functions -fno-default-inline)` and `add_link_options(-fprofile-arcs)` in `if (ENABLE_CODE_COVERAGE)` guard.
	- Coverage instrumentation now only applied when explicitly enabled.

TODO backlog:
- Future hardening enhancement: reduce/eliminate pathname race window around UNIX domain socket `unlink` -> `bind` in privileged request flow (evaluate private 0700 runtime dir, Linux abstract namespace sockets, or descriptor-only alternatives).
