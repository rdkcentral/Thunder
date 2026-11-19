# Distributed Tracing

## Goal

Allow Thunder to reconstruct the causal path of an operation across synchronous calls, WorkerPool jobs, notifications, and process boundaries.

The result is a distributed causal trace: local stack snapshots connected across hops, rather than one native stack spanning multiple threads or processes.

## Model

`TraceContext` is small, temporary propagation state:

- `TraceId`: identifies the complete operation.
- `CurrentSpanId`: identifies the currently executing span.
- `Flags`: carries recording decisions with the trace.

```cpp
enum Flags : uint8_t {
	NONE          = 0x00,
	SAMPLED       = 0x01, // Create and publish SpanRecords.
	CAPTURE_STACK = 0x02  // Also capture native stacks for new spans.
};
```

Flags are not permanent thread settings. They propagate with `TraceId` when the context crosses threads or processes. A boundary may remove `CAPTURE_STACK` when only IDs and timing are needed, while still recording the span.

The current context is held in thread-local storage. It is copied into COM-RPC messages and queued work, installed while that work executes, and then restored or discarded.

`SpanRecord` is the stored description of one operation or hop:

- Trace, span, and parent span IDs.
- Start and end times.
- Process, thread, callsign, and operation details.
- Result and an optional local stack snapshot.

Records sharing a `TraceId` are joined through their parent span IDs to reconstruct the complete trace.

```text
Thread-local          Stack-local              Per-process transport             Daemon-wide
TraceContext ---> SpanScope/SpanRecord ---> MessageUnit/Messaging buffer ---> DistributedTracing
```

Only `TraceContext` lives in TLS. `SpanScope` owns the in-progress `SpanRecord`; when the scope ends, it publishes the completed record as a new Messaging event. The existing per-process `MessageUnit` and cyclic buffer transport it to Thunder, so no separate process-local storage is needed.

```cpp
// Simplified SpanScope lifecycle.
class SpanScope {
private:
	TraceContext _previous;
	SpanRecord _record;
};

SpanScope::SpanScope()
	: _previous(Current()) // { TraceId = 100, CurrentSpanId = 10 }
	, _record {
		.TraceId = _previous.TraceId,             // 100
		.SpanId = GenerateSpanId(),               // 11
		.ParentSpanId = _previous.CurrentSpanId   // 10
	}
{
	SetCurrent({ .TraceId = _record.TraceId, .CurrentSpanId = _record.SpanId });
}

SpanScope::~SpanScope()
{
	MessageUnit::Instance().Push(DistributedTracingEvent(_record));
	SetCurrent(_previous); // { TraceId = 100, CurrentSpanId = 10 }
}

{
	SpanScope span;
	Operation(); // Current context is { TraceId = 100, CurrentSpanId = 11 }.
} // The record is submitted and the previous context is restored.
```

### Inter-thread

The submitting thread copies its current context into the queued work, and the worker thread installs that context before creating its own child span:

```cpp
// Thread A: capture the cause when submitting the work.
QueuedJob queuedJob {
	.Job = job,
	.Context = Current() // { TraceId = 100, CurrentSpanId = 10 }
};
WorkerPool.Submit(queuedJob);

// Thread B: continue the trace when dispatching the work.
TraceContext previous = Current();
SetCurrent(queuedJob.Context);
{
	SpanScope span; // Creates span 11 with ParentSpanId = 10.
	queuedJob.Job->Dispatch();
}
SetCurrent(previous);
```

The queue provides propagation between threads; TLS provides the current context within each thread. COM-RPC messages and asynchronous notifications use the same capture, carry, install, and restore pattern.

### Inter-process

Process boundaries work the same way, but the context is serialized into the COM-RPC message instead of being copied into a queued job:

```cpp
// Process A: create an outgoing span and send its context.
SpanScope outgoing;
message.Trace = Current(); // { TraceId = 100, CurrentSpanId = 20 }
channel->Invoke(message);

// Process B: install the received context and create an incoming child span.
TraceContext previous = Current();
SetCurrent(message.Trace);
{
	SpanScope incoming; // Creates span 21 with ParentSpanId = 20.
	stub->Invoke();
}
SetCurrent(previous);
```

The wire representation must use fixed-width fields, and peers must negotiate support so tracing remains compatible with older COM-RPC implementations.

## Boundaries

Spans are created only at meaningful framework boundaries:

- External request entry.
- Incoming and outgoing COM calls.
- Decorated in-process COM calls.
- WorkerPool submission and dispatch.
- Notification production and delivery.
- Timer callbacks.

Ordinary functions continue the current span. Log and trace messages normally become events associated with the current span instead of creating new spans.

