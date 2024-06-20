# What is Thunder?
!!! note
	The terms "**Thunder**" and "**Thunder**" may be used interchangeably throughout this documentation. The project was originally known as Thunder (since it was developed by the Web Platform for Embedded, or WPE, team).	The name was changed to Thunder when it was incorporated into RDK, but the code still uses the name Thunder internally.

## Introduction

Thunder (aka Thunder) is developed by [Metrological](https://www.metrological.com/) (a Comcast company), and provides a way for STB operators to implement business-logic in a modular way using plugins, and a consistent way for applications to control and query those plugins. By using a plugin-based architecture, it is possible to build a device with only the specific features that are required for that particular device.

!!! tip
	Do not confuse Thunder and [WPEWebKit](https://github.com/WebPlatformForEmbedded/WPEWebKit/). Whilst both are maintained by the Metrological/WPE team, they are not related. [WPEWebKit](https://github.com/WebPlatformForEmbedded/WPEWebKit/) is a fork of the WebKit browser for embedded devices, and shares no code with Thunder

To communicate with plugins, Thunder provides multiple RPC mechanisms which can be chosen depending on which is more appropriate for the client app and the IPC channel. However, a plugin author will simply develop their plugin against an interface and the exact RPC mechanism can be chosen at runtime.

By itself, Thunder does not provide much user-facing functionality, and is expected that plugins are developed to add the required business functionality.

The original aim for Thunder was to provide a bridge between the web-based Javascript world and a native Set Top Box (STB) device. This would allow web apps to query information about the device and invoke device functionality as required.

## Development

Thunder is maintained by a development team at Metrological and Comcast on GitHub. Development takes place in the open on feature branches before being merged to master. Tagged releases are then made from master on a semi-regular basis

### Versioning

Mainline Thunder versions are released by Metrological and are versioned **Rx** (e.g. R2, R3, R4,...). There can be many minor versions in a major release train (e.g. R4.1, R4.2)

Each release is tagged in the git repository and a tagged release has been fully QA tested.

When working with Thunder, it is strongly recommended to take a tagged release. The **master** branch is the active development branch and may be broken in weird and wonderful ways. There is no guarantee of stability on master.

### CI
Builds on Linux and Windows and unit test runs are performed automatically on all PRs using GitHub Actions.

### License

Thunder is Copyright 2018 Metrological and licensed under the Apache License, Version 2.0. See the LICENSE and NOTICE files in the top level directory for further details.

## Relationship with RDK-V

Thunder was developed as a standalone component for use by Metrological in their products before Comcast acquired Metrological. It was the incorporated into the [RDK-V](https://rdkcentral.com/) software stack by Comcast and released as part of RDK-V 4.

Thunder is still used on non-RDK platforms, and the core Thunder framework does not have any coupling to RDK-V. Metrological maintain their own repository of plugins for use on their platforms, and RDK host their own RDKServices repository which contains RDK-V specific Thunder plugins.

RDK-V is currently using a fork of the R2 branch with many changes backported from R3/R4.
