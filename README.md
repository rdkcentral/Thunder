# Thunder

A C++ platform abstraction layer for generic functionality.

### Thunder dependencies
After the JsonGenerator.py and StubGenerator.py were modified to run with the python 3.5 version, some action might be required. When using buildroot or yocto, no action is necessary. Upon running these scripts manually or on Windows, make sure python 3.5 or higher is used, like so:
```Shell
$ python --version
```
You might also need to fulfill a requirement of the **jsonref** library, with the following line:
```Shell
$ pip install jsonref
``` 

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
C:\Users\Me\Thunder 
C:\Users\Me\ThunderNanoServices

Than to run, the config file is assuming that all is installed on W: 
You can set this by mapping the artifacts directory created, to the 
W drive. A typical use case would be:

net use w: "\\localhost\c$\Users\Me\Thunder\artifacts"

Before the first run make sure you creat a volatile and a peritent directory
here mkdir w:\Temp and mkdir w:\Persistent.

If you want to use the Thunder UI page, copy the ThunderUI\build\* to 
W:\<build type>\Controller\UI

Depending on the filesystem used by the windows OS, symbolic links are supported 
or not. There is 1 Symbolic link in the Thuder repository (Thunder/Source/tools
-> "../Tools"). If this symbolic link does not exist, you will experience build 
errors (error 2, GenrateProxyStub.py not found). 
or create a copy of the Tools directory in Source/tools (xcopy /d ..\Tools tools)
or create the symlink manually (mklink /D tools "..\Tools")


TEST