An RAII `SpanScope` creates the record, temporarily updates the current context, publishes the finalized record through Messaging, and restores the previous context.

### In-Process Calls

Out-of-process calls can be instrumented in the existing COM-RPC proxy and dispatch paths. Direct in-process calls use the real implementation vtable and have no common invocation boundary, so the ProxyStubGenerator should additionally generate typed local COM decorators:

```text
caller -> generated local decorator -> real implementation
```

Each generated method creates a `SpanScope`, forwards the call without marshalling, records its result, and completes the span. A generic `LocalDecoratorManager` in the COM layer registers generated factories and caches decorators by real `IUnknown` identity and interface ID. Thunder routes local interface acquisition, including `QueryInterfaceByCallsign()`, through this manager before returning the pointer.

Wrapping only the initially acquired interface is insufficient. Generated decorators must also wrap interfaces obtained through `QueryInterface()`, returned from methods, or passed as callbacks. The cache must return the same decorator for matching registration and unregistration calls, while preserving `AddRef()`, `Release()`, and COM identity semantics.

Decorators should remain installed while tracing is disabled so pointers acquired earlier become traceable when it is enabled later. Their disabled path is only an inexpensive runtime check. Any local COM acquisition path that bypasses `LocalDecoratorManager` remains untraced and should therefore be redirected to the shared wrapping helper.

### Generated Metadata

The generic COM-RPC layer sees numeric interface and method IDs, but not their human-readable names. Generated code should therefore register static metadata such as:

```text
(InterfaceId, MethodId) -> Exchange::IFoo::Configure
```

Out-of-process spans remain centralized in `UnknownProxy::Invoke()` and `Administrator::Invoke()`, which record the numeric IDs. `DistributedTracing` resolves them through the generated metadata when presenting a trace. Generated local decorators create in-process spans and use the same metadata. This keeps span lifecycle logic out of generated OOP proxy/stub methods while retaining readable interface and method names.

### Notifications

COM notification delivery can be traced automatically like any other COM call: generic COM-RPC instrumentation handles out-of-process callbacks, while generated local decorators handle direct in-process callbacks. Each subscriber invocation creates a separate child span.

To identify the recipient, the standard notification registration path must associate each callback interface with subscriber metadata such as callsign, process ID, interface ID, and COM instance identity. Generated registration code can wrap the callback with a tracing decorator carrying this metadata. The resulting delivery span then identifies both the notification method and its recipient.

For asynchronous delivery, the queue must also propagate the producer's `TraceContext`, using the same capture and restore mechanism as WorkerPool jobs. Generated decorators can observe delivery, but cannot recover causality or recipient ownership that was discarded by a custom queue or registration mechanism.

## Storage

`DISTRIBUTED_TRACING` is introduced as a new Messaging message type with a compact binary event containing the `SpanRecord`. Each process writes these events through its existing `MessageUnit` and cyclic buffer. Thunder's `MessageClient` pops and deserializes them like other messages, then routes them to a new `DistributedTracing` backend instead of the regular output backends.

`DistributedTracing` owns bounded daemon-wide storage, groups records by `TraceId`, reconstructs their parent-child relationships, and exposes completed traces to a UI, file, or external backend. Existing Messaging controls provide enablement and routing; dropped-span counters report records lost when buffers or storage are full.

### Continuation Counting

Storage lifetime uses continuation counting rather than counting children already received. A continuation represents queued, remote, or otherwise deferred work that may still create a child span. Creating such work acquires a continuation for the current span; cancellation or completion releases it. A span is logically complete only after the span itself has finished and its continuation count reaches zero, meaning no future descendants can be created.

Per-process Messaging buffers are ordered independently, so a child `SpanRecord` may reach `DistributedTracing` before its parent. Only the central backend decides that a trace is complete. It stores child-first records under `(TraceId, SpanId)` with unresolved parent placeholders and joins them when parent records arrive. A subtree can be finalized when its own record has arrived, it is logically complete, and every expected child subtree is complete.

Completed subtrees may then be streamed or compacted to reduce memory use. Hard limits on total bytes, trace age, span count, and unresolved placeholders remain necessary. A timeout finalizes traces as incomplete after process failure, unbalanced continuations, or dropped Messaging events.

To limit overhead, tracing uses sampling, configurable stack capture, raw stack addresses with deferred symbolization, and explicit dropping under pressure. Publishing and processing `DISTRIBUTED_TRACING` events is excluded from instrumentation so that exporting a span cannot generate another span recursively. Tracing must never block normal execution, starve ordinary messages, or grow memory without a fixed limit.