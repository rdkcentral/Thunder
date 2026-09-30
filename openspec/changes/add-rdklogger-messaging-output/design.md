## Context

`SYSLOG` currently admits and constructs a message using Thunder's local category state, then `MessageUnit::Push` routes it to DirectOutput and/or the shared handler buffer according to `OutputMode`. MessageControl consumes the handler route asynchronously.

RDKLogger has its own module/level enablement state. The external destination must therefore be admitted independently from local Thunder output. The producer may construct a message when either destination is interested, then submit only to the destinations that accepted it.

The checked RDKLogger implementation invokes Log4C synchronously. The adapter will treat submission as synchronous unless target deployment evidence changes that assumption.

## Decisions

### Routing model

Add `EXTERNAL_DIRECT` beside the existing `HANDLER` and `DIRECT` routes.

- `HANDLER`: shared buffer and MessageControl only.
- `DIRECT`: Thunder DirectOutput only.
- `EXTERNAL_DIRECT`: RDKLogger only.
- `ALL`: handler, DirectOutput, and RDKLogger.

The route is selected by Messaging configuration. RDKLogger enablement still applies independently within the external route.

### Admission and construction

Local Thunder enablement and cached RDKLogger interest are independent gates. A message may be constructed when either gate is interested. Each destination then applies its own gate before submission.

When neither destination is interested, the producer avoids message construction and submission. This preserves the existing fast path while allowing RDKLogger to receive messages when local Thunder output is disabled.

### RDKLogger module and level mapping

The initial module format is:

`LOG.RDK.THUNDER.<MODULE_NAME>`

`MODULE_NAME` is the existing Thunder module identifier and is already unique per plugin. Thunder will pass it as agreed with the RDKLogger team; this change does not introduce a second normalization, truncation, validation, or collision-resolution policy.

The mapping covers all Thunder message types. Telemetry is not duplicated to RDKLogger when the same message is routed to the handler, because MessageControl forwards that path to the T2 backend. If telemetry is routed externally without the handler destination, the normal telemetry mapping applies.

Tracing uses the same named severity categories as logging. `Fatal`/`Crash` map to `FATAL`, `Error` to `ERROR`, `Warning`/`Warn` to `WARN`, and `Notice` to `NOTICE`. Other tracing categories map to `TRACE`.

When a logging or tracing message uses the default external level (`INFO` for logging or `TRACE` for tracing), the adapter prefixes the payload with the non-empty category in the form `<CATEGORY>: <PAYLOAD>`. Explicitly classified messages retain the original payload unchanged.

| Thunder type | RDKLogger level |
| --- | --- |
| ASSERT / fatal-like | FATAL |
| REPORTING | WARN |
| TRACING Fatal/Crash | FATAL |
| TRACING Error | ERROR |
| TRACING Warning/Warn | WARN |
| TRACING Notice | NOTICE |
| Other TRACING | TRACE |
| TELEMETRY | NOTICE |
| OPERATIONAL_STREAM | TRACE |
| LOGGING crash/fatal | FATAL |
| LOGGING error | ERROR |
| LOGGING warning/warn | WARN |
| LOGGING notice | NOTICE |
| Other LOGGING | INFO |

The initial payload submitted to RDKLogger is the message payload, except for default-level logging and tracing messages, where a non-empty category is prefixed as `<CATEGORY>: <PAYLOAD>`. Thunder metadata is otherwise used only for module and level selection.

No additional normalization is performed by Thunder.

### Enablement cache

After Messaging opens, the adapter eagerly queries each known module/level pair once. The cache key is `MODULE_NAME` and the mapped RDKLogger level. Duplicate queries are suppressed.

When a new control/module is announced, the adapter immediately queries that new pair. This phase does not add polling, callbacks, generation tracking, or a full refresh operation.

### Lifecycle and failure behavior

Every process that opens Messaging owns its own RDKLogger initialization and teardown. There is no daemon-only forwarding path and no second copy sent through MessageControl.

RDKLogger initialization or query failure disables only external routing. Local DirectOutput and handler delivery continue under their existing rules. Deinitialization occurs during Messaging teardown.

### Build boundary

RDKLogger support is optional and OFF by default. RDKLogger headers and libraries are referenced only by the Messaging integration when the feature is enabled. Core primitives and non-RDK builds must remain free of the dependency and preserve current behavior.

### Deployment category responsibility

Thunder documents the selected `LOG.RDK.THUNDER.<MODULE_NAME>` category mapping and uses the RDKLogger API. The RDKLogger team owns registration of those categories in Log4C or an equivalent target configuration and owns deployment-time enablement/disablement. Thunder does not generate, install, or maintain the RDKLogger configuration in this change.

### Public adapter contract and mock backend

The adapter SHALL follow the existing Telemetry backend pattern: a small public C interface defines lifecycle, enablement, and submission operations, while Messaging selects the real or mock implementation at build time. The public contract is intentionally narrow and does not expose RDKLogger or other backend internals.

The mock backend implements the same interface, prints submissions to the console, and may expose test-only call inspection. It allows SDK-independent builds and tests without weakening the production boundary.

## Alternatives Considered

### Change MessageControl

Rejected. MessageControl is a consumer and should not own external sink policy. Changing it would couple asynchronous local delivery to the independently controlled RDKLogger route.

### Send all messages through RDKLogger first

Rejected. This would make external availability and latency affect existing local output, and would prevent local-only deployments from preserving current behavior.

### Poll RDKLogger state on every message

Rejected. It adds producer overhead and does not match the agreed cached-query model. The first phase uses eager initialization and immediate queries for newly announced controls.

### Make RDKLogger submission asynchronous in Messaging

Deferred. The inspected implementation is synchronous, and introducing a queue/thread would change ordering, lifecycle, and failure semantics. It can be revisited with deployment evidence.

## Confirmed Boundaries Before Implementation

- Thunder uses the existing `MODULE_NAME` contract without additional normalization.
- The RDKLogger team owns category registration and deployment configuration.
- Messaging owns a public C adapter contract, modeled on the Telemetry backend separation pattern.
- The real and mock backends implement the same contract and are selected by CMake.
- Buildroot, Lithosphere, SDK packaging, and target deployment integration are outside this change.
