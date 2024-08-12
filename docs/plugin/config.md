In addition to the global Thunder config file (see [here](../introduction/config.md) for more details), each plugin has its own configuration file.

This file contains some generic information about the plugin (name of the library, callsign, execution mode etc), but can easily be extended by developers to include their own options. Using this method for configuring plugins ensures consistency between plugins - there is therefore a single place to configure all plugins.

!!! note
	Any options that are children of a parent option are documented as `parent.child`. E.G `parentOption.childOption = true` equates to the following JSON
	

	```json
	{
	   "parentOption":{
	      "childOption":true
	   }
	}
	```


## Default Options

These are the options applicable to all plugins

| Option Name                      | Description                                                  | Data Type | Default     | Example                          |
| -------------------------------- | ------------------------------------------------------------ | --------- | ----------- | -------------------------------- |
| callsign                         | The callsign of the plugin. This is arbitrary and does not need to reflect any class names in the code<br /><br />Some people like to use reverse domain names for their plugin callsigns, although it's not a requirement | string    | -           | com.example.SamplePlugin         |
| locator                          | The name of the library (.so/.dll) that contains the plugin code. | string    | -           | libSamplePlugin.so               |
| classname                        | The name of the class to be instantiated when loading the plugin | string    | -           | SamplePlugin                     |
| startmode                        | Default start state of the plugin when loading (Unavailable, Deactivated, Activated).<br /><br />Setting to Activated will automatically start the plugin | enum      | Deactivated | Activated                        |
| resumed                          | When starting a plugin that supports suspend/resume (IStateControl), when activating the plugin start it in a resumed state instead of suspended | bool      | false       | true                             |
| webui                            | A plugin can be configured to act as a web server hosting generic files, typically used for hosting a UI.<br /><br />This config option sets the URL the server should run under, relative to the plugin callsign. Files will be served from a corresponding directory in the plugin's data dir.<br /><br />If not set, web server functionality disabled | string    | -           | UI                               |
| precondition                     | Array of subsystems that are preconditions for plugin activation[^1]. If any of the provided subsystems aren't marked as active, the plugin will not activate until those preconditions are met. | array     | -           | ["GRAPHICS"]                     |
| termination                      | Array of subsystems that, when not present, will cause the plugin to deactivate if it's running[^1]. Typically paired with preconditions.<br /><br />E.G If a plugin requires the graphics subsystem, adding `NOT_GRAPHICS` in the termination options will cause the plugin to deactivate if the graphics subsystem is marked as down. | array     | -           | ["NOT_GRAPHICS"]                 |
| communicator                     | Custom (private) COM-RPC socket to use just for this plugin instead of the main Thunder communicator socket.<br /><br />If set to null, will create a unix domain socket named after the callsign of the plugin.<br /><br />If set to a string, will create a socket at the specified address (unix domain socket path or TCP socket). Path must be unique. | string    | -           | null                             |
| configuration.root.locator       | When running out-of-process, the name of the library to load in the out-of-process host. Only needed if plugin is split into a core and Implementation library | string    | -           | libSamplePluginImplementation.so |
| configuration.root.user          | When running out of process, the linux user to run the process as | string    | -           | plugin-user                      |
| configuration.root.group         | When running out of process, the linux group to run the process as | string    | -           | plugin-group                     |
| configuration.root.threads       | When running out of process, the max number of threads that the ThunderPlugin host worker pool will use | int       | 1           | 2                                |
| configuration.root.priority      | When running out of process, the priority of the process     | int       | -           | -                                |
| configuration.root.outofprocess  | :warning: **Deprecated**: use `configuration.root.mode` instead.<br /><br />Set to true to run plugin out of process | bool      | false       | true                             |
| configuration.root.mode          | The execution mode the plugin should run as. Includes: **Off** (in-process), **Local** (out-of-process), **Container** (out-of-process, in a container), **Distributed** (out-of-process, running on another device on the network) | string    | Local[^2]   | Off                              |
| configuration.root.remoteaddress | If running in distributed mode, the address of the COM-RPC socket on the network to connect to | string    | -           | -                                |
| persistentpathpostfix            | Instead of using the plugin callsign, use this as the persistent path postfix. Useful if you are cloning plugins and want them to use the same persistent directory | string    | -           | sharedPersistentDirectory        |
| volatilepathpostfix              | Instead of using the plugin callsign, use this as the volatile path postfix. Useful if you are cloning plugins and want them to use the same volatile directory | string    | -           | sharedVolatileDirectory          |
| systemrootpath                   | Custom directory to search for the plugin .so files          | string    | -           |                                  |
| startuporder                     | A simple mechanism for prioritising autostart plugins. Plugins will be started based on their startup order value - e.g. lower values will cause plugins to be started earlier than plugins with higher values | int       | 50          | 10                               |

