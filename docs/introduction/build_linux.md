These instructions are written for Ubuntu 22.04, but should work on the Raspberry Pi or any other Linux distribution.

These instructions are designed for Thunder R4 or newer. Older versions of Thunder may differ.

!!! note
	The Thunder projects can be customized with additional cmake options `-D`. To obtain a list of all possible project-specific options, add `-L` to the `cmake` commands below.
	
	The cmake commands can contain many options, so it is convenient to format them into separate lines with `\`.

## Build & Installation

The following instructions will use the `CMAKE_INSTALL_PREFIX` option to install Thunder in a subdirectory of the current working directory instead of in the system-wide directories. This is recommended for development, but for production use you may need to change/remove this option to install Thunder in standard linux locations.

### 1.  Install Dependencies

These instructions are based on Ubuntu 22.04 - you may need to change this for your distros package manager

```
sudo apt install build-essential cmake ninja-build libusb-1.0-0-dev zlib1g-dev libssl-dev
```

Thunder also uses Python 3 for code and documentation generation scripts. Ensure you have at least **Python 3.5** installed and install the [**jsonref**](https://pypi.org/project/jsonref/) library with pip:

```
pip install jsonref
```

### 2. Build  Thunder Tools

Thunder Tools are various scripts and code generators used to build Thunder and any plugins. In older Thunder versions, they lived inside the main Thunder repo but have now been moved to their own repo.

First, change the directory to where you want to build Thunder.

Then clone ThunderTools repo:
```shell
git clone https://github.com/rdkcentral/ThunderTools.git
```
Next, we need to run the following commands to build and then install the code generators inside ThunderTools:
```shell
cmake -G Ninja -S ThunderTools -B build/ThunderTools -DCMAKE_INSTALL_PREFIX="install/usr"

cmake --build build/ThunderTools --target install
```

### 3. Build Thunder

Clone the Thunder repo:

```shell
git clone https://github.com/rdkcentral/Thunder.git
```

Run the following commands to build and then install Thunder. The available `-DCMAKE_BUILD_TYPE` options are: `[Debug, Release, MinSizeRel]`.

```shell
cmake -G Ninja -S Thunder -B build/Thunder \
-DBUILD_SHARED_LIBS=ON \
-DBINDING="127.0.0.1" \
-DCMAKE_BUILD_TYPE="Debug" \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules" \
-DDATA_PATH="${PWD}/install/usr/share/Thunder" \
-DPERSISTENT_PATH="${PWD}/install/var/thunder" \
-DPORT="55555" \
-DPROXYSTUB_PATH="${PWD}/install/usr/lib/thunder/proxystubs" \
-DSYSTEM_PATH="${PWD}/install/usr/lib/thunder/plugins" \
-DVOLATILE_PATH="tmp"

cmake --build build/Thunder --target install
```

### 4. Build ThunderInterfaces

!!! note
	The ThunderInterfaces repo is tagged/versioned the same as the main Thunder repo. So if you are using Thunder R4.1 for example, you should also use the R4.1 tag of ThunderInterfaces

Clone the ThunderInterfaces repo:

```shell
git clone https://github.com/rdkcentral/ThunderInterfaces.git
```

Run the following commands to build and then install ThunderInterfaces:

```shell
cmake -G Ninja -S ThunderInterfaces -B build/ThunderInterfaces \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules"

cmake --build build/ThunderInterfaces --target install
```

### 5. Build Plugins

The exact repo and steps here will depend on exactly which plugins you want to build. 

#### ThunderNanoServices

Clone the ThunderNanoServices repo:
```shell
git clone https://github.com/rdkcentral/ThunderNanoServices.git
```

Run the following commands to build and then install ThunderNanoServices. 

In the command below, there is a complete list of plugins that do not require any outside software/hardware dependencies. However, you should customise this to include the plugins you require for your platform.

```shell
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules" \
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

