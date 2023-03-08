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
First, change the directory to where you want to build Thunder, and then clone ThunderTools repo:

```Shell
git clone https://github.com/rdkcentral/ThunderTools.git
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
Clone the Thunder repo:

```Shell
git clone https://github.com/rdkcentral/Thunder.git
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
Clone the ThunderInterfaces repo:

```Shell
git clone https://github.com/rdkcentral/ThunderInterfaces.git
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
#### **5. Build ThunderClientLibraries**
Clone the ThunderClientLibraries repo:

```Shell
git clone https://github.com/rdkcentral/ThunderClientLibraries.git
```

Run the following commands to build and then install ThunderClientLibraries:

Note: In the command below, there is a complete list of client libraries that do not require any outside dependencies; therefore, each of them can be successfully built in this simple fashion.

```Shell
cmake -G Ninja -S ThunderClientLibraries -B build/ThunderClientLibraries \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/WPEFramework/Modules" \
-DBLUETOOTHAUDIOSINK=ON \
-DDEVICEINFO=ON \
-DDISPLAYINFO=ON \
-DLOCALTRACER=ON \
-DSECURITYAGENT=ON \
-DPLAYERINFO=ON \
-DPROTOCOLS=ON \
-DVIRTUALINPUT=ON
```

```Shell
cmake --build build/ThunderClientLibraries --target install
```

-------------------------------------------------------------------------------------------
#### **6. Build ThunderNanoServices**
Clone the ThunderNanoServices repo:

```Shell
git clone https://github.com/rdkcentral/ThunderNanoServices.git
```

Run the following commands to build and then install ThunderNanoServices:

Note: In the command below, there is a complete list of plugins that do not require any outside dependencies; therefore, each of them can be successfully built in this simple fashion.

```Shell
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/WPEFramework/Modules" \
-DPLUGIN_COMMANDER=ON \
-DPLUGIN_DHCPSERVER=ON \
-DPLUGIN_DIALSERVER=ON \
-DPLUGIN_DICTIONARY=ON \
-DPLUGIN_FILETRANSFER=ON \
-DPLUGIN_IOCONNECTOR=ON \
-DPLUGIN_INPUTSWITCH=ON \
-DPLUGIN_NETWORKCONTROL=ON \
-DPLUGIN_PROCESSMONITOR=ON \
-DPLUGIN_RESOURCEMONITOR=ON \
-DPLUGIN_SYSTEMCOMMANDS=ON \
-DPLUGIN_SWITCHBOARD=ON \
-DPLUGIN_WEBPROXY=ON \
-DPLUGIN_WEBSERVER=ON \
-DPLUGIN_WEBSHELL=ON
```

```Shell
cmake --build build/ThunderNanoServices --target install
```

-------------------------------------------------------------------------------------------
#### **7. Build ThunderNanoServicesRDK**
Clone the ThunderNanoServicesRDK repo:

```Shell
git clone https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git
```

Run the following commands to build and then install ThunderNanoServicesRDK:

Note: In the command below, there is a complete list of plugins that do not require any outside dependencies; therefore, each of them can be successfully built in this simple fashion.

```Shell
cmake -G Ninja -S ThunderNanoServicesRDK -B build/ThunderNanoServicesRDK \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/WPEFramework/Modules" \
-DPLUGIN_DEVICEIDENTIFICATION=ON \
-DPLUGIN_DEVICEINFO=ON \
-DPLUGIN_LOCATIONSYNC=ON \
-DPLUGIN_MESSAGECONTROL=ON \
-DPLUGIN_MESSENGER=ON \
-DPLUGIN_MONITOR=ON \
-DPLUGIN_OPENCDMI=ON \
-DPLUGIN_PERFORMANCEMETRICS=ON
```

```Shell
cmake --build build/ThunderNanoServicesRDK --target install
```

-------------------------------------------------------------------------------------------
#### **8. Build ThunderUI**
Clone the ThunderUI repo:

```Shell
git clone https://github.com/rdkcentral/ThunderUI.git
```

First, you have to install NodeJS + NPM, and this can be done with the following lines:

```Shell
sudo apt install nodejs
sudo apt install npm
```

Run the following commands to build and then install ThunderUI:

```Shell
cmake -G Ninja -S ThunderUI -B build/ThunderUI \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/WPEFramework/Modules"
```

```Shell
cmake --build build/ThunderUI --target install
```

-------------------------------------------------------------------------------------------
### **Use Thunder**
After everything built and installed correctly, we can run Thunder and use it.

```Shell
LD_LIBRARY_PATH=${PWD}/install/usr/lib:${LD_LIBRARY_PATH} \
PATH=${PWD}/install/usr/bin:${PATH} \
WPEFramework -c ${PWD}/install/etc/WPEFramework/config.json
```

Now, the only thing left is to open a browser and connect to Thunder.

```Shell
http://127.0.0.1:55555
```

-------------------------------------------------------------------------------------------
## Windows Build, using Visual Studio 2019

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
