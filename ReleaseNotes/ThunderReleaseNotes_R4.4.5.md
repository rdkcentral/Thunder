# Thunder Release Notes R4.4.5

## Introduction

This document describes the changes introduced in Thunder R4.4.5 (compared to the R4.4.4 release).

The changes described here are present on the [R4_4 branch](https://github.com/rdkcentral/Thunder/tree/R4_4) and represent all commits since the [R4.4.4 tag](https://github.com/rdkcentral/Thunder/compare/R4.4.4...R4_4).

# Thunder

## Features

Normally a patch release does not contain new features but on explicit request one was backported from Thunder R5:

### Custom Error Codes

The custom error codes feature can be used to override the default error codes used and reported from Thunder and to provide a meaningful error message for a custom error code.
More information can be found [here](https://rdkcentral.github.io/Thunder/utils/customcodes/)

## Changes and Bug Fixes

### Fix: Offloading of Dangling Proxies Notification

When the Communication thread receives a broken pipe, the dangling notification is sent to the `CommunicationServer`. Previously this caused the communication thread to wait on the notifier lock when activation or deactivation of any plugin was in progress and the notifier lock for `IPlugin::INotification` was already taken.

The notification to `ICOMLink::INotification` subscribers is now offloaded to a Worker Pool Job in `CommunicationServer`, decoupling the communication thread from the notification delivery and eliminating the potential deadlock.

Additionally, a missing `Release()` call in the Dangling Notification path was fixed.

### Fix: Crash when Invalid Enum Passed as Argument

A crash was fixed that occurred when an invalid enum value was passed as an argument through the JSON-RPC or COM-RPC interface. The fix adds appropriate validation and graceful handling so that an out-of-range enum value no longer causes undefined behaviour.

### Fix: Handling of Truncated UTF-8 Code on Parsing Empty or Uninitialized Input

The JSON parser now correctly handles the case where a truncated UTF-8 sequence is encountered while parsing an empty, null-terminated, or uninitialized input buffer, preventing reads beyond the end of valid data.

### Fix: Only Hand Out Plugin Interfaces if the Plugin is Active

Plugin interfaces are now only returned via `QueryInterface` if the plugin is in the `ACTIVATED` state. Previously, interfaces could be handed out while the plugin was still starting up or already deactivating, potentially leading to use of an unready or partially torn-down plugin which could cause crashes.

### Fix: Starting COM after Controller is Initialized

The COM-RPC port is now opened after the Controller plugin is fully initialized. This prevents race conditions where external clients attempt to connect and use COM-RPC services before the framework is ready to handle them.

### Fix: Porting of Fixes from Thunder R5 to R4_4

A large set of stability and correctness fixes identified during Thunder 5 development were backported to R4_4. This includes:

- **Fix RAM/swap reporting to use `sysinfo` `mem_unit` scaling** — `SystemInfo` now correctly multiplies `sysinfo` memory fields by `mem_unit` so that total RAM, free RAM, total swap, and free swap are reported in bytes regardless of the kernel's internal scaling factor (PR [#2057](https://github.com/rdkcentral/Thunder/pull/2057)).
- **JSON improvements** — Multiple improvements to `JSON.h` / `JSON.cpp`:
  - Integer overflow detection during JSON number parsing (using compiler builtins on GCC/Clang, manual check on MSVC).
  - Correct handling of `null` arrays and containers — they now serialise as `null` rather than `[]` / `{}`.
  - `ArrayType` and `Container` gain explicit `Null()` and `Set()` helpers.
  - Improved `IsEscaped()` logic for correct detection of escaped quotes inside opaque strings.
  - Fixed `Container::FindField()` to do a full string comparison, preventing partial-match false positives.
  - `snprintf` used instead of `sprintf` for float serialisation.
  - `noexcept` added to several move-assignment operators.
  - `OptionalType<T>` assignment operators added to `NumberType`, `FloatType`, `Boolean`, `String`, `EnumType`, and `ArrayType`.
  - `String::RawString()` accessor added; `Variant` uses it to avoid re-quoting already-quoted strings.
  - `Variant` deserialization rewritten to correctly classify quoted strings (STRING), unquoted booleans (BOOLEAN), numbers (NUMBER / DOUBLE), objects (OBJECT), and arrays (ARRAY) without misclassifying them on reuse.
  - `VariantContainer` got various issues fixed, among others the Remove method was fixed
  - New unit tests added in `Tests/unit/core/test_jsonobject.cpp` covering key removal, partial-match safety, key-value access, round-trip correctness, and `Variant` type classification.
- **Safeguard against self-revocation of WorkerPool jobs** — `WorkerPool::Revoke()` no longer tries to wait on the external queue from the thread that owns the joined-thread ID, preventing a potential deadlock when a job revokes itself.
- **`@opaque` removed from `IController::Environment`** — The `@opaque` annotation was removed from the `environment` out-parameter, which corrects the generated proxy/stub code for this method.

### Fix: Use-After-Free SIGSEGV in UnknownProxy Release/Invalidate

A critical race condition causing a `SIGSEGV` crash in `UnknownProxy` was fixed. The crash manifested in `PluginInitializerService::PluginStarter::NotifyInitiator()` when a COM-RPC proxy (`IActivationCallback`) that had already been freed was accessed.

### Fix: Use RawString in Variant


`JSON::Variant` now uses `RawString()` instead of the quoted-aware `Value()` accessor internally, ensuring that string variants parsed from JSON are not double-quoted when serialised back. Unit tests for `JSON::Variant` round-trip behaviour were extended.

### Fix: Worker Pool Queue Size

The `WorkerPoolImplementation` queue depth was increased from `8 * THREADPOOL_COUNT` to `64 * THREADPOOL_COUNT` to reduce the likelihood of job submission failures under heavy load.

### Fix: Proper Cleanup of Service on Channel Close

An atomic `compare_exchange_strong` guard was added to the channel service cleanup path to ensure that either the ResourceMonitor thread or the WorkerPool thread (but not both) performs the service teardown, eliminating a race condition that could result in a crash.

## Additional Improvements

### Change: SocketPort Remote Closed State

A new `REMOTE_CLOSED` state bit (`0x200`) was added to `SocketPort` to distinguish a remote peer closing the connection from a local exception. `IsSuspended()` was updated accordingly, and the `Closed()` path now calls `DestroySocket()` and `ResourceMonitor::Unregister()` before clearing the `SHUTDOWN` flag to avoid operating on a destroyed socket.

### Change: SingletonProxyType Returns Reference

`SingletonProxyType<T>::Instance()` now returns `ProxyType<T>&` (a reference) instead of a value, and the `ConnectorType` default invoke server was updated to use `SingletonProxyType` so that it is properly managed as a true singleton.

### Change: IPCConnector InProgress Thread-Safety

`SocketChannelLink::InProgress()` now acquires the internal lock before checking the outbound message state, making the check thread-safe.

### Change: Communicator Connection Termination Order

In `CommunicatorServer`, the `connection->Terminate()` call is now made after the connection has been removed from the internal map and its reference decremented, preventing use of a connection that has already been released from the map.

## Breaking Changes

Thunder R4.4.5 does not introduce intentional breaking changes relative to R4.4.4. All interface signatures remain unchanged. The custom error codes feature is additive.

