<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Controller_Plugin"></a>
# Controller Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

Controller plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
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

<a id="head_Description"></a>
# Description

The Controller plugin controls (activates and deactivates) the configured plugins. The plugin is part of the framework and cannot be activated or deactivated itself, thus as well cannot not be unloaded.

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

- Controller::ISystemManagement ([IController.h](https://github.com/rdkcentral/Thunder/tree/R4_4/Source/plugins/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IDiscovery ([IController.h](https://github.com/rdkcentral/Thunder/tree/R4_4/Source/plugins/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IConfiguration ([IController.h](https://github.com/rdkcentral/Thunder/tree/R4_4/Source/plugins/IController.h)) (version 1.0.0) (uncompliant-extended format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::ILifeTime ([IController.h](https://github.com/rdkcentral/Thunder/tree/R4_4/Source/plugins/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- Controller::IMetadata ([IController.h](https://github.com/rdkcentral/Thunder/tree/R4_4/Source/plugins/IController.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- [Controller.json](https://github.com/rdkcentral/Thunder/tree/R4_4/Source/WPEFramework/Controller.json) (version 1.0.0) (legacy)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the Controller plugin:

Controller SystemManagement interface methods:

| Method | Description |
| :-------- | :-------- |
| [reboot](#method_reboot) / [harakiri](#method_reboot) | Reboots the device |
| [delete](#method_delete) | Removes contents of a directory from the persistent storage |
| [clone](#method_clone) | Creates a clone of given plugin with a new callsign |

Controller Discovery interface methods:

| Method | Description |
| :-------- | :-------- |
| [startdiscovery](#method_startdiscovery) | Starts the network discovery |

Controller Configuration interface methods:

| Method | Description |
| :-------- | :-------- |
| [persist](#method_persist) / [storeconfig](#method_persist) | Stores the configuration to persistent memory |

Controller LifeTime interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method_activate) | Activate a plugin, |
| [deactivate](#method_deactivate) | Deactivate a plugin, |
| [unavailable](#method_unavailable) | Set a plugin unavailable for interaction |
| [hibernate](#method_hibernate) | Set a plugin in Hibernate state |
| [suspend](#method_suspend) | Suspend a plugin |
| [resume](#method_resume) | Resumes a plugin |

<a id="method_reboot"></a>
## *reboot [<sup>method</sup>](#head_Methods)*

Reboots the device.

> ``harakiri`` is an alternative name for this method.

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
  "method": "Controller.reboot"
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
  "method": "Controller.delete",
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
  "method": "Controller.clone",
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

<a id="method_startdiscovery"></a>
## *startdiscovery [<sup>method</sup>](#head_Methods)*

Starts the network discovery. Use this method to initiate SSDP network discovery process.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.ttl | integer | optional | *...* (default: *1*) |

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
  "method": "Controller.startdiscovery",
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

Stores the configuration to persistent memory.

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
  "method": "Controller.persist"
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

Activate a plugin, i.e. move from Deactivated, via Activating to Activated state.

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
  "method": "Controller.activate",
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

Deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state.

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
  "method": "Controller.deactivate",
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

Set a plugin unavailable for interaction.

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
  "method": "Controller.unavailable",
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

Set a plugin in Hibernate state.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of plugin to be hibernated |
| params.timeout | integer | mandatory | Allowed time in ms |

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
  "method": "Controller.hibernate",
  "params": {
    "callsign": "...",
    "timeout": 10000
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

Suspend a plugin.

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
  "method": "Controller.suspend",
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
  "method": "Controller.resume",
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

Controller SystemManagement interface properties:

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

Controller Metadata interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [proxies](#property_proxies) | read-only | Currently running proxies list |
| [status](#property_status) | read-only | Services metadata |
| [links](#property_links) | read-only | Active connection details |
| [processinfo](#property_processinfo) | read-only | Framework process info, like worker pools details |
| [subsystems](#property_subsystems) | read-only | Currently active subsystem details |
| [version](#property_version) | read-only | Framework version |
| [callstack](#property_callstack) | read-only | Thread callstack information |

<a id="property_environment"></a>
## *environment [<sup>property</sup>](#head_Properties)*

Provides access to the environment variable value.

> This property is **read-only**.

> The *index* parameter shall be passed as the index to the property, i.e. ``environment@<index>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| index | string | mandatory | *...* |

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
  "method": "Controller.environment@xyz"
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
| (property) | opaque object | mandatory | SSDP network discovery results |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.discoveryresults"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": []
}
```

<a id="property_configuration"></a>
## *configuration [<sup>property</sup>](#head_Properties)*

Provides access to the service configuration.

> The *callsign* parameter shall be passed as the index to the property, i.e. ``configuration@<callsign>``.

### Index

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
  "method": "Controller.configuration@xyz"
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
  "method": "Controller.configuration@xyz",
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

<a id="property_proxies"></a>
## *proxies [<sup>property</sup>](#head_Properties)*

Provides access to the currently running proxies list.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Currently running proxies list |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.proxies"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": []
}
```

<a id="property_status"></a>
## *status [<sup>property</sup>](#head_Properties)*

Provides access to the services metadata.

> This property is **read-only**.

### Description

If callsign is omitted, metadata of all services is returned.

> The *index* parameter shall be passed as the index to the property, i.e. ``status@<index>``. The index is optional for the get request.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| index | string | optional | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Services metadata |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.status@xyz"
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

<a id="property_links"></a>
## *links [<sup>property</sup>](#head_Properties)*

Provides access to the active connection details.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Active connection details |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.links"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": []
}
```

<a id="property_processinfo"></a>
## *processinfo [<sup>property</sup>](#head_Properties)*

Provides access to the framework process info, like worker pools details.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Framework process info, like worker pools details |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.processinfo"
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

<a id="property_subsystems"></a>
## *subsystems [<sup>property</sup>](#head_Properties)*

Provides access to the currently active subsystem details.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Currently active subsystem details |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.subsystems"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": []
}
```

<a id="property_version"></a>
## *version [<sup>property</sup>](#head_Properties)*

Provides access to the framework version.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Framework version |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.version"
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

<a id="property_callstack"></a>
## *callstack [<sup>property</sup>](#head_Properties)*

Provides access to the thread callstack information.

> This property is **read-only**.

> The *index* parameter shall be passed as the index to the property, i.e. ``callstack@<index>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| index | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | opaque object | mandatory | Thread callstack information |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.callstack@xyz"
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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Controller plugin:

Controller LifeTime interface events:

| Notification | Description |
| :-------- | :-------- |
| [statechange](#notification_statechange) | Notifies of a plugin state change |

Legacy Controller interface events:

| Notification | Description |
| :-------- | :-------- |
| [all](#notification_all) | Signals each and every event in the system |
| [subsystemchange](#notification_subsystemchange) | Signals a subsystem state change |

<a id="notification_statechange"></a>
## *statechange [<sup>notification</sup>](#head_Notifications)*

Notifies of a plugin state change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Plugin callsign |
| params.state | string | mandatory | New state of the plugin (must be one of the following: *Activated, Activation, Deactivated, Deactivation, Destroyed, Hibernated, Precondition, Unavailable*) |
| params.reason | string | mandatory | Reason for state change (must be one of the following: *Automatic, Conditions, Failure, InitializationFailed, MemoryExceeded, Requested, Shutdown, Startup, WatchdogExpired*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.register",
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
  "method": "myid.statechange",
  "params": {
    "callsign": "...",
    "state": "Deactivated",
    "reason": "Automatic"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.statechange``.

<a id="notification_all"></a>
## *all [<sup>notification</sup>](#head_Notifications)*

Signals each and every event in the system. The Controller plugin is an aggregator of all the events triggered by a specific plugin. All notifications send by any plugin are forwarded over the Controller socket as an event.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | Callsign of the originator plugin of the event |
| params.data | object | mandatory | Object that was broadcasted as an event by the originator plugin |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.register",
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
    "callsign": "WebKitBrowser",
    "data": {}
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.all``.

<a id="notification_subsystemchange"></a>
## *subsystemchange [<sup>notification</sup>](#head_Notifications)*

Signals a subsystem state change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | array | mandatory | *...* |
| params[#] | object | mandatory | *...* |
| params[#].subsystem | string | mandatory | Subsystem name |
| params[#].active | boolean | mandatory | Denotes whether the subsystem is active (true) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Controller.register",
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
      "subsystem": "Platform",
      "active": true
    }
  ]
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.subsystemchange``.

