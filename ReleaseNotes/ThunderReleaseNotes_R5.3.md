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

### Feature: loggign in casze second comrpc call on chennel deadlock


NOTE: hier gebleven

https://github.com/rdkcentral/Thunder/commit/8da778df7a574b8e6872ef31d1c2c458689bb0f5

### Feature: Sink now has WaitReleased

https://github.com/rdkcentral/Thunder/commit/3f85bc5d2adc2da705603f94b9797dc408ecaf93

### Change: imporved dangling proxies notification

https://github.com/rdkcentral/Thunder/commit/bd88dec72296bc867b732b78ed8539d2103dd52f

now allowed  via comrpc call


### Change: THUNDER_PERFORMANCE 

working again

https://github.com/rdkcentral/Thunder/commit/5c281b3514bafc3d1bdc9cd6840aa2e1a2e13fb2_

### Change: connector timelout setable

In code, makes smartlinktype timeout for xonnwection possible

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

### Feature: support status listeners for lookup objects

bug fix 

https://github.com/rdkcentral/ThunderTools/commit/2da9dd7895b7c9961c2548f5be37cc9168ab8d03

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#object-lookup) for more info and how to use this.

### StatusListener closedc channel 


https://github.com/rdkcentral/Thunder/commit/bc8965bf8eded200c3bdcacf9025624b0980ca1a

### Feature: new buffer encoding options

encode:hex and encode:mac (note that it kmight be better to the natiove tupoe that is suported see below) (next to existing ecnode:bas64) wotk with all types that have an arry (arry, vector, even iterator)


Core::

https://github.com/rdkcentral/ThunderTools/commit/ef7940f4ed23e3f7cbd2cff99840cae5ee2d165e

now also supported in events: https://github.com/rdkcentral/ThunderTools/commit/b7203bd5d4c4bb9a1da82bae75ce9ef94e2fa768

### Feature: New supported type: class MACAddress

Core::MACAddress

https://github.com/rdkcentral/ThunderTools/commit/7e3bc68e46dd5ce8842c909cdee01d7034fac7b3

### Feature: Use string instance id 

instance id can be a string now instead of number

https://github.com/rdkcentral/ThunderTools/commit/4ab8b4a458233e4461e87f5ffaa4f8d3b9436734

### Feature: support custom object lookup

