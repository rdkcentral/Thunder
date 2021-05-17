# Thunder

A C++ platform abstraction layer for generic functionality.

#Thunder dependencies
After the JsonGenerator.py and StubGenerator.py were modified to run with the python 3.5 version, some action might be required. When using buildroot or yocto, no action is necessary. Upon running these scripts manually or on Windows, make sure python 3.5 or higher is used, like so:
```Shell
$ python --version
```
You might also need to fulfill a requirement of the **jsonref** library, with the following line:
```Shell
$ pip install jsonref
``` 

**Internal plugins**
* [Controller](Source/WPEFramework/doc/ControllerPlugin.md)

## Linux (Desktop) Build
---

These instructions should work on Raspberry PI or any Linux distro. 

Note: All our projects can be custom configured with extra cmake (```-D```) options. To get a list of the posible project specific options add ```-L``` to the ```cmake``` commands below. 

### **Build and install**
---
#### **1. Setup a workspace**
   
```shell
export THUNDER_ROOT=${HOME}/thunder
export THUNDER_INSTALL_DIR=${THUNDER_ROOT}/install
mkdir -p ${THUNDER_INSTALL_DIR}
cd ${THUNDER_ROOT}
```

#### **2. Thunder**
1. Get Source

      ```shell
      git clone https://github.com/rdkcentral/Thunder.git
      ```

2. Build and install tools

      Note: Thunder tools need to be installed before building Thunder

      ```shell
      cmake -HThunder/Tools -Bbuild/ThunderTools \
            -DCMAKE_INSTALL_PREFIX=${THUNDER_INSTALL_DIR}/usr \
            -DCMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake \
            -DGENERIC_CMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake 

      make -C build/ThunderTools && make -C build/ThunderTools install
      ```

3. Build and install Thunder

      Note: -DBUILD_TYPE options are: ```Production/Release/ReleaseSymbols/DebugOptimized/Debug```

      ```shell
      cmake -HThunder -Bbuild/Thunder \
            -DCMAKE_INSTALL_PREFIX=${THUNDER_INSTALL_DIR}/usr \
            -DCMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake \
            -DBUILD_TYPE=Debug -DBINDING=127.0.0.1 -DPORT=55555

      make -C build/Thunder && make -C build/Thunder install
      ```

#### **3. Thunder Interfaces**
   
```shell
git clone https://github.com/rdkcentral/ThunderInterfaces

cmake -HThunderInterfaces -Bbuild/ThunderInterfaces \
      -DCMAKE_INSTALL_PREFIX=${THUNDER_INSTALL_DIR}/usr \
      -DCMAKE_MODULE_PATH=${THUNDER_INSTALL_DIR}/tools/cmake \

make -C build/ThunderInterfaces && make -C build/ThunderInterfaces install
```

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
