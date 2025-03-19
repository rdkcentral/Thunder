# Thunder Release Notes R5.2

## introduction

This document describes the new features and changes introduced in Thunder R5.2 (compared to the latest R5.1 release).
See [here](https://github.com/rdkcentral/Thunder/blob/master/ReleaseNotes/ThunderReleaseNotes_R5.1.md) for the release notes of Thunder R5.1.
This document describes the changes in Thunder, ThunderTools and ThunderInterfaces as they are related. 

# Thunder 

##  Process Changes and new Features 

### Feature: Github actions extensions

The Github actions were extended to have better build coverage, they are ran on every Pull Request commit. These actions now for example also include MacOS builds.

### Warning is Error and stricter warning checking

The build options were changed for Thunder code to have stricter warning checking and also to treat all warnings as errors.

### Change: Unit test improvements

The existing Thunder unit tests were improved again and new tests were added (e.g. (web)sockets and TLS). These are also triggered from a GitHub action on each commit added to a Thunder Pull Request.

### Change: Thunder Documentation

The Thunder documentation was extended with new content, please find the documentation [here](https://rdkcentral.github.io/Thunder/). More content still to come!

### Feature: CIEC support team

To be able to support the Thunder community better a dedicated Thunder support team was setup in CIEC.

## Changes and new Features

### Feature: Thunder changes to support new Tooling features

Internal Thunder code was adapted to be able to support the new ThunderTooling features (e.g. new JSON-RPC casing infrastructure, JSON-RPC session support and async interfaces, std::vector support). See the ThunderTools section below for more details on these features.

### Feature: TLS Client side certificates and Server 

The Thunder SecureSockets implementation now also supports client side certificates verification and the option to run a SecureSocketPort server (previously only connecting to a SecureSocketPort server was possible). 

### Feature: Assert also handled by Message Engine

Asserts are now also handled by the message engine providing the same configuration and export flexibility as for Traces, Syslogs, stdout and sterr messages.
This was added as we notice configurations being used where Asserts are enabled but not configured to lead to a process exit (with callstack). Of course it is advisable to investigate Asserts being triggered and fix them as they might indicate the cause of issues seen later on.

### Change: Message Engine Improvements

The message engine was improved again internally. 
On very early Thunder startup where the message engine is not fully up and running it is now made sure messages are not completely missed but outputted to std output.

### Change: Suspend and Resume via controller

Suspend and Resume notifications for plugins can now also be subscribed on using a Controller event instead of only on a specific plugin.

### Change: Leaked Proxy detection and reporting

The detection and reporting of leaked proxies was improved (also backported to Thunder 4.4)

### Change: Thunder Meta data

The Thunder meta data (that can be retrieved via a call to the Controller) was extended and now contains more info (mainly intended for QA and debug purposes)

### Change: Process container updates

Some flavours of the ProcessContainer implementations were updated to reflect the changes made in Thunder 5.1 where the ability to run multiple container flavour implementation at runtime was added.

### Change: JSONRPCLink improvements

The JSONRPCLink now also makes sure events it was subscribed to for will be restored when a connection was lost and restored. 

### Change: Windows socket handling improvements

The Windows socket handling code was improved (e.g. in case an overlapped IO operation was reported)

### Feature: Custom JSONRPC error messages and error codes supported.

Although not advised as it will break consistency and generated documentation it is now possible to override the default errormessage and errorcode for JSON-RPC when generated from the COM-RPC method call return code. See [here](https://github.com/rdkcentral/ThunderNanoServices/tree/master/examples/DynamicJSONRPCErrorMessage) for an example how it can be used.


### Change: General bug fixes

A number of issues found in Thunder 5.1 have been corrected in Thunder 5.2. 

## Breaking Changes

Although Thunder 5.2 is a minor upgrade and therefore should not contain breaking changes there is one however. This due to a request to the Thunder team to change the behavior of the ThunderTooling regarding the default casing generated for the JSON-RPC API.
In itself this change in the ThunderTooling should not lead to different behaviour in Interfaces used in RDKServices as far as we are aware. The old Thunder Interfaces used in RDKServices were all adapted to generate the exact same interface as in 5.1 and others used different @text options already to force the behaviour that has now become the default.

### default case for generated JSON-RPC code.

The ThunderTools now generate a different casing for the JSON-RPC code by default then it did before. Thunder 5.1 and older generated lowercase function, parameters, PoD members and event names by default from the interface files while the enums where PascalCase by default. Thunder 5.2 generates camelCase function, parameters, PoD members and event names by default while the enums are UPPER_SNAKE by default.
See for more details below and also on new options to influence the generated case.

# Thunder Tools

## Changes and new Features 

### Feature: JSON-RPC case options

Up to now the JSON-RPC code generators generated lowercase function, parameters, PoD members and event names by default from the interface files while the enums were PascalCase by default. Quite a number of interfaces used different @text options to deviate from this and it was decided to make camelCase the default for function names, parameters, PoD members and event names while using UPPER_SNAKE for enums. 
This is now what Thunder 5.2 will generate as default.

There are quite a number of new options to influence the generated casing for the JSON-RPC interface.
E.g. use @text:legacy_lowercase to have an interface generate the pre 5.2 casing. It is even possible to specify a completely custom pattern using @text:custom. There are also two new parameters for the code generator to influence the behaviour at generation time, --case-convention and --ignore-source-case-convention.

All these new options are already documented in the Thunder documentation (https://rdkcentral.github.io/Thunder/)

Please note in Thunder 6.0 we will make all lowercases JSON-RPC interface adhere to the new default standard. This will be a breaking change but will make all interfaces consistent regarding the JSON-RPC casing.

### Feature: Session based interfaces

The support for interfaces that expose in the COM-RPC world access to objects created via a call to a method is increased greatly, simplifying the process required for this to basically needing in most cases only to provide and implement the COM-RPC interface. 
The code generator automatically detects these interfaces and generates the necessary code for this.
It even works for notification used in such a created object (session) where one can subscribe and be notified of only the events for this object/session.
In the functional JSON-RPC world an automatically generated object ID is used to connect functional world to the OO COM-RPC /C++ world.

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#object-lookup) for more info and how to use this.

### Feature: Async interfaces

When an action triggered by a method on a COM-RPC interface which takes some time to complete this method will be made asynchronous, meaning it will return when the action is started and it will expect a callback interface to be passed as input parameter to the method so a method on the callback interface will be called when the action that was started is finished or failed.
This is now also supported on the JSON-RPC interface. 
One can indicate a method on the JSON-RPC interface should be asynchronous by specifying the @async tag.
The code generators will then generate code to fully automate this again, meaning no additional code needs to be written for the JSON-RPC interface.

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#asynchronous-functions) for more info and how to use this.

### Feature: std::vector Support

The usage of std::vector< Type > is now supported by the Proxy Stub generators as well as the JSON-RPC generators (where it translates to a json array).
It is also allowed to be used inside events/notifications (see the Thunder documentation on the new feature [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#overview) to see why you should be careful when using this). 

### Feature: plugin skeleton generator

The PluginSkeletonGenerator (PSG) is a tool that can be used to generate a skeleton for a new plugin, giving you a quick start when developing a new plugin but also preventing common mistakes to be made in the generic plugin code.

Please note that the tool is in beta release at the moment. Although it was very carefully designed and reviewed of course it cannot be ruled out that there are any issues with it, certainly given the number of different configurations it can generate. If you find any issues with the generated code please contact the Thunder team.

More information can be found [here](https://rdkcentral.github.io/Thunder/plugin/devtools/pluginskeletongenerator/).

Please note also a main entry for all ThunderDevelopmentTools was added but for now only contains only the Plugin Skeleton Generator, more information to be found [here](https://rdkcentral.github.io/Thunder/plugin/devtools/devtools/).

### Feature: pre- and postconditions

It is now possible to add pre- and postconditions tags for a method to document the pre- and postconditions. So for (generated) documentation purposes only.
See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#precondition)

### Change: not optional means mandatory

To prevent any confusion, the Document generator will now explicitly mark non-optional elements as mandatory. So this change only influences the generated Documentation, not any behaviour.

### Change: Core::OptionalType and fixed arrays

Core::OptionalType is now also supported for fixed array types, meaning a fixed array 
can contain elements of Core::OptionalTypes, the array itself logically cannot be an OptionalType. 

### Change: interfaces with an out buffer and in/out length

 Interfaces with an out buffer and in/out length are now handled correctly when the buffer passed is a nullptr (this is a typical use case to first get the required length and then do a second with the properly allocated buffer to get the full data). Perviously this use case could lead to unexpected crashes (it was not guaranteed to crash 100% of the time). [This](https://github.com/rdkcentral/ThunderInterfaces/blob/16d1459ae85940fa71d6836301dc2b4288e9f3b4/interfaces/IOCDM.h#L180) would be an example of such a method that when used with a nullptr for the buffer could lead to such a crash.

 ### Known Issue: statuslistener for session based interfaces

 Note that using the @statuslistener inside session based interfaces will not work at the moment.

# Thunder Interfaces

## Changes and new Features 

### Change:

To guarantee backward compatibility with the new JSON-RPC casing options and add the possibility to move to a new casing option in the future the old Metrological interfaces used in RDKServices were converted from json meta file to an Interface (.h) file.

### Change: external interface offset added

An external offset was added to the interface ID range used by Thunder so a specific range can be used for interfaces outside ThunderInterfaces whose ID is not added to the IDs.h. This to prevent collisions with interfaces inside ThunderInterfaces.

### Change: ITestAutomation

The ITestAutomation interface was extended to make it possible to add more testing features and to explicitly test the new casing options.

### Change: IDictionary improved

The IDictionary interface was improved to add full JSON-RPC support and to make usage of the interface more intuitive and consistent.