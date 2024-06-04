<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Controller_Plugin"></a>
# Controller Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

Controller plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Controller plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.FQDN">FQDN</a> | Fully-Qualified Domain Name |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |
| <a name="acronym.SSDP">SSDP</a> | Simple Service Discovery Protocol |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Controller*) |
| classname | string | Class name: *Controller* |
| locator | string | Library name: *(built-in)* |
| startmode | string | Determines in which state the plugin should be moved to at startup of the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- Controller::ISystem ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- Controller::IDiscovery ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- Controller::IConfiguration ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (uncompliant-extended format)
- Controller::ILifeTime ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- Controller::ISubsystems ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (uncompliant-collapsed format)
- Controller::IEvents ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- Controller::IMetadata ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Controller plugin:

Controller System interface methods:

| Method | Description |
| :-------- | :-------- |
| [reboot](#method.reboot) / [harakiri](#method.reboot) | Reboots the device |
| [delete](#method.delete) | Removes contents of a directory from the persistent storage |
| [clone](#method.clone) | Creates a clone of given plugin with a new callsign |

Controller Discovery interface methods:

| Method | Description |
| :-------- | :-------- |
| [startdiscovery](#method.startdiscovery) | Starts SSDP network discovery |

Controller Configuration interface methods:

| Method | Description |
| :-------- | :-------- |
| [persist](#method.persist) / [storeconfig](#method.persist) | Stores all configuration to the persistent memory |

Controller LifeTime interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method.activate) | Activates a plugin |
| [deactivate](#method.deactivate) | Deactivates a plugin |
| [unavailable](#method.unavailable) | Makes a plugin unavailable for interaction |
| [hibernate](#method.hibernate) | Hibernates a plugin |
| [suspend](#method.suspend) | Suspends a plugin |
| [resume](#method.resume) | Resumes a plugin |

<a name="method.reboot"></a>
## *reboot [<sup>method</sup>](#head.Methods)*

Reboots the device.

> ``harakiri`` is an alternative name for this method. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Description

Depending on the device this call may not generate a response.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.reboot"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.delete"></a>
## *delete [<sup>method</sup>](#head.Methods)*

Removes contents of a directory from the persistent storage.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.path | string | Path to the directory within the persisent storage |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.delete",
  "params": {
    "path": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.clone"></a>
## *clone [<sup>method</sup>](#head.Methods)*

Creates a clone of given plugin with a new callsign.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of the plugin |
| params.newcallsign | string | Callsign for the cloned plugin |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| response | string | *...* |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.clone",
  "params": {
    "callsign": "...",
    "newcallsign": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "..."
}
```

<a name="method.startdiscovery"></a>
## *startdiscovery [<sup>method</sup>](#head.Methods)*

Starts SSDP network discovery.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.ttl | integer | Time to live, parameter for SSDP discovery |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.startdiscovery",
  "params": {
    "ttl": 0
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.persist"></a>
## *persist [<sup>method</sup>](#head.Methods)*

Stores all configuration to the persistent memory.

> ``storeconfig`` is an alternative name for this method.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.persist"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.activate"></a>
## *activate [<sup>method</sup>](#head.Methods)*

Activates a plugin.

### Description

Use this method to activate a plugin, i.e. move from Deactivated, via Activating to Activated state. If a plugin is in Activated state, it can handle JSON-RPC requests that are coming in. The plugin is loaded into memory only if it gets activated.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of plugin to be activated |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.activate",
  "params": {
    "callsign": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.deactivate"></a>
## *deactivate [<sup>method</sup>](#head.Methods)*

Deactivates a plugin.

### Description

Use this method to deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state. If a plugin is deactivated, the actual plugin (.so) is no longer loaded into the memory of the process. In a Deactivated state the plugin will not respond to any JSON-RPC requests.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of plugin to be deactivated |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.deactivate",
  "params": {
    "callsign": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.unavailable"></a>
## *unavailable [<sup>method</sup>](#head.Methods)*

Makes a plugin unavailable for interaction.

### Description

Use this method to mark a plugin as unavailable, i.e. move from Deactivated to Unavailable state. It can not be started unless it is first deactivated (what triggers a state transition).

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of plugin to be set as unavailable |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.unavailable",
  "params": {
    "callsign": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.hibernate"></a>
## *hibernate [<sup>method</sup>](#head.Methods)*

Hibernates a plugin.

### Description

Use *activate* to wake up a hibernated plugin. In a Hibernated state the plugin will not respond to any JSON-RPC requests.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of plugin to be hibernated |
| params.timeout | integer | Allowed time |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROC``` | The plugin is running in-process and thus cannot be hibernated |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.hibernate",
  "params": {
    "callsign": "...",
    "timeout": 0
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.suspend"></a>
## *suspend [<sup>method</sup>](#head.Methods)*

Suspends a plugin.

### Description

This is a more intelligent method, compared to *deactivate*, to move a plugin to a suspended state depending on its current state. Depending on the *startmode* flag this method will deactivate the plugin or only suspend the plugin.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of plugin to be suspended |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.suspend",
  "params": {
    "callsign": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.resume"></a>
## *resume [<sup>method</sup>](#head.Methods)*

Resumes a plugin.

### Description

This is a more intelligent method, compared to *activate*, to move a plugin to a resumed state depending on its current state. If required it will activate and move to the resumed state, regardless of the flags in the config (i.e. *startmode*, *resumed*)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Callsign of plugin to be resumed |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.resume",
  "params": {
    "callsign": "..."
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Controller plugin:

Controller System interface properties:

| Property | Description |
| :-------- | :-------- |
| [environment](#property.environment) (read-only) | Environment variable value |

Controller Discovery interface properties:

| Property | Description |
| :-------- | :-------- |
| [discoveryresults](#property.discoveryresults) (read-only) | SSDP network discovery results |

Controller Configuration interface properties:

| Property | Description |
| :-------- | :-------- |
| [configuration](#property.configuration) | Service configuration |

Controller Subsystems interface properties:

| Property | Description |
| :-------- | :-------- |
| [subsystems](#property.subsystems) (read-only) | Subsystems status |

Controller Metadata interface properties:

| Property | Description |
| :-------- | :-------- |
| [services](#property.services) / [status](#property.services) (read-only) | Services metadata |
| [links](#property.links) (read-only) | Connections list |
| [proxies](#property.proxies) (read-only) | Proxies list |
| [version](#property.version) (read-only) | Framework version |
| [threads](#property.threads) (read-only) | Workerpool threads |
| [pendingrequests](#property.pendingrequests) (read-only) | Pending requests |
| [callstack](#property.callstack) (read-only) | Thread callstack |

<a name="property.environment"></a>
## *environment [<sup>property</sup>](#head.Properties)*

Provides access to the environment variable value.

> This property is **read-only**.

### Value

> The *variable* argument shall be passed as the index to the property, e.g. ``Controller.1.environment@xyz``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Environment variable value |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.environment@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "..."
}
```

<a name="property.discoveryresults"></a>
## *discoveryresults [<sup>property</sup>](#head.Properties)*

Provides access to the SSDP network discovery results.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | SSDP network discovery results |
| result[#] | object | *...* |
| result[#].locator | string | Locator for the discovery |
| result[#].latency | integer | Latency for the discovery |
| result[#]?.model | string | <sup>*(optional)*</sup> Model |
| result[#].secure | boolean | Secure or not |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.discoveryresults"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "locator": "...",
      "latency": 0,
      "model": "...",
      "secure": false
    }
  ]
}
```

<a name="property.configuration"></a>
## *configuration [<sup>property</sup>](#head.Properties)*

Provides access to the service configuration.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | opaque object | Service configuration |

> The *callsign* argument shall be passed as the index to the property, e.g. ``Controller.1.configuration@xyz``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | opaque object | Service configuration |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.configuration@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {}
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.configuration@xyz",
  "params": {}
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a name="property.subsystems"></a>
## *subsystems [<sup>property</sup>](#head.Properties)*

Provides access to the subsystems status.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Subsystems status |
| (property)[#] | object | *...* |
| (property)[#].subsystem | string | Name of the subsystem (must be one of the following: *Bluetooth, Cryptography, Decryption, Graphics, Identifier, Installation, Internet, Location, Network, Platform, Provisioning, Security, Streaming, Time, WebSource*) |
| (property)[#].active | boolean | Denotes if the subsystem is currently active |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.subsystems"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "subsystem": "Platform",
      "active": false
    }
  ]
}
```

<a name="property.services"></a>
## *services [<sup>property</sup>](#head.Properties)*

Provides access to the services metadata.

> This property is **read-only**.

> ``status`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Description

If callsign is omitted, metadata of all services is returned.

### Value

> The *callsign* argument shall be passed as the index to the property, e.g. ``Controller.1.services@xyz``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Services metadata *(if only one element is present then the array will be omitted)* |
| result[#] | object | *...* |
| result[#].callsign | string | Plugin callsign |
| result[#].locator | string | Shared library path |
| result[#].classname | string | Plugin class name |
| result[#].module | string | Module name |
| result[#].state | string | Current state (must be one of the following: *Activated, Activation, Deactivated, Deactivation, Destroyed, Hibernated, Precondition, Resumed, Suspended, Unavailable*) |
| result[#].startmode | string | Startup mode (must be one of the following: *Activated, Deactivated, Unavailable*) |
| result[#].resumed | boolean | Determines if the plugin is to be activated in resumed or suspended mode |
| result[#].version | object | Version |
| result[#].version.hash | string | SHA256 hash identifying the source code |
| result[#].version.major | integer | Major number |
| result[#].version.minor | integer | Minor number |
| result[#].version.patch | integer | Patch number |
| result[#]?.communicator | string | <sup>*(optional)*</sup> Communicator |
| result[#]?.persistentpathpostfix | string | <sup>*(optional)*</sup> Postfix of persistent path |
| result[#]?.volatilepathpostfix | string | <sup>*(optional)*</sup> Postfix of volatile path |
| result[#]?.systemrootpath | string | <sup>*(optional)*</sup> Path of system root |
| result[#]?.precondition | opaque object | <sup>*(optional)*</sup> Activation conditons |
| result[#]?.termination | opaque object | <sup>*(optional)*</sup> Deactivation conditions |
| result[#]?.configuration | opaque object | <sup>*(optional)*</sup> Plugin configuration |
| result[#]?.observers | integer | <sup>*(optional)*</sup> Number or observers |
| result[#]?.processedrequests | integer | <sup>*(optional)*</sup> Number of API requests that have been processed by the plugin |
| result[#]?.processedobjects | integer | <sup>*(optional)*</sup> Number of objects that have been processed by the plugin |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.services@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "callsign": "...",
      "locator": "...",
      "classname": "...",
      "module": "...",
      "state": "Unavailable",
      "startmode": "Unavailable",
      "resumed": false,
      "version": {
        "hash": "...",
        "major": 0,
        "minor": 0,
        "patch": 0
      },
      "communicator": "...",
      "persistentpathpostfix": "...",
      "volatilepathpostfix": "...",
      "systemrootpath": "...",
      "precondition": {},
      "termination": {},
      "configuration": {},
      "observers": 0,
      "processedrequests": 0,
      "processedobjects": 0
    }
  ]
}
```

<a name="property.links"></a>
## *links [<sup>property</sup>](#head.Properties)*

Provides access to the connections list.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Connections list |
| result[#] | object | *...* |
| result[#].remote | string | IP address (or FQDN) of the other side of the connection |
| result[#].state | string | State of the link (must be one of the following: *COMRPC, Closed, RawSocket, Suspended, WebServer, WebSocket*) |
| result[#]?.name | string | <sup>*(optional)*</sup> Name of the connection |
| result[#].id | integer | A unique number identifying the connection |
| result[#].activity | boolean | Denotes if there was any activity on this connection |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.links"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "remote": "...",
      "state": "Closed",
      "name": "...",
      "id": 0,
      "activity": false
    }
  ]
}
```

<a name="property.proxies"></a>
## *proxies [<sup>property</sup>](#head.Properties)*

Provides access to the proxies list.

> This property is **read-only**.

### Value

> The *linkid* argument shall be passed as the index to the property, e.g. ``Controller.1.proxies@0``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Proxies list |
| result[#] | object | *...* |
| result[#].interface | integer | Interface ID |
| result[#].instance | instanceid | Instance ID |
| result[#].count | integer | Reference count |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.proxies@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "interface": 0,
      "instance": "0x...",
      "count": 0
    }
  ]
}
```

<a name="property.version"></a>
## *version [<sup>property</sup>](#head.Properties)*

Provides access to the framework version.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Framework version |
| result.hash | string | SHA256 hash identifying the source code |
| result.major | integer | Major number |
| result.minor | integer | Minor number |
| result.patch | integer | Patch number |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.version"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "hash": "...",
    "major": 0,
    "minor": 0,
    "patch": 0
  }
}
```

<a name="property.threads"></a>
## *threads [<sup>property</sup>](#head.Properties)*

Provides access to the workerpool threads.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Workerpool threads |
| result[#] | object | *...* |
| result[#].id | instanceid | Thread ID |
| result[#].job | string | Job name |
| result[#].runs | integer | Number of runs |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.threads"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "id": "0x...",
      "job": "...",
      "runs": 0
    }
  ]
}
```

<a name="property.pendingrequests"></a>
## *pendingrequests [<sup>property</sup>](#head.Properties)*

Provides access to the pending requests.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Pending requests |
| result[#] | string | *...* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.pendingrequests"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "..."
  ]
}
```

<a name="property.callstack"></a>
## *callstack [<sup>property</sup>](#head.Properties)*

Provides access to the thread callstack.

> This property is **read-only**.

### Value

> The *thread* argument shall be passed as the index to the property, e.g. ``Controller.1.callstack@0``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Thread callstack |
| result[#] | object | *...* |
| result[#].address | instanceid | Memory address |
| result[#].module | string | Module name |
| result[#]?.function | string | <sup>*(optional)*</sup> Function name |
| result[#]?.line | integer | <sup>*(optional)*</sup> Line number |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.callstack@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "address": "0x...",
      "module": "...",
      "function": "...",
      "line": 0
    }
  ]
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Controller plugin:

Controller LifeTime interface events:

| Event | Description |
| :-------- | :-------- |
| [statechange](#event.statechange) | Notifies of a plugin state change |

Controller Subsystems interface events:

| Event | Description |
| :-------- | :-------- |
| [subsystemchange](#event.subsystemchange) | Notifies a subsystem change |

Controller Events interface events:

| Event | Description |
| :-------- | :-------- |
| [all](#event.all) | Notifies all events forwarded by the framework |

<a name="event.statechange"></a>
## *statechange [<sup>event</sup>](#head.Notifications)*

Notifies of a plugin state change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Plugin callsign |
| params.state | string | New state of the plugin (must be one of the following: *Activated, Activation, Deactivated, Deactivation, Destroyed, Hibernated, Precondition, Unavailable*) |
| params.reason | string | Reason for state change (must be one of the following: *Automatic, Conditions, Failure, InitializationFailed, MemoryExceeded, Requested, Shutdown, Startup, WatchdogExpired*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.statechange",
  "params": {
    "callsign": "...",
    "state": "Unavailable",
    "reason": "Requested"
  }
}
```

<a name="event.subsystemchange"></a>
## *subsystemchange [<sup>event</sup>](#head.Notifications)*

Notifies a subsystem change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| subsystems | array | Subsystems that have changed |
| subsystems[#] | object | *...* |
| subsystems[#].subsystem | string | Name of the subsystem (must be one of the following: *Bluetooth, Cryptography, Decryption, Graphics, Identifier, Installation, Internet, Location, Network, Platform, Provisioning, Security, Streaming, Time, WebSource*) |
| subsystems[#].active | boolean | Denotes if the subsystem is currently active |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.subsystemchange",
  "params": [
    {
      "subsystem": "Platform",
      "active": false
    }
  ]
}
```

<a name="event.all"></a>
## *all [<sup>event</sup>](#head.Notifications)*

Notifies all events forwarded by the framework.

### Description

The Controller plugin is an aggregator of all the events triggered by a specific plugin. All notifications send by any plugin are forwarded over the Controller socket as an event.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object | *...* |
| params.callsign | string | Origin of the message |
| params.data | opaque object | Contents of the message |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.all",
  "params": {
    "callsign": "...",
    "data": {}
  }
}
```

