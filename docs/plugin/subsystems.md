Thunder provides "Subsystems" which are abstract categories of functionality (Network, Graphics, Internet) that can be marked as active/inactive by plugins. This mechanism allows plugins to delay activation until certain conditions are met, or ensure that plugins are deactivated in a suitable order.

For example, a web-browser plugin could be prevented from starting until the Internet and Graphics subsystems are ready. The Network subsystem could be marked as active by a network control plugin, and the graphics by a window manager/compositor plugin.

The state of each subsystem is tracked by the framework and can be queried or modified by individual plugins.


## Supported Subsystems

Thunder supports the following subsystems (enumerated in `Source/plugins/ISubSystem.h`)

| **Subsystem** | **Description**                                              |
| ------------- | ------------------------------------------------------------ |
| PLATFORM      | Platform is available                                        |
| SECURITY      | A security system can validate external requests (JSONRPC/WebRequest) |
| NETWORK       | Network connectivity has been established.                   |
| IDENTIFIER    | System identification has been accomplished.                 |
| GRAPHICS      | Graphics screen EGL is available.                            |
| INTERNET      | Network connectivity to the outside world has been established. |
| LOCATION      | Location of the device has been set.                         |
| TIME          | Time has been synchronized.                                  |
| PROVISIONING  | Provisioning information is available.                       |
| DECRYPTION    | Decryption functionality is available.                       |
| WEBSOURCE     | Content exposed via a local web server is available.         |
| STREAMING     | Content can be streamed.                                     |
| BLUETOOTH     | The Bluetooth subsystem is up and running.                   |
| INSTALLATION  | The Installation (e.g. Pakager) subsystem is up and running. |

All subsystems have a negated equivalent - e.g. `NOT_PLATFORM` and `NOT_SECURITY` that indicates the absence of those subsystems.

When the Thunder process starts, Controller will check to see which subsystems are provided by plugins (as defined in the plugin metadata or the Controller configuration). These subsystems are sometimes referred to "external", since their lifetime is managed by a plugin external to the Thunder core.

If the subsystem is not provided by a plugin, then Controller will mark the subsystem as active at startup. The opposite happens at shutdown, with Controller marking the subsystems as inactive.

### Subsystem Metadata

!!! warning
	Subsystem metadata is currently only accessible over COM-RPC, there are no corresponding JSON-RPC interfaces or datatypes generated for the subsystem COM-RPC interfaces. Only subsystem status can be retrieved over JSON-RPC


Since each subsystem has a corresponding COM-RPC interface, all subsystems can have metadata associated with them that describes the state of the subsystem in more detail.

For example, the `INTERNET` subsystem has fields to hold the public IP address of the device and the network type. When marking the subsystem as active, this can be populated by a plugin so other plugins can obtain this information easily.

```c++
struct EXTERNAL IInternet : virtual public Core::IUnknown {

    enum { ID = RPC::ID_SUBSYSTEM_INTERNET };

    enum { SUBSYSTEM = INTERNET };

    enum network_type : uint8_t {
        UNKNOWN,
        IPV4,
        IPV6
    };

    // Network information
    virtual string PublicIPAddress() const = 0;
    virtual network_type NetworkType() const = 0;

    static const TCHAR* ToString(const network_type value);
};
```

Default implementations of these interfaces are provided in `Source/Thunder/SystemInfo.h`, which populate the values with sane defaults, but it is expected plugins that provide that subsystem provide their own implementations as required.

## Using Subsystems

Plugins can use the `ISubsystem` interface (as implemented by `Source/Thunder/SystemInfo.h`) to retrieve information about the state of the subsystems and mark subsystems as active/inactive. This interface can be retrieved from the plugin shell provided to the plugin at initialisation.

### Mark Subsystem as Active

To mark a subsystem as active, call the `Set()` method on `ISubsystem`. 

```c++ linenums="1" hl_lines="8"
void TestPlugin::Initialize(PluginHost::IShell* service) {
    _service = service;
    
    // Other init code...
    
    PluginHost::ISubSystem* subSystems = _service->SubSystems();
    if (subSystems != nullptr) {
        subSystems->Set(PluginHost::ISubSystem::INTERNET, nullptr);
    }
}
```

If the subsystem contains metadata, then a plugin can register to be a provider of that subsystem metadata by doing the following:

* Create an implementation of `PluginHost::ISubSystem::I<Subsystem>` interface
* When setting the subsystem status, register that implementation with Thunder

Below is a simplistic implementation of `IInternet` that can be instantiated and provided to Thunder by a plugin:

```c++
class InternetInfo : public PluginHost::ISubSystem::IInternet {
public:
    InternetInfo()
        : _ipAddress("Unknown")
        , _networkType(network_type::UNKNOWN)
    {
    }
    ~InternetInfo() override = default;

    InternetInfo(const InternetInfo&) = default;
    InternetInfo(InternetInfo&&) = default;
    InternetInfo& operator=(const InternetInfo&) = default;
    InternetInfo& operator=(InternetInfo&&) = default;

    BEGIN_INTERFACE_MAP(InternetInfo)
    INTERFACE_ENTRY(PluginHost::ISubSystem::IInternet)
    END_INTERFACE_MAP

    // IInternet methods
    string PublicIPAddress() override {
        return _ipAddress;
    }

    network_type NetworkType() const override {
        return _networkType;
    };

private:
    string _ipAddress;
    network_type _networkType;
};
```

