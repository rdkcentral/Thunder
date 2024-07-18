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

The existing Thunder unittests were improved and new tests were added. These are also triggered from a GitHub action on each merge of a Pull Request.

### Change: Thunder Documentation

The Thunder documentation was extended with new content, please find the documentation [here](https://rdkcentral.github.io/Thunder/). More content still to come!

### Change: QA interfaces

To make it clear what interfaces are specifically added for QA purposes these have been put into a separate folder and namespace. They can be found [here](https://github.com/rdkcentral/ThunderInterfaces/tree/master/qa_interfaces) 

## Major Changes and new Features

### JSON-RPC non happy day scenarios
bla die bla die bla

## Minor Changes and new Features 

### Installation and Cryptography subystem
sjdklsadjklsaD

### General bug fixes

A lot of issues were fixed in Thunder R5.0 improving stability and resource usage. 

## Breaking Changes

### WPEFramework renamed to Thunder
dsajklsaDJSAL


# Thunder Tools


-------------------------------------------





Major changes/features
F - json rpc non happy day (messenger example)
F - Private COM-RPC
F - Off host plugin support
F - Aggregated plugin support
F - Thundershark [Application]
F - proxy leakage detection and reporting
F - MacOS support
F - IController suspend en resume via IController ILifetime)
     Moving from JSONRPC/COMRPC specification -> COMRPC specification only
F - Make delegated release configurable. Unbalanced reference counted objects
    are for the non-happyday scenarios now automatically destructed. This can lead to 
	unexpected crashes. This automatic cleanup feature can be turned off.


Minor changes/features
F - Installtion and Cryptography subystem added (backport R4)
F - Mesaging enhancement 
		stdin/stdout added.
C - messagecontrol efficiency improvements 
C - Messaging no longer fwd messages when -f-F
C - fixes
C - yocto meta lagen (Refactored to latest CMake standard)
C - Plugin/interface versioning improved
	Metadata is now also inserted through the COMRPC definitions. 
C - json (de)serializer improvements (e.g. hex strings, utf-8 handling, escaping) (backport to R4)
C - websocket improvements, WebSocket: add masking bytes also while enabling MASKING_FRAME flag
C - Higher warning level and warning fixed (also GCC12)
C - Verify SSL certificates by default. Validate the SNI against the root-certificates installed on the box.


Breaking
C - Rename (WPEFramework -> Thunder)
    Mitigation measures: namespace, binaries, build files and names etc.
C - Plugin/interface versioning (Breaking interface change Version is now an object in Plugin metadata)
C - JSONRPC error codes aligned with the specification (expect different error-codes)
C - JSONRPC suspending and resuming plugins is now done through a dedicated method, no longer possible through the state properties.
C - Old config meta files weg bij Thunder repos (nog wel gesupport volgende versie weg) nieuwe msanier is nu de default
C - auto start (boolean) and resume (boolean) -> Mode[Unavailable,Activated,Deactivated,Resumes,Suspend] reason for change is the added Unavailable.
C - naming alignment, there where we used: MetaData -> Metadata. (Casing)
C - pluging status via controller always an array (can still request 1 plugin but it is enclosed in an array)!
C - Addref has return value (backport R4, should cause no issues should not be used)
C - JSONRPC event forwarding (Controller all event): nolonger wrapped in an additional layer called Data.

TO BE CHECKED!
- automatic deinitialize after failed initialize is now default (hadden we toc al gechecked was in R4 al zo??

-> yes dus al in R4, niet opnemen nu

- Document plugin subsystem init and de-init sequence changes (METROL-927)

-> Nog niet in code, niet opnemen nu

- enums notificaties en data jsonrpc lowecase

-> lijkt niet te zijn veranderd



Tools
- keep: thingies (backp 4)
- support for nested PODs (backp 4)
- core::time supported
- Support Context passing and index in notifications
- length support
- Intruduce length:return 
- Support JSON::InstanceId
- Add @extract keyword (bitwise mapping op enums ->  array)
- @text options
- Support multiline description tags
- Support POD inheritance 
- float support
- code generation warning cleanup
-  Support multiple methods for a single JSON event
- Support iterators in events
- default support in interfaces
- optional support in interfaces


NanoServices
C - Messenger adopted for R5 json session support
C - webkit 2.42 support
F - Doggo
F - Mesa compositor
F - NetworkControl, persist MAC address for NIC..
F - Introduce VaultProvisioning (#752)F 

* Breaking bwd comp
- network config changed

- ThunderUI
- updated for R5 changes

- ClientLibraries:
- Adapted for the Realtek (separated pipelines, audio/video) 
- Bluetooth audio in/out maybe interesting to add!
