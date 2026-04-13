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
- Next item: mkstemp fd leak fix in Source/extensions/privilegedrequest/include/privilegedrequest/PrivilegedRequest.h.

TODO backlog:
- Investigate and document the newly identified issue in a different Thunder branch (details pending).
- Future hardening enhancement: reduce/eliminate pathname race window around UNIX domain socket `unlink` -> `bind` in privileged request flow (evaluate private 0700 runtime dir, Linux abstract namespace sockets, or descriptor-only alternatives).
