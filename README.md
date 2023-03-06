# Thunder
A C++ platform abstraction layer for generic functionality.

-------------------------------------------------------------------------------------------
## Dependencies
After JsonGenerator.py and StubGenerator.py were modified to run with the Python 3.5 version, some additional actions might be required. However, no extra actions are necessary while using Buildroot or Yocto. When running these scripts manually or on Windows, make sure that Python 3.5 or higher is used:

```Shell
python --version
```

You must also satisfy the requirement of having the **jsonref** library installed. This can be accomplished with the following command:

```Shell
pip install jsonref
```

-------------------------------------------------------------------------------------------
**Internal plugins**
* [Controller](Source/WPEFramework/doc/ControllerPlugin.md)

-------------------------------------------------------------------------------------------
## Linux (Desktop) Build
These instructions should work on Raspberry PI or any Linux distribution.

Note: All our projects can be customized with additional cmake options ```-D```. To obtain a list of possible project-specific options, add ```-L``` to the ```cmake``` commands below.

-------------------------------------------------------------------------------------------
### **Build and install**

#### **1. Install necessary packages**
```Shell
sudo apt install build-essential cmake ninja-build libusb-1.0-0-dev zlib1g-dev libssl-dev
```

-------------------------------------------------------------------------------------------
#### **2. Build ThunderTools**
First, create a new folder called ```ThunderTools```, and then clone ThunderTools repo into it:

```Shell
mkdir ThunderTools
cd ThunderTools
git clone https://github.com/rdkcentral/ThunderTools.git
cd ..
```

Next, we just need to run the following commands to build and then install the generators inside ThunderTools:

Note: The cmake commands can contain many options, so it is convenient to format them into separate lines with ```\```.

```Shell
cmake -G Ninja -S ThunderTools -B build/ThunderTools \
-DCMAKE_INSTALL_PREFIX="install/usr"
```

```Shell
cmake --build build/ThunderTools --target install
```

-------------------------------------------------------------------------------------------
#### **3. Build Thunder**
Create a new folder called ```Thunder```, and then clone the Thunder repo into it:

```Shell
mkdir Thunder
cd Thunder
git clone https://github.com/rdkcentral/Thunder.git
cd ..
```

Run the following commands to build and then install Thunder:

Note: The available ```-DBUILD_TYPE``` options are: ```[Debug, Release, MinSizeRel]```.

```Shell
cmake -G Ninja -S Thunder -B build/Thunder \
-DBINDING="127.0.0.1" \
-DCMAKE_BUILD_TYPE="Debug" \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/WPEFramework/Modules" \
-DDATA_PATH="${PWD}/install/usr/share/WPEFramework" \
-DPERSISTENT_PATH="${PWD}/install/var/wpeframework" \
-DPORT="55555" \
-DPROXYSTUB_PATH="${PWD}/install/usr/lib/wpeframework/proxystubs" \
-DSYSTEM_PATH="${PWD}/install/usr/lib/wpeframework/plugins" \
-DVOLATILE_PATH="tmp"
```

```Shell
cmake --build build/Thunder --target install
```

-------------------------------------------------------------------------------------------
#### **4. Build ThunderInterfaces**
Create a new folder called ```ThunderInterfaces```, and then clone the ThunderInterfaces repo into it:

```Shell
mkdir ThunderInterfaces
cd ThunderInterfaces
git clone https://github.com/rdkcentral/ThunderInterfaces.git
cd ..
```

Run the following commands to build and then install ThunderInterfaces:

```Shell
cmake -G Ninja -S ThunderInterfaces -B build/ThunderInterfaces \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/WPEFramework/Modules"
```

```Shell
cmake --build build/ThunderInterfaces --target install
```

-------------------------------------------------------------------------------------------
#### **4. Thunder Client Libraries**
   
```shell
git clone https://github.com/rdkcentral/ThunderClientLibraries.git

cmake -HThunderClientLibraries -Bbuild/ThunderClientLibraries \
      -DCMAKE_INSTALL_PREFIX=${THUNDER_INSTALL_DIR}/usr \
      -DCMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake \
      -DVIRTUALINPUT=ON

make -C build/ThunderClientLibraries && make -C build/ThunderClientLibraries install
```
#### **5. Thunder WebUI**

Note: The WebUI is not supporting a full out of tree build. 

```shell
git clone https://github.com/rdkcentral/ThunderUI.git

cmake -HThunderUI -BThunderUI/build \
      -DCMAKE_INSTALL_PREFIX=${THUNDER_INSTALL_DIR}/usr \
      -DCMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake \

make -C ThunderUI/build && make -C ThunderUI/build install
```

#### **6. Thunder Nano Services**

```shell
git clone https://github.com/rdkcentral/ThunderNanoServices.git

cmake -HThunderNanoServices -Bbuild/ThunderNanoServices \
      -DCMAKE_INSTALL_PREFIX=${THUNDER_INSTALL_DIR}/usr \
      -DCMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake \
      -DPLUGIN_DICTIONARY=ON

make -C build/ThunderNanoServices && make -C build/ThunderNanoServices install
```
### **Use Thunder**
---
1. Run 
      ```shell
      PATH=${THUNDER_INSTALL_DIR}/usr/bin:${PATH} \
      LD_LIBRARY_PATH=${THUNDER_INSTALL_DIR}/usr/lib:${LD_LIBRARY_PATH} \
      WPEFramework -c ${THUNDER_INSTALL_DIR}/etc/WPEFramework/config.json
      ```
2. Open ```http://127.0.0.1:55555``` in a browser.

## WINDOWS Build, using Visual Studio 2019
---

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
