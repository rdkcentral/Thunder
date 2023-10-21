## Overview

[*ThunderShark*](https://github.com/WebPlatformForEmbedded/ThunderShark) is a tool for analyzing COM-RPC traffic. It allows inspecting the out-of-process communication in a human-friendly presentation. 

In particular it aids: 
- profiling - surveying call duration and frame sizes, 
- debugging - inspecting the parameters and return values passed, 
- flow analysis - investigating the order of calls and notifications, keeping track of interfaces instance's lifetime. 

It can also have an educational value: by showing their internals dissected it lets developers examine how COM frames are built.

*ThunderShark* consists of a dissector plugin for [*Wireshark*](https://www.wireshark.org) written in Lua and an extension to the *ProxyStubGenerator* (called "LuaGenerator") that compiles the 
interface definitions to a format understandable by the plugin. *ThunderShark* is thus completely platform and operating system agnostic.

> *Wireshark* version 4.0 or later is required.

## Prerequisites

Firstly, the file ```protocol-thunder-comrpc.lua``` file needs to be placed in *Wireshark*'s plugins folder. In Windows this is typically ```%APPDATA%\Wireshark\plugins```
or ```%APPDATA%\Roaming\Wireshark\plugins``` folder, while on Linux it's the ```~/.local/lib/wireshark/plugins``` folder.

Secondly, using the [*LuaGanerator*](https://github.com/rdkcentral/ThunderTools/tree/master/LuaGenerator) tool, interface definitions need to be created. 

Typical usage:
```
./GenerateLua.sh [<Thunder_dir> <ThunderInterfaces_dir>]
```
```
LuaGenerator.bat [<Thunder_dir> <ThunderInterfaces_dir>]
```

The tool will produce a ```protocol-thunder-comrpc.data``` file that holds all interface definitions combined and converted to Lua data tables. This file must be
placed in the same folder as ```protocol-thunder-comrpc.lua``` file (i.e. *Wireshark*'s plugins folder). This step should be repeated each time there is a change in the interfaces.

> No instrumentation (code changes) of *Thunder* core or plugins is required!

## Configuration

COM servers that are going to be eavesdropped need to be configured to a socket port that can be easily captured, like TCP/IP. This is typically done by adjusting the appropriate
configuration field.

For example in *Thunder*'s ```config.json```:
```json
"communicator":"127.0.0.1:62000",
```

> Refer to plugin documentation on how to configure custom COM servers provided by plugins (e.g. *OpenCDM*).

This port configuration must be reflected in *ThunderShark*'s config in *Wireshark*: ```Edit/Preferences/Protocols/Thunder COM-RPC Protocol```.

An option to set instance ID size is also provided. It needs to be ensured that it matches the implementation (most often it will be 32-bit long).

## Capture

The COM traffic needs to be captured in a *Wireshark*-readable format, like *pcap*. 

On Linux *tcpdump* is the typical choice, but any other tool able to capture TCP/IP traffic and save it to a *pcap* file can be used. On Windows *Wireshark* itself can be used for this purpose.

Example usage (executed on the DUT, normally before starting Thunder):
```
tcpdump -i lo port 62000 -w /tmp/comrpc-traffic-dump.pcap
```

## Analysis

Once the *pcap* dump is loaded in *Wireshark* it's best to filter for ```thunder-comrpc``` protocol name to display only the relevant messages. Note that by the nature of TCP/IP protocol multiple COM-RPC 
frames can be carried within a single TCP/IP packet and a single COM-RPC frame can be split over multiple TCP/IP packets.

All fields dissected by the plugin can be filtered by, sorted by or searched for. Amongst others, they include: callsign, class, exchange_id for *announce* messages and method name, 
parameters, results, interface instance_id and assigned tag for *invoke* messages. Each message is denoted from which process it originates and which process it addresses. Call and return messages are
tied together and additionally the total call duration is calculated.

> Tags are automatically assigned to instance IDs for convenince (i.e. the user can refer to "Shell_A" instead of actual value like 0x0074c18c).

> The *TimeSync* plugin can alter system time during packet capture – this may break message order in the capture file and COM-RPC call duration calculation. For 100% reliable results the TimeSync plugin should be disabled.

> With current Thunder COM-RPC implementation all ```AddRef()``` and many ```Release()``` calls are optimized away, being piggy-backed on other COM-RPC calls (note *Cached AddRef*, and *Cached Release* fields).

Standard *Wireshark* practices can be used to build display filters and colorizing rules (e.g. to see failed calls use ```thunder-comrpc.invoke.hresult != 0```). Refer to *Wireshark* 
[documentation](https://www.wireshark.org/docs/) for more information about creating filtering rules.

Several shortcuts in ```Tools/ThunderShark``` menu in *Wireshark* main window are provided for convenience.
