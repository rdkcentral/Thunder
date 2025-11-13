# Thunder Release Notes R5.3

## introduction

This document describes the new features and changes introduced in Thunder R5.3 (compared to the latest R5.2 release).
See [here](https://github.com/rdkcentral/Thunder/blob/master/ReleaseNotes/ThunderReleaseNotes_R5.2.md) for the release notes of Thunder R5.2.
This document describes the changes in Thunder, ThunderTools and ThunderInterfaces as they are related. 

# Thunder 

##  Process Changes and new Features 

### windows build artifacts cleaup

### scripts before thunder start in cmake


https://github.com/rdkcentral/Thunder/commit/cbe2b251dcc471033c007497c97fddfa4c0f178a

### action build with performsance monitorgin feature aan


voore meten json rpc perfmance


### copilut scanning and issues fixed


## Changes and new Features

### change process_t pid_t????

deprectated er weer afv en later fixen issue aanmaken om process_t wweer terug te brengen_

### Feature: Warning reporting for websocket open and close

https://github.com/rdkcentral/Thunder/commit/fb56cf6048d5d866f9a3cbed8fe990bc90d0e2d1

### server certiofivcates for TLS 

nu niet melden, foxen we later wel samen


### Change: open comrpc port later

https://github.com/rdkcentral/Thunder/commit/8b78526a13a3d9405c36d6199918660fdc894c26


### make thunder uclib compatible

https://github.com/rdkcentral/Thunder/commit/3307f2554bf84d6f0c487c388d8120d72447de43

### Feature WarningReporting for flowcontrol

https://github.com/rdkcentral/Thunder/commit/8c2c4bdd9f89b6ee9fc9ed06f9988cb33a3fd14c

### Feature close channel on comrpc timeout

https://github.com/rdkcentral/Thunder/commit/5bdfe1fd155f76eaa976784980c639b7e7ff41cd

### COMRPCTerminator thread onlys atarted when needed

One less thread in ThunderPlugin

https://github.com/rdkcentral/Thunder/commit/500d71b6148e9e85386344bdfaca8cd08d3ebf98

### Feature: mac address class

https://github.com/rdkcentral/Thunder/commit/4cb7e2be272cd013bb7ac968806c6bcf5f340f92

### Change: if comrpc string does not fit it will assert and not send any data at all

https://github.com/rdkcentral/Thunder/commit/9986dcb4b67da4b7deea11fd55613ad0ddebfd5f

### Feature: dooir compisit naar remote plugin 

### version hANDLING FOR distribuited  https://github.com/rdkcentral/Thunder/commit/e4c6a3f28807a745c3bc60ee30d6e44c27c4e42c

sebas details

### Change: metadata loading confihurable

laod meta data uitxetten

https://github.com/rdkcentral/Thunder/commit/648fa59038a68fb8f9b404afafc3b5aefe22702b

### Feature: forgiving josn roc Pascal CamelCase handling

https://github.com/rdkcentral/Thunder/commit/fc4ce5fe44862196dd42fe42e629be5fe33eaff9

### Change breaking error message suspend not supported

https://github.com/rdkcentral/Thunder/commit/b338461beff67cf6bf23835a995237b7d82e70fa

### Change: Revoked

Revoked deprecated, weg in 6

https://github.com/rdkcentral/Thunder/commit/6fb08f74b4dfd7e509b8f36d4eefd4080fece05a

## Feature: prio queue

https://github.com/rdkcentral/Thunder/commit/7045853176846b5778f77a9bf4f57e093dcac77b
https://github.com/rdkcentral/Thunder/commit/59a06647baede98f8832a18caae34439ae35e73f
https://github.com/rdkcentral/Thunder/commit/b1e19a25da5ea2f2b0d6aa057a290a83a4be1f57
https://github.com/rdkcentral/Thunder/commit/c2ab8d2d5dab539e0b12999d17440e287ce62cce

ThunderPlugin confihureerbaar maken issue aanmaken? Issue maken

### Feature: syslog on comrpc timeout

https://github.com/rdkcentral/Thunder/commit/768e0765ceaebad616dec1d9a7fce64d0d12cf2c

### Change: fix exists and version and versions

sebas vragen

https://github.com/rdkcentral/Thunder/commit/7eb4b83541307fb11e39a2a0ed842730ebdf50ec
https://github.com/rdkcentral/Thunder/commit/41a3d4d33ced679e034be9fa27adc645771f9b65
https://github.com/rdkcentral/Thunder/commit/e6cc6520f6df854dd32e4a6b1b2046031448494f
https://github.com/rdkcentral/Thunder/commit/f4c077e2db2ba3d1edff86e644f8ca193c703c1f
https://github.com/rdkcentral/Thunder/commit/4f858a0b1a920d1282372f0ed5b99d92883dba4d

###

Library and service discovery improvents: https://github.com/rdkcentral/Thunder/commit/d500fd921ffb42fb526d0c9a2bce05e9bca38831
Service does no need to be unique anymorre


### Feature: allow per callsign registartionin IShell

https://github.com/rdkcentral/Thunder/commit/348dd900d3e52093e4e0f4ab88c8d49ba2a79418

""Add optional callsign to Register, notify Activated in Register" laatste deden we toch al

Eh, hebben we hibernated nu ook doorgekoppeld... nope (en moet destroyed ook, moeten we dit nog voor 5.3 doen?)

aLso Smart types updated to use this feature

### Change: TriState class removed

Not used, more modern variants available (e.g. optional<bool>)

### Change: Acivate reason

https://github.com/rdkcentral/Thunder/commit/06c633faceab10e2fdd0b1f6e1a618eec0e40a27

### Feature: allow per callsign in IController

sebas vragen voor details?

breaked backward compagtibility?? (zie PR wel intern bwd comp) ook meer snapshots (ben ff vergeten wat we ook alweer hadden afgsproken)

https://github.com/rdkcentral/Thunder/commit/bd5b93dd7dcec748e0bdadbf0494b71f32705482#diff-917c688c2223748d9405b9a33fbf1a47f2b1029699919261ebb12fad392026c7
https://github.com/rdkcentral/Thunder/commit/5ed8aef8a9d4a7a539ae5cd65719dcf16d41fc5e

### Feature: loggign in casze second comrpc call on chennel deadlock

https://github.com/rdkcentral/Thunder/commit/8da778df7a574b8e6872ef31d1c2c458689bb0f5

### Feature: Sink now has WaitReleased

https://github.com/rdkcentral/Thunder/commit/3f85bc5d2adc2da705603f94b9797dc408ecaf93

### Change: imporved dangling proxies notification

https://github.com/rdkcentral/Thunder/commit/bd88dec72296bc867b732b78ed8539d2103dd52f

now allowed  via comrpc call


### Change: THUNDER_PERFORMANCE 

working again

https://github.com/rdkcentral/Thunder/commit/5c281b3514bafc3d1bdc9cd6840aa2e1a2e13fb2_

### Change: thrttling configurable

Default hal;pf worerpool thread (moeten we hoer nog met de priority ppol rekening houden?)
https://github.com/rdkcentral/Thunder/commit/d28c40a9dc948a33bf02c3a7ddd255f357d7a9ed

https://github.com/rdkcentral/Thunder/commit/eea443cde3827ec3a44fcc8790176f39bf674737#diff-cb405958fdffcb85a8aeaa1bd1204df978e2525eb9bbab4db014266c39825d73

### Change: connector timelout setable

In code, makes smartlinktype timeout for xonnwection possible

### Change: General bug fixes

A number of issues found in Thunder 5.2 have been corrected in Thunder 5.3.

## Breaking Changes

Although Thunder 5.3 is a minor upgrade and therefore should not contain breaking changes there is one however. This due to a request to the Thunder team to change the behavior of the ThunderTooling regarding the default casing generated for the JSON-RPC API.
In itself this change in the ThunderTooling should not lead to different behaviour in Interfaces used in RDKServices as far as we are aware. The old Thunder Interfaces used in RDKServices were all adapted to generate the exact same interface as in 5.1 and others used different @text options already to force the behaviour that has now become the default.

### X


# Thunder Tools

## Changes and new Features 

### Feature: support status listeners for lookup objects

https://github.com/rdkcentral/ThunderTools/commit/2da9dd7895b7c9961c2548f5be37cc9168ab8d03

See [here](https://rdkcentral.github.io/Thunder/plugin/interfaces/interfaces/#object-lookup) for more info and how to use this.

### StatusListener closedc channel 

https://github.com/rdkcentral/Thunder/commit/bc8965bf8eded200c3bdcacf9025624b0980ca1a

### Feature: new buffer encoding options

https://github.com/rdkcentral/ThunderTools/commit/ef7940f4ed23e3f7cbd2cff99840cae5ee2d165e

now also supported in events: https://github.com/rdkcentral/ThunderTools/commit/b7203bd5d4c4bb9a1da82bae75ce9ef94e2fa768

### Feature: New supported type: class MACAddress

https://github.com/rdkcentral/ThunderTools/commit/7e3bc68e46dd5ce8842c909cdee01d7034fac7b3

### Feature: Use string instance id 

https://github.com/rdkcentral/ThunderTools/commit/4ab8b4a458233e4461e87f5ffaa4f8d3b9436734

### Feature: support custom object lookup

https://github.com/rdkcentral/ThunderTools/commit/be4ad6be7299b4992218ace5b9947e91ca66783f

https://github.com/rdkcentral/Thunder/commit/8280f2ac5d38b87bbd5fc3ceb000e0dae9beefd6

https://github.com/rdkcentral/Thunder/commit/58f642c8e75906a2a0b9797fc6288cbbae541604

https://github.com/rdkcentral/Thunder/commit/a6645b4d85c96c60fe295d12226aa40e8f9ad265

### Feature: serialize non empty arrays and containers

https://github.com/rdkcentral/ThunderTools/commit/0304e4519178956048b6cc9ecf3cd05a38460a49

### Feature: encode:lookup

https://github.com/rdkcentral/ThunderTools/commit/b19295114ba37be801464407aa31ebf6bf8e5e44


### include enum/struct from other header file??????


### Feature: eneble enum conversion generatrion for non json rpc interfaces

https://github.com/rdkcentral/ThunderTools/commit/e20510ef35356856f7398a7441cd25658efbc65c

### Feature: support ootional iterators:

https://github.com/rdkcentral/ThunderTools/commit/49dec3e3ad8b108797e329ae5839d9c364ec93ee

### Feature: support restrict for iterators

https://github.com/rdkcentral/ThunderTools/commit/c8f5665a55d0392cc2f91c5d04f4dbab1834fb34

### Feature: allow only lower bound restrict for strings

https://github.com/rdkcentral/ThunderTools/commit/4354bda52520bb43519fb125495fa1e59ebdf104

(https://github.com/rdkcentral/ThunderTools/commit/37a5f43d0f2d54d7dc4bdf45348f268ff6a00677)

### Feature: colated iterators

https://github.com/rdkcentral/ThunderTools/commit/b87925af70680181ed66b9754f2273d0c25b9d6b

### Feature: build in methods in documentation:

https://github.com/rdkcentral/ThunderTools/commit/a47fbcb56a9e555219bc1cc47411643474c1f265

### Feature: event index with @

https://github.com/rdkcentral/ThunderTools/commit/01924d640d2f3f8ff6d9ff49615540063b6fb9a8

also mention deprecated


### Beta Feature: PSG with interface parsing

### *** tags ***

https://github.com/rdkcentral/ThunderTools/commit/1cbab5907240b9115de13530f8cdc776a7e69358

 @optional              - indicates that a parameter, struct member or property index is optional (deprecated, use Core::OptionalType instead)")
        print("   @default {value}       - set a default value for an optional parameter")
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

