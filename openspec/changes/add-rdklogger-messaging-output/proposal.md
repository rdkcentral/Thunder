## Why

Thunder Messaging currently has local output routes, but no independent route to RDKLogger. Adding that route must preserve the existing MessageControl and DirectOutput behavior while allowing RDKLogger to decide whether an external module/level is enabled.

The integration belongs at the Messaging layer. MessageControl remains an asynchronous consumer of the handler buffer, and DirectOutput remains responsible for local console/syslog output. RDKLogger is an additional destination, not a replacement for either existing destination.

## What Changes

- Add an opt-in RDKLogger output adapter in Thunder Messaging.
- Add `EXTERNAL_DIRECT` as a routing destination.
- Define `ALL` as handler + DirectOutput + RDKLogger.
- Keep local Thunder enablement independent from cached RDKLogger interest.
- Cache RDKLogger enabled-state queries per distinct `MODULE_NAME` and level.
- Initialize and deinitialize RDKLogger independently in every process that opens Messaging.
- Preserve existing behavior when RDKLogger support is disabled, unavailable, or fails during initialization.

The first implementation phase uses the existing Messaging output path and a small adapter around the RDKLogger C API. A later architectural phase may separate socket-like ownership concerns where needed, but that is outside this change.

## Goals

- Allow external RDKLogger control without changing MessageControl APIs.
- Avoid paying for message construction when neither local nor external output is interested.
- Ensure `HANDLER`, `DIRECT`, `EXTERNAL_DIRECT`, and `ALL` have explicit, testable semantics.
- Keep the core library independent of the optional RDKLogger SDK.
- Make behavior deterministic in-process and out-of-process deployments.

## Non-Goals

- Replacing MessageControl or changing its consumer protocol.
- Changing DirectOutput formatting or local enablement semantics.
- Adding runtime polling, callbacks, generations, or full cache refresh.
- Owning or generating the target platform's `debug.ini`.
- Supporting arbitrary RDKLogger category administration in the first phase.
- Redesigning RDKLogger itself or making its submission asynchronous.

## Impact

The change is limited to Thunder Messaging, its optional build integration, focused tests, and documentation. Non-RDK builds retain the current output behavior. Existing callers of `SYSLOG` and existing MessageControl consumers require no API changes.

## Resolved Design Questions

1. Thunder SHALL pass the existing `MODULE_NAME` value agreed with the RDKLogger team. Thunder does not normalize, truncate, validate, or resolve collisions beyond the existing `MODULE_NAME` contract, which is unique per plugin.
2. Thunder SHALL document the emitted RDKLogger category mapping. The RDKLogger team owns category registration, Log4C configuration, and deployment-time enablement/disablement.
3. The adapter contract SHALL be a small public C interface in Messaging, following the same backend boundary used by the Telemetry output path. The implementation and backend selection remain owned by Messaging.
4. Tests and SDK-independent builds SHALL select a mock backend implementing the same public C interface. The mock prints submissions to the console and can record calls for focused tests.
5. Optional build integration means Thunder's CMake option and target/library selection only. Buildroot, Lithosphere, SDK packaging, and target deployment integration are outside this change.