### Sample Configuration

```json
{
   "locator":"libThunderSamplePlugin.so",
   "classname":"SamplePlugin",
   "startmode":"Activated",
   "configuration":{
      "root":{
         "mode":"Off"
      }
   }
}
```

## Creating custom configuration options

As a developer, it is possible to extend the default plugin configuration with your own options specific to plugin requirements.

These extended options will be available in the `configuration` property in the config file. Below is a worked example on creating a custom plugin configuration.

### 1. Define config structure

For this example, we will create a config file for an example plugin that returns a greeting to the user. In the config file, we would like to choose which greetings could be returned. The goal is to have a config file that looks as follows, where `greetings` is our custom greetings option

```json
{
   "locator":"libThunderGreeterPlugin.so",
   "classname":"Greeter",
   "startmode":"Activated",
   "configuration":{
      "greetings": ["Hello", "Good Morning", "Hi"]
      "root":{
         "mode":"Off"
      }
   }
}
```

First, create the JSON container object to hold your configuration

```cpp
using namespace Thunder;

class GreeterPluginConfiguration : public Core::JSON::Container {
public:
    GreeterPluginConfiguration()
        : Core::JSON::Container()
        , Greetings()
    {
        Add(_T("greetings"), &Greetings); // (1)
    }
    ~GreeterPluginConfiguration() = default;

    GreeterPluginConfiguration(GreeterPluginConfiguration&&) = delete;
    GreeterPluginConfiguration(const GreeterPluginConfiguration&) = delete;
    GreeterPluginConfiguration& operator=(GreeterPluginConfiguration&&) = delete;
    GreeterPluginConfiguration& operator=(const GreeterPluginConfiguration&) = delete;

public:
    Core::JSON::ArrayType<Core::JSON::String> Greetings; // (2)
};
```

1. Map a json object name to c++ object that will store the value
2. This will hold the value set in the config file. In this case, an array of strings

### 2. Define default build-time configuration values

Using the code-generator tooling in Thunder, it is possible to set default values for the auto-generated config file. This allows setting sane default values for a plugin configuration at build time.

There are two versions of the config generator. Both will produce JSON files containing the final config, but the modern version is recommended.

#### Modern Config Generator

Create a file called `<PluginName>.conf.in` in your plugin source code. This will hold the default config values. The file is a python source file, and the final config will be built from the variables defined in this file.

