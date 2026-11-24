# Thunder Release Notes R4.4.8

## Introduction

This document describes the changes introduced in Thunder R4.4.8 (compared to the R4.4.7 release) and in ThunderTools.

The changes described here are present on the [R4_4 branch](https://github.com/rdkcentral/Thunder/tree/R4_4) and represent all commits since the [R4.4.7 tag](https://github.com/rdkcentral/Thunder/compare/R4.4.7...R4_4).


---

## Thunder and ThunderTools

### Features

#### Feature: JSON-RPC rich error support

Methods can now report a structured error (an error code, a message and optional additional data) through JSON-RPC. A new `Core::ErrorDetails<DATA>` type (marked with `@richerror` for ThunderTools) describes such an error:

```cpp
template<typename DATA = void>
struct ErrorDetails {
    OptionalType<int32_t> code;    // Type of the error
    OptionalType<string> message;  // A short message describing the error
    OptionalType<DATA> data;       // Additional information about the error (not present for ErrorDetails<void>)
};
```

Handlers that report such an error are registered with the new `RegisterWithError<INBOUND, OUTBOUND, ERROR>()` method (available on `Core::JSONRPC::Handler` and `PluginHost::JSONRPC`), both for plain and for real object (`this`) based handlers.

**Key properties:**
- Only the fields that are set in the returned `ErrorDetails` are used; the Thunder framework default error code and message are preserved for any field that is not provided.
- Handlers with an index or a context (channel/request information) are supported.
- Existing `Register()` based handlers are not affected.

---

#### Feature: OpenTelemetry support and W3C trace context propagation

An OpenTelemetry extension (`Source/extensions/opentelemetry`) has been added, together with a per-job trace context (`Core::JobContext`) that carries the W3C `traceparent` and `tracestate` through Thunder.

Both are **disabled by default** and can be enabled at build time:

| CMake option | Effect |
|---|---|
| `WPEFRAMEWORK_OPENTELEMETRY` | Builds the OpenTelemetry extension. |
| `W3C_TRACE_CONTEXT_ENABLED` | Allows propagation of the W3C trace context: `traceparent` / `tracestate` are accepted in JSON-RPC requests, are made available to the job handling the request and are forwarded over COM-RPC. |

Note that enabling `W3C_TRACE_CONTEXT_ENABLED` extends the COM-RPC message header, so Thunder and all (out-of-process) plugins/clients must be built with the same setting.

---

#### Feature: Messaging External Direct output (RDK Logger)

The Messaging component can now route messages directly to an external output backend in addition to the existing `direct` and `handler` outputs. A new routing mode `external_direct` is available (`Core::Messaging::OutputMode::EXTERNAL_DIRECT`) and the routing of Assert, Warning Reporting, Operational Stream, Telemetry, Tracing and Logging messages can be directed to it.

The backend is a small shared library (`MessagingExternalOutput`) that implements the C interface in `Source/messaging/ExternalOutput.h` (`ThunderExternalOutput_Initialize`, `_IsEnabled`, `_Submit` and `_Deinitialize`). This feature is meant to be used together with the [RDKLogger adapter](https://github.com/rdkcentral/ThunderExtensions/tree/R4_4/MessagingControl/RDKLogger) in the R4_4 branch of ThunderExtensions, which lets RDK platforms use the `rdklogger` library to output the Thunder logs instead of the Thunder Messaging outputs:

- Messages are written with `RDK_LOG` under the module name `LOG.RDK.THUNDER.<module>`.
- Whether a message is generated at all is decided by the rdklogger configuration (`rdk_logger_is_logLevel_enabled`), so the usual RDK log level control applies to Thunder messages as well.
- Message types are mapped to RDK log levels: Logging and Tracing categories (`Notice`, `Warning`/`Warn`, `Error`, `Fatal`/`Crash`) map to `NOTICE`, `WARN`, `ERROR` and `FATAL` (other Logging categories to `INFO`, other Tracing categories to `TRACE`), Operational Stream to `TRACE`, Telemetry to `NOTICE`, Warning Reporting to `WARN` and Assert to `FATAL`.

The feature is **disabled by default** and is enabled at build time with the following CMake options:

| CMake option | Effect |
|---|---|
| `MESSAGING_EXTERNAL_OUTPUT` | Enables External Direct output support. The adapter is taken from the [ThunderExtensions](https://github.com/rdkcentral/ThunderExtensions) repository (`MESSAGING_EXTERNAL_OUTPUT_REPOSITORY`, version `MESSAGING_EXTERNAL_OUTPUT_VERSION`, default `R4_4`), or from a local directory set with `MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR`. |
| `MESSAGING_EXTERNAL_OUTPUT_USE_MOCK` | Builds the adapter against a mock rdklogger instead of the real library (`rdk_logger.h` / `librdkloggers`), for builds and tests without the RDK SDK. |

When the real library is used, `rdk_logger.h` and `librdkloggers` must be available at build time.

Additionally, the time it takes to initialize Messaging is now reported at startup ("Messaging latency").

---

#### Feature: COM-RPC timeout diagnostics

The error messages logged when a COM-RPC call times out (`Invoke`, remote `QueryInterface` and remote proxy release) now also contain the PID of the remote process (`RemotePid`), making it much easier to identify which plugin process did not respond.

For debugging purposes the new `COMRPC_ABORT_ON_TIMEOUT` CMake option (default `OFF`) makes Thunder abort when a COM-RPC invoke times out, so a core dump of the unresponsive state can be collected. Only the side that owns the COM-RPC server (Thunder) aborts, clients do not.

---

#### Feature: `WorkerPool::WaitForStop()`

`Core::WorkerPool` (and `Core::ThreadPool`) gain a `WaitForStop(uint32_t timeout = Core::infinite)` method that waits until all worker threads have stopped (or are blocked) and returns `false` if this did not happen within the timeout.

---

### General bug fixes

#### Fix: Worker pool job lifetime during shutdown

The lifetime of worker pool jobs was not handled correctly while a process (e.g. `WPEProcess`) was shutting down. Jobs are now revoked and the worker pool is waited for before the process is torn down. In addition, `Core::QueueType::Remove()` now also removes entries when the queue has been disabled, so revoking a job during shutdown no longer leaves it in the queue.

#### Fix: `JSON::String` serialization of non-ASCII characters

When exactly 256, 512, ... bytes remained in the string being serialized, the remaining length was truncated to 8 bits before it was handed to `ToCodePoint`, causing a non-ASCII character to be serialized incorrectly (for example `\u0003` instead of `\u00E9`). The length is now clamped. A unit test was added.

#### Fix: `Frame::Reader::LockFixedBuffer()`

Reading a fixed size buffer from a `FrameType` reader (`LockFixedBuffer`) was not working correctly and has been fixed. A unit test was added.

#### Fix: `SocketPort` Closed logic

The change to the `Closed()` handling of `SocketPort` that was introduced in an earlier release (destroying the socket before the `SHUTDOWN` state was cleared) has been reverted to the original logic, as it did not take into account that during the state change the socket can be recreated and moved to the listening state.

#### Fix: Memory retained by JSON-RPC messages

`JSON::String::Clear()` gained an overload that takes the capacity that may be retained. The JSON-RPC message (`Parameters`, `Result`, `Error`) now shrinks buffers that grew beyond that size once the message is cleared, so a single large message no longer keeps its memory allocated after it has been handled.

#### Fix: Controller documentation synchronized with the interface

The documentation of the Controller (`IController.h`, `Controller.json`, `ControllerPlugin.json` and `ControllerPlugin.md`) was synchronized with the implementation, adding missing descriptions, parameters and return values. The `timeout` argument of `ILifeTime::Hibernate()` is now declared as `uint32_t` (the underlying type of `Core::hresult`), so the type of the argument is unchanged.

---

### Deprecations

- **Custom error codes** (`Core::CustomCode()`, `Core::IsCustomCode()` and the `CustomCodeLibrary` configuration) are obsolete. See Breaking Changes below. Use the new rich error support to report custom error codes and messages.
- **Dynamic JSON-RPC error messages**: `PluginHost::JSONRPCErrorAssessor` is now marked `DEPRECATED` and will generate a compiler warning when used. Use the rich error support instead.

---

### Breaking Changes

- **Custom error codes are disabled by default.** The custom error code feature (introduced in R4.4.5) is now obsolete and is only compiled in when the new `CUSTOM_ERROR_CODES_ENABLED` CMake option is turned on (it is `OFF` by default). When it is off, the `CUSTOM_ERROR` define, `Core::CustomCode()`, `Core::IsCustomCode()` and the loading of the custom code library (`CustomCodeLibrary` setting) are no longer available. Code that uses them must either enable `CUSTOM_ERROR_CODES_ENABLED` or migrate to the rich error support described above.
- **`Core::Messaging::OutputMode` values changed.** To make room for `EXTERNAL_DIRECT` the values are now `ALL = 0`, `HANDLER = 1`, `DIRECT = 2` and `EXTERNAL_DIRECT = 3` (previously `HANDLER = 0`, `DIRECT = 1`, `ALL = 2`). The value is exchanged between Thunder and the processes hosting plugins, so Thunder and all plugins/libraries must be rebuilt against R4.4.8 and not be mixed with R4.4.7 binaries.
- When `W3C_TRACE_CONTEXT_ENABLED` is turned on (off by default) the COM-RPC message header changes, see above.

Other than the items above, Thunder R4.4.8 does not introduce breaking changes relative to R4.4.7. All interface signatures remain unchanged.

---
