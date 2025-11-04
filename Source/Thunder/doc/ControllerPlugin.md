<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Controller_Plugin"></a>
# Controller Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

Controller plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Properties](#head_Properties)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the Controller plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a id="head_Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a id="head_Acronyms,_Abbreviations_and_Terms"></a>
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

<a id="head_References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Controller*) |
| classname | string | mandatory | Class name: *Controller* |
| locator | string | mandatory | Library name: *(built-in)* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- Controller::ISystem ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IDiscovery ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IConfiguration ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (uncompliant-extended format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::ILifeTime ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::ISubsystems ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (uncompliant-collapsed format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IEvents ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IMetadata ([IController.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the Controller plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

Controller System interface methods:

| Method | Description |
| :-------- | :-------- |
| [reboot](#method_reboot) / [harakiri](#method_reboot) | Reboots the device |
| [delete](#method_delete) | Removes contents of a directory from the persistent storage |
| [clone](#method_clone) | Creates a clone of given plugin with a new callsign |
| [destroy](#method_destroy) | Destroy given plugin |

Controller Discovery interface methods:

| Method | Description |
| :-------- | :-------- |
| [startdiscovery](#method_startdiscovery) | Starts SSDP network discovery |

Controller Configuration interface methods:

| Method | Description |
| :-------- | :-------- |
| [persist](#method_persist) / [storeconfig](#method_persist) | Stores all configuration to the persistent memory |

Controller LifeTime interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method_activate) | Activates a plugin |
| [deactivate](#method_deactivate) | Deactivates a plugin |
| [unavailable](#method_unavailable) | Makes a plugin unavailable for interaction |
| [hibernate](#method_hibernate) | Hibernates a plugin |
| [suspend](#method_suspend) | Suspends a plugin |
| [resume](#method_resume) | Resumes a plugin |

<a id="method_versions"></a>
## *versions [<sup>method</sup>](#head_Methods)*

Retrieves a list of JSON-RPC interfaces offered by this service.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | A list ofsinterfaces with their version numbers<br>*Array length must be at most 255 elements.* |
| result[#] | object | mandatory | *...* |
| result[#].name | string | mandatory | Name of the interface |
| result[#].major | integer | mandatory | Major part of version number |
| result[#].minor | integer | mandatory | Minor part of version number |
| result[#].patch | integer | mandatory | Patch part of version version number |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JMyInterface",
      "major": 1,
      "minor": 0,
      "patch": 0
    }
  ]
}
```

<a id="method_exists"></a>
## *exists [<sup>method</sup>](#head_Methods)*

Checks if a JSON-RPC method or property exists.

### Description

This method will return *True* for the following methods/properties: *environment, discoveryresults, configuration, subsystems, services, links, proxies, framework, threads, pendingrequests, callstack, buildinfo, versions, exists, register, unregister, reboot, delete, clone, destroy, startdiscovery, persist, activate, deactivate, unavailable, hibernate, suspend, resume*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.method | string | mandatory | Name of the method or property to look up |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Denotes if the method exists or not |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.exists",
  "params": {
    "method": "environment"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

<a id="method_register"></a>
## *register [<sup>method</sup>](#head_Methods)*

Registers for an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[statechange](#notification_statechange), [statecontrolstatechange](#notification_statecontrolstatechange), [subsystemchange](#notification_subsystemchange), [all](#notification_all)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_REGISTERED``` | Failed to register for the notification (e.g. already registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.register",
  "params": {
    "event": "statechange",
    "id": "myapp"
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

<a id="method_unregister"></a>
## *unregister [<sup>method</sup>](#head_Methods)*

Unregisters from an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[statechange](#notification_statechange), [statecontrolstatechange](#notification_statecontrolstatechange), [subsystemchange](#notification_subsystemchange), [all](#notification_all)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_UNREGISTERED``` | Failed to unregister from the notification (e.g. not yet registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.unregister",
  "params": {
    "event": "statechange",
    "id": "myapp"
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

<a id="method_reboot"></a>
## *reboot [<sup>method</sup>](#head_Methods)*

Reboots the device.

> ``harakiri`` is an alternative name for this method. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Description

Depending on the device this call may not generate a response.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_delete"></a>
## *delete [<sup>method</sup>](#head_Methods)*

Removes contents of a directory from the persistent storage.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.path | string | mandatory | Path to the directory within the persisent storage |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_clone"></a>
## *clone [<sup>method</sup>](#head_Methods)*

Creates a clone of given plugin with a new callsign.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of the plugin |
| params.newcallsign | string | mandatory | Callsign for the cloned plugin |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | *...* |

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

<a id="method_destroy"></a>
## *destroy [<sup>method</sup>](#head_Methods)*

Destroy given plugin.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of the plugin |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.destroy",
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

<a id="method_startdiscovery"></a>
## *startdiscovery [<sup>method</sup>](#head_Methods)*

Starts SSDP network discovery.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.ttl | integer | optional | Time to live, parameter for SSDP discovery<br>*Value must be in range [1..255].* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.startdiscovery",
  "params": {
    "ttl": 1
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

<a id="method_persist"></a>
## *persist [<sup>method</sup>](#head_Methods)*

Stores all configuration to the persistent memory.

> ``storeconfig`` is an alternative name for this method.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_activate"></a>
## *activate [<sup>method</sup>](#head_Methods)*

Activates a plugin.

### Description

Use this method to activate a plugin, i.e. move from Deactivated, via Activating to Activated state. If a plugin is in Activated state, it can handle JSON-RPC requests that are coming in. The plugin is loaded into memory only if it gets activated.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be activated |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_deactivate"></a>
## *deactivate [<sup>method</sup>](#head_Methods)*

Deactivates a plugin.

### Description

Use this method to deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state. If a plugin is deactivated, the actual plugin (.so) is no longer loaded into the memory of the process. In a Deactivated state the plugin will not respond to any JSON-RPC requests.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be deactivated |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_unavailable"></a>
## *unavailable [<sup>method</sup>](#head_Methods)*

Makes a plugin unavailable for interaction.

### Description

Use this method to mark a plugin as unavailable, i.e. move from Deactivated to Unavailable state. It can not be started unless it is first deactivated (what triggers a state transition).

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be set as unavailable |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_hibernate"></a>
## *hibernate [<sup>method</sup>](#head_Methods)*

Hibernates a plugin.

### Description

Use *activate* to wake up a hibernated plugin. In a Hibernated state the plugin will not respond to any JSON-RPC requests.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be hibernated |
| params.timeout | integer | mandatory | Allowed time |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_suspend"></a>
## *suspend [<sup>method</sup>](#head_Methods)*

Suspends a plugin.

### Description

This is a more intelligent method, compared to *deactivate*, to move a plugin to a suspended state depending on its current state. Depending on the *startmode* flag this method will deactivate the plugin or only suspend the plugin.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be suspended |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_resume"></a>
## *resume [<sup>method</sup>](#head_Methods)*

Resumes a plugin.

### Description

This is a more intelligent method, compared to *activate*, to move a plugin to a resumed state depending on its current state. If required it will activate and move to the resumed state, regardless of the flags in the config (i.e. *startmode*, *resumed*)

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be resumed |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the Controller plugin:

Controller System interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [environment](#property_environment) | read-only | Environment variable value |

Controller Discovery interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [discoveryresults](#property_discoveryresults) | read-only | SSDP network discovery results |

Controller Configuration interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [configuration](#property_configuration) | read/write | Service configuration |

Controller Subsystems interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [subsystems](#property_subsystems) | read-only | Subsystems status |

Controller Metadata interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [services](#property_services) / [status](#property_services) | read-only | Services metadata |
| [links](#property_links) | read-only | Connections list of Thunder connections |
| [proxies](#property_proxies) | read-only | Proxies list |
| [framework](#property_framework) / [version](#property_framework) | read-only | Framework version |
| [threads](#property_threads) | read-only | Workerpool threads |
| [pendingrequests](#property_pendingrequests) | read-only | Pending requests |
| [callstack](#property_callstack) | read-only | Thread callstack |
| [buildinfo](#property_buildinfo) | read-only | Build information |

<a id="property_environment"></a>
## *environment [<sup>property</sup>](#head_Properties)*

Provides access to the environment variable value.

> This property is **read-only**.

> The *variable* parameter shall be passed as the index to the property, i.e. ``environment@<variable>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| variable | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Environment variable value |

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

<a id="property_discoveryresults"></a>
## *discoveryresults [<sup>property</sup>](#head_Properties)*

Provides access to the SSDP network discovery results.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | SSDP network discovery results |
| (property)[#] | object | mandatory | *...* |
| (property)[#].locator | string | mandatory | Locator for the discovery |
| (property)[#].latency | integer | mandatory | Latency for the discovery |
| (property)[#]?.model | string | optional | Model |
| (property)[#].secure | boolean | mandatory | Secure or not |

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

<a id="property_configuration"></a>
## *configuration [<sup>property</sup>](#head_Properties)*

Provides access to the service configuration.

> The *callsign* parameter shall be passed as the index to the property, i.e. ``configuration@<callsign>``.

### Index (Get)

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | *...* |

### Index (Set)

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Service configuration |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Service configuration |

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

<a id="property_subsystems"></a>
## *subsystems [<sup>property</sup>](#head_Properties)*

Provides access to the subsystems status.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Subsystems status |
| (property)[#] | object | mandatory | *...* |
| (property)[#].subsystem | string | mandatory | Name of the subsystem (must be one of the following: *Bluetooth, Cryptography, Decryption, Graphics, Identifier, Installation, Internet, Location, Network, Platform, Provisioning, Security, Streaming, Time, WebSource*) |
| (property)[#].active | boolean | mandatory | Denotes if the subsystem is currently active |

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
      "subsystem": "Security",
      "active": false
    }
  ]
}
```

<a id="property_services"></a>
## *services [<sup>property</sup>](#head_Properties)*

Provides access to the services metadata.

> This property is **read-only**.

> ``status`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Description

If callsign is omitted, metadata of all services is returned.

> The *callsign* parameter shall be passed as the index to the property, i.e. ``services@<callsign>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Services metadata *(if only one element is present then the array will be omitted)* |
| (property)[#] | object | mandatory | *...* |
| (property)[#].callsign | string | mandatory | Plugin callsign |
| (property)[#].locator | string | mandatory | Shared library path |
| (property)[#].classname | string | mandatory | Plugin class name |
| (property)[#].module | string | mandatory | Module name |
| (property)[#].state | string | mandatory | Current state (must be one of the following: *Activated, Activation, Deactivated, Deactivation, Destroyed, Hibernated, Precondition, Resumed, Suspended, Unavailable*) |
| (property)[#].startmode | string | mandatory | Startup mode (must be one of the following: *Activated, Deactivated, Unavailable*) |
| (property)[#].resumed | boolean | mandatory | Determines if the plugin is to be activated in resumed or suspended mode |
| (property)[#].version | object | mandatory | Version |
| (property)[#].version.hash | string | mandatory | SHA256 hash identifying the source code<br>*String length must be equal to 64 chars.* |
| (property)[#].version.major | integer | mandatory | Major number |
| (property)[#].version.minor | integer | mandatory | Minor number |
| (property)[#].version.patch | integer | mandatory | Patch number |
| (property)[#]?.communicator | string | optional | Communicator |
| (property)[#]?.persistentpathpostfix | string | optional | Postfix of persistent path |
| (property)[#]?.volatilepathpostfix | string | optional | Postfix of volatile path |
| (property)[#]?.systemrootpath | string | optional | Path of system root |
| (property)[#]?.precondition | opaque object | optional | Activation conditons |
| (property)[#]?.termination | opaque object | optional | Deactivation conditions |
| (property)[#]?.control | opaque object | optional | Conditions controlled by this service |
| (property)[#].configuration | opaque object | mandatory | Plugin configuration |
| (property)[#].observers | integer | mandatory | Number or observers |
| (property)[#]?.processedrequests | integer | optional | Number of API requests that have been processed by the plugin |
| (property)[#]?.processedobjects | integer | optional | Number of objects that have been processed by the plugin |

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
      "state": "Deactivated",
      "startmode": "Deactivated",
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
      "control": {},
      "configuration": {},
      "observers": 0,
      "processedrequests": 0,
      "processedobjects": 0
    }
  ]
}
```

<a id="property_links"></a>
## *links [<sup>property</sup>](#head_Properties)*

Provides access to the connections list of Thunder connections.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Connections list of Thunder connections |
| (property)[#] | object | mandatory | *...* |
| (property)[#].remote | string | mandatory | IP address (or FQDN) of the other side of the connection |
| (property)[#].state | string | mandatory | State of the link (must be one of the following: *COMRPC, Closed, RawSocket, Suspended, WebServer, WebSocket*) |
| (property)[#].id | integer | mandatory | A unique number identifying the connection |
| (property)[#].activity | boolean | mandatory | Denotes if there was any activity on this connection |
| (property)[#]?.name | string | optional | Name of the connection |

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
      "state": "WebServer",
      "id": 0,
      "activity": false,
      "name": "..."
    }
  ]
}
```

<a id="property_proxies"></a>
## *proxies [<sup>property</sup>](#head_Properties)*

Provides access to the proxies list.

> This property is **read-only**.

> The *linkid* parameter shall be passed as the index to the property, i.e. ``proxies@<linkid>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| linkid | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Proxies list |
| (property)[#] | object | mandatory | *...* |
| (property)[#].interface | integer | mandatory | Interface ID |
| (property)[#].name | string | mandatory | The fully qualified name of the interface |
| (property)[#].instance | instanceid | mandatory | Instance ID |
| (property)[#].count | integer | mandatory | Reference count |
| (property)[#]?.origin | string | optional | The Origin of the assocated connection |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.proxies@xyz"
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
      "name": "...",
      "instance": "0x...",
      "count": 0,
      "origin": "..."
    }
  ]
}
```

<a id="property_framework"></a>
## *framework [<sup>property</sup>](#head_Properties)*

Provides access to the framework version.

> This property is **read-only**.

> ``version`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Framework version |
| (property).hash | string | mandatory | SHA256 hash identifying the source code<br>*String length must be equal to 64 chars.* |
| (property).major | integer | mandatory | Major number |
| (property).minor | integer | mandatory | Minor number |
| (property).patch | integer | mandatory | Patch number |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.framework"
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

<a id="property_threads"></a>
## *threads [<sup>property</sup>](#head_Properties)*

Provides access to the workerpool threads.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Workerpool threads |
| (property)[#] | object | mandatory | *...* |
| (property)[#].id | instanceid | mandatory | Thread ID |
| (property)[#].job | string | mandatory | Job name |
| (property)[#].runs | integer | mandatory | Number of runs |

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

<a id="property_pendingrequests"></a>
## *pendingrequests [<sup>property</sup>](#head_Properties)*

Provides access to the pending requests.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Pending requests |
| (property)[#] | string | mandatory | *...* |

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

<a id="property_callstack"></a>
## *callstack [<sup>property</sup>](#head_Properties)*

Provides access to the thread callstack.

> This property is **read-only**.

> The *thread* parameter shall be passed as the index to the property, i.e. ``callstack@<thread>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| thread | integer | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Thread callstack |
| (property)[#] | object | mandatory | *...* |
| (property)[#].address | instanceid | mandatory | Memory address |
| (property)[#].module | string | mandatory | Module name |
| (property)[#]?.function | string | optional | Function name |
| (property)[#]?.line | integer | optional | Line number |

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

<a id="property_buildinfo"></a>
## *buildinfo [<sup>property</sup>](#head_Properties)*

Provides access to the build information.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Build information |
| (property).systemtype | string | mandatory | System type (must be one of the following: *Linux, MacOS, Windows*) |
| (property).buildtype | string | mandatory | Build type (must be one of the following: *Debug, DebugOptimized, Production, Release, ReleaseWithDebugInfo*) |
| (property)?.extensions | array | optional | *...* |
| (property)?.extensions[#] | string | mandatory | *...* (must be one of the following: *Bluetooth, Hiberbate, ProcessContainers, WarningReporting*) |
| (property).messaging | boolean | mandatory | Denotes whether Messaging is enabled |
| (property).exceptioncatching | boolean | mandatory | Denotes whether there is an exception |
| (property).deadlockdetection | boolean | mandatory | Denotes whether deadlock detection is enabled |
| (property).wcharsupport | boolean | mandatory | *...* |
| (property).instanceidbits | integer | mandatory | Core instance bits |
| (property)?.tracelevel | integer | optional | Trace level |
| (property).threadpoolcount | integer | mandatory | *...* |
| (property).comrpctimeout | integer | mandatory | *...* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.buildinfo"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "systemtype": "Linux",
    "buildtype": "DebugOptimized",
    "extensions": [
      "Bluetooth"
    ],
    "messaging": false,
    "exceptioncatching": false,
    "deadlockdetection": false,
    "wcharsupport": false,
    "instanceidbits": 0,
    "tracelevel": 0,
    "threadpoolcount": 0,
    "comrpctimeout": 0
  }
}
```

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Controller plugin:

Controller LifeTime interface events:

| Notification | Description |
| :-------- | :-------- |
| [statechange](#notification_statechange) | Notifies of a plugin state change |
| [statecontrolstatechange](#notification_statecontrolstatechange) | Notifies of a plugin state change controlled by IStateControl |

Controller Subsystems interface events:

| Notification | Description |
| :-------- | :-------- |
| [subsystemchange](#notification_subsystemchange) | Notifies a subsystem change |

Controller Events interface events:

| Notification | Description |
| :-------- | :-------- |
| [all](#notification_all) | Notifies all events forwarded by the framework |

<a id="notification_statechange"></a>
## *statechange [<sup>notification</sup>](#head_Notifications)*

Notifies of a plugin state change.

### Description

If registered for empty callsign, notifications for all services will be sent.

> This notification may also be triggered by client registration.

### Parameters

> The *callsign* parameter shall be passed as index to the ``register`` call, i.e. ``register@<callsign>``.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.callsign | string | optional | Plugin callsign |
| params.state | string | mandatory | New state of the plugin (must be one of the following: *Activated, Deactivated, Unavailable*) |
| params.reason | string | mandatory | Reason for state change (must be one of the following: *Automatic, Conditions, Failure, InitializationFailed, MemoryExceeded, Requested, Shutdown, Startup, WatchdogExpired*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.register@Messenger",
  "params": {
    "event": "statechange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.statechange@Messenger",
  "params": {
    "callsign": "Messenger",
    "state": "Deactivated",
    "reason": "Automatic"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.statechange@<callsign>``.

<a id="notification_statecontrolstatechange"></a>
## *statecontrolstatechange [<sup>notification</sup>](#head_Notifications)*

Notifies of a plugin state change controlled by IStateControl.

### Description

If registered for empty callsign, notifications for all services will be sent.

> This notification may also be triggered by client registration.

### Parameters

> The *callsign* parameter shall be passed as index to the ``register`` call, i.e. ``register@<callsign>``.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.callsign | string | optional | Plugin callsign |
| params.state | string | mandatory | New state of the plugin (must be one of the following: *Resumed, Suspended, Unknown*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.register@Messenger",
  "params": {
    "event": "statecontrolstatechange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.statecontrolstatechange@Messenger",
  "params": {
    "callsign": "Messenger",
    "state": "Suspended"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.statecontrolstatechange@<callsign>``.

<a id="notification_subsystemchange"></a>
## *subsystemchange [<sup>notification</sup>](#head_Notifications)*

Notifies a subsystem change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | array | mandatory | Subsystems that have changed |
| params[#] | object | mandatory | *...* |
| params[#].subsystem | string | mandatory | Name of the subsystem (must be one of the following: *Bluetooth, Cryptography, Decryption, Graphics, Identifier, Installation, Internet, Location, Network, Platform, Provisioning, Security, Streaming, Time, WebSource*) |
| params[#].active | boolean | mandatory | Denotes if the subsystem is currently active |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.register",
  "params": {
    "event": "subsystemchange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.subsystemchange",
  "params": [
    {
      "subsystem": "Security",
      "active": false
    }
  ]
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.subsystemchange``.

<a id="notification_all"></a>
## *all [<sup>notification</sup>](#head_Notifications)*

Notifies all events forwarded by the framework.

### Description

The Controller plugin is an aggregator of all the events triggered by a specific plugin. All notifications send by any plugin are forwarded over the Controller socket as an event.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the message |
| params?.callsign | string | optional | Origin of the message |
| params?.params | opaque object | optional | Contents of the message |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.1.register",
  "params": {
    "event": "all",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.all",
  "params": {
    "event": "...",
    "callsign": "...",
    "params": {}
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.all``.

