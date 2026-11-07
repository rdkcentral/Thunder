# OpenTelemetry in Thunder plugins

[OpenTelemetry](https://opentelemetry.io/docs/what-is-opentelemetry/) is an open standard and a collection of APIs and tools for producing *observability data*: information that helps explain what software did while it was running. Instead of relying only on a message printed by one process, an application can describe an operation, attach useful details, and follow that operation as it crosses process or service boundaries. The data can then be exported to an OpenTelemetry-compatible collector or other observability backend for inspection. OpenTelemetry defines common concepts and formats; it does not require plugin developers to use one particular tracing UI or monitoring service.

OpenTelemetry groups this data into three related kinds of *signals*:

* **Traces** describe the path of a request or operation through a system. A trace contains one or more **spans**. Each span names an operation and records when it began and ended; spans can also carry attributes such as an operation name or result. A parent/child relationship connects spans so that work performed by a plugin can be associated with the client request that caused it.
* **Logs** are individual records about events, such as a failure or a completed operation. A log record can carry structured attributes and, when trace identifiers are attached, can be correlated with the operation that produced it.
* **Metrics** are numerical measurements collected over time, such as the number of operations or their duration. Counters and histograms let you answer questions about volume and performance without examining every individual trace.

For example, imagine a client calling a Calculator plugin. The client can start a span for the request, the plugin can start a child span for the multiplication, and a worker thread can start another span when it sends the result. Those spans can appear together in one trace even though the work crosses IPC and thread boundaries. A log emitted during the calculation can identify the same trace; a counter can record that another multiplication happened. You do not have to use all three signals for every operation: add the ones that help answer the questions relevant to your plugin.

The connection between spans is carried by a **trace context**. The W3C `traceparent` header identifies the trace and the current parent span, while the optional `tracestate` header carries additional vendor-specific trace information. Propagating these headers does not itself create a span or export data: it lets the next piece of work continue the same trace. Within Thunder, trace context is available through `Core::JobContext::TraceContextW3C`; when using a transport outside Thunder IPC or moving work to another thread, you must carry the context yourself as described below. Thunder takes of propagating the trace context for you automatically when using COMRPC or JSONRPC using the trace context available in thread executing the IPC command. It will also inject it automatically in outgoing COMRPC and JSONRPC and read it from incoming COMRPC and JSONRPC requests and make it available to the thread handling the invocation, so not something you need to care about as Plugin developer. Only when the trace contex needs to be moved to another thread (e.g. when code is executed in a WorkerPool job where it needs to be connect to the running trace context) or when it needs to be received or injected in another IPC mechanism aditional coding is needed.

Data also needs a destination. An **exporter** sends telemetry to a console or an OTLP-over-HTTP endpoint. An OpenTelemetry Collector is one possible receiver for OTLP data and can forward it to a backend, but Thunder does not require any particular collector or backend. A **sampler** decides which traces to record, while a **processor** determines how recorded spans are prepared for export. These are separate decisions: setting an HTTP endpoint does not force every request to be sampled.

Thunder's [OpenTelemetry layer](../../Source/addons/opentelemetry/OpenTelemetry.h) is a thin integration with [OpenTelemetry C++](https://opentelemetry-cpp.readthedocs.io/en/latest/index.html#), not a full wrapper around that extensive library. Its job is to make the common configuration choices easier and bridge OpenTelemetry to Thunder's trace-context propagation through IPC. It deliberately exposes some OpenTelemetry C++ interfaces, such as spans, loggers, and meters, so you can use the underlying API for more specialized needs. This is separate from Thunder's existing [messaging facilities](messaging.md): you opt in to OpenTelemetry where distributed traces, correlated records, or metrics are useful; this guide does not require replacing existing plugin messages.

The rest of this guide shows how to enable the library, configure it once for a component, and use each signal from a plugin or client. The [ThunderNanoServices examples](https://github.com/rdkcentral/ThunderNanoServices/tree/master/examples/ThunderOpenTelemetry) show complete Calculator plugins and COM-RPC/JSON-RPC clients.

## Plugin CMake setup

There are two build steps: the Thunder build must include OpenTelemetry support, and the plugin must link against the resulting library. Make the upstream `opentelemetry-cpp` CMake package available, then build and install Thunder with `-DTHUNDER_OPENTELEMETRY=ON -DW3C_TRACE_CONTEXT_ENABLED=ON` so the OpenTelemetry library and its CMake package are available. In a plugin's `CMakeLists.txt`, add the package lookup alongside the other Thunder packages and link the imported target to the plugin:

```cmake
find_package(${NAMESPACE}OpenTelemetry REQUIRED)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}OpenTelemetry::${NAMESPACE}OpenTelemetry
)
```

Here `${MODULE_NAME}` is the plugin target already defined by `add_library()`; keep the plugin's existing dependencies in its `target_link_libraries()` call. The imported target supplies the dependency needed by your plugin; the CMake addition alone does not configure or initialize telemetry at runtime. The same package and imported target can be used by a standalone client. See the [Calculator plugin CMakeLists.txt](https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/ThunderOpenTelemetry/Calculator/CMakeLists.txt) and [COM-RPC client CMakeLists.txt](https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/ThunderOpenTelemetry/CalculatorClientCOMRPC/CMakeLists.txt) for complete examples. If the package cannot be found, check that the Thunder build included `OPENTELEMETRY=ON` and make its installation available to CMake using the same package search setup as for the other Thunder packages.

## Configuration and lifecycle

After linking, include `<opentelemetry/OpenTelemetry.h>` (adjust the include path for your build layout). `Config` holds the settings used when the telemetry providers are initialized. Configure **before** the first `Initialize()`; do not wait until after creating the first span, log, or instrument. For a component that owns initialization:

```cpp
Thunder::Telemetry::OpenTelemetry::Config config;
config.Sampler = Thunder::Telemetry::OpenTelemetry::SamplerType::PARENT_ON;
config.Processor = Thunder::Telemetry::OpenTelemetry::ProcessorType::BATCH;
config.Exporter = Thunder::Telemetry::OpenTelemetry::ExporterType::HTTP;
config.TracerEndpoint = "http://127.0.0.1:4318/v1/traces";
config.ComponentName = "Calculator";
config.ComponentVersion = "1.0.0";
config.Logger.Enabled = true;
config.Logger.Endpoint = "http://127.0.0.1:4318/v1/logs";
config.Metrics.Enabled = true;
config.Metrics.Endpoint = "http://127.0.0.1:4318/v1/metrics";

Thunder::Telemetry::OpenTelemetry::Configure(config);
Thunder::Telemetry::OpenTelemetry::Instance().Initialize();
// Use telemetry while the component is running.
Thunder::Telemetry::OpenTelemetry::Instance().Deinitialize();
```

The example selects an HTTP exporter so traces, logs, and metrics can be sent to their respective OTLP endpoints. For a first local experiment, keep the default console exporter and start with only the settings you need. `ComponentName` and `ComponentVersion` identify the source of telemetry; choose values that help distinguish your component's data from that of other components.

Call `Deinitialize()` in the corresponding shutdown path after telemetry work has finished. Configuration and providers are process-wide; plugins in the same process should coordinate ownership rather than configuring independently. In particular, do not treat each span as an initialization cycle: initialize for the component's lifetime, then create spans as operations occur. For **entservices** plugins, initialization and deinitialization helper functions are planned in `entservices-helpers`; use them instead of duplicating setup when available.

`Config` also accepts JSON (for example via `FromString()`):

```json
{
  "sampler": "ParentOn",
  "processor": "Batch",
  "exporter": "HTTP",
  "tracerendpoint": "http://127.0.0.1:4318/v1/traces",
  "componentname": "Calculator",
  "componentversion": "1.0.0",
  "logger": { "enabled": true, "endpoint": "http://127.0.0.1:4318/v1/logs" },
  "metrics": { "enabled": true, "endpoint": "http://127.0.0.1:4318/v1/metrics", "intervalms": 10000, "timeoutms": 5000 }
}
```

| Setting | Options and defaults |
| --- | --- |
| `sampler` | `AlwaysOn`, `AlwaysOff`, `ParentOn` (default), or `ParentOff`. Parent-based modes follow a parent's sampling decision; without a parent they respectively sample or do not sample. |
| `processor` | `Synchronous` (default) or `Batch`. |
| `exporter` | `Console` (default) or `HTTP` (OTLP over HTTP). |
| `tracerendpoint` | Defaults to `http://127.0.0.1:4318/v1/traces`. |
| `componentname`, `componentversion` | Default to `Thunder` and `1.0.0`. |
| `logger` | `enabled` defaults to `true`; HTTP `endpoint` defaults to `http://127.0.0.1:4318/v1/logs`. |
| `metrics` | `enabled` defaults to `false`; HTTP `endpoint` defaults to `http://127.0.0.1:4318/v1/metrics`; `intervalms` and `timeoutms` default to 10000 and 5000. |
| `resources` | Optional `schemaURL` and `attributes` array of `{ "key": "...", "value": "..." }` pairs. |

Only specified fields override the defaults. `ParentOn` is often useful when a request enters with existing trace context: the upstream sampling choice is followed, and a request without a parent starts sampled. `ParentOff` also follows an upstream decision but does not sample a new root trace. `Synchronous` exports work as spans are processed; `Batch` groups recorded data for export. Consider the data volume and the cost of exporting when choosing between them. With the console exporter, output goes to the console rather than the HTTP endpoints. The default metrics setting is disabled, so set `metrics.enabled` to `true` before expecting measurements in an exporter. See the [COM-RPC client example](https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/ThunderOpenTelemetry/CalculatorClientCOMRPC/CalculatorClientCOMRPC.cpp) for a complete client setup. Note with logging and /or metrics disabled it is still safe to call their API, they will just result in a no operation being called.,

## Tracing

A span represents a specific unit of work. Choose a name that will help someone recognize that work in a trace viewer, such as a plugin method or a meaningful processing stage. Start it near the beginning of the work, keep it alive while that work happens, and end it when the operation is complete. After initialization, a synchronous operation needs only one line to create a span:

```cpp
auto span = Thunder::Telemetry::OpenTelemetry::Instance().StartSpan("Calculator Multiply");
```

Keep `span` alive for the operation. Its destructor calls `End()`, so an explicit `span->End()` is optional; use it if the operation ends before the variable leaves scope. If the current thread has a W3C trace context, the new span uses it as its parent and updates `Core::JobContext` with the new span's context. This is how related work can become child spans in one trace instead of unrelated root traces. The context is restored when the span ends. You can also use the underlying OpenTelemetry C++ span API, e.g. `(*span)->SetAttribute("operation", "multiply");`, to make a span more informative. Keep attribute values useful and avoid putting sensitive or unbounded request data into telemetry.

### Propagate trace context outside Thunder IPC

Thunder propagates trace context through its supported IPC paths, but `Core::JobContext` itself is thread-local. Copying a job to another thread does not copy that thread-local value. If you start a span there without first passing the context, it cannot be linked to the operation that scheduled the job. For work submitted to another thread, copy the context when scheduling the job and install it on the destination thread **before** starting its span:

```cpp
// On the submitting thread, pass context to the job with its other arguments:
Thunder::Core::JobContext::W3CTraceContext context =
    Thunder::Core::JobContext::Instance().TraceContextW3C();

// On the receiving thread:
Thunder::Core::JobContext::Instance().TraceContextW3C() = context;
{
    auto span = Thunder::Telemetry::OpenTelemetry::Instance().StartSpan("Send result");
    // Perform the job here.
}
Thunder::Core::JobContext::Instance().TraceContextW3C().ClearTraceContext();
```

The saved value is a copy: it can travel with the job's other arguments, then be assigned to the worker's context. Clearing the worker thread afterward matters because a worker pool may reuse that thread for a different request. See the [Calculator implementation (capture)](https://github.com/rdkcentral/ThunderNanoServices/blob/45d4a8100b8abd4742f3a9c1038dfe3a8d6a1739/examples/ThunderOpenTelemetry/Calculator/CalculatorImplementation.cpp#L132) and [dispatch](https://github.com/rdkcentral/ThunderNanoServices/blob/45d4a8100b8abd4742f3a9c1038dfe3a8d6a1739/examples/ThunderOpenTelemetry/Calculator/CalculatorImplementation.cpp#L158).

For non-Thunder transports, such as a separate HTTP request or an application-defined message, the sender and receiver must agree to carry the W3C `traceparent` and `tracestate` headers as strings. Read the current context while the relevant span is in scope. Send `traceparent` as the required parent identifier and send `tracestate` only if present:

```cpp
auto& trace = Thunder::Core::JobContext::Instance().TraceContextW3C();
const std::string traceparent = trace.TraceParent();
const std::string tracestate = trace.TraceState();
// Add traceparent and (if present) tracestate to outgoing transport headers.
```

On receipt, install the headers before creating the next span. Clear them after the work completes to avoid leaking context through a reused thread:

```cpp
auto& trace = Thunder::Core::JobContext::Instance().TraceContextW3C();
trace.ClearTraceContext();
trace.TraceParent(receivedTraceparent);
if (!receivedTracestate.empty()) {
    trace.TraceState(receivedTracestate);
}
{
    auto span = Thunder::Telemetry::OpenTelemetry::Instance().StartSpan("Handle request");
    // Handle the request here.
}
trace.ClearTraceContext();
```

Use a valid W3C `traceparent` header, not an arbitrary string. If no `traceparent` was received, omit its setter to start a new trace instead. Installing received context establishes a parent for subsequent spans; it does not automatically create a span for the receiving operation, which is why the example calls `StartSpan()` explicitly.

### Activate the OpenTelemetry runtime context

There are two related notions of "current context" here. Thunder uses `Core::JobContext` to propagate the W3C headers across its own work and IPC. Opentelemetry C++ library API instead uses its own OpenTelemetry C++ **runtime context** to find the current span implicitly. `StartSpan()` updates Thunder's `JobContext` but does not automatically make the span current in that OpenTelemetry runtime context. Activate it before calling such code, and optionally deactivate (will automatically deactivate when span ends) it when those calls are complete:

```cpp
auto span = Thunder::Telemetry::OpenTelemetry::Instance().StartSpan("Call external library");
span->Activate();
// Call code that reads the OpenTelemetry C++ runtime context.
span->Deactivate();
```

Activation is only necessary for code that relies on the OpenTelemetry runtime context; normal Thunder propagation through `Core::JobContext` does not require it. Alternatively, if the thread already has a Thunder job context and you need to expose it to OpenTelemetry-aware code **without** creating a new span, use a scope:

```cpp
{
    auto scope = Thunder::Telemetry::OpenTelemetry::Instance().StartScopeFromJobContext();
    // OpenTelemetry-aware calls here see Core::JobContext's context.
}
```

The scope temporarily attaches the context and detaches it when the scope object is destroyed. Keep it alive for the calls that need it. Do not move an active scope across threads: propagate the W3C context and establish a new scope or span on the destination thread instead.

## Logging

Logs are useful for events that are worth recording individually, such as an operation failing or a state changing. OpenTelemetry log records can be exported alongside traces, but a log does not become a span: it is a separate event that can be correlated with a trace if it carries trace and span identifiers. Use the OpenTelemetry C++ logger for a simple record when correlation is not required:

```cpp
auto& telemetry = Thunder::Telemetry::OpenTelemetry::Instance();
telemetry.Logger()->Log(opentelemetry::logs::Severity::kInfo, "Multiply requested");
```

When a request is being traced, the more useful option is often a structured record associated with its span. `CreateTraceFilledLogRecord()` creates a record and fills in identifiers from the current Thunder job context when available. Set a severity and useful attributes, then emit the record:

```cpp
auto& telemetry = Thunder::Telemetry::OpenTelemetry::Instance();
auto record = telemetry.CreateTraceFilledLogRecord();
record->SetSeverity(opentelemetry::logs::Severity::kInfo);
record->SetAttribute("operation", "multiply");
telemetry.Logger()->EmitLogRecord(std::move(record));
```

Create the record while the correct request context is present; otherwise it cannot be correlated with that request. Of course also as described in the tracing section an Activated span or created scop can be used to make the opentelemetry C++ runtime context aware of the current span.

```cpp
auto scope = Thunder::Telemetry::OpenTelemetry::Instance().StartScopeFromJobContext();
auto& telemetry = Thunder::Telemetry::OpenTelemetry::Instance();
telemetry.Logger()->Log(opentelemetry::logs::Severity::kInfo, "Multiply requested");
```

If logging is disabled, `Logger()` is a no-op logger; calls remain safe. The [Calculator implementation](https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/ThunderOpenTelemetry/Calculator/CalculatorImplementation.cpp) demonstrates a trace-correlated log record.

## Metrics

Metrics summarize behavior across many operations rather than showing the steps of one request. A **counter** increases when an event occurs, making it suitable for counting calls or failures. A **histogram** records a distribution of measurements, which is useful for tracking operation durations rather than storing just one total. Enable `metrics.enabled` during configuration, then obtain the OpenTelemetry C++ meter and create instruments:

```cpp
auto& telemetry = Thunder::Telemetry::OpenTelemetry::Instance();
auto count = telemetry.Meter()->CreateUInt64Counter(
    "multiplications", "Number of multiplications processed", "{multiplications}");
auto duration = telemetry.Meter()->CreateDoubleHistogram(
    "multiplications.duration", "Multiplication duration", "ms");

std::map<std::string, std::string> attributes{{"multiplications.provider", "thunder"}};
auto view = opentelemetry::common::KeyValueIterableView<decltype(attributes)>{attributes};
count->Add(1, view);
duration->Record(42.0, view, opentelemetry::context::Context{}); // Use the measured duration.
```

Here `count->Add(1, view)` records one processed multiplication and `duration->Record(...)` records one observed duration in the declared unit, milliseconds. Replace `42.0` with a real measurement. The attribute describes the measurement, so the resulting series can be broken down by provider; avoid attributes with a distinct value for every request, such as a request ID, because they produce too many metric series. Create instruments once and retain them for repeated measurements instead of recreating them for every request. `Meter()` uses a no-op meter if metrics are disabled, so metric calls remain safe. See the [COM-RPC client](https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/ThunderOpenTelemetry/CalculatorClientCOMRPC/CalculatorClientCOMRPC.cpp) for metrics with attributes.

This is the first version of Thunder's OpenTelemetry integration; its interfaces and capabilities will continue to be improved and extended.
