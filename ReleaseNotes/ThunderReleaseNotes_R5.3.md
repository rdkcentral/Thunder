# Thunder Release Notes R5.3

## Introduction

This document describes the new features and changes introduced in Thunder R5.3 (compared to the latest R5.2 release).
See [here](https://github.com/rdkcentral/Thunder/blob/master/ReleaseNotes/ThunderReleaseNotes_R5.2.md) for the release notes of Thunder R5.2.
This document describes the changes in Thunder, ThunderTools and ThunderInterfaces as they are related. 

# Thunder 

##  Process Changes and new Features 

### Feature: Windows build artifacts cleanup

The Windows build (Visual Studio Projects) now fully cleanup the build artifacts when the solution is cleaned guaranteeing a complete fresh build after cleaning.

### Feature: Scripts before thunder start in CMake

A feature was added to the CMake files to enable the inclusion of scripts to be executed before Thunder startup (Linux).

### Feature: action build with Performance Monitoring feature aan

The GitHub actions are extended with a build where the Performance Monitoring feature (enables code that will enable a special plugin to measure JSON-RPC performance) is turned on to verify it is not broken by a pull request.

### Feature: Copilot scanning enabled and issues fixed

Next to Coverity also Copilot is now enabled for all Thunder repositories to check the pull requests for issues. The few remarks found when enabled have been cleared up.

## Changes and new Features

## Feature: WorkerPool PriorityQueue

The Thunder WorkerPool (and therefore the ThreadPool as well) now has the ability to use priorities for its jobs. Three priorities ara available, high medium and low. 
In the Thunder config the total number of WorkerPool threads used by THunder itself, as well as the thresholds of maximum parallel scheduled jobs for medium and low priority jobs can be configured (note if not overridden here the already existing THREADPOOL_COUNT is still taken into account).
See more info [here](https://rdkcentral.github.io/Thunder/introduction/config/).

The scheduling algorithm used by the Thunder WorkerPool/ThreadPool can be compile time configured to be either static or dynamic. With static only the maximum number of jobs for that priority will be scheduled in parallel, with dynamic additionally the total load of the system is also taken into account.
When both static and dynamic are (compile time) compile time disabled the previous non priority solution will be used.

Within Thunder itself at the moment only low and high priority jobs are used. When a COM-RPC request is received from a channel (so in an IPC situation, not in process) that has already an outgoing request, the job to handle the incoming request will be submitted with high priority, this to mitigate deadlocks that were seen previously when the thread pool was already filled with COM-RPC jobs that all required a call back into Thunder to be handled before they would be released (note any returning COM-RPC call will be elevated to high when there is already an outgoing call, not only the ones as the result from an outgoing call). Note: this will only mitigate this as one keeps calling (out of process) COM-RPC calls from the returning call depending on the amount of WorkerPool threads configured one might run out of threads to handle them at some point.

For the previous it is advised to always configure the lower priority threshold to at least one lower than the total amount of worker pool threads to guarantee at least one thread always to be available for the high priority jobs.
In case of anb incorrect configuration, e.g. medium and/or low thresholds higher than the total amount of threads will give an ASSERT.

As this a relatively new but potentially complex feature to Thunder the Thunder documentation will be extended with a more detailed description than the above.

### Feature: Warning reporting for WebSocket open and close

We now have added a WarningReporting message when the opening and/or closing of a WebSocket takes longer than an (configurable as always with WarningReporting) minimum amount of time.

### Change: Open COM-RPC port later

We moved the opening of the COM-RPC port to later in the startup of Thunder to prevent issues from clients trying to access it too soon.

### Feature: make Thunder uClibc compatible

Changes where made to the Thunder code to make it uClibc compatible.

### Feature: JSON-RPC FlowControl configurable

NOTE: Discuss with Pierre

Pierre vragen ~0 gewoon zo groot dat je er geen last van hebt? default half worker pool

The JSON-RPC FlowControl feature (see the the Thunder 5.1 release notes for more info) is now configurable from the Thunder configuration file.
Thunder "throttle" en "channel_throttle", plugin "maxrequests" en "throttle"

Missen niet de IsSet checks op sommige plekken (en kan ik het ook uizetten of alleen heel groot zetten (if ((_used < _slots) || (_slots == 0)) {) )

https://github.com/rdkcentral/Thunder/commit/d28c40a9dc948a33bf02c3a7ddd255f357d7a9ed

https://github.com/rdkcentral/Thunder/commit/eea443cde3827ec3a44fcc8790176f39bf674737#diff-cb405958fdffcb85a8aeaa1bd1204df978e2525eb9bbab4db014266c39825d73

### Feature: WarningReporting for JSON-RPC FlowControl

We now have added a WarningReporting message when the FlowControl feature (see above and in the Thunder 5.1 release notes) leads to longer than configured handling times.

### Change: COMRPCTerminator thread only started when needed

The COM-RPC terminator thread (a cleanup thread used within Thunder) is now only started when required, leading to one less thread in processes that so not need it (like ThunderPlugin)

### Feature: MAC Address class

A class was added to Thunder core to represent a MAC Address (and as can be read below is also fully supported by the Thunder code generators)

### Feature: door Composite naar remote plugin 

NOTE: Pierre vragen waarom dit er nu stond, dit kon toch al?

Thunder now supports accessing a Plugin living in a remote Thunder instance to be made accessible in the local Thunder instance using the Composite plugin feature (see also the new BridgeLink plugin in the ThunderNanoServicesRDK repository, read the release notes for that here NOTE hier nog link toevoegen)

### Feature: version handling for distributed plugins

The plugin version handling now takes the Composite plugin into account. One can now use versioned methods also for the "composited" plugin by putting the version after the composite plugin delimiter.

### Feature: metadata loading configurable

One can now disable the loading of Plugin metadata (plugin version, subsystem being controlled and dependencies) with the Thunder configuration flag "discovery". With this flag set to true (default) means loading the metadata is enabled.
When disabled this means this information will not be loaded before the plugin is actually activated making some features of Thunder not work correctly.
This feature on request as on some devices just loading the library to get this information took much longer than in normal situations (so only turn this flag to false in special situations). 

### Feature: Forgiving JSON-RPC method Pascal/CamelCase handling

A build option was introduced to enable "forgiving" JSON-RPC method casing handling in Thunder. When turned on, default is off, Thunder will accept the method name both as camelCase and PascalCase (to be more forgiving as method name casing was not always done consistently in interfaces).

### Change: ICOMLink::INotifcation Revoked notification

When in the past the ICOMLink::INotifcation Revoked method was renamed to Dangling it was not removed or made deprecated and now might also confuse people with the Revoked notification when an offered interface is revoked from COM-RPC communicator.
It is now made deprecated and should in this context not be used anymore (it cannot yet be removed as that would break plugins that implement ICOMLink::INotifcation). When used make sure only Dangling is handled from this notification, Revoked will be removed in Thunder 6.

### Feature: Syslog on COM-RPC timeout

When a COM-RPC call is canceled because is takes longer than the configured COM-RPC timeout this is now logged as a Syslog message to help the investigation of situations where this happens. 
Note: in this case we now also close the connection as can be read above.

### Change: Improvements to Library and Service Discovery handling

The way Thunder loads the available services from Plugin libraries was improved in general (preventing deadlocks for example which were seen in rare cases by our QA department).
This now also makes it possible to have services with a duplicate name which previously would have lead to issues.

### Feature: allow per callsign registration IShell

It is now possible when registering via COM-RPC on the IShell interface to be notified on plugin state changes to be notified only for the state changes of a specific plugin instead of changes for all plugins by optionally providing the callsign of the plugin for which the state changes should be sent.
This change was made in a way that is backwards compatible with existing code.

Note the applicable SmartInterfaceTypes (which use the IPlugin::INotification internally) have been adopted to use this feature reducing COM-RPC traffic and overhead.

### Change: Activate reason

The reporting of the failure for a plugin to activate has been improved and an additional failure state was added called "INSTANTIATION_FAILED" if instantiating the plugin before the IPlugin::Initialize is called failed (which would be a very rare error).

### Feature: allow per callsign state change notifications in IController and improvements

The IController state notifications have been improved (so this also affects the JSON-RPC plugin state notifications)

It is now possible to register only to be updated for the state changes of a specific plugin instead of state changes of all plugins, while the latter of of course is also still possible, by optionally providing the callsign of the plugin for which one wants to receive the notifications.
This also applies to the JSON-RPC interface, for details here best to consult the generated documentation for the IController interface available in the Thunder repository.

StateChange (which notifies of changes to plugin state that not includes Plugin::IStateControl) will when registering send a notification for all plugins already activated at that moment (when registering to be notified for all plugin state changes). If registering for a specific plugin it will sent a Activated notification in case that specific plugin was already activated at the moment of registering.

StateControlStateChange (which notifies of changes to plugin state from Plugin::IStateControl, so suspended and resumed) will when registering for notifications for a specific plugin immediately notify about the current state for that plugin and send no current state notification when registering for all plugins.

### Feature: Logging second invoke on COM_RPC channel

Now a message is printed to Syslog when a second COM-RPC invocation is made to a channel warning about this possible deadlock (so in an IPC situation, not in process). This to help in the investigation of these plugin issues.

### Feature: SinkType now has WaitReleased

The SinkType now has a WaitReleased method. This can be used in the special situation when a Service is put into the SinkType but there can be a possible racecondion between closing the channel(s) from which the Service is used and receiving the actual Release call(s) from the using side(s).
With WaitReleased the server side can now wait for the Release() to be received from the client side(s), with timeout. 
See for a scenario in which this is useful [here](https://github.com/rdkcentral/ThunderPluginActivator/blob/89f789624305f4473fb5b605e8ae3cc4fa39fd15/source/COMRPCStarter.cpp#L94) and for the actual call to WaitReleased [here](https://github.com/rdkcentral/ThunderPluginActivator/blob/89f789624305f4473fb5b605e8ae3cc4fa39fd15/source/COMRPCStarter.cpp#L152). 

### Change: Improved dead proxies handling

Previously calling a COM-RPC method (over an IPC channel) from the (PluginServer) ICOMLink::INotification Dangling notification could lead to a possible deadlock this been fixed now.
This means that all Dangling handling code (so both in PluginServer as well as CommunicatorServer) is now safe for this usage pattern.

### Change: THUNDER_PERFORMANCE code made to work again

The Thunder code using the THUNDER_PERFORMANCE has been made working again (this enables code in Thunder that can be used by a special plugin developed in the past to do JSON-RPC performance measurements)
The flag has a generic name so that future possible performance measurement code can also be put inside this flag.

### Change: Connector timeout can be specified.

It is now possible to set the timeout to bes used in the SmartLinkType connect and disconnect (before it was always Core::infinite so it would wait indefinitely for the event to happen)

### Change: General bug fixes

A number of issues found in Thunder 5.2 have been corrected in Thunder 5.3.

## Breaking Changes

In principle Thunder 5.3 does not contain breaking changes. There are some changes that are not expected to cause any issue but that do deviate from how they behaved before Thunder 5.3, they are listed below.

### Feature: close channel on COM-RPC timeout

To prevent issue as a result of "collateral damage" after a COM-RPC time out has happened we now close the channel on which the COM-RPC time happen automatically.

### Change: Error message changed for suspend and resumed when not supported

When a Suspend or Resume request for a Plugin was received via the Thunder Controller (both COM-RPC and JSON-RPC) and the plugin does not support the PluginHost::IStateControl interface previously the error ERROR_UNAVAILABLE was reported, this has been changed to ERROR_NOT_SUPPORTED.

### Change: improved handling when a COM-RPC string is too big

If a COM-RPC string transferred over IPC is too big for the max size that was reserved with @restrict no longer the string will be capped but now this will assert when asserts are enabled in the build and otherwise no data will be sent at all to better indicate to the receiver not the full data was transferred.

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

Using indexes for event has improved. 
The indexes now use @< index > at the end of designator in the JSON-RPC interface.
This had to be changed as indexes could contain dots itself (e.g. if the index is a callsign) so having it before the first dot in the clientid would not suffice.
Although the old dot format was in use only for a short time in Thunder 5 the @index:deprecated can be used with an event parameter to indicate it is an index for that event and the deprecated dot format should be used.

Indexes now also support OptionalType indexes to indicate subscribing to the index or all is in principle optional. For this to work it is now also allowed to put the @index keyword at the Register and the event is then looked up by name of the indexed parameter.
(it is also allowed and advised to also put the @index then at the Unregister)
Please see this section in the Thunder documentation that has been added to describe indexed events: NOTE: add link


NOTE: sebasian guess the "rams" use case works as well (did not test)? -> I tested it and it does!!! :)
NOTE: sebastian with the optional paremeter the documentation is a little inconsistent (not saying we should change that now :) ) notification parameter is optional (which it isn't) and it has this text: "The callsign parameter shall be passed as index to the register call, i.e. register@<callsign>." (shall better to be changed to optionaly?)
NOTE: question on non optional and optional and send all allowed (not that we should change anything it is nice and flexible) (but just to indicate I now just not explicietly mention the at all without optionaltype in the documentation)

NOTE: sebastian:
for 
            virtual void StateChange(const string& callsign /* @index */ , const bool marcel) = 0;

            (registers without callsign index but that does not matter for the issue)

it also generates:
            template<typename MODULE>
            static void StateChange(const MODULE& module_, const bool marcel, typename MODULE::SendIfMethodIndexed sendIfMethod_ = nullptr)
            {
                JsonData::Huppel5::StateChangeParamsData params_;
                params_.Marcel = marcel;

                StateChange(module_, marcel, params_, sendIfMethod_);
            }

which I don't understand and also does not compile as StateChange(module_, marcel, params_, sendIfMethod_); is not generated.

If too risky perhaps we should not fix as it is not a problem as long as you don't call it because of the template method :)

### Change: auto object lookup now requires @encode:autolookup

The JSON-RPC session auto lookup feature introduced in Thunder 5.2 now requires the @encode:autolookup tag, this for improved detection and consistency.
See more [here]() NOTE: add link to autolookup documentation section.
Note the autolookup introduced in Thunder 5.2 without @encode:autolookup (it should not be breaking as Thunder 5.2 was not used to define new (session based) interfaces).

NOTE: sebastian the generated documentation is now confusing with the id as number and id1, but perhaps I'm mssing something
NOTE: to self after published check ik autolook up links in documentation are still still correct now code has changed
NOTE: to self also document use of handlers and necesity to use special base classes think that is not done yet: https://github.com/rdkcentral/ThunderNanoServices/blob/fd91c5f012bfecac1ee4f9fa40cf7db6e7fb90ec/examples/GeneratorShowcase/GeneratorShowcase.h#L39-L42
NOTE: is this also new 5.3? https://github.com/rdkcentral/ThunderNanoServices/blob/fd91c5f012bfecac1ee4f9fa40cf7db6e7fb90ec/examples/GeneratorShowcase/GeneratorShowcase.h#L1537
      (understand how used here but Aquire and Relinquish are also called so just an extra step to get json handling attachyed if I'm right?) did a blame and it is new :)

### Feature: support custom object lookup

NOTE: - sebastion: small improvement: make https://github.com/rdkcentral/ThunderInterfaces/blob/8de7db327a585f27c6a7e1e4eb944aa29ccef070/example_interfaces/ISimpleCustomObjects.h#L85 
                   also accessory instance ID to be more in line with the autogenerated comments (makes it easier to connect them together when reading)
      - sebastian: objectlookup stores nothing, if you want lifetime you do that internally, but what if the channel closes (example is static) and you have them dynamically created on channel, you would need to get the channel from the context in the lambda and listen to the PluginHost::IShell::IConnectionServer::INotification, right?
      - sebastian: just out of curiosity: why lambda's IUknown?
      - sebastian: do not completely get how the added/removed notification is supposed to work in the example, Removed is not connected and Added not as I would expect but I could be missing something

 @encode:lookup 

 https://github.com/rdkcentral/ThunderInterfaces/blob/master/example_interfaces/ISimpleCustomObjects.h
 https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/GeneratorShowcase/GeneratorShowcase.h#L1435
 https://github.com/rdkcentral/ThunderNanoServices/blob/master/examples/GeneratorShowcase/doc/GeneratorShowcasePlugin.md#property_accessory

### Feature: Use string instance id 

Next to a number type the instance ID for an custom lookup interface can also be a string.

NOTE: sebastian guess only relevant when adding the lamnbdas?

### Feature: support status listeners for lookup objects

Object lookup (see above) now also supports status listeners to be used for session object creation and destruction. See the object look up section and the code generator examples refered to from there for more information on this.

### Feature: StatusListener unregistered also on channel closed event

When the @statuslistener is used for an event in an interface the status listener is also called with state Status::unregistered when a client explicitly unregisters from the event.
Starting 5.3 this is also done when the unregister is a result of the channel from which the client registered closing without the client calling unregister explicitly before that (note the internal cleanup inside Thunder did already happen, there were no leaks, just the statuslistener was not called when this happened).

### Beta Feature: PSG with interface parsing

NOTE: Tym updates the Thunder documentation add a link to it here as well (see below).

The Plugin Skeleton Generator has been greatly extended to now parse the IDL interface file(s) you want the generated plugin code to implement and therefore generates code already completely providing all methods for the Plugin, only the implementation needs to be added!
It does generate code for both COM-RPC and JSON-RPC interfaces just by checking if the interface IDL header file indicates also specifies a JSON-RPC interface should be generated.
It has also been extended with example code for dangling proxies (if applicable fot the interface it implements) and support for subsystem handling code generation.

Note due to all the permutations possible with the interfaces it can encounter the PSG remains in beta. If you see issues or have doubts on the code it generates please contact us.

Note: see here for the (updated) Thunder documentation on the Plugin Skeleton Generator here

### Feature: wrapped format

The newly added wrapped tag will for a single output parameter also add the parameter name to the result, making it always a JSON object. It can also be used for arrays, std::vector, iterator and POD's.
Of course it is preferable to keep the JSON-RPC interface as whole consistent but this was added as there are interface where workarounds are used to achieve the wrapped effect so having this tag will make it easier to achieve the wrapped format.

See here for more info;

NOTE: add link to documentation.

### Feature: new buffer encoding options

NOTE: Pierre/Sebastian do we still want to remove before the release the encode:mac now we have the natively supported type? (any drawbacks here?)
(after this decision I'll update the Thunder documentation)
NOTE: sebastian: I might see an issue here, let me show you

There are new encoding tags supported for @encode (next to the already existing base64):

@encode:hex will encode/decode the buffer as hex value into/from the JSON_RPC string (so works for both input and out parameters). Buffer can be an array, std::vector or even an iterator 

alle encodings can now also be used in events.

### Feature: New supported type: class MACAddress

The newly added Thunder type Core::MACAddress (to hold MAC adrresses) is now fully supported in both COM-RPC and JSON-RPC from the IDL header file by the code generators.

### Feature: serialize empty non optional arrays and containers

Previously empty arrays (and also std::vectors and iterators) and containers (if all members are optional and not set) output parameters are not serialized to the JSON-RPC output at all. 
Starting 5.3 they will be included by default when empty (as [] and {}) (this on request of the Comcast Thunder user base).

When it is desired they are not included when empty they can be made optional (with OptionalType<>(var)) in the interface, then when not set they will not be included.

Note this is not considered breaking as both are valid JSON (and it is on specific request).

### Feature: Allow inclusion of enum and POD from other IDL header file

It is now possible to use an enum or POD (struct with data members) defined in one IDL header file in another by including the other header file with @insert

### Feature: Enable enum <-> string conversion for non JSON-RPC interfaces

Previously the enum to string (and vice versa) conversion tables were only generated when the enum was used in an interface used in JSON-RPC generation (so with @json tag).
Starting 5.3 they can be generated for COM-RPC only interfaces as well. 
For this use the @enumsonly tag

NOTE: sebastian cannot get this to work...
NOTE: self still add to Thunder documentation

### Feature: support optional iterators:

Iterators in the IDL header file can now also be of OptionalType<>.

### Feature: support restrict for iterators

The @restrict tag can now, next to strings, arrays and std::vector, also be used to set the minimum and maximum allowed size for iterator types in the IDL header file.

### Feature: allow only lower bound restrict for strings

When an input string in a method in the IDL is not allowed to be empty (but it is not desirable to set a maximum length, if that is the case the @restrict:x..y tag can be used) it can be flagged with the @restrict:nonempty tag.
If the string is empty this will already generate an error when validating the input in the generated proxy stub code.

NOTE: self still add to Thunder documentation 

### Feature: collated iterators

Iterators in the past always got their values one by one via the COM-RPC interface leading to overhead in an IPC situation as multiple COM-RPC calls are needed while most of the times the iterators were used to in the end get all the values.
It is now possible to make all the iterators get all the values in one go and after that the COM-RPC calls to get the values will only be local calls.
To enable this mode for all iterators you can pass the flag --collated-iterators to the proxy stub generator.
With this mode enabled one can by specifying the tag @ASINTERFACE with an iterator make that one work like it did in the past and get the values one by one.

Note in Thunder 6 this mode might become the default, that has not been done yet to give the opportunity to put the @ASINTERFACE at the iterators that potentially deal with huge amounts of data where this might pose a too big of a memory penalty.

NOTE: sebastian: is @ASINTERFACE with capitals? I guess put it as /* ASINTERFACE */ after the iterator name?
https://github.com/rdkcentral/ThunderTools/commit/b87925af70680181ed66b9754f2273d0c25b9d6b

### Feature: build in methods in documentation:

The JSON-RPC API documentation generation now for every plugin also includes the Thunder predefined functions ("versions", "exists", "register" and "unregister") so that a complete API overview is created.
        
### Change: General bug fixes

A number of fixes and improvements were made to make the generated code more robust and compliant with higher warning levels and compiler versions

# Thunder Interfaces

## Changes and new Features 

### Change:

### Feature: there is now an example section for interfaces

https://github.com/rdkcentral/Thunder/commit/c7705e92117ac12acdecf3023ba35f3d7cb6c8ba

(whichn includes the examples for lookup and async )


- contributyed plugininterfaces geupdate waarnmodig om @josn te gebruiken zodat de interface lowercase (legacy) is, 
    -  update noodzajelijk andxers breek bwd comp
    - e.,g timesync, webkit opencdm, memorymonitor, locationsyn, sec agent, device id, 
    -  update sowieso nodig vanwege Thunder 5.x veranderingten


- new : plugin async state control intefrface


-------------

ThunderNanoServices

meerdere plugin fixes en updates voor 5.x


PIS is new


TestPrioQueue added (in tests)


-------------

ThunderNanoServicesRDK


all plugins updated for 5.x and where applicable for josn rpoc interface case 

updated for connection closed assert: https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK/commit/29e23976212d93e82ad6b1948c152078672c3d82
Did we update the PSG (I know I discussed with Tym) and the plugins in TNS? NOPE WE DID NOT!!

ook: dangling wanneer niet nodig... (ook derived can Comlink::Notification)
en ook register notificatie bij impl wanneer er geen event is (register method bestaat niet eens)
en ook geen ICOMLink::Notification register wat dan wel klopt maar botst met punt 1
    en idd als er wel dangling zou moeten zijn registreren we ook niet...

Monitor bug fixes

MessageControl bug fixes

BridgeLink plugin added (example using thge composit plugin feature to create a distrubted plugin)

OCDM fix
 issue aanmaken om de ocdm lib fix terug te draaien


-------------

Clienbt libs


Compositor changes

-------------

- missing still:
    - Batch plugin
     