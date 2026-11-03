## Context

`SYSLOG` currently admits and constructs a message using Thunder's local category state, then `MessageUnit::Push` routes it to DirectOutput and/or the shared handler buffer according to `OutputMode`. MessageControl consumes the handler route asynchronously.

The external adapter has its own module/level enablement state. The external destination must therefore be admitted independently from local Thunder output. The producer may construct a message when either destination is interested, then submit only to the destinations that accepted it.

The example adapter invokes its logging API synchronously. The adapter will treat submission as synchronous unless target deployment evidence changes that assumption.

## Decisions

### Routing model

Add `EXTERNAL_DIRECT` beside the existing `HANDLER` and `DIRECT` routes.

- `HANDLER`: shared buffer and MessageControl only.
- `DIRECT`: Thunder DirectOutput only.
- `EXTERNAL_DIRECT`: external adapter only.
- `ALL`: handler, DirectOutput, and external adapter.

The route is selected by Messaging configuration. External adapter enablement still applies independently within the external route.

When neither destination is interested, the producer avoids message construction and submission. This preserves the existing fast path while allowing the external adapter to receive messages when local Thunder output is disabled.

### Adapter-specific mapping

The external adapter owns the mapping from Thunder message metadata to its backend categories, levels, and payload format. The adapter receives the existing `MODULE_NAME` unchanged; Thunder does not introduce a second normalization, truncation, validation, or collision-resolution policy.

Adapter-specific mapping, category registration, and deployment configuration belong with the adapter and its platform integration. Thunder only defines the generic External Direct contract and passes the module, message type, category, and payload to the adapter.

### Enablement cache

After Messaging opens, the adapter eagerly queries each known module/level pair once. The cache key is `MODULE_NAME` and the mapped external level. Duplicate queries are suppressed.

When a new control/module is announced, the adapter immediately queries that new pair. This phase does not add polling, callbacks, generation tracking, or a full refresh operation.

### Lifecycle and failure behavior

Every process that opens Messaging owns its own external adapter initialization and teardown. There is no daemon-only forwarding path and no second copy sent through MessageControl.

External adapter initialization or query failure disables only external routing. Local DirectOutput and handler delivery continue under their existing rules. Deinitialization occurs during Messaging teardown.

### Build boundary

External output support is optional and OFF by default. Adapter headers and libraries are referenced only by the Messaging integration when the feature is enabled. Core primitives and builds without external output must remain free of the dependency and preserve current behavior.

### Public adapter contract and mock backend

The adapter SHALL follow the existing Telemetry backend pattern: a small internal C interface defines lifecycle, enablement, and submission operations between Messaging and its selected backend implementation. Messaging selects the real or mock implementation at build time. The internal contract is intentionally narrow and does not expose any backend internals as part of the installed Thunder SDK.

The mock backend implements the same interface, prints submissions to the console, and may expose test-only call inspection. It allows SDK-independent builds and tests without weakening the production boundary.

## Alternatives Considered

### Change MessageControl

Rejected. MessageControl is a consumer and should not own external sink policy. Changing it would couple asynchronous local delivery to the independently controlled external route.

### Send all messages through the external adapter first

Rejected. This would make external availability and latency affect existing local output, and would prevent local-only deployments from preserving current behavior.

### Poll external state on every message

Rejected. It adds producer overhead and does not match the agreed cached-query model. The first phase uses eager initialization and immediate queries for newly announced controls.

### Make external submission asynchronous in Messaging

Deferred. The inspected implementation is synchronous, and introducing a queue/thread would change ordering, lifecycle, and failure semantics. It can be revisited with deployment evidence.

## Confirmed Boundaries Before Implementation

- Thunder uses the existing `MODULE_NAME` contract without additional normalization.
- The adapter/platform team owns category registration and deployment configuration.
- Messaging owns a public C adapter contract, modeled on the Telemetry backend separation pattern.
- The real and mock backends implement the same contract and are selected by CMake.
- Buildroot, Lithosphere, SDK packaging, and target deployment integration are outside this change.
