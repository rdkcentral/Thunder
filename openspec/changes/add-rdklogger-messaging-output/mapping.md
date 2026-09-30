# Thunder to RDKLogger Mapping

## Purpose

This document defines how Thunder Messaging sends messages to RDKLogger when the optional external output route is enabled. It is intended for platform configuration, operational review, and cross-team agreement.

## Category Name

Thunder sends every message under this RDKLogger category format:

```
LOG.RDK.THUNDER.<MODULE_NAME>
```

`MODULE_NAME` is Thunder's existing module identifier. Thunder does not alter it: it does not normalize, truncate, validate, or resolve collisions.

Examples:

| Thunder module | RDKLogger category |
| --- | --- |
| `Thunder` | `LOG.RDK.THUNDER.Thunder` |
| `WebKitBrowser` | `LOG.RDK.THUNDER.WebKitBrowser` |
| `DeviceInfo` | `LOG.RDK.THUNDER.DeviceInfo` |

## Level Mapping

| Thunder message | RDKLogger level |
| --- | --- |
| Assertion or fatal-like event | `FATAL` |
| Warning report | `WARN` |
| Trace: Fatal or Crash | `FATAL` |
| Trace: Error | `ERROR` |
| Trace: Warning or Warn | `WARN` |
| Trace: Notice | `NOTICE` |
| Other trace | `TRACE` |
| Telemetry | `NOTICE` |
| Operational stream | `TRACE` |
| Log: Crash or Fatal | `FATAL` |
| Log: Error | `ERROR` |
| Log: Warning or Warn | `WARN` |
| Log: Notice | `NOTICE` |
| Other log | `INFO` |

## Payload Mapping

Thunder sends the message payload unchanged for explicitly classified levels, such as `ERROR`, `WARN`, `NOTICE`, and `FATAL`.

For default-level logging and tracing, Thunder adds the non-empty Thunder category to retain useful context:

```
<CATEGORY>: <PAYLOAD>
```

Examples:

| Thunder input | RDKLogger payload |
| --- | --- |
| Logging category `Information`, payload `Connected` | `Information: Connected` |
| Trace category `Timing`, payload `Frame rendered` | `Timing: Frame rendered` |
| Error logging payload `Connection failed` | `Connection failed` |

Thunder metadata is otherwise used only to select the RDKLogger category and level.

## Delivery Rules

- RDKLogger is an independent output destination; enabling it does not replace MessageControl or Thunder DirectOutput.
- RDKLogger enablement is evaluated per Thunder module and mapped level.
- Telemetry is not sent to RDKLogger when it is also routed to the Thunder handler, because MessageControl forwards that handler route to T2.
- Telemetry routed externally without the handler uses the `NOTICE` mapping above.
- If RDKLogger is unavailable or rejects a module/level, existing Thunder local routes continue unchanged.

## Ownership

Thunder owns the mapping and calls the RDKLogger API. The RDKLogger/platform team owns registration and enablement of `LOG.RDK.THUNDER.<MODULE_NAME>` categories in Log4C or the target-equivalent configuration. Thunder does not generate, install, or maintain that platform configuration.
