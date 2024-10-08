# Thunder Release Notes R5.1

## introduction

This document describes the new features and changes introduced in Thunder R5.1 (compared to the latest R5.0 release).
See [here](https://github.com/rdkcentral/Thunder/blob/R5_0/ReleaseNotes/ThunderReleaseNotes_R5.0.md) for the release notes of Thunder R5.0.
This document describes the changes in Thunder, ThunderTools and ThunderInterfaces as they are related. 

# Thunder 

##  Process Changes and new Features 

### Feature: Github actions extensions

The Github actions were extended to have better build coverage, they are ran on every Pull Request commit. More components are now included in the builds. Also the 32bit build and Process Containers are now included as target.

### Change: Unit test improvements

The existing Thunder unit tests were improved again and new tests were added. These are also triggered from a GitHub action on each commit added to a Thunder Pull Request.

### Change: Thunder Documentation

The Thunder documentation was extended with new content, please find the documentation [here](https://rdkcentral.github.io/Thunder/). More content still to come!

## Changes and new Features

### New Feature: Whole plugin OOP (out of process)

It so now possible to run the full plugin out of process (OOP). Before this feature was added a plugin had to be specifically designed to be run OOP and part of it was always in process. The "whole plugin OOP" feature is added mainly as a development/debug option, for example to test leaks and crashes with plugins only designed to run in process. There is also the prerequisite that the plugin must have a JSON-RPC interface, a prerequisite that may be removed in a future Thunder version. 
You use this feature by adding a root section including mode="Local" outside the config part of the plugin (like you normally would have inside the plugin config). Any other modes should work as well but have not been tested. 

### New Feature: PluginSmartInterfaceType

Next to the already existing SmartInterfaceType that can be used to more easily communicate with plugins exposing interfaces over a COM-RPP connection from native code a new PluginSmartInterfaceType was added that enables the same but then from plugin to plugin. Of course this could already be done but the new PluginSmartInterfaceType makes this much easier and requires less boilerplate code. 
Find the code [here](https://github.com/rdkcentral/Thunder/blob/ac3f7d42d22715897ddce76d49ee164d36d5cc10/Source/plugins/Types.h#L403) and an example on how to use this feature can be found [here](https://github.com/rdkcentral/ThunderNanoServices/tree/master/examples/PluginSmartInterfaceType).

### New Feature: Concurrent Process Containers

On request of an external partner it is now possible to have multiple container flavours (container providers) active at the same time.
This feature was added fully backwards compatible regarding the Thunder and plugin config files, meaning if only one provider is active no changes to the config files are needed. However a change is required to the Container implementation as well and this has been done for runC and LXC. For Dobby and AWC we are discussing this with the owners of these implementations.
To use the feature first enable multiple container providers in your build. Then in the Thunder config file add the following section to set the default container implementation to be used for a plugin:
```json
"processcontainers" : {
    "default" : "lxc"
  }
```
Other valid options are: "runc", "crun", "dobby" and "awc". 
If in a plugin you want to use another container flavour than the default one you can do so by adding this to the config file of the container (of course the "mode : Container" should always be there when you want to run a plugin inside a container):
```json
"root":{
   "mode":"Container",
   "configuration" : {
      "containertype":"runc"
   }
}
```
More detailed documentation on this feature and process containers in general will be added to the Thunder documentation in the near future.

### New Feature: Flowcontrol 

To mitigate issues with plugins that have methods that have a relatively long processing time while at the same time have an interface that for example invite the user of the interface to do a lot of calls to the plugin and/or the plugin emits a lot of events Thunder 5.1 now has a FlowControl feature. 
Per channel (e.g. websocket) there can now be only one JSON-RPC call at a time, the next call received in parallel while the first one has not been handled will only be executed once the first one has been completed. Next to the channel flow control also a feature is added that will only allow one JSON-RPC call at a time to a certain plugin, so even if they come from different channels. 
In a future Thunder version the number of allowed parallel calls (both for a channel as to a plugin) will be made configurable.

### New Feature: Destroy plugin option

Next to the already existing possibility to Clone a plugin it is now also possible to Destroy a (cloned) plugin using the Controller COM-RPC or JSON-RPC interface.

### Change: General bug fixes

As Thunder R5.0 was quite a change compared to Thunder R4 a lot of issue found in the mean time in Thunder 5.0 have been fixed in Thunder 5.1. If you are using Thunder 5.0 we would strongly advice to upgrade to Thunder 5.1. As Thunder 5.1 is released relatively short after Thunder 5.0 it does not contain that many new features and changes but more importantly does contain quite some fixes for issues found in Thunder 5.0.

### Change: Message Engine improvements

There were quite some improvements to the message engine. UDP support for exporting messages was enhanced greatly.

## Breaking Changes

Although Thunder 5.1 is a minor upgrade and therefore should not contain breaking changes there is one however. This because of the request to the Thunder team to include this fix and only after consulting Thunder user representatives.

### Change: JSON-RPC over HTTP error code

We were made aware that the HTTP 202 result code that was returned in case there was an error handling a JSON-RPC call over HTTP was not as one would expect.
We did some investigation and although there is no official RFC for JSON-RPC over HTTP the consensus is indeed a 200 code should be returned, also when there is an error handling the JSON-RPC call. So Thunder 5.1 will return a HTTP 200 result code in this case. 
Note in case this does cause issues in certain situations the old behaviour can be simply brought back by setting the LEGACY_JSONRPCOVERHTTP_ERRORCODE build flag when building Thunder.

# Thunder Tools

## Changes and new Features 

### Case handling improvements

There were may changes in the code generators to better support the different Case options for the JSON-RPC methods, parameters, POD members etc.

### camelCase COM-RPC methods

camelCase COM-RPC methods names are now allowed and supported by the generators (please note this is not the standard used in the Thunder repositories)

### Enhanced fixed array support

Fixed arrays are now also allowed as method parameters and POD members in interfaces. Please note only primitive types are allowed as already was the case with dynamic arrays. Thunder documentation will be updated soon with more information on this.

### OptionalType enhancements

OptionalType is now also allowed for property indexes and iterators in interfaces. The OptionalType for the property index is for example used in the Thunder Controller interface: if filled it indicates the callsign of the plugin for which info is requested, if omitted (so empty) information for all plugins will be returned.

### null returned for property setter

In case a property setter was used with an array or object it might return something else than null in previous versions of Thunder. The generated documentation however did (correctly) specify null would be returned. This is now also actually the case for the generated code.

# Thunder Interfaces

## Changes and new Features 

### IDolby

IDolby was made backwards compatible again (this was broken in ThunderInterfaces R5.0)
