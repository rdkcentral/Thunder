-------------------------------------------
JIRA ticket for Session Based JSONRPC describing HowTo:


Process changes:
F - issue template added
C - Unit test improvements
C - Documentation extended and improved (add URL in notes)
C - Seperate QA interfaces



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
- Document plugin subsystem init and de-init sequence changes (METROL-927)
- enums notificaties endata jsonrpc lowecase


- OOP for non OOP plugins (zouden we pas bij 5.1 melden toch?)


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
