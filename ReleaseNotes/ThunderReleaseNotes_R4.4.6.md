# Thunder Release Notes R4.4.6

## Introduction

This document describes the changes introduced in Thunder and ThunderTools R4.4.6 (compared to the R4.4.5 release).

Normally a patch release will only contain bug fixes but in this case also a number of much requested features where backported from Thunder 5, mostly on the code generators.

The changes described here are present on the [R4_4 branch](https://github.com/rdkcentral/Thunder/tree/R4_4) and represent all commits since the [R4.4.5 tag](https://github.com/rdkcentral/Thunder/compare/R4.4.5...R4_4).

# Thunder

## Changes and Bug Fixes

### Feature: Telemetry Support

Thunder now, via the MessageEngine, has support for telemetry output. 
For now two Telemetry engines are supported, a mock one that just prints the messages and a T2 one.

For more details please see the documentation on it [here](https://rdkcentral.github.io/Thunder/plugin/messaging/#telemetry)

### Feature: ThunderExtensions support

Thunder now has support for ThunderExtensions. These are services specifically meant to extend Thunder functionality.
In the Thunder config it can be defined which extensions are allowed to run and in which order they need to be activated.

### Change: Messaging Subsystem Backport from R5

Backported messaging improvements from Thunder R5 to R4_4, improved message handling, opt-in stdout/stderr redirection (disabled by default), and a compatibility API to retrieve message controls for all modules. These changes preserve compatibility with existing integrations.
(PR [#2114](https://github.com/rdkcentral/Thunder/pull/2114)).

### Change: Thunder Core Updated for Generator Toolchain Compatibility

Thunder core has been updated to work correctly with the upgraded JsonGenerator and ProxyStubGenerator now available in ThunderTools R4_4 (backported from R5.x). Changes include support for indexed notifications, backward-compatible status listener code, and additional COM-layer accessors.

### Change: Plugin config override behaviour

The Plugin config override behaviour is now only enabled in a Debug build or when explicitly enabled by build flag (default off).

### Change: More detailed syslog entries for plugin activation

Syslog statements have been added to the plugin initialization and activation area to improve diagnostics when a plugin fails to start or takes longer than expected to activate (PR [#2144](https://github.com/rdkcentral/Thunder/pull/2144)).

### Change: JSON FromString Trailing Whitespace Fix

Fixed an edge case in `JSON::FromString()` where trailing whitespace at the end of the input string caused a spurious parse failure.
### Fix: Infinite Loop in JSON Stream on Garbage Input

An issue was fixed where Thunder could get into an infinite spin when a an invalid JSON-RPC request was received.
Detection for incorrect / garbage data has improved preventing the spin and this event is now explicitly logged.
(PR [#2135](https://github.com/rdkcentral/Thunder/pull/2135))

### Fix: Infinite Loop on Thunder Exit

Fixed an infinite loop that could occur during the Thunder framework shutdown sequence.
(PR [#2145](https://github.com/rdkcentral/Thunder/pull/2145)).

### Fix: WebSocket Consecutive Payloads Exceeding 65 KB not working

Fixed a WebSocket framing bug where sending two or more consecutive messages larger than 65 KB caused incorrect payload handling. The zero-length header-size edge case is also covered by this fix.
(PR [#2124](https://github.com/rdkcentral/Thunder/pull/2124)).

### Fix: Thread Safety for Core::SystemInfo Environment Access

Added a mutex lock around `Core::SystemInfo::GetEnvironment()` and `SetEnvironment()` to prevent data races when these methods are called concurrently from multiple threads. The fix applies to both Linux and Windows builds. The C standard library does not always guarantee thread safeness for these calls but when using the Thunder versions they now are.
(PR [#2122](https://github.com/rdkcentral/Thunder/pull/2122))

## Breaking Changes

Thunder R4.4.6 does not introduce intentional breaking changes relative to R4.4.5. All interface signatures remain unchanged.

---

# ThunderTools

## Introduction

The ThunderTools changes in R4.4.6 are all backports of JsonGenerator and ProxyStub Generator improvements from ThunderTools R5.x. The changes refine the code generators used to produce JSON-RPC and COM-RPC glue code from Thunder interface definitions.


### Feature: std::vector support

The usage of std::vector< Type > is now supported by the Proxy Stub generators as well as the JSON-RPC generators (where it translates to a json array).
It is also allowed to be used inside events/notifications (see the Thunder documentation on the new feature [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#overview) to see why you should be careful when using this). 

### Enhanced fixed array support

Fixed arrays are now also allowed as method parameters and POD members in interfaces. Fixed arrays can supports all types (incl. strings, Core::Time or PODs) with the exception of iterators.

### Optional

The tooling now allows to specify that a parameter is optional in the IDL header file using Core::OptionalType (this superseded @optional). In COM-RPC the OptionalType can be used to see if a value was set and in JSON-RPC it is then allowed to omit this parameter.
@optional still has a purpose: making parameter checks lenient without modifying the C++ interface.

### OptionalType enhancements

OptionalType is now also allowed for property indexes and iterators in interfaces. The OptionalType for the property index is for example used in the Thunder Controller interface: if filled it indicates the callsign of the plugin for which info is requested, if omitted (so empty) information for all plugins will be returned.

### Change: Core::OptionalType and fixed arrays

Core::OptionalType is now also supported for fixed array types, meaning a fixed array 
can contain elements of Core::OptionalTypes, the array itself logically cannot be an OptionalType. 

### Feature: support optional iterators:

Iterators in the IDL header file can now also be of OptionalType<>.

### Default

When using optional types using the @optional it is now also possible to provide a default value to be used in case the parameter value was not specified. 
E.g. for a string it could be @default:"[unset]" if empty string would have a logical meaning, also it is semi-required if a integer/enum/bool parameter is @optional otherwise it'll get {} value.

### Feature: support restrict for iterators

The @restrict tag can now, next to strings, arrays and std::vector, also be used to set the minimum and maximum allowed size for iterator types in the IDL header file.

### Core::Time

The Core::time is type is now also supported in the IDL header files.

### StatusListeners

Support for installing hooks for notification registration status (new `@statuslistener` tag)
See the documentation [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#statuslisteners)

### Indexed notifications

Support for indexed notifications.
Please see the documentation [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#indexed-events)

### Improved naming case conventions and support

The @text tag provides now more options to influence the code generator case for json-rpc.
For more info see [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#text)

### Non optional parameter check

If json-rpc parameters are not optional they are checked to be present and correct.
If this is not the case an error is logged but the message is handled like it was in 4.4.5.
(in thunder 5 this behaviour will be changed and the message refused and an error response will be sent)

### Feature: wrapped format

The newly added wrapped tag will for a single output parameter also add the parameter name to the result, making it always a JSON object. It can also be used for arrays, std::vector, iterator etc. 
Of course it is preferable to keep the JSON-RPC interface as whole consistent but this was added as there are interface where workarounds are used to achieve the wrapped effect so having this tag will make it easier to achieve the wrapped format.

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#wrapped) for more info.

### Feature: new buffer encoding options

There is a new encoding tag supported with @encode:

@encode:hex will encode/decode the buffer as hex value into/from the JSON-RPC string (so works for both input and out parameters). Buffer can be an array or buffer+len parameter with base type uint8_t or char. Encode hex support for std::vector will follow in the next release.

All encodings can now also be used in events.
See for more info [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#encode)

### Feature: allow only lower bound restrict for strings

When an input string in a method in the IDL is not allowed to be empty (but it is not desirable to set a maximum length, if that is the case the @restrict:x..y tag can be used) it can be flagged with the @restrict:nonempty tag.
If the string is empty this will already generate an error when validating the input in the generated proxy stub code.

### Feature: JSON container move support

JSON containers can now be moved when using the generated code

### Feature: Allow inclusion of enum and POD from other IDL header file

It is now possible to use an enum or POD (struct with data members) defined in one IDL header file in another by including the other header file with @insert

### Change: not optional means mandatory

To prevent any confusion, the Document generator will now explicitly mark non-optional elements as mandatory. So this change only influences the generated Documentation, not any behaviour.

### Change: document generator no longer adds .1 version to examples

As the json rpc interface versioning support was already deprecated and will be removed in Thunder 5 the examples generated by the document generator will no longer output an example where it explicitly adds a .1 to the json rpc call (this was never needed anyway and would already have resulted in version 1 being used)

### Change: Warning emitted if a method name collides with the built-in methods 

If an interface indicating a json-rpc interface should be generated you will not get a warning if the interface contains any of the  built-in methods: `register`, `unregister` or `exists`

### Change: Warning emitted if a notification has a return value

If a notification has a return value the code generator will now output a warning (as that cannot be supported in json-rpc)

## Breaking Changes

ThunderTools R4.4.6 does not introduce intentional breaking changes relative to R4.4.5.
