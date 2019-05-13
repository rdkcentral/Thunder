<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Controller_Plugin"></a>
# Controller Plugin

**Version: 1.0**

Controller functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Controller plugin. It includes detailed specification of its configuration, methods provided and notifications sent.

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
| <a name="ref.WPEF">[WPEF](https://github.com/WebPlatformForEmbedded/WPEFramework/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | WPEFramework API Reference |

<a name="head.Description"></a>
# Description

The Controller plugin controls (activates and deactivates) the configured plugins. The plugin is part of the framework and cannot be activated or deactivated itself, thus as well cannot not be unloaded.

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

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

The following API is provided by the plugin via JSON-RPC:

- [activate](#method.activate)
- [deactivate](#method.deactivate)
- [exists](#method.exists)
- [status](#method.status)
- [links](#method.links)
- [process](#method.process)
- [subsystems](#method.subsystems)
- [startdiscovery](#method.startdiscovery)
- [discovery](#method.discovery)
- [getenv](#method.getenv)
- [getconfig](#method.getconfig)
- [setconfig](#method.setconfig)
- [storeconfig](#method.storeconfig)
- [download](#method.download)
- [delete](#method.delete)
- [harakiri](#method.harakiri)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.activate"></a>
## *activate*

Activates a plugin.

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
| 12 | ```ERROR_INPROGRESS``` | Operation in progress |
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
## *deactivate*

Deactivates a plugin.

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
| 12 | ```ERROR_INPROGRESS``` | Operation in progress |
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
<a name="method.exists"></a>
## *exists*

Checks if a JSON-RPC method exists.

### Description

Use this method to check if a method is currently callable. 

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.designator | string | Method designator; if callsign is omitted then the Controller itself will be queried |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | number | Code specifying the availability (0: available, 2: unavailable callsign, 22: method unavailable, 38: version unavailable |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.exists", 
    "params": {
        "designator": "DeviceInfo.1.system"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": 0
}
```
<a name="method.status"></a>
## *status*

Retrieves information about plugins.

### Description

Use this method to fetch information about plugins, including their configuration.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.callsign | string | <sup>*(optional)*</sup> Callsign of the plugin (if omitted or empty, then status of all plugins is returned) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | A list of loaded plugins |
| result[#] | object | (a plugin entry) |
| result[#].callsign | string | Instance name of the plugin |
| result[#].locator | string | Library name |
| result[#].classname | string | Class name |
| result[#].autostart | string | Determines if the plugin is to be started automatically along with the framework |
| result[#]?.precondition | array | <sup>*(optional)*</sup> List of subsystems the plugin depends on |
| result[#]?.precondition[#] | string | <sup>*(optional)*</sup> (a subsystem entry) (must be one of the following: *Platform*, *Network*, *Security*, *Identifier*, *Internet*, *Location*, *Time*, *Provisioning*, *Decryption,*, *Graphics*, *WebSource*, *Streaming*) |
| result[#]?.configuration | object | <sup>*(optional)*</sup> Custom configuration properties of the plugin |
| result[#].state | string | State of the plugin (must be one of the following: *Deactivated*, *Deactivation*, *Activated*, *Activation*, *Suspended*, *Resumed*, *Precondition*) |
| result[#].processedrequests | number | Number of API requests that have been processed by the plugin |
| result[#].processedobjects | number | Number of objects that have been processed by the plugin |
| result[#].observers | number | Number of observers currently watching the plugin (WebSockets) |
| result[#]?.module | string | <sup>*(optional)*</sup> Name of the plugin from a module perspective (used e.g. in tracing) |
| result[#]?.hash | string | <sup>*(optional)*</sup> SHA256 hash identifying the sources from which this plugin was build |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The plugin does not exist |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.status", 
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
            "state": "activated", 
            "processedrequests": 2, 
            "processedobjects": 0, 
            "observers": 0, 
            "module": "Plugin_DeviceInfo", 
            "hash": "custom"
        }
    ]
}
```
<a name="method.links"></a>
## *links*

Retrieves information about active connections.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | List of active connections |
| result[#] | object | (a connection entry) |
| result[#].remote | string | IP address (or FQDN) of the other side of the connection |
| result[#].state | string | State of the link (must be one of the following: *WebServer*, *WebSocket*, *RawSocket*, *Closed*, *Suspended*) |
| result[#].activity | boolean | Denotes if there was any activity on this connection |
| result[#].id | number | A unique number identifying the connection |
| result[#]?.name | string | <sup>*(optional)*</sup> Name of the connection |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.links"
}
```
#### Response

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
<a name="method.process"></a>
## *process*

### Description

Retrieves information about the framework process.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.threads | array | Thread pool |
| result.threads[#] | number | (a thread entry) |
| result.pending | number | Pending requests |
| result.occupation | number | Pool occupation |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.process"
}
```
#### Response

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
<a name="method.subsystems"></a>
## *subsystems*

Retrieves status of subsystems.

### Description

Use this method to get a list of subsystems and their status.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#]?.name | string | <sup>*(optional)*</sup> Subsystem name (must be one of the following: *Platform*, *Network*, *Security*, *Identifier*, *Internet*, *Location*, *Time*, *Provisioning*, *Decryption,*, *Graphics*, *WebSource*, *Streaming*) |
| result[#]?.active | boolean | <sup>*(optional)*</sup> Denotes whether the subsystem is active (true) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.subsystems"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "name": "Platform", 
            "active": true
        }
    ]
}
```
<a name="method.startdiscovery"></a>
## *startdiscovery*

Starts the network discovery.

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
<a name="method.discovery"></a>
## *discovery*

Retrieves network discovery results.

### Description

Use this method to retrieve SSDP network discovery results.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | List of network discovery results |
| result[#] | object | (a discovery result entry) |
| result[#].locator | string |  |
| result[#].latency | number |  |
| result[#].model | string |  |
| result[#].secure | boolean |  |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.discovery"
}
```
#### Response

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
<a name="method.getenv"></a>
## *getenv*

Retrieves the value of an environment variable.

### Description

Use this method to get values of shell environment variables.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.variable | string | Name of the environment variable to get the value of |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Value of the variable |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The given variable is not defined |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.getenv", 
    "params": {
        "variable": "SHELL"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "/bin/sh"
}
```
<a name="method.getconfig"></a>
## *getconfig*

Retrieves the configuration of a service.

### Description

Use this method to get configurations of framework services.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Name of the service to get the configuration of |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | The requested configuration object |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The service does not exist |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.getconfig", 
    "params": {
        "callsign": "WebKitBrowser"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {}
}
```
<a name="method.setconfig"></a>
## *setconfig*

Updates the configuration of a service.

### Description

Use this method to set configurations of framework services.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Name of the service to set the configuration of |
| params.configuration | object | Configuration object to set |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The service does not exist |
| 1 | ```ERROR_GENERAL``` | Failed to update the configuration |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.setconfig", 
    "params": {
        "callsign": "WebKitBrowser", 
        "configuration": {}
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
## *storeconfig*

Stores the configuration.

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
<a name="method.download"></a>
## *download*

Downloads a file to the persistent memory.

Also see: [downloadcompleted](#event.downloadcompleted)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.source | string | Source URL pointing to the file to download |
| params.destination | string | Path within the persistent storage where to save the file |
| params.hash | string | Base64-encoded binary SHA256 signature for authenticity verification |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 12 | ```ERROR_INPROGRESS``` | Operation in progress |
| 15 | ```ERROR_INCORRECT_URL``` | Incorrect URL given |
| 30 | ```ERROR_BAD_REQUEST``` | The given destination path or hash was invalid |
| 40 | ```ERROR_WRITE_ERROR``` | Failed to save the file to the persistent storage (e.g. the file already exists) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Controller.1.download", 
    "params": {
        "source": "http://example.com/test.txt", 
        "destination": "test", 
        "hash": "ODE5NjNFMEEzM0VDQ0JBOTI0MDRFOTY4QzY2NTAwNkQ="
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
<a name="method.delete"></a>
## *delete*

Removes contents of a directory from the persistent storage.

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
## *harakiri*

Reboots the device.

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
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following notifications are provided by the plugin:

- [all](#event.all)
- [statechange](#event.statechange)
- [downloadcompleted](#event.downloadcompleted)

<a name="event.all"></a>
## *all*

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
## *statechange*

Signals a plugin state change.

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
        "state": "activated", 
        "reason": "Requested"
    }
}
```
<a name="event.downloadcompleted"></a>
## *downloadcompleted*

Signals that a file download has completed.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.result | number | Download operation result (0: success) |
| params.source | string | Source URL identifying the downloaded file |
| params.destination | string | Path to the downloaded file in the persistent storage |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.downloadcompleted", 
    "params": {
        "result": 0, 
        "source": "http://example/com/test.txt", 
        "destination": "test.txt"
    }
}
```
