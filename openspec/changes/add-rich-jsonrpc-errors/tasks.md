## 1. Core Error Model
## 1. Native Error Model

- [x] 1.1 Add `Core::ErrorDetails<DATA>` with optional code, message, and data.
- [x] 1.2 Keep the type available to legacy C++11 integrations.
- [x] 1.3 Add the code/message-only `Core::ErrorDetails<void>` specialization.

## 2. Error-aware Registration

- [x] 2.1 Add callable `RegisterWithError` support to the core handler.
- [x] 2.2 Add plugin-host forwarding overloads.
- [x] 2.3 Add input/output tag dispatch for error-aware handlers.
- [x] 2.4 Support context, index, and instance-ID routing combinations.
- [x] 2.5 Use a single return point in new implementation helpers.
- [x] 2.6 Add `REALOBJECT*` registration through a C++11-compatible binder.

## 3. Serialization and Response Construction

- [x] 3.1 Initialize rich errors from Thunder's framework-error mapping.
- [x] 3.2 Override only explicitly supplied code and message fields.
- [x] 3.3 Serialize typed data when present.
- [x] 3.4 Ignore error details on success.
- [x] 3.5 Suppress normal outbound results on failure.
- [x] 3.6 Parse rich output into a temporary response object.
- [x] 3.7 Overlay only fields present in valid rich output.
- [x] 3.8 Preserve malformed and unrecognized output as legacy error text.
- [x] 3.9 Use JSON container `IsSet()` for presence checks.
- [x] 3.10 Serialize data conditionally for error containers that expose it.

## 4. Unit Tests

- [x] 4.1 Add inbound-only data coverage.
- [x] 4.2 Verify typed `subcode` and `reason` round-trip serialization.
- [x] 4.3 Add outbound-only code override coverage.
- [x] 4.4 Add inbound/outbound success coverage.
- [x] 4.5 Add message-only override coverage.
- [x] 4.6 Verify malformed inbound JSON does not invoke the implementation.
- [x] 4.7 Add indexed method coverage and index forwarding.
- [x] 4.8 Select lambda traits when default function traits do not describe the callable.
- [x] 4.9 Add the test source to Linux and non-Linux core test targets.
- [x] 4.10 Add `REALOBJECT*` coverage for all input/output forms.
- [x] 4.11 Add indexed `REALOBJECT*` rich-error coverage.
- [x] 4.12 Add callable and `REALOBJECT*` regressions for instance ID without index.
- [x] 4.13 Verify code/message-only errors omit the data field.
- [x] 4.14 Add direct-lambda context, index, instance-ID, and combined-routing regressions.
- [x] 4.15 Add free-function-by-name regressions for regular and rich-error registration.
- [x] 4.16 Add substitution-safe callable detection and `std::bind` regressions.
- [x] 4.17 Cover all input/output forms with context and instance-ID routing.
- [x] 4.18 Add indexed inbound and outbound method coverage.
- [ ] 4.19 Run the complete Linux `Thunder_test_core` suite after integration.

## 5. Follow-up

- [ ] 5.1 Refactor regular `InternalRegister` overloads with input/output tags in a separate structural changeset.
