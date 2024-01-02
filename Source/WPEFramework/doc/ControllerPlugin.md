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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Controller*) |
| classname | string | Class name: *Controller* |
| locator | string | Library name: *(built-in)* |
| startmode | string | Determines if the plugin shall be started automatically along with the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- ISystemManagement ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- IDiscovery ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- IConfiguration ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (uncompliant-extended format)
- ILifeTime ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
- IMetadata ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Controller plugin:

SystemManagement interface methods:

| Method | Description |
| :-------- | :-------- |
| [reboot](#method.reboot) / [harakiri](#method.reboot) | Reboots the device |
| [delete](#method.delete) | Removes contents of a directory from the persistent storage |
| [clone](#method.clone) | Creates a clone of given plugin to requested new callsign |

Discovery interface methods:

| Method | Description |
| :-------- | :-------- |
| [startdiscovery](#method.startdiscovery) | Starts the network discovery |

Configuration interface methods:

| Method | Description |
| :-------- | :-------- |
| [persist](#method.persist) / [storeconfig](#method.persist) | Stores the configuration to persistent memory |

LifeTime interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method.activate) | Activates a plugin, |
| [deactivate](#method.deactivate) | Deactivates a plugin, |
| [unavailable](#method.unavailable) | Sets a plugin unavailable for interaction |
| [hibernate](#method.hibernate) | Sets a plugin in Hibernate state |
| [suspend](#method.suspend) | Suspends a plugin |
| [resume](#method.resume) | Resumes a plugin |

<a name="method.reboot"></a>
## *reboot [<sup>method</sup>](#head.Methods)*

Reboots the device.

> ``harakiri`` is an alternative name for this method.

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
| params | object |  |
| params.path | string |  |

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

Creates a clone of given plugin to requested new callsign.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |
| params.newcallsign | string |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| response | string |  |

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

Starts the network discovery. Use this method to initiate SSDP network discovery process.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ttl | integer |  |

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

Stores the configuration to persistent memory.

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

Activates a plugin, i.e. move from Deactivated, via Activating to Activated state.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |

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

Deactivates a plugin, i.e. move from Activated, via Deactivating to Deactivated state.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |

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

Sets a plugin unavailable for interaction.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |

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

Sets a plugin in Hibernate state.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |
| params.timeout | integer |  |

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

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |

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

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string |  |

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

SystemManagement interface properties:

| Property | Description |
| :-------- | :-------- |
| [environment](#property.environment) (read-only) | Provides the value of request environment variable |

Discovery interface properties:

| Property | Description |
| :-------- | :-------- |
| [discoveryresults](#property.discoveryresults) (read-only) | Provides SSDP network discovery results |

Configuration interface properties:

| Property | Description |
| :-------- | :-------- |
| [configuration](#property.configuration) | Provides configuration value of a request service |

Metadata interface properties:

| Property | Description |
| :-------- | :-------- |
| [services](#property.services) / [status](#property.services) (read-only) | Provides status of a service, including their configurations |
| [links](#property.links) (read-only) | Provides active connections details |
| [proxies](#property.proxies) (read-only) | Provides details of a proxy |
| [subsystems](#property.subsystems) (read-only) | Provides status of subsystems |
| [version](#property.version) (read-only) | Provides version and hash of WPEFramework |
| [threads](#property.threads) (read-only) | Provides information on workerpool threads |
| [pendingrequests](#property.pendingrequests) (read-only) | Provides information on pending requests |
| [callstack](#property.callstack) (read-only) | Provides callstack associated with the given thread |

<a name="property.environment"></a>
## *environment [<sup>property</sup>](#head.Properties)*

Provides access to the provides the value of request environment variable.

> This property is **read-only**.

### Value

> The *variable* argument shall be passed as the index to the property, e.g. ``Controller.1.environment@xyz``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Provides the value of request environment variable |

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

Provides access to the provides SSDP network discovery results.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides SSDP network discovery results |
| result[#] | object |  |
| result[#].locator | string |  |
| result[#].latency | integer |  |
| result[#]?.model | string | <sup>*(optional)*</sup>  |
| result[#].secure | boolean |  |

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

Provides access to the provides configuration value of a request service.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | opaque object | Provides configuration value of a request service |

> The *callsign* argument shall be passed as the index to the property, e.g. ``Controller.1.configuration@xyz``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | opaque object | Provides configuration value of a request service |

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

<a name="property.services"></a>
## *services [<sup>property</sup>](#head.Properties)*

Provides access to the provides status of a service, including their configurations.

> This property is **read-only**.

> ``status`` is an alternative name for this property.

### Value

> The *callsign* argument shall be passed as the index to the property, e.g. ``Controller.1.services@xyz``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides status of a service, including their configurations<br>*If only one element is present the array will be omitted.* |
| result[#] | object |  |
| result[#].callsign | string | Plugin callsign |
| result[#].locator | string | Shared library path |
| result[#].classname | string | Plugin class name |
| result[#].module | string | Module name |
| result[#].state | string | Current state (must be one of the following: unavailable, deactivated, deactivation, activated, activation, destroyed, precondition, hibernated, suspended, resumed) |
| result[#].startmode | string | Startup mode (must be one of the following: Unavailable, Deactivated, Activated) |
| result[#].version | object |  |
| result[#].version.hash | string | SHA256 hash identifying the source code |
| result[#].version.major | integer | Major number |
| result[#].version.minor | integer | Minor number |
| result[#].version.patch | integer | Patch number |
| result[#]?.communicator | string | <sup>*(optional)*</sup>  |
| result[#]?.persistentpathpostfix | string | <sup>*(optional)*</sup>  |
| result[#]?.volatilepathpostfix | string | <sup>*(optional)*</sup>  |
| result[#]?.systemrootpath | string | <sup>*(optional)*</sup>  |
| result[#]?.precondition | opaque object | <sup>*(optional)*</sup> Activate conditons |
| result[#]?.termination | opaque object | <sup>*(optional)*</sup> Deactivate conditions |
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
      "state": "unavailable",
      "startmode": "Unavailable",
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

Provides access to the provides active connections details.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides active connections details |
| result[#] | object |  |
| result[#].remote | string | IP address (or FQDN) of the other side of the connection |
| result[#].state | string | State of the link (must be one of the following: Closed, WebServer, WebSocket, RawSocket, COMRPC, Suspended) |
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

Provides access to the provides details of a proxy.

> This property is **read-only**.

### Value

> The *linkid* argument shall be passed as the index to the property, e.g. ``Controller.1.proxies@0``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides details of a proxy |
| result[#] | object |  |
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

<a name="property.subsystems"></a>
## *subsystems [<sup>property</sup>](#head.Properties)*

Provides access to the provides status of subsystems.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides status of subsystems |
| result[#] | object |  |
| result[#].subsystem | string | Subsystem name (must be one of the following: Platform, Security, Network, Identifier, Graphics, Internet, Location, Time, Provisioning, Decryption, WebSource, Streaming, Bluetooth, Cryptography) |
| result[#].active | boolean | Denotes if currently active |

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

<a name="property.version"></a>
## *version [<sup>property</sup>](#head.Properties)*

Provides access to the provides version and hash of WPEFramework.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Provides version and hash of WPEFramework |
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

Provides access to the provides information on workerpool threads.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides information on workerpool threads |
| result[#] | object |  |
| result[#].id | instanceid |  |
| result[#].job | string |  |
| result[#].runs | integer |  |

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

Provides access to the provides information on pending requests.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides information on pending requests |
| result[#] | string |  |

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

Provides access to the provides callstack associated with the given thread.

> This property is **read-only**.

### Value

> The *thread* argument shall be passed as the index to the property, e.g. ``Controller.1.callstack@0``.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides callstack associated with the given thread |
| result[#] | object |  |
| result[#].address | instanceid | Address |
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

LifeTime interface events:

| Event | Description |
| :-------- | :-------- |
| [statechange](#event.statechange) | Notifies a plugin state change |

<a name="event.statechange"></a>
## *statechange [<sup>event</sup>](#head.Notifications)*

Notifies a plugin state change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Plugin callsign |
| params.state | string | New state of the plugin (must be one of the following: Unavailable, Deactivated, Deactivation, Activated, Activation, Precondition, Hibernated, Destroyed) |
| params.reason | string | Reason of state change (must be one of the following: Requested, Automatic, Failure, MemoryExceeded, Startup, Shutdown, Conditions, WatchdogExpired, InitializationFailed) |

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

