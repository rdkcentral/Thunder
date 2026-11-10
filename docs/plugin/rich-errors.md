# Rich error details for plugin methods

Thunder maps the `Core::hresult` returned by a method to a JSON-RPC error code and message. **Use the standard `Core::ERROR_*` codes in most cases.** They are simpler for both plugins and clients, and an error-details parameter is not required just to report a failure.

Rich errors are for exceptional cases where the standard error alone cannot convey information a client needs to act on: for example, a device-specific diagnostic, an actionable explanation, or a deliberately chosen JSON-RPC error code. They can also preserve backward compatibility when existing clients depend on custom error data; keep the expected data shape and values when migrating those methods. Rich errors are supported for **methods only**, not properties. The returned `Core::hresult` remains the framework result; rich details supplement it rather than replacing it.

`Core::ErrorDetails` is modeled after the JSON-RPC error fields, but it is a native interface type, not a JSON-RPC-only feature. A COM-RPC client can also receive the optional error-details output parameter and use its code, message, or typed data to understand a failure. The implementation supplies the same meaningful details regardless of whether a client uses COM-RPC or JSON-RPC; only the transport's presentation differs.

Dynamic error messages (the `PluginHost::JSONRPCErrorAssessor` approach) are deprecated in favor of rich error details for new interfaces. Existing integrations may continue to use them, but new methods that need more than a standard error should declare an `ErrorDetails` output parameter instead. Prefer the standard `Core::ERROR_*` codes when they are sufficient.

## Declare an interface method

Include the core error definition for interface generators with `@insert`, declare an optional trailing `@out` parameter, and choose a type for the `data` field if structured diagnostics are needed:

```cpp
#include "Module.h"

// @insert <core/Errors.h>

namespace Thunder {
namespace Example {

    // @json 1.0.0
    struct EXTERNAL ISimpleRichError : virtual public Core::IUnknown {
        enum { ID = ID_SIMPLERICHERROR };

        struct Diagnostic {
            uint8_t water_level;
            Core::OptionalType<string> info;
        };

        using Error = Core::ErrorDetails<Diagnostic>;

        // @brief Makes a coffee
        virtual Core::hresult MakeCoffee(uint16_t& duration /* @out */,
            Core::OptionalType<Error>& errorDetails /* @out */) = 0;

        // @brief Restarts the machine; this error has no data field
        virtual Core::hresult Restart(
            Core::OptionalType<Core::ErrorDetails<void>>& errorDetails /* @out */) = 0;
    };

} // namespace Example
} // namespace Thunder
```

`Core::ErrorDetails<DATA>` has independently optional `code`, `message`, and `data` members. `Core::ErrorDetails<void>` has only `code` and `message`. An alias such as `Error` can keep interface signatures short. The enclosing `Core::OptionalType` lets a method leave *all* details unset.

## Populate details in the implementation

Return a non-`Core::ERROR_NONE` framework result on failure. Set only those fields for which the standard mapping is insufficient:

```cpp
Core::hresult SimpleRichError::MakeCoffee(uint16_t& duration,
    Core::OptionalType<Example::ISimpleRichError::Error>& errorDetails)
{
    Core::hresult result = Core::ERROR_NONE;

    if (_waterLevel == 0) {
        Example::ISimpleRichError::Diagnostic diagnostic;
        diagnostic.water_level = 0;
        diagnostic.info = _T("Refill the water tank");

        errorDetails = Example::ISimpleRichError::Error();
        errorDetails.Value().data = diagnostic;
        result = Core::ERROR_UNAVAILABLE;
    }
    else {
        duration = 25;
    }

    return (result);
}
```

Here only `data` is supplied. Thunder still provides the JSON-RPC `code` and `message` corresponding to `Core::ERROR_UNAVAILABLE`. When an override is genuinely necessary, assign `errorDetails.Value().code` or `.message` as well. To supply an error without data, use `ErrorDetails<void>`; for example, a `Restart` failure can set only `.message` and return `Core::ERROR_ILLEGAL_STATE`, retaining Thunder's mapped code.

For example, if the `MakeCoffee` method above fails because the water level is zero, a request might receive a JSON-RPC response like this:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -31002,
    "message": "The service is not active.",
    "data": {
      "waterLevel": 0,
      "info": "Refill the water tank"
    }
  }
}
```

| Method result and details | JSON-RPC behavior |
| --- | --- |
| `Core::ERROR_NONE` | Normal result is returned; populated error details are ignored. |
| Failure, details unset | Standard Thunder error code and message are used. |
| Failure, only `data` set | Typed data is returned; code and message keep Thunder's defaults. |
| Failure, only `code` or `message` set | Only that field overrides the Thunder default. |
| Failure, all fields set | Explicit code, message, and typed data are returned. |

A failed method's ordinary output parameter is not returned as a successful JSON-RPC result. 

## Reading details over COM-RPC

For COM-RPC, `errorDetails` is an ordinary optional output parameter in the interface signature. A client checks the returned `Core::hresult` first, then checks whether the output parameter and its individual members are set. Unlike JSON-RPC, COM-RPC does not synthesize default `errorDetails.code` or `errorDetails.message` values: unset members remain unset, and the framework result still conveys the standard error. Define any explicit `code` value so it has a useful meaning to COM-RPC clients as well, rather than relying solely on JSON-RPC-specific conventions.
