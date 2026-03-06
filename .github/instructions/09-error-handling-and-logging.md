---
applyTo: '**'
---

# 09 — Error Handling and Logging

> Thunder is built with `-fno-exceptions`. Every rule in this file is a hard constraint, not a guideline.

## No Exceptions

Thunder is compiled with **`-fno-exceptions`** on all supported platforms. This is non-negotiable.

- **Never** use `throw` — it will compile but cause `std::terminate()` at runtime (or a linker error in some toolchains).
- **Never** use `try` or `catch` blocks.
- **Never** include headers that throw (e.g. `std::vector::at()` with range checking enabled).
- Use `ASSERT()` for programmer-error invariants that should never trigger in correct code.
- Use `Core::hresult` return values for all recoverable error conditions.

## `Core::hresult` Return Convention

Every COM interface method must return `Core::hresult`. This is a `uint32_t`:

| Value | Meaning |
|-------|---------|
| `Core::ERROR_NONE` (0) | Success |
| `Core::ERROR_UNAVAILABLE` | Resource or plugin not available |
| `Core::ERROR_ILLEGAL_STATE` | Operation invalid in current state |
| `Core::ERROR_BAD_REQUEST` | Invalid parameters |
| `Core::ERROR_TIMEDOUT` | Operation timed out (also `Core::ERROR_ASYNC_ABORTED` in some contexts) |
| `Core::ERROR_NOT_SUPPORTED` | Feature not supported on this platform |
| `Core::ERROR_GENERAL` | Unspecified general failure |
| `Core::ERROR_ALREADY_CONNECTED` | Connection already exists |
| `Core::ERROR_ASYNC_ABORTED` | Async operation was aborted |
| `(result & COM_ERROR) != 0` | Transport-level COM-RPC failure (not a plugin error) |

**Always return `Core::ERROR_NONE` on success** — never return 0 as a literal (use the named constant).

**Translating to human-readable text**:
```cpp
TRACE_L1("Operation failed: %s", Core::ErrorToString(result));
```

## `ASSERT` — Developer Invariants

`ASSERT(condition)` checks a programmer invariant. It is compiled **out** in release (`MinSizeRel`) builds.

```cpp
ASSERT(parent != nullptr);         // must be true by contract — development guard
ASSERT(data->Label() == InvokeMessage::Id());  // precondition for static_cast

// ❌ Wrong — never use <cassert> assert()
assert(parent != nullptr);

// ❌ Wrong — never use ASSERT for recoverable runtime conditions
ASSERT(file.Open());  // if Open can fail in production, check the return value instead
```

## `Initialize()` Error Reporting

In `IPlugin::Initialize()`, errors are reported by returning a non-empty string:

```cpp
const string Initialize(PluginHost::IShell* service) override
{
    _iface = service->QueryInterfaceByCallsign<Exchange::INetworkControl>(_T("NetworkControl"));
    if (_iface == nullptr) {
        return _T("Required plugin NetworkControl is not available");
    }
    // ...
    return {};  // empty string = success
}
```

- An empty string (`{}` or `""` or `string()`) signals success.
- A non-empty string signals failure — the plugin stays in `DEACTIVATED` state.
- The returned string is logged and surfaced via Controller status.
- Do **not** return `Core::ERROR_*` codes from `Initialize()` — it returns `string`, not `hresult`.

## Trace Categories

Thunder has a multi-level, per-module trace system. All trace is **compiled out** in `MinSizeRel` production builds.

| Macro | Level | Use for |
|-------|-------|--------|
| `TRACE_L1(fmt, ...)` | Level 1 (most verbose) | Developer diagnostics, frequent-path tracing |
| `TRACE_L2(fmt, ...)` | Level 2 | Module-level state changes |
| `TRACE_L3(fmt, ...)` | Level 3 | Less frequent diagnostics |
| `TRACE(category, (fmt, ...))` | Category-filtered | Named category trace (e.g. `TRACE(Trace::Information, (...))`) |

```cpp
TRACE_L1("Plugin activating: %s", callsign.c_str());
TRACE(Trace::Error, (_T("COM-RPC error %d on interface %u"), result, interfaceId));
```

Trace is routed via the `MessageControl` plugin. Enabling at runtime:
```json
{
  "defaultmessagingcategories": [
    { "category": "Trace", "module": "MyPlugin", "enabled": true }
  ]
}
```

## SYSLOG — Production Logging

`SYSLOG` is always present in production builds (not compiled out):

```cpp
SYSLOG(Logging::Notification, (_T("Plugin %s activated"), callsign.c_str()));
SYSLOG(Logging::Warning, (_T("Failed to acquire interface: %s"), Core::ErrorToString(result)));
SYSLOG(Logging::Fatal, (_T("Critical subsystem failure — shutting down")));
```

| Level | Use for |
|-------|--------|
| `Logging::Notification` | Normal significant events (plugin activated, subsystem change) |
| `Logging::Warning` | Unexpected but recoverable conditions |
| `Logging::Fatal` | Unrecoverable failures |

### Log Routing: Foreground vs Background Mode

How `SYSLOG` and critical process-lifecycle messages are routed depends on whether Thunder was started with the `-b` flag:

