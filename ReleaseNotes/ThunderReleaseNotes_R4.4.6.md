# Thunder Release Notes R4.4.6

## Introduction

This document describes the changes introduced in Thunder R4.4.6 (compared to the R4.4.5 release).

The changes described here are present on the [R4_4 branch](https://github.com/rdkcentral/Thunder/tree/R4_4) and represent all commits since the [R4.4.5 tag](https://github.com/rdkcentral/Thunder/compare/R4.4.5...R4_4).

ThunderTools ships its own set of changes in this release. ThunderTools R4.4.5 introduced no changes relative to R4.4.4 and therefore no R4.4.5 tag was created for ThunderTools. As a result, all ThunderTools changes described below are new in this release and represent the delta relative to R4.4.5. All ThunderTools changes in this release are backports of JsonGenerator and ProxyStub Generator improvements from ThunderTools R5.x.

# Thunder

## Changes and Bug Fixes

### Fix: Infinite Loop in JSON Stream on Garbage Input

Fixed two related bugs in `StreamJSONType::ReceiveData` that caused an infinite spin when a TCP stream carried non-JSON garbage bytes followed by a valid JSON message.

The first bug was an off-by-one in `IsNullValue()`: the guard fired one iteration too early, returning `UNKNOWN` without consuming the last byte of the input slice, causing the same byte to be presented again on the next call. The second bug was a missing no-progress guard in the receive loop: if `Deserialize()` returned 0 the loop spun indefinitely. The return value is now checked and the loop exits when no bytes are consumed. Error logging for JSON parse failures on a stream was also added (PR [#2135](https://github.com/rdkcentral/Thunder/pull/2135)).

### Fix: Infinite Loop on Thunder Exit

Fixed an infinite loop that could occur during the Thunder framework shutdown sequence (PR [#2145](https://github.com/rdkcentral/Thunder/pull/2145)).

### Fix: WebSocket Consecutive Payloads Exceeding 65 KB

Fixed a WebSocket framing bug where sending two or more consecutive messages larger than 65 KB caused incorrect payload handling. The zero-length header-size edge case is also covered by this fix (PR [#2124](https://github.com/rdkcentral/Thunder/pull/2124)).

### Fix: Thread Safety for Core::SystemInfo Environment Access

Added a mutex lock around `Core::SystemInfo::GetEnvironment()` and `SetEnvironment()` to prevent data races when these methods are called concurrently from multiple threads. The fix applies to both Linux and Windows builds (PR [#2122](https://github.com/rdkcentral/Thunder/pull/2122)).

## Additional Improvements

### Change: Messaging Subsystem Backport from R5

Backported messaging improvements from Thunder R5 to R4_4, including telemetry support, improved message handling, opt-in stdout/stderr redirection (disabled by default), and a compatibility API to retrieve message controls for all modules. These changes preserve compatibility with existing integrations (PR [#2114](https://github.com/rdkcentral/Thunder/pull/2114)).

### Change: ThunderExtensions Initial 4.4 Backport

The ThunderExtensions framework has been brought onto the R4_4 branch. This initial backport also includes a fix to extension config generation (PR [#2107](https://github.com/rdkcentral/Thunder/pull/2107)).

### Change: Thunder Core Updated for Generator Toolchain Compatibility

Thunder core has been updated to work correctly with the upgraded JsonGenerator and ProxyStubGenerator now available in ThunderTools R4_4 (backported from R5.x). Changes include support for indexed notifications, backward-compatible status listener code, and additional COM-layer accessors (PR [#2117](https://github.com/rdkcentral/Thunder/pull/2117)).

### Change: CommChannel ChannelMap Moved to commchannel.cpp

`ChannelMap` has been migrated from the header into `commchannel.cpp` and the `LinkType` anchor to WebSocket has been backported from R5, giving cleaner separation of interface and implementation (PR [#2147](https://github.com/rdkcentral/Thunder/pull/2147)).

### Change: Persist Gated on Debug or Persist Flag

Message persistence is now conditional on the debug or persist flag being explicitly enabled. This prevents unintentional message persistence in production builds where neither flag is set (PR [#2143](https://github.com/rdkcentral/Thunder/pull/2143)).

### Change: Syslog Entries Added to Plugin Initialization Path

Syslog statements have been added to the plugin initialization and activation area to improve diagnostics when a plugin fails to start or takes longer than expected to activate (PR [#2144](https://github.com/rdkcentral/Thunder/pull/2144)).

### Change: JSON FromString Trailing Whitespace Fix

Fixed an edge case in `JSON::FromString()` where trailing whitespace at the end of the input string caused a spurious parse failure.

## Breaking Changes

Thunder R4.4.6 does not introduce intentional breaking changes relative to R4.4.5. All interface signatures remain unchanged.

---

# ThunderTools

## Introduction

The ThunderTools changes in R4.4.6 are all backports of JsonGenerator and ProxyStub Generator improvements from ThunderTools R5.x. The changes refine the code generators used to produce JSON-RPC and COM-RPC glue code from Thunder interface definitions.

## Features

### R5.x Generators Backport

Selected features of the ProxyStub Generator and JsonGenerator have been backported from ThunderTools R5.x (PR [#251](https://github.com/rdkcentral/ThunderTools/pull/251)). The following new capabilities are available:

- `std::vector<T>` support
- C-style fixed array support
- `Core::OptionalType<T>` support
- `Core::Time` support
- Support for indexed notifications with `@index` (dot-delimited)
- Support for legacy indexed notifications with `@index:deprecated` (`@`-delimited)
- Custom naming case conventions
- Non-`@optional` parameters are runtime-checked; a trace is printed if absent on a call
- `@default` tag to initialise `@optional` parameters
- New `@wrapped` tag to enclose a single result parameter in an additional object
- New buffer encodings: `@encode:mac`, `@encode:hex`, `@encode:array`
- `@encode:base64` added as an alias for `@base64`; `@encode:bitmask` as an alias for `@bitmask`
- `@restrict:nonempty` for strings
- JSON container classes now include move constructors and assignment operators
- Event callers now always have a default send-if lambda parameter
- `@insert` / `@stubgen:include` now imports PODs in JSON-RPC
- Documentation now explicitly notes parameters as mandatory or optional
- Documentation no longer suggests `.1` version in examples
- Warning emitted if a method name collides with the built-in methods (`register`, `unregister`, `exists`)
- Warning emitted if a notification has a return value

## Breaking Changes

ThunderTools R4.4.6 does not introduce intentional breaking changes relative to R4.4.5.
