<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Controller_Plugin"></a>
# Controller Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

Controller plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Controller plugin. It includes detailed specification of its configuration, methods and properties provided, as well as notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.FQDN">FQDN</a> | Fully Qualified Domain Name |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |
| <a name="acronym.SSDP">SSDP</a> | Simple Service Discovery Protocol |
| <a name="acronym.URL">URL</a> | Uniform Resource Locator |

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

<a name="head.Description"></a>
# Description

The Controller plugin controls (activates and deactivates) the configured plugins. The plugin is part of the framework and cannot be activated or deactivated itself, thus as well cannot not be unloaded.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | <sup>*(optional)*</sup> Instance name of the plugin (default: *Controller*) |
| autostart | boolean | <sup>*(optional)*</sup> Determines if the plugin is to be started along with the framework (always *true* as Controller is a part of the framework) |
| configuration | object | <sup>*(optional)*</sup> Custom plugin configuration: |
| configuration?.downloadstore | string | <sup>*(optional)*</sup> Path within persistent storage to hold file downloads |
| configuration?.ttl | number | <sup>*(optional)*</sup> TTL to be set on the broadcast package for framework discovery in the network (default: 1) |
| configuration?.resumes | array | <sup>*(optional)*</sup> List of callsigns that have an *IStateControl* interface and need a resume at startup |
| configuration?.resumes[#] | string | <sup>*(optional)*</sup> Plugin callsign |
| configuration?.subsystems | array | <sup>*(optional)*</sup> List of subsystems configured for the system |
| configuration?.subsystems[#] | string | <sup>*(optional)*</sup> Subsystem name |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Controller plugin:

Controller interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method.activate) | Activates a plugin |
| [deactivate](#method.deactivate) | Deactivates a plugin |
| [startdiscovery](#method.startdiscovery) | Starts the network discovery |
| [storeconfig](#method.storeconfig) | Stores the configuration |
| [delete](#method.delete) | Removes contents of a directory from the persistent storage |
| [harakiri](#method.harakiri) | Reboots the device |

<a name="method.activate"></a>
## *activate <sup>method</sup>*

Activates a plugin

### Description

Use this method to activate a plugin, i.e. move from Deactivated, via Activating to Activated state. If a plugin is in the Activated state, it can handle JSON-RPC requests that are coming in. The plugin is loaded into memory only if it gets activated.

Also see: [statechange](#event.statechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Plugin callsign |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 31 | ```ERROR_PENDING_CONDITIONS``` | The plugin will be activated once its activation preconditions are met |
| 12 | ```ERROR_INPROGRESS``` | The plugin is currently being activated |
| 22 | ```ERROR_UNKNOWN_KEY``` | The plugin does not exist |
| 6 | ```ERROR_OPENING_FAILED``` | Failed to activate the plugin |
| 5 | ```ERROR_ILLEGAL_STATE``` | Current state of the plugin does not allow activation |
| 24 | ```ERROR_PRIVILEGED_REQUEST``` | Activation of the plugin is not allowed (e.g. Controller) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.activate", 
    "params": {
        "callsign": "DeviceInfo"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.deactivate"></a>
## *deactivate <sup>method</sup>*

Deactivates a plugin

### Description

Use this method to deactivate a plugin, i.e. move from Activated, via Deactivating to Deactivated state. If a plugin is Deactivated, the actual plugin (.so) is no longer loaded into the memory of the process. In a deactivated state, the plugin will not respond to any JSON-RPC requests.

Also see: [statechange](#event.statechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Callsign of the plugin |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 12 | ```ERROR_INPROGRESS``` | The plugin is currently being deactivated |
| 22 | ```ERROR_UNKNOWN_KEY``` | The plugin does not exist |
| 5 | ```ERROR_ILLEGAL_STATE``` | Current state of the plugin does not allow deactivation |
| 19 | ```ERROR_CLOSING_FAILED``` | Failed to activate the plugin |
| 24 | ```ERROR_PRIVILEGED_REQUEST``` | Deactivation of the plugin is not allowed (e.g. Controller) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.deactivate", 
    "params": {
        "callsign": "DeviceInfo"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.startdiscovery"></a>
## *startdiscovery <sup>method</sup>*

Starts the network discovery

### Description

Use this method to initiate SSDP network discovery process.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ttl | number | TTL (time to live) parameter for SSDP discovery |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.storeconfig"></a>
## *storeconfig <sup>method</sup>*

Stores the configuration

### Description

Use this method to save the current configuration to persistent memory.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to store the configuration |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.storeconfig"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.delete"></a>
## *delete <sup>method</sup>*

Removes contents of a directory from the persistent storage

### Description

Use this method to recursively delete contents of a directory

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.path | string | Path to directory (within the persistent storage) to delete contents of |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The given path was incorrect |
| 17 | ```ERROR_DESTRUCTION_FAILED``` | Failed to delete given path |
| 24 | ```ERROR_PRIVILEGED_REQUEST``` | The path points outside of persistent directory or some files/directories couldn't have been deleted |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.delete", 
    "params": {
        "path": "test/test.txt"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.harakiri"></a>
## *harakiri <sup>method</sup>*

Reboots the device

### Description

Use this method to reboot the device. Depending on the device, this call may not generate a response.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Rebooting procedure is not available on the device |
| 24 | ```ERROR_PRIVILEGED_REQUEST``` | Insufficient privileges to reboot the device |
| 1 | ```ERROR_GENERAL``` | Failed to reboot the device |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.harakiri"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="head.Properties"></a>
# Properties

The following properties are provided by the Controller plugin:

Controller interface properties:

| Property | Description |
| :-------- | :-------- |
| [status](#property.status) <sup>RO</sup> | Information about plugins, including their configurations |
| [links](#property.links) <sup>RO</sup> | Information about active connections |
| [processinfo](#property.processinfo) <sup>RO</sup> | Information about the framework process |
| [subsystems](#property.subsystems) <sup>RO</sup> | Status of the subsystems |
| [discoveryresults](#property.discoveryresults) <sup>RO</sup> | SSDP network discovery results |
| [environment](#property.environment) <sup>RO</sup> | Value of an environment variable |
| [configuration](#property.configuration) | Configuration object of a service |

<a name="property.status"></a>
## *status <sup>property</sup>*

Provides access to the information about plugins, including their configurations.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | A list of loaded plugins |
| (property)[#] | object | (a plugin entry) |
| (property)[#].callsign | string | Instance name of the plugin |
| (property)[#].locator | string | Library name |
| (property)[#].classname | string | Class name |
| (property)[#].autostart | string | Determines if the plugin is to be started automatically along with the framework |
| (property)[#]?.precondition | array | <sup>*(optional)*</sup> List of subsystems the plugin depends on |
| (property)[#]?.precondition[#] | string | <sup>*(optional)*</sup> (a subsystem entry) (must be one of the following: *Platform*, *Network*, *Security*, *Identifier*, *Internet*, *Location*, *Time*, *Provisioning*, *Decryption,*, *Graphics*, *WebSource*, *Streaming*) |
| (property)[#]?.configuration | object | <sup>*(optional)*</sup> Custom configuration properties of the plugin |
| (property)[#].state | string | State of the plugin (must be one of the following: *Deactivated*, *Deactivation*, *Activated*, *Activation*, *Suspended*, *Resumed*, *Precondition*) |
| (property)[#].processedrequests | number | Number of API requests that have been processed by the plugin |
| (property)[#].processedobjects | number | Number of objects that have been processed by the plugin |
| (property)[#].observers | number | Number of observers currently watching the plugin (WebSockets) |
| (property)[#]?.module | string | <sup>*(optional)*</sup> Name of the plugin from a module perspective (used e.g. in tracing) |
| (property)[#]?.hash | string | <sup>*(optional)*</sup> SHA256 hash identifying the sources from which this plugin was build |

> The *callsign* shall be passed as the index to the property, e.g. *Controller.1.status@DeviceInfo*. If the *callsign* is omitted, then status of all plugins is returned.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The plugin does not exist |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.status@DeviceInfo"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "callsign": "DeviceInfo", 
            "locator": "libWPEFrameworkDeviceInfo", 
            "classname": "DeviceInfo", 
            "autostart": "True", 
            "precondition": [
                "Platform"
            ], 
            "configuration": {}, 
            "state": "Activated", 
            "processedrequests": 2, 
            "processedobjects": 0, 
            "observers": 0, 
            "module": "Plugin_DeviceInfo", 
            "hash": "custom"
        }
    ]
}
```
<a name="property.links"></a>
## *links <sup>property</sup>*

Provides access to the information about active connections.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of active connections |
| (property)[#] | object | (a connection entry) |
| (property)[#].remote | string | IP address (or FQDN) of the other side of the connection |
| (property)[#].state | string | State of the link (must be one of the following: *WebServer*, *WebSocket*, *RawSocket*, *Closed*, *Suspended*) |
| (property)[#].activity | boolean | Denotes if there was any activity on this connection |
| (property)[#].id | number | A unique number identifying the connection |
| (property)[#]?.name | string | <sup>*(optional)*</sup> Name of the connection |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.links"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "remote": "localhost:52116", 
            "state": "RawSocket", 
            "activity": false, 
            "id": 1, 
            "name": "Controller"
        }
    ]
}
```
<a name="property.processinfo"></a>
## *processinfo <sup>property</sup>*

Provides access to the information about the framework process.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Information about the framework process |
| (property).threads | array | Thread pool |
| (property).threads[#] | number | (a thread entry) |
| (property).pending | number | Pending requests |
| (property).occupation | number | Pool occupation |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.processinfo"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "threads": [
            0
        ], 
        "pending": 0, 
        "occupation": 2
    }
}
```
<a name="property.subsystems"></a>
## *subsystems <sup>property</sup>*

Provides access to the status of the subsystems.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Status of the subsystems |
| (property)[#] | object |  |
| (property)[#].subsystem | string | Subsystem name (must be one of the following: *Platform*, *Network*, *Security*, *Identifier*, *Internet*, *Location*, *Time*, *Provisioning*, *Decryption,*, *Graphics*, *WebSource*, *Streaming*) |
| (property)[#].active | boolean | Denotes whether the subsystem is active (true) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.subsystems"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "subsystem": "Platform", 
            "active": true
        }
    ]
}
```
<a name="property.discoveryresults"></a>
## *discoveryresults <sup>property</sup>*

Provides access to the SSDP network discovery results.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of network discovery results |
| (property)[#] | object | (a discovery result entry) |
| (property)[#].locator | string |  |
| (property)[#].latency | number |  |
| (property)[#].model | string |  |
| (property)[#].secure | boolean |  |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.discoveryresults"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "locator": "", 
            "latency": 0, 
            "model": "", 
            "secure": true
        }
    ]
}
```
<a name="property.environment"></a>
## *environment <sup>property</sup>*

Provides access to the value of an environment variable.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Value of an environment variable |

> The *variable* shall be passed as the index to the property, e.g. *Controller.1.environment@SHELL*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The variable is not defined |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.environment@SHELL"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "/bin/sh"
}
```
<a name="property.configuration"></a>
## *configuration <sup>property</sup>*

Provides access to the configuration object of a service.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | The configuration JSON object |

> The *callsign* shall be passed as the index to the property, e.g. *Controller.1.configuration@WebKitBrowser*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The service does not exist |
| 1 | ```ERROR_GENERAL``` | Failed to update the configuration |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.configuration@WebKitBrowser"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {}
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.configuration@WebKitBrowser", 
    "params": {}
}
```
#### Set Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "null"
}
```
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following events are provided by the Controller plugin:

Controller interface events:

| Event | Description |
| :-------- | :-------- |
| [all](#event.all) | Signals each and every event in the system |
| [statechange](#event.statechange) | Signals a plugin state change |
| [subsystemchange](#event.subsystemchange) | Signals a subsystem state change |

<a name="event.all"></a>
## *all <sup>event</sup>*

Signals each and every event in the system. The Controller plugin is an aggregator of all the events triggered by a specific plugin. All notifications send by any plugin are forwarded over the Controller socket as an event.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Callsign of the originator plugin of the event |
| params.data | object | Object that was broadcasted as an event by the originator plugin |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.all", 
    "params": {
        "callsign": "WebKitBrowser", 
        "data": {}
    }
}
```
<a name="event.statechange"></a>
## *statechange <sup>event</sup>*

Signals a plugin state change

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Callsign of the plugin that changed state |
| params.state | string | State of the plugin (must be one of the following: *Deactivated*, *Deactivation*, *Activated*, *Activation*, *Suspended*, *Resumed*, *Precondition*) |
| params.reason | string | Cause of the state change (must be one of the following: *Requested*, *Automatic*, *Failure*, *MemoryExceeded*, *Startup*, *Shutdown*) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.statechange", 
    "params": {
        "callsign": "WebKitBrowser", 
        "state": "Activated", 
        "reason": "Requested"
    }
}
```
<a name="event.subsystemchange"></a>
## *subsystemchange <sup>event</sup>*

Signals a subsystem state change

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | array |  |
| params[#] | object |  |
| params[#].subsystem | string | Subsystem name (must be one of the following: *Platform*, *Network*, *Security*, *Identifier*, *Internet*, *Location*, *Time*, *Provisioning*, *Decryption,*, *Graphics*, *WebSource*, *Streaming*) |
| params[#].active | boolean | Denotes whether the subsystem is active (true) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.subsystemchange", 
    "params": [
        {
            "subsystem": "Platform", 
            "active": true
        }
    ]
}
```
