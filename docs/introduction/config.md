Thunder uses a JSON configuration file to modify the its behaviour. By default it looks for a config file in `/etc/WPEFramework/config.json` on Linux, although a custom path can be specified at launch.

When building Thunder, it will generate a default config file based on the options provided to CMake at configure-time using the generic config builder here: [https://github.com/rdkcentral/Thunder/blob/master/Source/WPEFramework/GenericConfig.cmake](https://github.com/rdkcentral/Thunder/blob/master/Source/WPEFramework/GenericConfig.cmake)

This section documents the available options for WPEFramework. This is different from the plugin-specific configuration which is documented elsewhere.

!!! note
	Any options that are children of a parent option are documented as `parent.child`. E.G `parentOption.childOption = true` equates to the following JSON
	
	```json
	{
	   "parentOption":{
	      "childOption":true
	   }
	}
	```

## Configuration Options

| Option Name                       | Description                                                  | Data Type | Default                                                      | Example                                               |
| --------------------------------- | ------------------------------------------------------------ | --------- | ------------------------------------------------------------ | ----------------------------------------------------- |
| model                             | Friendly name for the device Thunder is running on. Can be overridden with the `MODEL_NAME` env var | string    | -                                                            | My STB                                                |
| port                              | The port Thunder will listen on for HTTP(S) requests.        | integer   | 80                                                           | 9998                                                  |
| binding                           | The interface Thunder will bind to and listen on. Set to `0.0.0.0` to listen on all available interfaces. | string    | 0.0.0.0                                                      | 127.0.0.1                                             |
| interface                         | The network interface Thunder will bind on. If empty, will pick the first appropriate interface. | string    | -                                                            | `eth0`                                                |
| prefix                            | URL prefix for the REST/HTTP endpoint                        | string    | Service                                                      |                                                       |
| jsonrpc                           | URL prefix for the JSON-RPC endpoint                         | string    | jsonrpc                                                      |                                                       |
| persistentpath                    | Directory to store persistent data in.<br /><br /> Each plugin will have an associated directory underneath this corresponding to the callsign of the plugin. | string    | -                                                            | /opt/wpeframework/                                    |
| datapath                          | Read-only directory plugins can read data from. <br /><br />Each plugin will have an associated directory underneath this corresponding to the callsign of the plugin. | string    | -                                                            | usr/share/wpeframework                                |
| systempath                        | Directory plugin libraries are installed and available in    | string    | -                                                            | /usr/lib/wpeframework/                                |
| volatilepath                      | Directory to store volatile temporary data. <br /><br />Each plugin will have an associated directory underneath this corresponding to the callsign of the plugin | string    | /tmp                                                         | /tmp/                                                 |
| proxystubpath                     | Directory to search for the generated proxy stub libraries   | string    | -                                                            | /usr/lib/wpeframework/proxystubs                      |
| postmortempath                    | Directory to store debugging info (worker pool information, debug data) in the event of a plugin or server crash. <br /><br />If breakpad is found during build, will store breakpad mindumps here | string    | /opt/minidumps                                               | /opt/minidumps                                        |
| communicator                      | Socket to listen for COM-RPC messages. Can be a filesystem path on Linux for a Unix domain socket, or a TCP socket. <br /><br />For unix sockets, the file permissions can be specified by adding a `|` followed by the numeric permissions | string    | /tmp/communicator\|0777                                      | 127.0.0.1:4000                                        |
| redirect                          | Redirect incoming HTTP requests to the root Thunder URL to this address | string    | http://127.0.0.1/Service/Controller/UI                       | http://127.0.0.1/Service/Controller/UI                |
| idletime                          | Amount of time (in seconds) to wait before closing and cleaning up idle client connections. If no activity occurs over a connection for this time Thunder will close it. | integer   | 180                                                          | 180                                                   |
| softkillcheckwaittime             | When killing an out-of-process plugin, the amount of time to wait after sending a SIGTERM signal to the process before checking & trying again | integer   | 3                                                            | 3                                                     |
| hardkillcheckwaittime             | When killing an out-of-process plugin, the amount of time to wait after sending a SIGKILL signal to the process before trying again | integer   | 10                                                           | 10                                                    |
| legacyinitalize                   | Enables legacy Plugin initialization behaviour where the Deinitialize() method is not called on if Initialize() fails. For backwards compatibility | bool      | false                                                        | false                                                 |
| defaultmessagingcategories        | See "Messaging configuration" below                          | object    | -                                                            | -                                                     |
| defaultwarningreportingcategories | See "Warning Reporting Configuration" below                  | array     | -                                                            | -                                                     |
| process.user                      | The Linux user the WPEFramework process runs as              | string    | -                                                            | myusr                                                 |
| process.group                     | The Linux group the WPEFramework process runs under          | string    | -                                                            | mygrp                                                 |
| process.priority                  | The nice priority of the WPEFramework process                | integer   | -                                                            | 0                                                     |
| process.policy                    | The linux scheduling priority of the WPEFramework process. Valid values are: `Batch`, `FIFO`, `Idle`, `RoundRobin`, `Other` | string    | -                                                            | OTHER                                                 |
| process.oomadjust                 | The OOM killer score (see [here](https://lwn.net/Articles/317814/) for more info) | integer   | -                                                            | 0                                                     |
| process.stacksize                 | The default stack size in bytes for spawned threads. If not set or 0, will use to Linux defaults | integer   | -                                                            | 4096                                                  |
| process.umask                     | Set the WPEFramework umask value                             | integer   | -                                                            | 077                                                   |
| input.locator                     | If using Thunder input handling. Socket to receive key events over | string    | /tmp/keyhandler\|0766                                        | -                                                     |
| input.type                        | If using Thunder input handling.  Input device type (either `device` (`/dev/uinput`) or `virtual` (json-rpc api) | string    | Virtual                                                      | Device                                                |
| input.output                      | If using Thunder input handling.  Whether input events should be re-output for forwarding | bool      | true                                                         | -                                                     |
| configs                           | Directory to search for plugin config files                  | string    | If not set, will default to `<wpeframework-config-directory>/plugins`  (e.g. /etc/WPEFramework/plugins) | `/etc/thunder/plugins`                                |
| ethernetcard                      | :warning: **Deprecated** <br /><br />Using the MAC address of this interface, Thunder will generate a unique identifier | string    | -                                                            | `eth0`                                                |
| plugins                           | :warning: **Deprecated** <br /><br/> Array of plugin configurations. Not recommended  - each plugin should have its own config file. Normally only used for Controller plugin configuration | array     | -                                                            | -                                                     |
| environments                      | Array of environment variables to set for the WPEFramework process. Each item in the array should be an object with `key`, `value`, and `override` properties.<br /><br />Values can be built from path substitutions (e.g. `%persistentpath%`) | array     | -                                                            | `[{"name": "FOO", "value": "BAR", "override": true}]` |
| exitreasons                       | Array of plugin exit/deactivation reasons that should result in the postmortem handler being triggered (e.g. to create minidump, dump worker thread status) | array     | -                                                            | `["Failure","MemoryExceeded","WatchdogExpired"]`      |
| latitude                          | Default latitude value - can be overridden & retrieved by plugins to provide geolocation functionality<br /><br />Value is divided by 1,000,000 | int       | 51832547 (translates to 51.832547)                           | 51832547                                              |
| longitude                         | Default longitude value - can be overridden & retrieved by plugins to provide geolocation functionality<br /><br />Value is divided by 1,000,000 | int       | 5674899 (translates to 5.674899)                             | 5674899                                               |
| messagingport                     | By default, the messaging engine sends log/trace messages over a unix socket. Provide a TCP port here to use that port instead if desired | int       | -                                                            | 3000                                                  |
| processcontainers.logging         | Path for container logs if using process container. Behaviour will vary depending on container backend | string    | -                                                            | -                                                     |
| linkerpluginpaths                 | Array of additional directories to search for .so files      | array     | -                                                            | -                                                     |
| observe.proxystubpath             | Directory to monitor for new proxy stub libraries. If libraries are added during runtime, WPEFramework will load these new proxystubs | string    | -                                                            | /root/wpeframework/dynamic/proxystubs                 |
| observe.configpath                | Directory to monitor for new plugin configuration files. If config files are added during runtime, WPEFramework will load them | string    | -                                                            | /root/wpeframework/dynamic/config                     |
| hibernate.locator                 | Configuration for the process hibernation feature (alpha)    | string    | -                                                            | -                                                     |

## Messaging Configuration

The config file can be used to enable/disable the default messaging categories used for logging, tracing and warning reporting. It is possible to use the MessageControl plugin to edit these values at runtime.

Messages are split into 2 types, logging and tracing. Each type has a list of categories and modules which can be marked as enabled or disabled. If a category/module is not present in the config, it will be disabled.

Below is an example of the messaging section in the config:

```json
{
    "messaging": {
        "logging": {
            "abbreviated": true, // (1)
            "settings": [
                {
                    "category": "Notification", // (2)
                    "enabled": true
                },
                {
                    "module": "Plugin_SamplePlugin", // (3)
                    "enabled": true
                }
            ]
        },
        "tracing": {
            "settings": [
                {
                    "category": "Fatal", // (4)
                    "enabled": true
                },
                {
                    "module": "Plugin_SamplePlugin", // (5)
                    "enabled": true
                }
            ]
        }
    }
}
```

1. Reduce the amount of information in the log messages (e.g. removing line number and class name)
2. Enable logging messages in the `Notification` category
3. Enable **all** logging categories used in the SamplePlugin plugin. The module name reflects the `MODULE_NAME` definition in the plugin `Module.h`
4. Enable trace messages in the `Fatal` category
5. Enable **all** tracing categories used in the SamplePlugin plugin. The module name reflects the `MODULE_NAME` definition in the plugin `Module.h`

!!! tip
	It is also possible to supply a filepath instead of an object to allow storing messaging configuration in a seperate file
	```json
	{
		"messaging": "/path/to/messagingconfig.json"
	}
	```

Out of the box, the following categories are available:

* Logging
	* Startup
	* Shutdown
	* Notification
	* Error
	* ParsingError
	* Fatal
	* Crash
* Tracing
	* Constructor
	* Destructor
	* CopyConstructor
	* AssignmentOperator
	* MethodEntry
	* MethodExit
	* Duration
	* Text
	* Initialisation
	* Information
	* Warning
	* Error
	* Fatal

In addition to these, it is possible for a plugin to define its own categories that can be enabled/disabled. See **TODO** for more info on how to do this. 

## Warning Reporting Configuration

!!! note
	Warning Reporting is only available if Thunder is compiled with the `WARNING_REPORTING` option.

Warning Reporting enabled various runtime checks for potentially erroneous conditions, and can be enabled on a per-category basis. These are typically time-based - i.e. a warning will be reported if something exceeded an allowable time. 

Each category can also have its own configuration to tune the thresholds for triggering the warning.

Below is an example Warning Reporting configuration

```json
{
    "warningreporting": [
        {
            "category": "TooLongWaitingForLock", // (1)
            "enabled": true,
            "excluded": {
            	"callsigns": [
                    "com.example.SamplePlugin" // (2)
                ],
                "modules": [
                    "Plugin_SamplePlugin" // (3)
                ]
            }
            "config": {
                "reportbound": 1000, // (4)
                "warningbound": 2000 // (5)
            }
        }
    ]
}
```

1. Name of category to enable
2. Callsigns of plugins to exclude from the warning reporting category
3. Module names to exclude from this warning reporting category
4. Report bound indicates the amount of time (ms) that must be exceeded for a report to be generated
5. Warning bound indicates the amount of time (ms) that must be exceeded for a warning to be generated (considered higher severity than a report). Note warning bound should be >= report bound

Out of the box, the following warning reporting categories are available:

* TooLongWaitingForLock
* SinkStillHasReference
* TooLongInvokeRPC
* JobTooLongToFinish
* JobTooLongWaitingInQueue
* TooLongDecrypt
* JobActiveForTooLong
