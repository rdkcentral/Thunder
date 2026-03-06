---
applyTo: '**'
---

# 07 — Interface-Driven Development

> All shared plugin interfaces live in `ThunderInterfaces/interfaces/`. These are the **public API contracts** of the Thunder ecosystem. Their design and evolution rules are stricter than any other code.

## Interface File Location and Naming

- All shared COM interfaces: `ThunderInterfaces/interfaces/IMyFeature.h`
- File name: `I` + `PascalCase` + `.h` — matching the interface name exactly
- Private (plugin-internal, non-shared) interfaces may live in the plugin's own directory
- JSON-RPC schema wrappers are auto-generated into `ThunderInterfaces/jsonrpc/` — never hand-edit them

## Required Elements in Every Interface Header

```cpp
#ifndef __IMYFEATURE_H__
#define __IMYFEATURE_H__

#include <com/com.h>

namespace Thunder {
namespace Exchange {

    struct EXTERNAL IMyFeature : virtual public Core::IUnknown {

        enum { ID = RPC::ID_MY_FEATURE };  // unique ID from Ids.h — mandatory

        ~IMyFeature() override = default;   // pure-virtual destructor, no body

        // All methods must return Core::hresult
        virtual Core::hresult DoSomething(const string& input, string& output /* @out */) = 0;
        virtual Core::hresult GetValue(uint32_t& value /* @out */) const = 0;
    };

} // namespace Exchange
} // namespace Thunder

#endif // __IMYFEATURE_H__
```

**Mandatory elements checklist**:
- `virtual public Core::IUnknown` — virtual inheritance is mandatory; never non-virtual
- `enum { ID = RPC::ID_XXX }` — unique registered ID; see `06-comrpc-fundamentals.md` for ID allocation
- `~IMyFeature() override = default` — destructor must be `= default`, never defined with a body
- `EXTERNAL` on the struct — required for cross-dylib visibility
- All methods return `Core::hresult` — no exceptions to this rule
- All methods are pure virtual (`= 0`)

## Interface Immutability Rule

**Once a published interface is released, it is frozen.** This rule is absolute.

- Methods must **never** be changed — not signature, not name, not return type
- Methods must **never** be removed
- New methods may **only** be appended at the **end** of the interface
- New methods must be assigned new IDs in `Ids.h` if they are themselves interfaces
- Changing an existing method is an ABI break that corrupts all deployed systems using old proxy/stubs

**Extension pattern** — add a new versioned interface that inherits the old one:
```cpp
struct EXTERNAL IMyFeature2 : virtual public IMyFeature {
    enum { ID = RPC::ID_MY_FEATURE_2 };
    ~IMyFeature2() override = default;
    virtual Core::hresult NewMethod(uint32_t newParam) = 0;
};
```

## Method Signature Rules

| Rule | Correct | Wrong |
|------|---------|-------|
| Return type | `Core::hresult` | `void`, `bool`, `uint32_t`, etc. |
| Input string | `const string&` | `const char*`, `std::string_view` |
| Output string | `string& /* @out */` | `string*`, `char*` |
| Output scalar | `uint32_t& /* @out */` | `uint32_t*` |
| Collection | `IMyIterator*& /* @out */` | `std::vector<>`, any STL container |
| Constant method | `const` qualifier | (mutable methods that only read state) |

**Never** use raw pointer parameters for output (e.g. `uint32_t* out`) — use references with `/* @out */` annotation.  
**Never** use STL containers as parameters — use `RPC::IIteratorType<>` for collections (see `06-comrpc-fundamentals.md`).

## ProxyStubGenerator Annotation Tags

These tags appear as comments in interface method declarations and control code generation:

