# Thunder JSON-RPC Versioning Notes

This document explains how JSON-RPC versioning works in Thunder, what the code generators do and do not do, and what behavior the test framework is exercising.

The goal is to remove the common confusion between these two separate concepts:

1. Interface version metadata reported by the built-in `versions` method.
2. Handler dispatch versioning used by method names such as `Plugin.1.method`.

These are related by convention, but they are not the same mechanism in Thunder.

> **Thunder Version Compatibility Notice**
>
> Interface version metadata (concept 1 above) is fully supported across all Thunder versions.
>
> Handler dispatch versioning via `CreateHandler`/`GetHandler` (concept 2 above) is **not recommended in Thunder v5**.
> The Thunder source code explicitly marks these APIs as *"not preferred"* and *"for backwards compatibility only"* (present since R4.0, continuing through all R5.x releases and master).
> New plugins targeting Thunder v5 should not rely on `CreateHandler`/`GetHandler` for versioned dispatch.
> The pattern is retained only to avoid breaking existing deployments in the field.

## 1. Two Separate Version Concepts

### 1.1 Interface version metadata

The code generators support an annotation like:

```cpp
// @json 1.0.0
```

This version is used to describe the JSON-RPC interface version and is surfaced through the built-in `versions` method, for example:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "Plugin.versions",
  "params": {}
}
```

The generated registration code calls `RegisterVersion(...)` to record metadata such as:

- interface name
- major
- minor
- patch

This metadata is what Thunder returns from `Plugin.versions`.

Important:

- `@json x.y.z` does **not** automatically create a handler for `Plugin.x.method`.
- `RegisterVersion(...)` does **not** create or select runtime versioned handlers.

### 1.2 Handler dispatch versioning

> **Deprecated in Thunder v5.**
> The `CreateHandler`/`GetHandler` APIs are marked as *"not preferred"* in the Thunder source code and kept only for backwards compatibility. Do not use them in new plugins targeting Thunder v5.

The runtime supports versioned JSON-RPC method designators such as:

```text
Plugin.1.method
Plugin.2.method
```

This dispatch behavior is implemented by `PluginHost::JSONRPC` at runtime using handler objects and handler lists.

This is **not** generated automatically from `@json x.y.z`.

If you want to test or implement `Plugin.1.method` and `Plugin.2.method`, you need handwritten runtime setup, typically using:

- `CreateHandler({version})`
- `CreateHandler({version}, sourceHandler)`
- `GetHandler(version)`
- explicit `Register(...)` calls on the returned handler

This pattern is only valid on Thunder v4.x. On Thunder v5.x it exists solely for backwards compatibility with existing deployments.


## 2. How Thunder Parses a JSON-RPC Method Name

Thunder parses JSON-RPC method designators using the format:

```text
[Callsign.][version.][prefix[#instanceid]::]method[@index]
```

Relevant implementation:

- `Thunder/Source/core/JSONRPC.h`
- `Thunder/Source/plugins/JSONRPC.h`

For handler dispatch, Thunder extracts only the numeric segment immediately before the method name.

Examples:

- `TestPlugin.1.echoUInt8` -> explicit major version `1`
- `TestPlugin.2.echoUInt8` -> explicit major version `2`
- `TestPlugin.echoUInt8` -> no explicit version

Important:

- handler selection uses only the **major** version number
- minor and patch are not used for runtime dispatch
- semver metadata and runtime handler version selection are separate systems

## 3. How Thunder Chooses the Handler

Thunder stores JSON-RPC handlers in a list.

The selection rules are:

### 3.1 If the call specifies a version

Thunder scans the handler list and selects the first handler whose `HasVersionSupport(version)` returns true.

Examples:

- `Plugin.1.method` -> first handler supporting `1`
- `Plugin.2.method` -> first handler supporting `2`

### 3.2 If the call does not specify a version

Thunder returns the **first handler in the list**.

This is the most important rule to understand.

Unversioned call behavior is **not**:

- always version 1
- always highest version
- always latest registered version

It is simply:

```text
first handler in the list
```

That means the result depends on how the plugin constructed and ordered its handlers.

## 4. Default Behavior When No Version Is Present

By default, `PluginHost::JSONRPC()` constructs the base handler with support for version `1`.

That means many plugins behave as if:

```text
Plugin.method == Plugin.1.method
```

But this is not a universal rule.

It only happens because the default front handler typically supports `{1}`.

If a plugin is constructed differently, for example with a base handler supporting `{2}`, then:

```text
Plugin.method == Plugin.2.method
```

So the correct statement is:

> If no version is specified, Thunder dispatches to the first handler, whatever version(s) that handler supports.

## 5. Unknown Version Behavior

If the call specifies a major version and Thunder cannot find any handler supporting it, the call is rejected.

Typical outcome:

- `ERROR_INVALID_SIGNATURE`

Examples:

- `Plugin.99.method` -> rejected if no handler supports `99`
- no handler is invoked
- no event is emitted from that call path

## 6. What `RegisterVersion()` Actually Does

`RegisterVersion()` only records interface metadata for the built-in `versions` method. This mechanism is **fully supported in Thunder v5** and is the recommended way to expose interface version information.

It does **not**:

- create a handler
- attach methods to a handler version
- influence `Plugin.1.method` dispatch
- clone handlers
- select a default runtime version

This means a plugin can accidentally get out of sync:

- advertise a version in `versions` but not dispatch it
- dispatch a version but not advertise it

Thunder does not enforce those two systems to stay aligned.

## 7. What the Code Generators Actually Do

Given an interface with `@json x.y.z`, the generated code typically does two things:

1. Emits a `Version` namespace with `Major`, `Minor`, and `Patch`.
2. Calls `RegisterVersion(...)` when the generated `Register(...)` helper runs.

The generated code also registers methods and properties using the normal `PluginHost::JSONRPC::Register(...)` and `Property(...)` APIs.

Important boundary:

- generated code registers methods on the base/front handler
- generated code reports interface semver metadata
- generated code does **not** automatically create alternate runtime handlers for `.1.` or `.2.` designators

So this statement is false:

> The JSON-RPC handler version can be autogenerated from `@json x.y.z`.

The correct statement is:

> Interface version metadata can be autogenerated from `@json x.y.z`, but handler version dispatch must be wired manually if multiple runtime handler versions are needed.

## 8. How ThunderNanoServices Does It

> **Note:** The pattern described in this section uses `CreateHandler`/`GetHandler`, which are marked deprecated (not preferred) in Thunder v5. This example remains in the codebase for reference and backwards-compatibility testing only. New plugins on Thunder v5 should not adopt this pattern.

The example in `ThunderNanoServices/examples/JSONRPC/Plugin/JSONRPCPlugin.cpp` uses the handwritten runtime pattern.

Pattern:

1. Register the default handler methods on the base JSONRPC object.
2. Create a second handler for a specific major version using `CreateHandler(...)`.
3. Clone the base handler into the new one if needed.
4. Override only the methods that differ for that version.

This was the canonical "changed interface per major version" model in Thunder v4.

Conceptually:

```cpp
Core::JSONRPC::Handler& legacy = JSONRPC::CreateHandler({1}, *this);
legacy.Register("clueless", ...); // override only this method on v1
```

That means:

- version 1 and version 2 can share most methods
- only changed methods need to be overridden
- the runtime chooses the right handler by parsing `Plugin.1.method` or `Plugin.2.method`

## 9. Can One Interface Header Support Multiple Runtime Versions?

### 9.1 Same behavior across multiple versions

Yes.

One handler can support multiple major versions if the plugin constructs that handler with a version list like `{1, 2}`.

Then the same method table serves:

- `Plugin.1.method`
- `Plugin.2.method`

### 9.2 Different behavior across multiple versions

Also yes, but not automatically through one generated `@json` annotation.

If behavior differs between versions, you need handwritten runtime logic:

- create a new handler for the alternate version
- optionally clone from the base handler
- override the methods that changed

So one interface concept can serve multiple runtime versions, but the split is done by handler wiring, not by generator magic.

## 10. Can the Same Autogenerated Code Be Reused?

### 10.1 Reuse for identical behavior

Yes.

If one handler supports multiple major versions and the behavior is identical, the generated registration code can be reused without modification because everything targets the same handler.

### 10.2 Reuse for mostly identical behavior with a few overrides

Yes, partially.

A common pattern is:

1. Let generated code register the base handler.
2. Clone that handler to a new version using `CreateHandler({2}, *base)`.
3. Override only the methods that changed.

So the autogenerated code is reused as the baseline registration, but the version-specific override logic is handwritten.

### 10.3 Reuse to auto-create two handler versions directly from one generated interface

No.

That is not what `@json x.y.z` does.

## 11. Built-in Methods and Versioning

Thunder has built-in methods such as:

- `versions`
- `exists`
- `register`
- `unregister`

These also follow handler dispatch rules:

- `Plugin.exists` uses the first handler
- `Plugin.2.exists` uses the handler for version 2

This means `exists` can produce different answers depending on the handler selected.

Example:

- `Plugin.exists` may say a v2-only method does not exist if the first handler is v1
- `Plugin.2.exists` may say the same method does exist

`versions` is different:

- it reports metadata collected through `RegisterVersion(...)`
- it does not prove that handler versions are available for dispatch

## 12. Notifications and Versioning

Notifications also need careful interpretation.

Important points:

1. The method that triggers the notification can be versioned.
2. The handler chosen for that method call determines which implementation runs.
3. That implementation may emit different event payloads depending on the versioned handler.

So versioned behavior for notifications usually means:

- different handler selected
- same event name
- different payload or logic depending on handler version

This is exactly how the ThunderNanoServices example uses `clock`:

- modern version sends one payload format
- legacy version sends another payload format

The version influences the handler path that emits the notification, not a separate autogenerated event-versioning system.

## 13. Scenarios to Understand Clearly

### Scenario A: Only one base handler exists

Base handler supports `{1}`.

Behavior:

- `Plugin.method` -> v1 handler
- `Plugin.1.method` -> v1 handler
- `Plugin.2.method` -> rejected

### Scenario B: One handler supports multiple versions

Base handler supports `{1, 2}`.

Behavior:

- `Plugin.method` -> front handler
- `Plugin.1.method` -> same handler
- `Plugin.2.method` -> same handler
- behavior is identical for both versions unless handwritten logic branches internally

### Scenario C: Two handlers exist, first is v1, second is v2

Behavior:

- `Plugin.method` -> v1 handler
- `Plugin.1.method` -> v1 handler
- `Plugin.2.method` -> v2 handler

### Scenario D: Two handlers exist, first is v2, second is v1

Behavior:

- `Plugin.method` -> v2 handler
- `Plugin.1.method` -> v1 handler
- `Plugin.2.method` -> v2 handler

This is why the rule "missing version means version 1" is wrong.

### Scenario E: Method exists only on v2

Behavior:

- `Plugin.2.method` -> works
- `Plugin.1.method` -> rejected or unknown
- `Plugin.method` -> depends on whether the first handler is v1 or v2

### Scenario F: Event-triggering method differs by version

Behavior:

- `Plugin.1.emitEvent` -> may emit payload A
- `Plugin.2.emitEvent` -> may emit payload B
- `Plugin.emitEvent` -> payload from the first handler

## 14. What the Tests Cover

The current JSON-RPC versioning test is in:

- `ThunderTools/tests/FunctionalTests/jsonrpc/tests/TestVersioningJsonRpc.cpp`

The test is self-contained and does **not** depend on generated interface headers.
It creates its own plugin classes inline, so it can run in isolation from the rest
of the JSON-RPC functional test suite.

Supporting infrastructure (shared with other JSON-RPC tests):

- `ThunderTools/tests/FunctionalTests/jsonrpc/JsonRpcServer.h`
- `ThunderTools/tests/FunctionalTests/jsonrpc/JsonRpcTestHarness.h`

Enable at configure time with `-DTEST_VERSIONING_JSONRPC=ON` (default: ON).

The test setup uses one generated interface plus handwritten versioned handler wiring.

It covers:

- explicit version 1 method dispatch
- explicit version 2 method dispatch
- inherited methods on the cloned v2 handler
- a method existing only on v2
- unversioned dispatch to the first handler
- unknown version rejection
- `exists` behavior with and without explicit version
- `versions` behavior and its separation from handler versioning
- version-dependent notifications and unversioned notification fallback

This gives a practical, runtime-level view of how Thunder actually behaves.

## 15. Practical Rules To Remember

1. `@json x.y.z` is interface metadata, not handler dispatch setup. Supported in all Thunder versions.
2. `RegisterVersion(...)` feeds `Plugin.versions`, not `Plugin.1.method` routing. Supported in all Thunder versions.
3. `Plugin.1.method` dispatch is a runtime `PluginHost::JSONRPC` handler feature using `CreateHandler`/`GetHandler`. **Deprecated in Thunder v5 (kept for backwards compatibility only).**
4. If no version is specified, Thunder uses the first handler in the list.
5. Unknown explicit versions are rejected.
6. Multiple runtime versions require handwritten setup if different handlers are needed — but this approach is not recommended on Thunder v5.
7. Generated code can be reused as a base, but handler versioning logic is still manual.
8. `versions` can report interface metadata that is not equivalent to runtime handler coverage.

## 16. Recommended Mental Model

Use this model when reasoning about Thunder JSON-RPC versioning:

- `@json 1.0.0` tells the world what the interface version is. **Use this in all Thunder versions.**
- `RegisterVersion(...)` records that information for discovery. **Use this in all Thunder versions.**
- `CreateHandler(...)` and `GetHandler(...)` decide runtime method routing behavior. **Deprecated in Thunder v5 — avoid in new code.**
- `Register(...)` on a specific handler decides what that handler actually exposes.
- missing version means "first handler", not "version 1 by rule".

That model matches both the Thunder runtime implementation and the handwritten versioning example in ThunderNanoServices.

### Thunder v4 vs Thunder v5 Summary

| Capability | Thunder v4 | Thunder v5 |
|---|---|---|
| `@json x.y.z` interface metadata | Supported | Supported |
| `RegisterVersion(...)` / `Plugin.versions` | Supported | Supported |
| `CreateHandler(...)` / `GetHandler(...)` dispatch | Supported (discouraged) | Not recommended (backwards compat only) |
| `Plugin.1.method` versioned dispatch | Supported | Backwards compat only |

The handler dispatch versioning APIs have been marked *"not preferred"* in the Thunder source code (`Source/plugins/JSONRPC.h`, later moved to `Source/common/JSONRPC.h`) since R4.0, with the explicit comment: *"methods are here for backwards compatibility with functionality out there in the field"*. Thunder v5 continues to include them but they are not part of the recommended plugin development pattern.