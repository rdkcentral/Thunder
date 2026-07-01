# Thunder Release Notes R5.3

## Introduction

This document describes the new features and changes introduced in Thunder R5.3 (compared to the latest R5.2 release).
See [here](https://github.com/rdkcentral/Thunder/blob/master/ReleaseNotes/ThunderReleaseNotes_R5.2.md) for the release notes of Thunder R5.2.
This document describes the changes in Thunder, ThunderTools and ThunderInterfaces as they are related. 

# Thunder 

##  Process Changes and new Features 

### Feature: Windows build artifacts cleanup

The Windows build (Visual Studio Projects) now fully cleans up the build artifacts when the solution is cleaned guaranteeing a complete fresh build after cleaning.

### Feature: Scripts before Thunder start in CMake

A feature was added to the Thunder CMake files to enable the inclusion of scripts to be executed before Thunder is started by the (Linux) startup script. These scripts should be placed [here](https://github.com/rdkcentral/Thunder/tree/master/Source/Thunder/scripts) and see there as well for some examples.

### Feature: Actions added that build with Performance Monitoring feature on

The GitHub Actions workflow now includes a build with the Performance Monitoring feature enabled (this adds code to Thunder that enables a Performance Monitor plugin to measures JSON-RPC performance) so pull requests are checked for regressions in this area.

### Feature: Copilot scanning enabled and issues fixed

Copilot is now enabled for all Thunder repositories alongside Coverity.  The few remarks found when by Copilot have been addressed.

## Changes and new Features

## Feature: WorkerPool PriorityQueue

The Thunder WorkerPool (and therefore the ThreadPool as well) now has the ability to use priorities for its jobs. Three priorities are available, high medium and low. 
In the Thunder config the total number of WorkerPool threads used by Thunder itself, as well as the thresholds of maximum parallel scheduled jobs for medium and low priority jobs can be configured (note if not overridden here the already existing THREADPOOL_COUNT is still taken into account).
See more info [here](https://rdkcentral.github.io/Thunder/introduction/config/).

The scheduling algorithm used by the Thunder WorkerPool/ThreadPool can be compile time configured to be either static or dynamic. With static only the maximum number of jobs for that priority will be scheduled in parallel, with dynamic additionally the total load of the system is also taken into account.
When both static and dynamic are disabled (at compile time) the previous non priority solution will be used.

Currently, only low and high priority jobs are used in Thunder. If a COM-RPC request is received from a channel that already has an outgoing request (IPC, not in-process), the job to handle the incoming request is submitted with high priority. This mitigates deadlocks seen previously when the thread pool is filled with COM-RPC jobs that require callbacks into Thunder before they complete. Note: if returning COM-RPC calls keep spawning out-of-process calls, you may still exhaust WorkerPool threads, depending on the configured thread count.

For this reason, it is advised to configure the low-priority threshold to be at least one less than the total number of worker pool threads to ensure at least one thread is always available for high-priority jobs.

In case of an incorrect configuration, e.g. medium and/or low thresholds higher than the total amount of threads will trigger an ASSERT.

As this is a relatively new but potentially complex feature, the Thunder documentation will be extended with a more detailed description than the above.

### Feature: Custom (Error) Codes.

Before Thunder 5.3 error codes used in Thunder were predefined, which sufficed up to now. These error codes (due to the "JSON-RPC in terms of COM-RPC" feature) are also translated into JSON-RPC error codes and there was a request to support a more flexible error scheme (mainly to have more flexibility in the error codes reported in the JSON-RPC error object).
So with this new Custom Codes feature a mechanism was introduced to have custom error codes that are consistent for both COM-RPC and JSON-RPC (or any other future protocol that will be implemented in terms of COM-RPC), allow for custom (not hardcoded inside Thunder) code to string translation and allow for direct influence on the error code returned by JSON-RPC.

For more info see [here](https://rdkcentral.github.io/Thunder/utils/customcodes/) in the Thunder documentation.

### Feature: Warning reporting for WebSocket open and close

We now have added a WarningReporting message when the opening and/or closing of a WebSocket takes longer than an (configurable as always with WarningReporting) minimum amount of time.

### Change: Open COM-RPC port later

We moved the opening of the COM-RPC port to later in the startup of Thunder to prevent issues from clients trying to access it too soon.

### Feature: make Thunder uClibc compatible

Changes were made to the Thunder code to make it uClibc compatible.

### Feature: JSON-RPC FlowControl configurable

The JSON-RPC FlowControl feature (see the the Thunder 5.1 release notes for more info) is now configurable from the Thunder configuration file and plugin configuration file.

With "channel_throttle" in the Thunder configuration file one can configure the maximum number of parallel JSON-RPC requests allowed for any channel.
With "throttle" in the Thunder configuration file one can configure the maximum number of parallel JSON-RPC requests allowed to a particular plugin.
The default for both is half the number of workerpool threads (with a minimum of one).

With the option "throttle" in the plugin configuration file one can for this specific plugin override the generic value set in the Thunder configuration file for the maximum number of parallel JSON-RPC requests allowed.

### Feature: WarningReporting for JSON-RPC FlowControl

We now have added a WarningReporting message when the FlowControl feature (see above and in the Thunder 5.1 release notes) leads to longer than configured handling times.

### Change: COMRPCTerminator thread only started when needed

The COM-RPC terminator thread (a cleanup thread used within Thunder) is now only started when required, leading to one less thread in processes that do not need it (like ThunderPlugin)

### Feature: allow per callsign registration IShell

When registering for plugin state notifications on the IShell interface over COM-RPC, it is now also possible to subscribe to state changes for a single plugin by optionally providing its callsign, next to subscribing to state changes for all plugins.
This change was made in a way that is backwards compatible with existing code.

Note the applicable SmartInterfaceTypes (which use the IPlugin::INotification internally) have been adopted to use this feature reducing COM-RPC traffic and overhead.

### Feature: allow per callsign state change notifications in IController and improvements

The IController state notifications have been improved (so this also affects the JSON-RPC plugin state notifications).

It is now possible to register only to be updated for the state changes of a specific plugin instead of state changes of all plugins, while the latter of course is also still possible, by optionally providing the callsign of the plugin for which one wants to receive the notifications.
This also applies to the JSON-RPC interface, for details here best to consult the generated documentation for the IController interface available in the Thunder repository.

StateChange (which notifies of changes to plugin state that do not includes Plugin::IStateControl) will, when registering, send a notification for all plugins already activated at that moment (when registering to be notified for all plugin state changes). If registering for a specific plugin it will send a Activated notification in case that specific plugin was already activated at the moment of registering.

StateControlStateChange (which notifies of changes to plugin state from Plugin::IStateControl, so suspended and resumed) will when registering for notifications for a specific plugin immediately notify about the current state for that plugin and send no current state notification when registering for all plugins.

### Feature: MAC Address class

A class was added to Thunder core to represent a MAC Address (and as noted below is also fully supported by the Thunder code generators)
The class can be found [here](https://github.com/rdkcentral/Thunder/blob/d57c0345bb14f2c0c1a845786f6e349bf7a9da47/Source/core/MACAddress.h#L29).

### Feature: Enable to reach remote plugin via the Composite plugin feature

Thunder now fully supports accessing a Plugin living in a remote Thunder instance to be made accessible in the local Thunder instance using the Composite plugin feature (see also the new BridgeLink plugin in the ThunderNanoServicesRDK repository, read the release notes for that [here](https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK/blob/master/ReleaseNotes/ThunderReleaseNotes_R5.3_rdkservices.md) )

The ThunderUI has also been updated to support this feature and will now also show the remote plugins now.

### Feature: version handling for distributed plugins

The plugin version handling now takes the Composite plugin into account. One can now use versioned methods also for the "composited" plugin by putting the version after the composite plugin delimiter.

### Feature: metadata loading configurable

One can now disable the loading of Plugin metadata (plugin version, subsystem being controlled and dependencies) with the Thunder configuration flag "discovery". With this flag set to true (default) means loading the metadata is enabled.
When disabled this means this information will not be loaded before the plugin is actually activated making some features of Thunder not work correctly.
This feature was added on request because, on some devices, loading the library to read metadata took much longer than normal. So only set this flag to false in special circumstances.

### Feature: Forgiving JSON-RPC method Pascal/CamelCase handling

A build option was introduced to enable "forgiving" JSON-RPC method casing handling in Thunder. When turned on (disabled by default) Thunder will accept the method name both as camelCase and PascalCase (to be more forgiving as method name casing was not always done consistently in interfaces).
The build option is called ENABLE_JSONRPC_FORGIVING_METHOD_CASE_HANDLING.

### Change: ICOMLink::INotification Revoked notification

When in the past the ICOMLink::INotification Revoked method was renamed to Dangling it was not removed or made deprecated and now might also confuse people with the Revoked notification when an offered interface is revoked from COM-RPC communicator.
It is now made deprecated and should in this context not be used anymore (it cannot yet be removed as that would break plugins that implement ICOMLink::INotifcation). When used make sure only Dangling is handled from this notification, Revoked will be removed in Thunder 6.

### Feature: Syslog on COM-RPC timeout

When a COM-RPC call is canceled because it takes longer than the configured COM-RPC timeout this is now logged as a Syslog message to help the investigation of situations where this happens. 
Note: in this case we now also close the connection as can be read above.

### Change: Improvements to Library and Service Discovery handling

The way Thunder loads the available services from Plugin libraries was improved in general (preventing deadlocks for example which were seen in rare cases by our QA department).
This now also makes it possible to have services with a duplicate name which previously would have led to issues.

### Change: Activate reason

The reporting of the failure for a plugin to activate has been improved and an additional failure state was added called "INSTANTIATION_FAILED" if instantiating the plugin before the IPlugin::Initialize is called failed (which would be a very rare error).

### Feature: Logging second invoke on COM-RPC channel

Now a message is printed to Syslog when a second COM-RPC invocation is made to a channel warning about this possible deadlock (so in an IPC situation, not in process). This to help in the investigation of these plugin issues.

### Feature: SinkType now has WaitReleased

The SinkType now has a WaitReleased method. This can be used in the special situation when a Service is put into the SinkType but there can be a possible race condition between closing the channel(s) from which the Service is used and receiving the actual Release call(s) from the using side(s).
With WaitReleased the server side can now wait for the Release() to be received from the client side(s), with timeout. 
See for a scenario in which this is useful [here](https://github.com/rdkcentral/ThunderPluginActivator/blob/89f789624305f4473fb5b605e8ae3cc4fa39fd15/source/COMRPCStarter.cpp#L94) and for the actual call to WaitReleased [here](https://github.com/rdkcentral/ThunderPluginActivator/blob/89f789624305f4473fb5b605e8ae3cc4fa39fd15/source/COMRPCStarter.cpp#L152). 

### Change: Improved dead proxies handling

Previously calling a COM-RPC method (over an IPC channel) from the (PluginServer) ICOMLink::INotification Dangling notification could lead to a possible deadlock this has been fixed now.
This means that all Dangling handling code (so both in PluginServer as well as CommunicatorServer) is now safe for this usage pattern.

### Change: THUNDER_PERFORMANCE code made to work again

The Thunder code using the THUNDER_PERFORMANCE has been corrected so it is functional again (this enables code in Thunder that can be used by a special plugin developed in the past to do JSON-RPC performance measurements)
The flag has a generic name so that future possible performance measurement code can also be put inside this flag.

### Change: Connector timeout can be specified.

It is now possible to set the timeout to be used in the SmartLinkType connect and disconnect (before it was always Core::infinite so it would wait indefinitely for the event to happen)

### Change: General bug fixes

A number of issues found in Thunder 5.2 have been corrected in Thunder 5.3.

## Breaking Changes

In principle Thunder 5.3 does not contain breaking changes. Some changes alter previous behavior but are not expected to cause any backward compatibility issues; they are listed below.

### Feature: close channel on COM-RPC timeout

To prevent issue as a result of "collateral damage" after a COM-RPC timeout has happened we now close the channel on which the COM-RPC timeout occurred automatically.

### Change: Error message changed for suspend and resumed when not supported

When a Suspend or Resume request for a Plugin was received via the Thunder Controller (both COM-RPC and JSON-RPC) and the plugin does not support the PluginHost::IStateControl interface previously the error ERROR_UNAVAILABLE was reported, this has been changed to ERROR_NOT_SUPPORTED.

### Change: improved handling when a COM-RPC string is too big

If a COM-RPC string transferred over IPC is too big for the maximum size that was reserved with @restrict no longer the string will be capped but now this will assert when asserts are enabled in the build and otherwise no data will be sent at all to better indicate to the receiver not the full data was transferred.

### Change: Controller Version change

There are some small changes to the Controller interface regarding the Version method:

The Controller Version method (to retrieve the Thunder version) has been renamed Framework in COM-RPC to prevent any confusion. For JSON-RPC "version" will still work but is made deprecated and will be removed in Thunder 6.

### Change: Exists and Register/Unregister changes

There are some small changes to the following generic JSON-RPC functions for a plugin:

#### Exists

The "exists" function (to inquire if a certain function is available on the JSON-RPC interface) is made consistent and can now (also) be used using "compliant" parameters format:
```json
params{ "name" : "<functionname>"}
```
This will now correctly return "result" : true or false.

Note the previous format can still be used:

```json
params : "<functionname>"}
```

which will return a JSON-RPC error object ERROR_UNKNOWN_KEY (code 22) or "result" : null to be fully Thunder 4.4 backwards compatible.

#### Register/Unregister

The generic plugin JSON-RPC "register" and "unregister"  functions to register for a JSON-RPC Notification have been changed to now return "result" : null if the registration/un-registration was successful and no longer return 0.
On error nothing changed (except of course when compared to Thunder 4.4 a change in the error numbers used as can be read in the Thunder 5.0 Release notes)

### Change: TriState class removed

The Thunder Core class TriState has been removed. It was suboptimal and nowadays alternatives exist like OptionalType<bool>. 
We do not expect this to be used anywhere nor was it used in Thunder internally.

# Thunder Tools

## Changes and new Features 

### Feature: event indexes improvement

Using indexes for events has been improved. 
The indexes now use @< index > at the end of designator in the JSON-RPC interface.
This had to be changed as indexes could contain dots itself (e.g. if the index is a callsign) so having it before the first dot in the clientid would not suffice.
Although the old dot format was in use only for a short time in Thunder 5 the @index:deprecated can be used with an event parameter to indicate it is an index for that event and the deprecated dot format should be used.

Indexes now also support OptionalType indexes to indicate subscribing to the index or all is in principle optional. For this to work it is now also allowed to put the @index keyword at the Register and the event is then looked up by name of the indexed parameter.
(it is also allowed and advised to also put the @index then at the Unregister)
Please see [this section](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#indexed-events) in the Thunder documentation that has been added to describe indexed events.

### Change: auto object lookup now requires @encode:autolookup

The JSON-RPC auto object lookup feature now requires the @encode:autolookup tag, this for improved detection and consistency.
Note the auto object lookup feature was introduced in Thunder 5.2 without needing @encode:autolookup to be specified (it should not be breaking as Thunder 5.2 was not used to define new object based JSON-RPC interfaces with auto object lookup).

A new feature for the auto object lookup is that it is now possible to register callbacks to be called when objects are acquired or relinquished in case special handling is needed for JSON-RPC (generic code can be put into the COM-RPC code for Acquire and Relinquish as that is called for both cases).

The (updated) documentation can be found [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#object-lookup)

### Feature: support custom object lookup

Next to auto object lookup the Thunder Tooling now also support custom object lookup for JSON-RPC interfaces.
Where auto lookup takes care of the creation of object ID's and linking them to the objects for you, custom lookup can be used when the objects already have a unique ID internally (e.g a unique name or number ID) that can be used.
This makes it easier for clients as the now can use a more meaningful ID instead of the abstract ID created by autolookup.
Custom lookup is indicated by specifying the @encode:lookup with the interface for the object type.

See the Thunder documentation for more info [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#custom-object-lookup)

### Feature: support status listeners for lookup objects

Object lookup (see above) now also supports status listeners to be used for session object creation and destruction. See the object look up section and the code generator examples referred to from there for more information on this.

### Feature: StatusListener unregistered also on channel closed event

When the @statuslistener is used for an event in an interface the status listener is also called with state Status::unregistered when a client explicitly unregisters from the event.
Starting 5.3 this is also done when the unregister is a result of the channel from which the client registered closing without the client calling unregister explicitly before that (note the internal cleanup inside Thunder did already happen, there were no leaks, just the statuslistener was not called when this happened).

### Beta Feature: PSG with interface parsing

The Plugin Skeleton Generator has been greatly extended to now parse the IDL interface file(s) you want the generated plugin code to implement and therefore generates code already completely providing all methods for the Plugin, only the implementation needs to be added!
It does generate code for both COM-RPC and JSON-RPC interfaces just by checking if the interface IDL header file indicates also specifies a JSON-RPC interface should be generated.
It has also been extended with example code for dangling proxies (if applicable for the interface it implements) and support for subsystem handling code generation.

Note due to all the permutations possible with the interfaces it can encounter the PSG remains in beta. If you see issues or have doubts on the code it generates please contact us.

Note: see [here](https://rdkcentral.github.io/Thunder/plugin/devtools/pluginskeletongenerator/) for the (updated) Thunder documentation on the Plugin Skeleton Generator

### Feature: wrapped format

The newly added wrapped tag will for a single output parameter also add the parameter name to the result, making it always a JSON object. It can also be used for arrays, std::vector, iterator etc. 
Of course it is preferable to keep the JSON-RPC interface as whole consistent but this was added as there are interface where workarounds are used to achieve the wrapped effect so having this tag will make it easier to achieve the wrapped format.

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/#wrapped) for more info.

### Feature: new buffer encoding options

There is a new encoding tag supported with @encode (next to the already existing base64):

@encode:hex will encode/decode the buffer as hex value into/from the JSON-RPC string (so works for both input and out parameters). Buffer can be an array, std::vector or buffer+len parameter with base type uint8_t or char.

All encodings can now also be used in events.

### Feature: New supported type: class MACAddress

The newly added Thunder type Core::MACAddress (to hold MAC addresses) is now fully supported in both COM-RPC and JSON-RPC from the IDL header file by the code generators.

### Feature: serialize empty non optional arrays and containers

Previously empty arrays (and also std::vectors and iterators) and containers (if all members are optional and not set) output parameters are not serialized to the JSON-RPC output at all. 
Starting 5.3 they will be included by default when empty (as [] and {}) (this on request of the Comcast Thunder user base).

When it is desired they are not included when empty they can be made optional (with OptionalType< >(var)) in the interface, then when not set they will not be included.

Note this is not considered breaking as both are valid JSON (and it is on specific request).

### Feature: Allow inclusion of enum and POD from other IDL header file

It is now possible to use an enum or POD (struct with data members) defined in one IDL header file in another by including the other header file with @insert

### Feature: Enable enum <-> string conversion for non JSON-RPC interfaces

Previously the enum to string (and vice versa) conversion tables were only generated when the enum was used in an interface used in JSON-RPC generation (so with @json tag).
Starting 5.3 they can be generated for COM-RPC only interfaces as well. 
For this use the @encode:text tag with the enum in the COM-RPC interface.

### Feature: support optional iterators:

Iterators in the IDL header file can now also be of OptionalType<>.

### Feature: support restrict for iterators

The @restrict tag can now, next to strings, arrays and std::vector, also be used to set the minimum and maximum allowed size for iterator types in the IDL header file.

### Feature: allow only lower bound restrict for strings

When an input string in a method in the IDL is not allowed to be empty (but it is not desirable to set a maximum length, if that is the case the @restrict:x..y tag can be used) it can be flagged with the @restrict:nonempty tag.
If the string is empty this will already generate an error when validating the input in the generated proxy stub code.

### Feature: collated iterators

Iterators in the past always got their values one by one via the COM-RPC interface leading to overhead in an IPC situation as multiple COM-RPC calls are needed while most of the times the iterators were used to in the end get all the values.
It is now possible to make all the iterators get all the values in one go and after that the COM-RPC calls to get the values will only be local calls.
To enable this mode for all iterators you can pass the flag --collated-iterators to the proxy stub generator.
With this mode enabled one can by specifying the tag @interface with an iterator or the iterator typedef make that iterator work like it did in the past and get the values one by one.

Note in Thunder 6 this mode might become the default, that has not been done yet to give the opportunity to put the @interface tag at the iterators that potentially deal with huge amounts of data where this might pose a too big of a memory penalty.

### Feature: build in methods in documentation:

The JSON-RPC API documentation generation now for every plugin also includes the Thunder predefined functions ("versions", "exists", "register" and "unregister") so that a complete API overview is created.
These methods are now also described in the Thunder documentation at the relevant places.
        
### Change: General bug fixes

A number of fixes and improvements were made to make the generated code more robust and compliant with higher warning levels and compiler versions

# Thunder Interfaces

## Changes and new Features 

### Change:

### Feature: there is now an example section for interfaces

The Thunder interfaces now contain a special section for example interfaces that are not meant to be used in production, see [here](https://github.com/rdkcentral/ThunderInterfaces/tree/master/example_interfaces)

This also includes new example interfaces for features added in Thunder 5.3.

### Change: interfaces updated for lower case legacy

A number of interfaces being used by contributed plugins still used the JSON meta file solution.
These have been changed in to full IDL header files so the legacy-lowercase feature could be used to make the interfaces backwards compatible in JSON-RPC casing now the default for this has changed.
(among these: timesync, webkit, opencdm, memorymonitor, locationsync, security agent, device-id)

### Feature: IPluginAsyncStateControl interface added

The [IPluginAsyncStateControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/R5_3/interfaces/IPluginAsyncStateControl.h) interface was added which can be used by services that want to implement plugin state control functionality.

     