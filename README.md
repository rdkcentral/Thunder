# Thunder
A C++ platform abstraction layer for generic functionality.

It can be built successfully on both Linux and Windows. First, make sure you have the dependencies sorted out, and then go to the proper OS section:
* [Linux Build](#Linux)
* [Windows Build](#Windows)

-------------------------------------------------------------------------------------------
## Dependencies
After JsonGenerator.py and StubGenerator.py were modified to run with the Python 3.5 version, some additional actions might be required. However, no additional actions are necessary while using Buildroot or Yocto. When running these scripts manually or on Windows, make sure that Python 3.5 or higher is used:

```Shell
python --version
```

You must also satisfy the requirement of having the **jsonref** library installed. This can be accomplished with the following command:

```Shell
pip install jsonref
```

-------------------------------------------------------------------------------------------
**Internal plugins**
* [Controller](Source/WPEFramework/doc/ControllerPlugin.md) - activates and deactivates the configured plugins.

-------------------------------------------------------------------------------------------
<a name="Linux"></a>
## Linux (Desktop) Build
These instructions should work on Raspberry PI or any Linux distribution.

Note: All our projects can be customized with additional cmake options ```-D```. To obtain a list of all possible project-specific options, add ```-L``` to the ```cmake``` commands below.

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
-DPLUGIN_WEBSHELL=ON \
-DPLUGIN_WIFICONTROL=ON
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
After everything has been built and installed correctly, we can run Thunder.

```Shell
LD_LIBRARY_PATH=${PWD}/install/usr/lib:${LD_LIBRARY_PATH} \
PATH=${PWD}/install/usr/bin:${PATH} \
WPEFramework -c ${PWD}/install/etc/WPEFramework/config.json
```

Now, the only thing left is to open a browser and connect to the framework:

```Shell
http://127.0.0.1:55555
```

You should see a page similar to this one:

<img src="https://i.imgur.com/0zBG9FJ.png" width="700">

Note: All logs will be displayed in the command window, which can be very useful for debugging purposes. To close the framework, press ```q``` and then ```enter```.

-------------------------------------------------------------------------------------------
<a name="Windows"></a>
## Windows Build

To build Thunder and its components on Windows, you will need Visual Studio installed.

The main solution file with all projects and their dependencies can be found in the ThunderOnWindows repo. Besides, this repository holds the binaries and the header files required to build the Thunder framework on Windows.

Just like in case of Linux, the first step is to clone everything. However, the main difference on Windows will be that you need to checkout ThunderOnWindows first, and then every other repo will go into it. Therefore, the structure should be like this:

* ThunderOnWindows
  + Thunder
  + ThunderClientLibraries
  + ThunderInterfaces
  + ThunderNanoServices
  + ThunderNanoServicesRDK
  + ThunderTools
  + ThunderUI

Make a dedicated folder called ```ThunderWin``` directly on the drive ```C:\```, clone ThunderOnWindows into it and change the directory.

```Shell
mkdir C:\ThunderWin
cd C:\ThunderWin
git clone https://github.com/WebPlatformForEmbedded/ThunderOnWindows.git
cd ThunderOnWindows
```

Then, clone the remaining repos.

```Shell
git clone https://github.com/rdkcentral/ThunderTools.git
git clone https://github.com/rdkcentral/Thunder.git
git clone https://github.com/rdkcentral/ThunderInterfaces.git
git clone https://github.com/rdkcentral/ThunderClientLibraries.git
git clone https://github.com/rdkcentral/ThunderNanoServices.git
git clone https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git
git clone https://github.com/rdkcentral/ThunderUI.git
```

The next step is to open the solution file ```ThunderOnWindows\Thunder.sln``` in  Visual Studio, right click on ```Solution Thunder``` and build it.

Note: This will build all project files in a similar order to the Linux cmake build. If you are interested in building only a specific part of Thunder, for example, just ThunderInterfaces, you can build only the ```Interfaces``` project file and it will automatically build its dependencies, so in this case ```bridge```. 

After the building process is finished, you still need to make a few adjustments before running Thunder. One of them is to create a volatile and a persistent directory in a specific location, this can be done with the following commands:

```Shell
mkdir ..\artifacts\temp
mkdir ..\artifacts\Persistent
```

Then, you must move two dlls with libs into the artifacts folder, which can be done with these commands:

```Shell
move lib\static_x64\libcrypto-1_1-x64.dll ..\artifacts\Debug\libcrypto-1_1-x64.dll
move lib\static_x64\libssl-1_1-x64.dll ..\artifacts\Debug\libssl-1_1-x64.dll
```

Next, to use ThunderUI on Windows, copy it into the artifacts folder. This can be done with the following line:

```Shell
robocopy ThunderUI\dist ..\artifacts\Debug\Plugins\Controller\UI /S
```

Now, right click on ```bridge``` project file and select ```Properties```. Go into ```Debugging``` tab, and put the following line into ```Command Arguments```:

```Shell
-c "$(ProjectDir)ExampleConfigWindows.json"
```

<img src="https://i.imgur.com/pbgAXSV.png" width="700">

Apply the changes, press ```F5``` to start Thunder, and open the link below in your browser. A page similar to the one in Linux build should be displayed.

```Shell
http://127.0.0.1:25555
```