```c++
PluginHost::ISubSystem* subSystems = _service->SubSystems();
if (subSystems != nullptr) {
	// _internetInfo is a member variable of type Core::Sink<InternetInfo>
    subSystems->Set(PluginHost::ISubSystem::INTERNET, _internetInfo);
}
```

### Mark Subsystem as Inactive

To mark a subsystem as inactive, again use the `Set()` method, but provide the negated subsystem identifier

```c++ linenums="1" hl_lines="8"
void TestPlugin::Initialize(PluginHost::IShell* service) {
    _service = service;
    
    // Other init code...
    
    PluginHost::ISubSystem* subSystems = _service->SubSystems();
    if (subSystems != nullptr) {
        subSystems->Set(PluginHost::ISubSystem::NOT_INTERNET, nullptr);
    }
}
```

### Checking subsystem status

#### From inside a plugin

The `ISubSystem` interface provides an `IsActive()` method to check a subsystem's state and returns true if it's active.

Calling the `Get<>()` method can return a pointer to the subsystem interface so its metdata can be queried.

```c++
PluginHost::ISubSystem* subSystems = _service->SubSystems();
if (subSystems != nullptr) {
    if (subSystems->IsActive(PluginHost::ISubSystem::INTERNET)) {
        const PluginHost::ISubSystem::IInternet* internet = subSystems->Get<PluginHost::ISubSystem::IInternet>();
        TRACE(Trace::Information, (_T("Public IP address is: %s"), internet->PublicIPAddress().c_str()));
    }
}
```

#### Using Controller

Controller provides a `Subsystems` method that can be used to obtain the current subsystem state over JSON or COM-RPC. JSON-RPC example:

:arrow_right: Request

```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"method": "Controller.1.subsystems"
}
```

:arrow_left: Response

```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"result": [
		{
			"subsystem": "Platform",
			"active": true
		},
		{
			"subsystem": "Security",
			"active": true
		},
		{
			"subsystem": "Network",
			"active": true
		},
		...
	]
}
```

### Subsystem Change Notifications

!!! warning
	It's currently only possible to receive subsystem change notifications over COM-RPC

To subscribe to subsystem change notifications, plugins should create an implementation of `ISubSystem::INotification` that implements the `Updated()` method, then register it at plugin start with the plugin shell. It is currently only possible to subscribe to a general notification that is fired when any subsystem changes state.

It is fairly unlikely that plugins will need to use this, since the use of preconditions/terminations allows the framework to handle and react to subsystem changes automatically, but is provided for plugins that require extra control.

```c++
class Notification : public PluginHost::ISubSystem::INotification {
public:
    Notification() = default
    ~Notification() override = default;

    Notification(const InternetInfo&) = delete;
    Notification(InternetInfo&&) = delete;
    Notification& operator=(const InternetInfo&) = delete;
    Notification& operator=(InternetInfo&&) = delete;

public:
    // Some change 
    void Updated() override {
        TRACE(Trace::Information, (_T("The state of a subsystem has changed")));
    }

    BEGIN_INTERFACE_MAP(Notification)
    INTERFACE_ENTRY(PluginHost::ISubSystem::INotification)
    END_INTERFACE_MAP
};
```

## Plugin Metadata

Subsystems can be used to define plugin dependencies, and ensure that plugins start/stop in the correct order. This is done by configuring the plugin metadata.

The metadata holds 3 lists of values related to subsystems:

**Preconditions**

:	A precondition is a list of subsystems that must be active in order for the plugin to activate. 
	
	If an attempt is made to activate the plugin before the preconditions are met, the plugin will be moved to the "Precondition" state where it will remain until the preconditions are met. Thunder will then automatically move the plugin to the "Activated" state. See the [plugin lifecycle](./lifecycle) page for more detail.

**Terminations**

:	A termination is a list of subsystems that will cause the plugin to deactivate if they are marked inactive whilst the plugin is running. 
	
	For example, if the graphics subsystem was set in a plugin termination list, then if the graphics subsystem was marked inactive then the plugin would be deactivated with the reason `IShell::CONDITIONS`

**Controls**

:	A list of the subsystems that are controlled by the plugin (i.e. the plugin will mark those subsystems as activate/inactive).
	

### Example
```c++
static Metadata<TestPlugin> metadata(
    // Version
    1, 0, 0,
    // Preconditions
    { PluginHost::ISubSystem::subsystem::NETWORK, PluginHost::ISubSystem::subsystem::INTERNET },
    // Terminations
    { PluginHost::ISubSystem::subsystem::NETWORK, PluginHost::ISubSystem::subsystem::INTERNET },
    // Controls
    { PluginHost::ISubSystem::subsystem::LOCATION });
```

This example defines the following subsystem rules:

* The plugin will not activate until the Network and Internet subsystems are active
* If either the Network or Internet subsystem are deactivated whilst the plugin is active, the plugin will be deactivated
* The plugin is responsible for setting the Location subsystem state



### Plugin Configuration

In addition to the metadata, the **preconditions** can also be set in the plugin config file. These will be added to the preconditions defined in the metadata.
