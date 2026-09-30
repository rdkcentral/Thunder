## Preparation

- [x] Confirm the current Messaging output-mode definitions and the owning implementation files.
- [ ] Confirm the exact RDKLogger SDK/library/API available on the target build.
- [ ] Confirm the existing `MODULE_NAME` value and document the agreed `LOG.RDK.THUNDER.<MODULE_NAME>` mapping for the RDKLogger team.
- [x] Define the public C external-output interface, following the existing Telemetry backend separation pattern.

## Routing and producer path

- [x] Add `EXTERNAL_DIRECT` while preserving the existing `HANDLER` and `DIRECT` route meanings.
- [x] Define `ALL` as handler + DirectOutput + RDKLogger.
- [x] Keep `HANDLER` and `DIRECT` behavior unchanged.
- [x] Add the external admission check to the producer path.
- [x] Ensure messages are constructed when either local or external output is interested.
- [x] Ensure messages are not constructed when neither destination is interested.
- [x] Submit the payload independently to each selected destination.

## RDKLogger adapter and cache

- [x] Add the optional adapter at the Messaging layer.
- [x] Add the real and mock implementations behind the same public C external-output interface.
- [x] Add a CMake option to select the mock backend for SDK-independent tests and development builds.
- [x] Pass the existing `MODULE_NAME` unchanged; do not add a second normalization or collision policy.
- [x] Implement the complete Thunder-type-to-RDK-level mapping, including category-aware tracing and telemetry de-duplication with the handler/T2 path.
- [x] Implement the agreed payload submission behavior, including category prefixes for default-level logging and tracing.
- [x] Eagerly populate the module/level cache after Messaging opens.
- [x] Deduplicate repeated enabled-state queries.
- [x] Query newly announced controls immediately.
- [x] Preserve local output when RDKLogger initialization or query fails.

## Lifecycle and build

- [x] Add an opt-in build option, OFF by default, without adding RDKLogger dependencies to core.
- [x] Initialize RDKLogger for every process that opens Messaging.
- [x] Deinitialize RDKLogger during Messaging teardown.
- [ ] Verify in-process and out-of-process ownership behavior.
- [ ] Verify non-RDK builds retain current behavior.

## Tests and documentation

- [x] Add adapter tests for module/level mapping and payload submission.
- [x] Add cache tests for eager population, duplicate suppression, and late announcements.
- [x] Add routing tests for each route and for independent local/external gates.
- [x] Add failure tests proving local output survives RDKLogger failure.
- [ ] Add an integration test or target-device check for category registration.
- [x] Add a regression test proving `ALL` does not duplicate external output through MessageControl.
- [x] Document the build option, RDKLogger-owned category registration, and synchronous submission assumption.
- [ ] Build and test both RDK-enabled and RDK-disabled configurations.
