# Thunder

A C++ platform abstraction layer for generic functionality.

### Thunder Options
**Platforms:**
* INTELCE
* RPI
* PC_UNIX
* PC_WINDOWS
* EOS
* DAWN
* Xi5

**Thunder options:**
*  -DCORE=ON ,Include the generics library.
*  -DCRYPTALGO=ON ,Include the encyption algorithm library.
*  -DPROTOCOLS=ON ,Include the protocols library.
*  -DTRACING=ON ,Include the tracing library.
*  -DPLUGINS=ON ,Include the plugin library.
*  -DDEBUG=ON ,Enable debug build for Thunder.
*  -DWCHAR_SUPPORT=ON ,Enable support for WCHAR in Thunder.

**Internal plugins**
* [Controller](Source/WPEFramework/ControllerPlugin.md)

**WINDOWS Build, using Visual Studio 2019**
The default solution is setup in such away that it can run and load 
the Thunder and Thunder Nano Services (capable of running in Windows)
if both repositories are next to each other. So checkout Thunder on 
disk and put the ThunderNanoServices next to it.

e.g.
C:\User\Me\Thunder 
C:\User\Me\ThunderNanoServices

Than to run, the config file is assuming that all is installed on W: 
You can set this by mapping the artifacts directory created, to the 
W drive. A typical use case would be:

net use w: "\\localhost\c$\Users\Pierre Wielders\Thunder\artifacts"

Before the first run make sure you creat a volatile and a peritent directory
here mkdir w:\Temp and mkdir w:\Persistent.

If yuo want to use the Thunder UI page, copy the ThunderUI\build\* to 
W:\<build type>\Controller\UI