Anything surrounded by `@` symbols will be replaced with a value from CMake - see [here](https://cmake.org/cmake/help/latest/command/configure_file.html) for more detail. This allows setting default values in the CMake file, which can then be customised at build time.

To edit config options, simply create variables with the corresponding name. Nested config options can be built by constructing `Json` objects. The `locator` and `classname` values will be automatically assumed at build time, although can be overridden with custom values if required.

For example, the below will create default values for our Greeter plugin

```python
startmode = "Activated"

configuration = JSON()
greetings = ["Hello", "Good Morning", "Hi"]
configuration.add("Greetings", greetings)

root = JSON()
root.add("mode", "Off")
configuration.add("root", root)
```


#### Legacy Config Generator

!!! info
	It is now recommended to use the modern config generator, which supports new CMake versions, is more flexible and easier to maintain. **If using CMake >3.20, the modern generator is the only option.**
	

	If both legacy and modern config files exist, the generator will prefer the modern one (although will generate both and warn if they produce different outputs)

The legacy config generator uses [CMakepp QuickMap](https://github.com/toeb/cmakepp/blob/master/cmake/quickmap/README.md) syntax to build the config JSON file. To use the legacy syntax, create a file called `<PluginName>.conf` in your plugin source code.

Example for our Greeter plugin:

```cmake
set (startmode "Activated")

map()
	kv(mode "Off")
end()
ans(rootobject)

map()
	kv("Greetings", "Hello" "Good Morning" "Hi")
end()
ans(configuration)

map_append(${configuration} root ${rootobject})
```

As this is CMake code, the `${}` syntax can be used to insert CMake variables into the config file.

### 3. Loading custom config

To read our custom config data, during plugin initialisation construct an object for the configuration class written earlier, and retrieve the config string from the IShell interface 

```cpp
const string GreeterPlugin::Initialize(PluginHost::IShell* service)
{
	// ...
    GreeterPluginConfiguration config;
    config.FromString(service->ConfigLine()); // (1)
}
```

1. IShell returns the config as a string, so parse this and build our JSON object

Now access the properties on the `config` object to retrieve values.

The default config options can be accessed directly from the `IShell` interface

```cpp
string className = service->ClassName();
```

### 4. Viewing/modifying plugin configurations at runtime

The Controller plugin can be used to retrieve and modify the configuration of a plugin at runtime.

#### Get Config

Make a JSON-RPC call to `Controller.configuration@<PluginName>` with no parameters

:arrow_right: Request

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "Controller.1.configuration@SamplePlugin"
}
```

:arrow_left: Response:

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "Greetings": [
            "Hello",
            "Good Morning",
            "Hi"
        ],
        "root": {
            "mode": "On"
        }
    }
}
```

#### Modify Config

Controller will allow modifying the in-memory config for the plugin. This will not survive restarts of the Thunder daemon

!!! warning
	It is only possible to modify plugin configuration when the plugin is not currently activated. If the plugin is activated and an attempt is made to modify the config, an `ERROR_GENERAL` error will be returned. Deactivate the plugin and try again.

Make a request to `Controller.configuration@<PluginName>` with the parameters containing the entire config object that should be set. It is not possible to modify some options, including callsign and locator

:arrow_right: Request

```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"method": "Controller.1.configuration@SamplePlugin",
	"params": {
		"Greetings": [
			"Goodbye",
			"Bye"
		],
		"root": {
			"mode": "On"
		}
	}
}
```

:arrow_left: Response

```
{
	"jsonrpc": "2.0",
	"id": 1,
	"result": null
}
```

## Path Substitution

If your custom plugin configuration contains values for filesystem paths, instead of hardcoding specific paths it is possible to use path variable substitution. This allows you to retrieve and use the values for paths set in the main Thunder config file in your configuration automatically.

For example, if the custom plugin config contained the following

```json
{
	"customDirectory": "%persistentpath%/myDirectory"
}
```

Then in the plugin code, calling `_service->Substitute(...)` would replace the `%persistentpath%` variable with the `persistentpath` value from the main config file.

```cpp
std::string configValue = _config.CustomDirectory().Value();
std::string realPath = _service->Substitute(configValue);
```

The following values are supported as substitution variables:

* `%datapath%`
* `%persistentpath%`
* `%systempath%`
* `%volatilepath%`
* `%proxystubpath%`
* `%postmortempath%`


[^1]:	Plugin metadata can enforce precondition/termination requirements in code, which can then be extended via the plugin config.
[^2]:	If the `root` config section is missing entirely from the plugin configuration, it will default to OFF (in-process) instead.
