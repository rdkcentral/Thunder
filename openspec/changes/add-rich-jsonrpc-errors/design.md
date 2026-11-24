## Context

A native JSON-RPC handler returns a Thunder framework error. `Core::JSONRPC::Message::Info::SetError` maps that value to the stable JSON-RPC code and message exposed to clients. Before this change, the handler response buffer could provide additional plain text but could not represent independently optional code, message, and structured data.

The new registration path accepts an additional generated or hand-written JSON error container with `Code`, `Message`, and `Data` elements. It remains separate from the normal registration path to avoid changing existing callback signatures.

## Goals / Non-Goals

**Goals:**
- Allow code, message, and typed data to be supplied independently on failure.
- Preserve Thunder's code and message whenever the corresponding detail is omitted.
- Preserve normal result serialization on success.
- Support the typed handler shapes and routing metadata already supported by JSON-RPC registration.
- Preserve legacy plain-text errors.
- Keep new code compatible with C++11 for backports.

**Non-Goals:**
- Change Thunder's framework-error mapping.
- Change direct `Handler::Property` semantics.
- Add rich-error support to properties.
- Require rich details from existing handlers.
- Refactor the regular `InternalRegister` overload family as part of this change.

## Decisions

### Decision 1 — Use an optional native error model

`Core::ErrorDetails<DATA>` contains optional `code`, `message`, and `data` members. `Core::ErrorDetails<void>` contains only optional `code` and `message` members and can be written as `ErrorDetails<>`. The enclosing value can also be optional at an interface boundary. This distinguishes an omitted field from a deliberately supplied value, including an explicitly supplied empty message.

Error serialization uses C++11 SFINAE to include `Data` only when the JSON error container exposes that member. Code/message-only error containers therefore do not advertise or emit a `data` field.

### Decision 2 — Add a separate RegisterWithError API

`RegisterWithError<INBOUND, OUTBOUND, ERROR>` preserves source compatibility with `Register` and makes the extra callback argument explicit.

Eight routing overloads cover combinations of context, index, and instance ID. Compile-time input/output tags select four implementation forms:

- no inbound and no outbound;
- inbound only;
- outbound only;
- inbound and outbound.

This keeps the error-specific implementation compact without changing the established regular registration code.

### Decision 3 — Initialize defaults before applying overrides

On failure, `SerializeError` first calls `Message::Info::SetError(frameworkError)`. It then replaces code, message, or data only when that element is set in the supplied error container.

This guarantees:
- data-only details retain Thunder's code and message;
- code-only details retain Thunder's message;
- message-only details retain Thunder's code.

If the detail container is not set, no structured output is produced and the existing plugin response path supplies the framework error.

### Decision 4 — Ignore error details on success

When the implementation returns `Core::ERROR_NONE`, normal outbound serialization is used and error details are ignored. When it returns an error, a normal outbound value is not serialized as a successful result.

### Decision 5 — Parse into a temporary response object

`PluginServer::Invoke` initializes `response->Error` from the framework result, then parses handler output into a temporary `Message::Info`.

If parsing succeeds and the temporary container is set, each present field is copied individually. Parsing directly into `response->Error` was rejected because malformed or partial input could clear defaults before validity was known.

Malformed output and valid JSON containing no recognized error element remain legacy error text.

### Decision 6 — Select traits for directly passed lambdas

The default `func_traits<METHOD>` reports unsupported callable classes with `Arguments == -1` and `nullptr_t` argument types. A callable-traits selector uses the existing `lambda_traits<METHOD>` in that case, allowing non-generic lambdas to expose their `Context`, index, and instance-ID arguments without an explicit `std::function` signature.

The method type is normalized with `std::decay` before consulting `func_traits`. This converts a free function passed by name into its function-pointer type, preserving registrations whose `METHOD` template argument was deduced as a function type.

Call-operator inspection is substitution-safe and is used only when the callable has one unambiguous, addressable `operator()`. Opaque callable wrappers such as `std::bind`, and generic lambdas with templated call operators, retain the default function traits and therefore preserve no-prefix dispatch. An explicit `std::function` signature remains available when routing metadata must be inferred for such wrappers.

Function pointers, member pointers, and `std::function` continue to use their existing `func_traits` specializations.

Index detection is relative to the optional `instance_id_follows_t` marker. The string immediately following that marker is the instance ID, not an index. For instance-aware handlers, only the next string slot is considered an index.

### Decision 7 — Bind member methods with a C++11 function object

The `REALOBJECT*` overload uses `BoundMethod`, a variadic function object that stores the member-function pointer and target object. It forwards arguments to `(object->*method)(...)` and reuses the same error-aware implementation helpers.

A C++14 generic lambda was rejected because the change must be backportable to C++11 Thunder integrations.

## Risks / Trade-offs

- Error containers are expected to expose `Code`, `Message`, `Data`, and `IsSet()`; misuse produces a compile-time failure.
- Generic lambdas require an explicit non-generic callable signature.
- Rich error output shares the existing handler response string with legacy text. Temporary parsing and recognized-field checks prevent accidental replacement of defaults.
- The `REALOBJECT*` path is covered for all input/output forms and indexed rich-error routing.

## Validation

Focused GTests cover:
- inbound-only data details containing numeric `subcode` and string `reason`;
- code/message-only errors with no `data` element;
- outbound-only code override;
- inbound/outbound success and message override;
- malformed inbound JSON without implementation invocation;
- indexed method routing and fallback values;
- `REALOBJECT*` invocation for all input/output forms and indexed rich errors;
- callable and `REALOBJECT*` instance-ID-only routing without an index.
- direct-lambda context and combined instance-ID/index routing.
- free functions passed by name for regular, rich-error, and context-aware registration.
- `std::bind` wrappers for regular and rich-error no-prefix registration.
- every input/output form with context, instance ID, and combined context plus instance ID;
- indexed inbound and outbound method paths.
