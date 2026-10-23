## Why
## Why

Thunder JSON-RPC handlers traditionally return a framework error code and may place plain text in the response buffer. This does not provide a typed way to attach structured operation-specific context or selectively override the JSON-RPC code and message.

Handlers need an optional rich-error channel whose code, message, and data fields can be supplied independently. Fields omitted by a handler must retain Thunder's established framework-error mapping, while handlers that still return plain-text errors must remain compatible.

## What Changes

- Add `Core::ErrorDetails<DATA>` with optional `code`, `message`, and typed `data` fields, plus a no-data `ErrorDetails<void>` specialization.
- Add `RegisterWithError` to core and plugin-host JSON-RPC registration APIs.
- Add error-aware dispatch for no-payload, inbound-only, outbound-only, and inbound/outbound handlers.
- Initialize rich errors from Thunder's framework error before applying explicitly supplied overrides.
- Parse rich handler output safely without allowing malformed or partial output to clear Thunder defaults.
- Support context, index, instance-ID, callable, and `REALOBJECT*` registration forms.
- Preserve C++11 compatibility for legacy Thunder integrations.
- Add focused GTest coverage for method registrations and indexed routing.

## Capabilities

### New Capabilities

- `jsonrpc-rich-errors`: JSON-RPC handlers can return independently optional code, message, and typed data while retaining Thunder defaults for omitted fields.

### Modified Capabilities

- `jsonrpc-handler-registration`: typed registration gains an error-aware callback form.
- `plugin-jsonrpc-response`: plugin invocation recognizes structured error output while preserving legacy text behavior.

## Impact

- **`Source/core/Errors.h`** — defines the reusable native `ErrorDetails<DATA>` model.
- **`Source/core/JSONRPC.h`** — adds error-aware registration, dispatch, serialization, and member binding.
- **`Source/common/JSONRPC.h`** — forwards error-aware registrations to the core handler.
- **`Source/Thunder/PluginServer.h`** — recognizes and selectively overlays structured handler errors.
- **`Tests/unit/core/test_jsonrpc_richerror.cpp`** — validates partial overrides, typed data, malformed input, and indexed routing.
- **`Tests/unit/core/CMakeLists.txt`** — includes the new tests in Linux and non-Linux core test runners.

Existing `Register` and direct `Property` behavior remains unchanged. Rich-error registration is supported for methods only; properties do not support rich errors. Existing handlers that provide no rich details continue to use Thunder's current error mapping.