@encode:autolookup see example interfaces -> breaks the 5.2 autollook without autolookup
 @encode:lookup (see add whete the lookup lambda's are registered)


https://github.com/rdkcentral/ThunderTools/commit/be4ad6be7299b4992218ace5b9947e91ca66783f

https://github.com/rdkcentral/Thunder/commit/8280f2ac5d38b87bbd5fc3ceb000e0dae9beefd6

https://github.com/rdkcentral/Thunder/commit/58f642c8e75906a2a0b9797fc6288cbbae541604

https://github.com/rdkcentral/Thunder/commit/a6645b4d85c96c60fe295d12226aa40e8f9ad265

https://github.com/rdkcentral/ThunderTools/commit/b19295114ba37be801464407aa31ebf6bf8e5e44

### Feature: serialize non empty arrays and containers

Core::OptionLType (if not optuional bracjest always omited, if optional and not set then null)


https://github.com/rdkcentral/ThunderTools/commit/0304e4519178956048b6cc9ecf3cd05a38460a49


### include enum/struct from other header file??????

@insert 

### Feature: eneble enum conversion generatrion for non json rpc interfaces

https://github.com/rdkcentral/ThunderTools/commit/e20510ef35356856f7398a7441cd25658efbc65c

### Feature: support optional iterators:

what is says

https://github.com/rdkcentral/ThunderTools/commit/49dec3e3ad8b108797e329ae5839d9c364ec93ee

### Feature: support restrict for iterators

see feature above 

https://github.com/rdkcentral/ThunderTools/commit/c8f5665a55d0392cc2f91c5d04f4dbab1834fb34

### Feature: allow only lower bound restrict for strings

@nonempty string only (will raise error when not empty)

https://github.com/rdkcentral/ThunderTools/commit/4354bda52520bb43519fb125495fa1e59ebdf104

(https://github.com/rdkcentral/ThunderTools/commit/37a5f43d0f2d54d7dc4bdf45348f268ff6a00677)

### Feature: colated iterators

data in iterators in one go (disabled) -> switch on the generator -> in future 
 --collated-iterators 

 @interface means pass it as interface instead all data put after iterator

 In 6.0 this might become becom ethe dafult behaviour

https://github.com/rdkcentral/ThunderTools/commit/b87925af70680181ed66b9754f2273d0c25b9d6b

### Feature: build in methods in documentation:

versions 
exists
register
unregiste 

documented in the plugin and interface doc so all interfaces in one place


https://github.com/rdkcentral/ThunderTools/commit/a47fbcb56a9e555219bc1cc47411643474c1f265

### Feature: event index with @

new, being cvhanged

https://github.com/rdkcentral/ThunderTools/commit/01924d640d2f3f8ff6d9ff49615540063b6fb9a8

also mention deprecated


index can be optional on the event (could alreayd be done for the property)

### Beta Feature: PSG with interface parsing

### *** tags ***

https://github.com/rdkcentral/ThunderTools/commit/1cbab5907240b9115de13530f8cdc776a7e69358

fixed  @optional              - indicates that a parameter, struct member or property index is optional (deprecated, use Core::OptionalType instead)")
legacy or         print("   @default {value}       - set a default value for an optional parameter")
        print("   @encode {method}       - encodes a buffer, array or vector to a string using the specified method (base64, hex, mac)")


  print("   @alt:deprecated {name} - provides an alternative deprecated name a JSON-RPC method can by called by")
        print("   @alt:obsolete {name}   - provides an alternative obsolete name a JSON-RPC method can by called by")
        print("   @alt-deprecated {name} - provides an alternative deprecated name a JSON-RPC method can by called by")
        print("   @alt-obsolete {name}   - provides an alternative obsolete name a JSON-RPC method can by called by")

 print("   @retval {desc}         - sets a description for a return value of a JSON-RPC method")
        print("   @retval {value} {desc} - sets a description for a return value of a JSON-RPC method or property")
        
### Change: General bug fixes

A number of fixes and improvements were made to make the generated code more robust and compliant with higher warning levels and compiler versions

??? https://github.com/rdkcentral/ThunderTools/commit/57ea36a3aba1dbcf9806e0c19b72d4e35a4d8054

https://github.com/rdkcentral/ThunderTools/commit/2f99361ea74aa09e6c98af171179b5377f0bb16d @container??

???? https://github.com/rdkcentral/ThunderTools/commit/50913316be0edb83845dc285323b47bee08a1b44


???? https://github.com/rdkcentral/ThunderTools/commit/94de3196e5ae4cff676d870457d941ac5b861e98

https://github.com/rdkcentral/ThunderTools/commit/007e2f0f10708ddeda8d78f84ca117724355a5e8 (breaking??? not compared to 4.4 as it was not available)

@optional fixed: https://github.com/rdkcentral/ThunderTools/commit/eb2cc7828bba22775b32db99063cb2f3d8d63f98


???? https://github.com/rdkcentral/ThunderTools/commit/3d41cfa3dc8e119db1c7c4a359041acee46d8663


???? https://github.com/rdkcentral/ThunderTools/commit/1d483e42d041190a0db8c20d8a3e34205f748037


??? https://github.com/rdkcentral/ThunderTools/commit/39f12403dc75d229580db01e0e5d91b983b521ef


??? https://github.com/rdkcentral/Thunder/commit/f687888251f54174f6a676e21152b82e39bc5342

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
    - new index
    - wrapped for single out params
    - CustomCode feature
    - Batch plugin
    - wrepped
     