| Condition | Log destination |
|-----------|----------------|
| **Foreground** (no `-b`) | `stdout` / `stderr` via `fprintf` |
| **Background** (`-b`) | `syslog()` (POSIX syslog, typically captured by `journald` or `syslogd`) |

This routing is implemented in `PluginHost.cpp`: the static `_background` flag (set by the `-b` CLI option) is checked at every lifecycle event (startup, signal, crash, shutdown) to select the appropriate output channel.

Additionally, `Messaging::MessageUnit::Open()` receives the `_background` flag and uses it to configure the messaging subsystem (TRACE / WARNING_REPORTING output). The `-f` / `-F` flags force `TRACE` and `WARNING_REPORTING` messages to also flush to syslog/console (non-abbreviated / abbreviated respectively), regardless of mode.

> **Developer note on macOS**: macOS uses `launchd` rather than `syslogd`. When running Thunder in foreground mode (the typical dev workflow on macOS with `-f`), all output goes to the terminal. Background mode output goes to the unified log system (`log stream --predicate 'process == "Thunder"'`). The `-f` flag is the most useful option for development — it forces trace output to the console regardless of background/foreground setting.

Do **not** use `printf`, `fprintf`, `std::cout`, or `syslog()` directly — always use `SYSLOG` or `TRACE`.

## Foreground vs Background — Log Routing

Thunder routes operational messages differently depending on how it is launched:

| Mode | Launch flag | Lifecycle messages | Trace / SYSLOG messages |
|------|-------------|-------------------|------------------------|
| **Foreground** (dev/macOS) | none (`-f` / `-F` to also flush) | `fprintf(stdout/stderr)` | Routed by `MessageUnit` to in-process subscribers; optionally also flushed to console if `-f`/`-F` flag is set |
| **Background** (production daemon) | `-b` | `syslog(LOG_NOTICE/LOG_ERR, ...)` | Routed by `MessageUnit` to in-process subscribers; optionally also forwarded to syslog if `-f`/`-F` flag is set |

**How it works** (from `Source/Thunder/PluginHost.cpp`):
- `openlog()` is always called at startup so the syslog handle is available in both modes.
- `MessageUnit::Instance().Open(volatilePath, config, background, flushMode)` is called once during startup. The `background` bool and `flushMode` (OFF / FLUSH / FLUSH_ABBREVIATED) control whether trace/SYSLOG messages are additionally written to the syslog/console sink as they are emitted.
- In **foreground** mode the main thread runs an interactive key-press loop (not a WorkerPool worker), and lifecycle messages go to `stdout`/`stderr`.
- In **background** mode the main thread calls `Core::WorkerPool::Instance().Join()` and becomes an extra WorkerPool thread. Lifecycle messages go to `syslog`.
- The `-f` flag sets `flushMode = FLUSH` — trace messages are also written to syslog/console in full.
- The `-F` flag sets `flushMode = FLUSH_ABBREVIATED` — same but abbreviated output.

> **Developer note**: In foreground mode you see trace output on the terminal immediately. In background mode all trace goes through `MessageUnit` and is only visible to the `MessageControl` plugin subscriber (or via syslog if `-f`/`-F` is set). If a bug only reproduces in background mode, the difference may be the extra WorkerPool thread (see `08-threading-and-synchronization.md`) or the absence of a console flush path.

## Warning Reporting

Conditional at build time (requires `-DWARNING_REPORTING=ON`). Used for performance anomaly detection:

```cpp
// Report if an operation exceeds expected duration
WARNING_REPORTING_DURATION(Core::Time::Now(), "MyOperation", 100 /* ms threshold */);

// Report if a buffer access is out of expected range
WARNING_REPORTING_OUTOFBOUNDS(index, maxIndex, "BufferAccess");
```

These are compiled out if `WARNING_REPORTING=OFF` (the default in production builds).

## PostMortem Dumps

On abnormal exit, Thunder writes a post-mortem dump to `postmortempath`. Triggered for exit reasons listed in `exitreasons` config (e.g. `WATCHDOG`, `FAILURE`).

Dump sections are added in `Source/Thunder/PostMortem.cpp` using the `Dump*()` function pattern. Never add crash diagnostics via ad-hoc `fprintf` or `syslog` calls outside this file.

## COM-RPC Error vs. Plugin Error

See `06-comrpc-fundamentals.md` for the `COM_ERROR` bit test pattern. In summary:
```cpp
Core::hresult result = remoteInterface->SomeMethod(param);
if (result & COM_ERROR) {
    // IPC transport failed (timeout, process died, marshal error)
    // Do not interpret as a plugin logic error
} else if (result != Core::ERROR_NONE) {
    // Plugin returned a logical error — handle per-interface semantics
}
```

## Error Handling in Deinitialize

`Deinitialize()` has no return value — all errors must be handled internally and logged:

```cpp
void Deinitialize(PluginHost::IShell* service) override
{
    Core::hresult result = _iface->Shutdown();
    if (result != Core::ERROR_NONE) {
        SYSLOG(Logging::Warning, (_T("Shutdown returned: %s"), Core::ErrorToString(result)));
        // Log and continue — Deinitialize must always complete
    }
    _iface->Release();
    _iface = nullptr;
}
```

`Deinitialize()` must **always complete** — never return early on error. Incomplete deinitialization leaves the plugin in an inconsistent state that prevents re-activation and clean shutdown.