| Tag | Meaning |
|-----|---------|
| `/* @out */` | Marks a parameter as an output parameter (passed by reference, filled by the callee) |
| `/* @in */` | Marks a parameter as input (the default; explicit for clarity) |
| `/* @inout */` | Parameter is both input and output |
| `/* @length:N */` | For pointer+length pairs: length of the array; `N` can be a literal or parameter name |
| `/* @maxlength:N */` | Maximum buffer size for output buffers |
| `@stubgen:omit` | Exclude the entire method from stub/proxy generation |
| `@stubgen:stub` | Generate only a stub (no proxy) for this method |
| `@stubgen:skip` | Skip generation of this specific parameter |

Example with array parameter:
```cpp
virtual Core::hresult ReadData(
    uint8_t* buffer /* @out @maxlength:bufferSize */,
    uint32_t bufferSize /* @in */,
    uint32_t& bytesRead /* @out */) = 0;
```

## JSON-RPC Schema Generation Tags

`JsonGenerator` reads the same interface headers. Additional tags for JSON-RPC schema generation:

| Tag | Meaning |
|-----|---------|
| `// @json` | Enable JSON-RPC wrapper generation for this interface |
| `// @json:omit` | Exclude this method from JSON-RPC (COM-RPC only) |
| `// @event` | This method is a JSON-RPC notification (event) |
| `// @property` | This method is a JSON-RPC property (getter + setter) |
| `// @brief <text>` | Method description for schema documentation |
| `// @param[in] name <text>` | Parameter description |
| `// @retval ERROR_CODE <text>` | Documents a specific error return value |

```cpp
// @json 1.0.0
struct EXTERNAL IMyFeature : virtual public Core::IUnknown {
    // @property
    // @brief Current active state of the feature
    virtual Core::hresult State(bool& active /* @out */) const = 0;

    // @event
    // @brief Fired when the feature state changes
    // @param[in] active New state
    virtual Core::hresult OnStateChanged(const bool active) = 0;
};
```

## Versioning Strategies

| Strategy | When to use | How |
|----------|------------|-----|
| Append method | Adding to an existing interface | Add at the end only; no ID change |
| New versioned interface | Breaking change needed | Create `IMyFeature2 : virtual IMyFeature` with new ID |
| Callsign version tag | JSON-RPC-only version bump | Add `// @json 2.0.0` and update callsign suffix |
| `DEPRECATED` wrapper | Retiring an existing method | Wrap with `DEPRECATED` macro; never remove |

## Notification Interfaces

For domain events (not plugin lifecycle events), define a dedicated notification interface:

```cpp
struct EXTERNAL IMyFeature : virtual public Core::IUnknown {
    enum { ID = RPC::ID_MY_FEATURE };
    ~IMyFeature() override = default;

    struct EXTERNAL INotification : virtual public Core::IUnknown {
        enum { ID = RPC::ID_MY_FEATURE_NOTIFICATION };
        ~INotification() override = default;

        // @event
        virtual Core::hresult OnValueChanged(const uint32_t newValue) = 0;
    };

    virtual Core::hresult Register(INotification* notification) = 0;
    virtual Core::hresult Unregister(const INotification* notification) = 0;

    virtual Core::hresult GetValue(uint32_t& value /* @out */) const = 0;
};
```

**Rules for notification interfaces**:
- Always pair `Register` + `Unregister` methods in the parent interface.
- `Unregister` takes `const INotification*` — it does not call `Release` on behalf of the caller.
- The implementation must call `AddRef` on register and `Release` on unregister to manage the subscriber's lifetime.

## Interface Documentation Guidelines

Every interface method must have a `// @brief` comment. Complex interfaces should include a usage example. Parameters must be annotated with `// @param`. Error codes returned must be documented with `// @retval`.

## `IPlugin::INotification` vs. Custom Notification Interfaces

| Use `IPlugin::INotification` when | Use a custom `IMyFeature::INotification` when |
|-----------------------------------|----------------------------------------------|
| Reacting to plugin state changes (activated, deactivated) | Subscribing to domain-specific events from a feature |
| Watching for another plugin to become available | Getting change notifications from an interface method |

Never mix these — `IPlugin::INotification` is only for plugin lifecycle; custom notification interfaces carry domain events.
