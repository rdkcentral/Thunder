# Thunder Release Notes R5.0

## introduction

This document describes the new features and changes introduced in Thunder R5.0 (compared to the latest R4 release).
See [here](https://github.com/rdkcentral/Thunder/blob/master/ReleaseNotes/ThunderReleaseNotes_R4.4.md) for the release notes of Thunder R4.4.
This document describes the changes in Thunder and ThunderTools as they are related. 

## WARNING 

Thunder R5.0 is a major step up from R4. For the new features quite a lot of the internal code was changed or rewritten. Although it was very carefully tested by QA it will most probably contain some issues that have not been found yet as the Thunder framework can be used in a lot of different ways. Therefore our advise will be to use R5.0 only to do an early integration as it also contains some breaking changes (see the Braking Changes paragraph below). Please report any issues you find so these can be fixed. Our advise will be to use R5.1 for production usage as it will have all the combined fixes for the issues found with R5.0.

# Thunder 

##  Process Changes and new Features 

### New Feature: Issue Template 

An issue template was added to the documentation for reporting Thunder issues. Please use the template that can be found [here](https://rdkcentral.github.io/Thunder/issuetemplate/issuetemplate/) when you want to report a Thunder issue.

### Change: Unit test improvements

The existing Thunder unit tests were improved and new tests were added. These are also triggered from a GitHub action on each merge of a Pull Request.

### Change: Thunder Documentation

The Thunder documentation was extended with new content, please find the documentation [here](https://rdkcentral.github.io/Thunder/). More content still to come!

### Change: QA interfaces

To make it clear what interfaces are specifically inteneded for QA purposes these have been put into a separate folder and namespace. They can be found [here](https://github.com/rdkcentral/ThunderInterfaces/tree/master/qa_interfaces) 

## Major Changes and new Features

### New Feature: JSON-RPC non happy day scenarios

One of the main focusses of Thunder R5 was improved support for JSON-RPC non happy day scenario's. When a JSON-RPC interface was for example "session based" there was little automated support for handling connection issues and correct session management based on that (one would need to write the complete interface by hand). In R5 it is now possible to specify a separate JSON-RPC interface for these purposes and this will bring also much more options when the JSON-RPC interface deviates from the COM-RPC interface. An example can be found [here](https://github.com/rdkcentral/ThunderInterfaces/blob/227976cc2fa9e7167414aa6e924d3767ae9cf2b0/interfaces/IMessenger.h#L66) but this will be fully documented in the Thunder documentation asap.

### New Feature: Private COM-RPC

A feature was added where the communicator used to access a plugin via COM-RPC can be changed from the default communicator (that can be used to access all plugins) to a specific port so that for example applications in a process container can only be given access to that port and will not be able to access other containers.

### New Feature: Off host plugin support

Infrastructure was added to enable running plugins on different devices/SOCs. A second instance of Thunder can be run on a different device handling the actual remote plugin but the plugin is exposed as a normal plugin in the local instance of Thunder (so can be activated/deactivated, communicated with via JSON-RPC and COM-RPC etc.)

### New Feature: Aggregated plugin support

It is now possible to expose multiple plugins within a parent plugin, so to have an aggregated plugin. These can then be access and communicated with via the parent plugin. This is for example convenient when a group of plugins share a common responsibility or to expose multiple plugins running on a different host.

### New Feature: Thundershark [Application]

A plugin was developed for the popular WireShark network analysis tool that enables analyzing Thunder COM-RPC in detail. All message exchange can be seen in detail (both online and offline) with details like which method is called on what interface using what parameters.
Details can be found [here](https://rdkcentral.github.io/Thunder/debugging/thundershark/)

### New Feature: proxy leakage detection and reporting

Thunder now has improved detection of proxies being leaked when an out of process plugin is deactivated/crashed. These can now also be reported in more detail, for example via a JSON-RPC interface for QA purposes.

### New Feature: Mac-OS support

Next to Linux and Microsoft Windows Thunder can now also be build and run on Mac-OS.

### Change: IController suspend en resume via IController ILifetime

Managing suspend and resume plugin lifetime now can be done from the IController ILifetime interface. Most imortanly this means that there are no longer both a JSON-RPC and COM-RPC interface file but all is specified in one IDL header file. The JSON-RPC interface has not changed because of this.

### New Feature: Delegated release configurable.

Unbalanced reference counted objects are for the non-happy day scenarios now automatically destructed. This can lead to unexpected crashes in case the plugin has an incorrectly implementation of the refcount handling of objects. This automatic cleanup feature can be turned off by using the "ccdr" (COM-RPC channel delegated releases) boolean option in the Thunder configuration file (default is on off course)


## Minor Changes and new Features 

### New Feature: Installation and Cryptography subsystems

Two new subsystems were added to the available subsystems: Installation and Cryptography.

### New Feature: Messaging enhancement 

The messaging engine now by default also reroutes all stdin and stdout messages to the message control plugin (where it then like all other messages can be printed, saved to file, forwarded to a websocket etc.). This behaviour can be turned of using the -f or -F startup parameter.

### Change: Messaging efficiency improvements 

The internal handling of messages in the messaging engine has been improved to reduce the duplication of messages and therefore improving the resource usage of the message engine in both memory as CPU usage.

### Change: Messaging -F -f behaviour

When the message engine -f or -F option is used no longer the messages (tracing, syslog, waring reporting etc.) are forwarded to the message control plugin, they are now only locally outputted improving performance in this situation. 


### Change: Yocto meta layers 

The Yocto meta layers for Thunder and its components have been refactored to comply with the latest CMake standards.


### Change: Plugin/interface versioning improved
    
The plugin MetaData (holding the version information) is now also inserted through the COM-RPC definitions. 

### Change: JSON serialization/deserialization improvements

There have been multiple improvements to the JSON serialization and deserialization to improve compliance with the JSON standard (e.g. utf-8 handling, escaping, type support, hex characters have been improved)
These changes have also been backported to R4.4.

### Change: Websocket improvements

WebSocket: Masking handling has been improved.

### Change: Compliance higher waring level

CMake as well as Thunder code has been changed to handle higher warning levels and prevent false positives. Also the default is now that a warning is treated as an error. 
Also compliance with GCC12 warning generation was achieved. 

### Change: Verify SSL certificates by default

In the secure websocket implementation the SNI is now always validated against the root-certificates installed on the box.

### General bug fixes

A lot of issues were fixed in Thunder R5.0 improving stability and resource usage. 

## Breaking Changes

### WPEFramework renamed to Thunder

All references to WPEFramework have been renamed to Thunder, this goes for example for the namespace in code, names of binaries, name of build files etc. 
For the namespace changes mitigation measures have been put in place to have old code still using the WPEFramework namespace still compile but due to the complexity of the C++ language this will not cover all cases. Also make sure to adapt your script referring to Thunder binary and/or make files.

### Plugin/interface versioning (
    
In the JSON-RPC interface the plugin version is now embedded in a MetaData structure.

### JSONRPC error codes 

The JSON-RPC error codes are now better aligned with the JSON-RPC specification so error codes can have different numbers compared to older Thunder versions.

### Old config meta files

The old config meta files are removed and the new config.in files are now the default. For plugins the old meta files are still supported.

### AutoStart

In plugin config files the "AutoStart" : boolean has been changed into "Mode" : [Unavailable,Activated,Deactivated,Resumes,Suspend] . Reason for this change was the addition of the Unavailable mode.
Note that tools will still handle plugin configuration meta files correclty that use "AutoStart", it will generate a correct config will (where "mode" will be used).

### Metadata

The "Metadata" strcuture in plugins has been renamed to "MetaData".

### Plugin status via Controller JSON-RPC

The plugin status that can be retrieved via the controller JSON-RPC will now always return an array. You can still request one plugin status but it will be enclosed in an array

###  Addref has return value

The IReferenceCounted AddRef virtual method now requires a return value. This should not cause any issues as it is not expected that there would be a need to implement this outside the Thunder framework itself.
This was also backported to R4.4.

### JSONRPC event forwarding 

The controller all event: the event data is no longer wrapped in an additional layer called Data.


# Thunder Tools

## Changes and new Features 

### @text 

The @text metatag was extended to have more options to influence the names used in generated code. E.g. there is now also an option, "keep",  to override the case used for generation of names per interface. See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#text) for all the possibilities in the documentation. 
(backported to R4.4.3)

### Nested pods

Nested pods are now fully supported in the IDL header files, even more than one level including its uses in iterators.
(Backported to R4.4)

### Core::Time

The Core::time is type is now also supported in the IDL header files.

### Notifications

Context and indexes are now also supported in Notifications

### Length support

The length (and maxlength) tags can be used to indicate the (max) length of a buffer used in the IDL header file. See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#length) for the detailed documentation.


### @length:return 

As an optimization the @length for out parameters can now be a return value. 

### JSON::InstanceId

The JSON::InstanceId is now also supported in the IDL header files.

### Desctiption tags

The description tag will now support multiple line descriptions.

### POD Inheritance

The IDL header file code generation tools now also support PODs that use inheritance (if for example there are shared members for different POD types)

### float support

The float type is now also supported in the IDL header files.

### Waring cleanup

The existing interfaces have been cleanup to no longer generate warnings when the code generator is executed.

### Multiple methods for event

It is now possible to have multiple event names to be emitted for a single event (fot backward compatibility cases) using the @alt tag.

### JSON-RPC Event iterators

The code generation tooling now also support the usage of Iterators for events.

### Optional

The tooling now allows to specify that a parameter is optional in the IDL header file using Core::OptionalType (this superseded @optional). In COM-RPC the OptionalType can be used to see if a value was set and in JSON-RPC it is then allowed to omit this parameter. Detailed documentation will be  added to the Thunder documentation.

### Default

When using optional types as described above it is now also possible to provide a default value to be used in case the parameter value was not specified. Also this will be described in more detail in the Thunder documentation.

