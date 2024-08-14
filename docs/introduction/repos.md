Thunder is made up of a number of different GitHub repositories, although you don't need all of them to use Thunder. 

!!! note
	The Thunder Tools repo is only used in versions of Thunder newer than R4.0. The repo contents used to live in the main Thunder repo itself, but was moved out to its own repo for ease of maintenance.

| Repository Name          | URL                                                          | Maintainer       | Description                                                  |
| ------------------------ | ------------------------------------------------------------ | ---------------- | ------------------------------------------------------------ |
| Thunder                  | [https://github.com/rdkcentral/Thunder/](https://github.com/rdkcentral/Thunder/) | Metrological/RDK | Core Thunder repository. Contains the Thunder daemon, core libraries and utilities. |
| Thunder Tools            | [https://github.com/rdkcentral/ThunderTools](https://github.com/rdkcentral/ThunderTools) | Metrological     | Supporting tooling for building Thunder & Thunder plugins. For example, stub and documentation generation. |
| Thunder Interfaces       | [https://github.com/rdkcentral/ThunderInterfaces/](https://github.com/rdkcentral/ThunderInterfaces/) | Metrological/RDK | Interface definitions for plugins                            |
| Thunder Client Libraries | [https://github.com/rdkcentral/ThunderClientLibraries](https://github.com/rdkcentral/ThunderClientLibraries) | Metrological     | C/C++ libraries that can be used for client applications to make it easier to work with some plugins |
| Thunder NanoServices     | [https://github.com/rdkcentral/ThunderNanoServices/](https://github.com/rdkcentral/ThunderNanoServices/) | Metrological     | Thunder plugins developed by Metrological for use on their platforms. **NOT used in RDK** |
| Thunder NanoServicesRDK  | [https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK](https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK) | Metrological     | Metrolgical maintained plugins that are used by both them and RDK. Acts as a staging area for Metrological development before changes land in RDK |
| RDKServices              | [https://github.com/rdkcentral/rdkservices](https://github.com/rdkcentral/rdkservices) | RDK              | Plugins developed and deployed on RDK platforms.             |
| Thunder UI               | [https://github.com/rdkcentral/ThunderUI](https://github.com/rdkcentral/ThunderUI) | Metrological     | Development and test UI that runs on top of Thunder          |
| ThunderOnWindows         | [https://github.com/WebPlatformForEmbedded/ThunderOnWindows](https://github.com/WebPlatformForEmbedded/ThunderOnWindows) | Metrological     | Solution file and headers to build/run Thunder on Windows with Visual Studio |
| ThunderShark             | [https://github.com/WebPlatformForEmbedded/ThunderShark](https://github.com/WebPlatformForEmbedded/ThunderShark) | Metrological     | Wireshark plugin for debugging COM-RPC                       |