cmake --build build/ThunderNanoServices --target install
```

#### ThunderNanoServicesRDK

Clone the ThunderNanoServicesRDK repo:

```
git clone https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git
```

Run the following commands to build and then install ThunderNanoServicesRDK.

In the command below, there is a complete list of plugins that do not require any outside software/hardware dependencies. However, you should customise this to include the plugins you require for your platform.

```shell
cmake -G Ninja -S ThunderNanoServicesRDK -B build/ThunderNanoServicesRDK \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules" \
-DPLUGIN_DEVICEIDENTIFICATION=ON \
-DPLUGIN_DEVICEINFO=ON \
-DPLUGIN_LOCATIONSYNC=ON \
-DPLUGIN_MESSAGECONTROL=ON \
-DPLUGIN_MESSENGER=ON \
-DPLUGIN_MONITOR=ON \
-DPLUGIN_OPENCDMI=ON \
-DPLUGIN_PERFORMANCEMETRICS=ON

cmake --build build/ThunderNanoServicesRDK --target install
```

### 6. Build Thunder Client Libraries (optional)

If you require a convenience library from the ThunderClientLibraries repo, follow the below steps:

Clone the ThunderClientLibraries repo:

```
git clone https://github.com/rdkcentral/ThunderClientLibraries.git
```

Run the following commands to build and then install ThunderClientLibraries:

In the command below, there is a complete list of client libraries that do not require any outside dependencies; therefore, each of them can be successfully built in this simple  fashion.

```shell
cmake -G Ninja -S ThunderClientLibraries -B build/ThunderClientLibraries \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules" \
-DBLUETOOTHAUDIOSINK=ON \
-DDEVICEINFO=ON \
-DDISPLAYINFO=ON \
-DLOCALTRACER=ON \
-DSECURITYAGENT=ON \
-DPLAYERINFO=ON \
-DPROTOCOLS=ON \
-DVIRTUALINPUT=ON

cmake --build build/ThunderClientLibraries --target install
```

### 7. Build Thunder UI (optional)

Clone the ThunderUI repo:

```shell
git clone https://github.com/rdkcentral/ThunderUI.git
```

First, you have to install NodeJS + NPM, and this can be done with the following command:

```shell
sudo apt install nodejs npm
```

Run the following commands to build and then install ThunderUI:

```shell
cmake -G Ninja -S ThunderUI -B build/ThunderUI \
-DCMAKE_INSTALL_PREFIX="install/usr" \
-DCMAKE_MODULE_PATH="${PWD}/install/usr/include/Thunder/Modules"

cmake --build build/ThunderUI --target install
```


## Run Thunder

After everything has been built and installed correctly, we can run Thunder.

Since we installed Thunder in a custom installation directory, we need to provide an `LD_LIBRARY_PATH` to that location and set `PATH` to include the `bin` directory. If the libraries are installed in system-wide locations (e.g. `/usr/lib` and `/usr/bin`) then those environment variables are not required

```shell
export LD_LIBRARY_PATH=${PWD}/install/usr/lib:${LD_LIBRARY_PATH}
export PATH=${PWD}/install/usr/bin:${PATH}

$ Thunder -f -c ${PWD}/install/etc/Thunder/config.json
```

The following arguments should be specified to the Thunder binary:

* `-f`: Flush plugin messages/logs directly to the console - useful for debugging. In production, you should use the `MessageControl` plugin to forward messages to a suitable sink. 
* `-c`: Path to Thunder config file

All being well, you should see Thunder start up:

```
[Tue, 06 Jun 2023 10:04:31]:[PluginHost.cpp:584]:[main]:[Startup]: Thunder
[Tue, 06 Jun 2023 10:04:31]:[PluginHost.cpp:585]:[main]:[Startup]: Starting time: Tue, 06 Jun 2023 09:04:31 GMT
[Tue, 06 Jun 2023 10:04:31]:[PluginHost.cpp:586]:[main]:[Startup]: Process Id:    25382
[Tue, 06 Jun 2023 10:04:31]:[PluginHost.cpp:587]:[main]:[Startup]: Tree ref:      engineering_build_for_debug_purpose_only
[Tue, 06 Jun 2023 10:04:31]:[PluginHost.cpp:588]:[main]:[Startup]: Build ref:     engineering_build_for_debug_purpose_only
[Tue, 06 Jun 2023 10:04:31]:[PluginHost.cpp:589]:[main]:[Startup]: Version:       4:0:0
...
[Tue, 06 Jun 2023 10:04:32]:[PluginHost.cpp:609]:[main]:[Startup]: Thunder actively listening
```

If you followed these instructions, Thunder will be listening for web requests on `localhost:55555`

To exit the framework, press `q` and then `enter`.
