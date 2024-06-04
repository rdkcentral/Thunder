As with all code, it is important that Thunder plugins handle errors gracefully and consistently. 

## Exceptions

By default, Thunder is compiled with `-fno-exceptions` to disable exception support in the framework. This can be changed by enabling the `EXCEPTIONS_ENABLE` CMake option. As a result, plugins should **never** be designed to throw exceptions.

If an exception does occur, the Thunder process will immediately shut down with an error to prevent any further issues and log the following message:

```
Thunder shutting down due to an uncaught exception.
```

If the `Crash` logging category is enabled, then more information about the faulting callstack will be available (only on debug builds). Thunder will attempt to resolve the callsign of the faulting plugin but this is not always possible.

```
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: -== Unhandled exception in: NoTLSCallsign [General] ==-
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [000] [0x7ffff7d22cba] /Thunder/install/usr/lib/libThunderCore.so.1 DumpCallStack [74]
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [001] [0x7ffff7e02b84] /Thunder/install/usr/lib/libThunderMessaging.so.1 Thunder::Logging::DumpException(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [88]
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [002] [0x555555641b35] /Thunder/install/usr/bin/Thunder
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [003] [0x7ffff7aae24c] /lib/x86_64-linux-gnu/libstdc++.so.6 
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [004] [0x7ffff7aae2b7] /lib/x86_64-linux-gnu/libstdc++.so.6 
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [005] [0x7ffff7aae518] /lib/x86_64-linux-gnu/libstdc++.so.6 
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [006] [0x7ffff49820ff] /Thunder/install/usr/lib/thunder/plugins/libThunderTestPlugin.so
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [007] [0x555555664a61] /Thunder/install/usr/bin/Thunder
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [008] [0x55555566b47e] /Thunder/install/usr/bin/Thunder
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [009] [0x555555643b35] /Thunder/install/usr/bin/Thunder
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [010] [0x7ffff7629d90] /lib/x86_64-linux-gnu/libc.so.6 
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [011] [0x7ffff7629e40] /lib/x86_64-linux-gnu/libc.so.6 __libc_start_main [128]
[Wed, 05 Jul 2023 10:43:38]:[SysLog]:[Crash]: [012] [0x555555596055] /Thunder/install/usr/bin/Thunder _start [37]
```

### Exception Catching

!!! danger
	This is almost always a bad idea. Catching exceptions at a high level in such a coarse way then continuing will often result in undesired behaviour!

If compiled with the `EXCEPTION_CATCHING` CMake option, then Thunder will install high-level exception catching at specific places in the framework. These will catch exceptions coming from plugins and continue execution instead of terminating the entire process. However, be aware this will not catch all exceptions so some exceptions will still result in the framework terminating.

## Error Codes

Thunder defines a list of common error codes in `Source/core/Portability.h`. Each error code has unique uint32_t ID associated with it. Error codes can be converted to a human-readable string by calling the `ErrorToString(uint32_t code)` function:

```c++
uint32_t error = Core::ERROR_TIMEDOUT;
printf("Got error code %d (%s)\n", error, Core::ErrorToString(error));

/* Output:
Got error code 11 (ERROR_TIMEDOUT)
*/
```

### COM-RPC Errors

When designing an interface that will be exposed over COM-RPC, all functions should return a `Core::hresult` to indicate if the function executed successfully. On success, the function should return `Core::ERROR_NONE`. 

Any data returned by the function should be stored in an output parameter instead of a return value. This ensures consistency across interfaces. If an error occurs over the COM-RPC transport or during marshalling/umarshalling the data, the most-significant bit will be used to indicate the error code is a COM error. 

```cpp
Core::hresult success = _remoteInterface->MyFunction();

if (success != Core::ERROR_NONE) {
	// An error occured, was this a result of the COM link or did the plugin return an error?
	if (success & COM_ERROR == 0) {
		printf("Plugin returned error %d (%s)\n", success, Core::ErrorToString(success));
	} else {
		printf("COM-RPC error %d (%s)\n", success, Core::ErrorToString(success));
	}
}
```

### JSON-RPC

As with COM-RPC, JSON-RPC methods should return a `Core::hresult` value to indicate success or failure.  If the JSON-RPC method returns an error code other than `Core::ERROR_NONE`, it is treated as a failure.

!!! note
	Some older RDK plugins return a `success` boolean in their response to indicate errors. This is not recommended or necessary - simply return the appropriate error code from the method and a valid JSON-RPC error response will be generated.

The returned JSON conforms to the [JSON-RPC 2.0](https://www.jsonrpc.org/) standard. In addition to the Thunder core error code, the response body may contain a JSON-RPC error as defined in the JSON-RPC specification

| code             | message          | meaning                                                      |
| ---------------- | ---------------- | ------------------------------------------------------------ |
| -32700           | Parse error      | Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text. |
| -32600           | Invalid Request  | The JSON sent is not a valid Request object.                 |
| -32601           | Method not found | The method does not exist / is not available.                |
| -32602           | Invalid params   | Invalid method parameter(s).                                 |
| -32603           | Internal error   | Internal JSON-RPC error.                                     |
| -32000 to -32099 | Server error     | Reserved for implementation-defined server-errors.           |

In the below example, an attempt is made to activate a non-existent plugin. The Controller plugin returns `ERROR_UNKNOWN_KEY` since to plugin exists with the given callsign.

:arrow_right: Request

```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"method": "Controller.1.activate",
	"params": {
		"callsign": "fakePlugin"
	}
}
```

:arrow_left: Response

```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"error": {
		"code": 22,
		"message": "ERROR_UNKNOWN_KEY"
	}
}
```

## Handling Unexpected COM-RPC Disconnections

From the perspective of a plugin, there are two COM-RPC disconnection scenarios to consider:

* When the out-of-process side of the plugin unexpectedly terminates (either due to a crash or being killed by an external entity such as the linux OOM killer)
* When a client that has registered for notifications crashes unepxectedly.

Whilst the framework can detect unexpected COM-RPC disconnects and handle updating reference counts accordingly, there are actions that should be taken in the plugin code to ensure safety.

### Out-Of-Process Disconnection

!!! warning
	It is essential that plugins implement this feature if they are expected to run out-of-process

If your plugin implements an interface that could run out of process, then it is very important the plugin subscribes to the remote connection notification `RPC::IRemoteConnection::INotification `. This is a common pattern that you will see across many plugins.

The framework will raise this notification whenever a COM-RPC connection or disconnection occurs. The plugin should check if the disconnected connection belongs to them, and if so take action. Typically, this action will be for the plugin to deactivate itself with a `Failure` reason. 

Without listening for this notification and taking action, if the out-of-process side of a plugin dies the plugin itself will not be deactivated.

By deactivating with the `Failure` reason, the post-mortem handler will kick in. This will dump the contents of some system files to the log (memory information, load averages) to help debugging.

!!! note
	The `Deactivated()` function might be called on a socket thread which we do not want to block. As a result, this function should return as quickly as possible and any work that needs doing (e.g. deactivating the plugin) must be done on a separate thread in the main worker pool.

Example:

```c++ linenums="1" title="TestPlugin.h" hl_lines="3-33 40 69"
class TestPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC {
private:
    class Notification : public RPC::IRemoteConnection::INotification {
    public:
        explicit Notification(TestPlugin* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }

        ~Notification() override = default;

        Notification(Notification&&) = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(Notification&&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        void Activated(RPC::IRemoteConnection* /* connection */) override
        {
        }
        void Deactivated(RPC::IRemoteConnection* connectionId) override
        {
            _parent.Deactivated(connectionId);
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

    private:
        TestPlugin& _parent;
    };

public:
    TestPlugin()
        : _connectionId(0)
        , _service(nullptr)
        , _testPlugin(nullptr)
        , _notification(this)
    {
    }
    ~TestPlugin() override = default;

    TestPlugin(TestPlugin&&) = delete;
    TestPlugin(const TestPlugin&) = delete;
    TestPlugin& operator=(TestPlugin&&) = delete;
    TestPlugin& operator=(const TestPlugin&) = delete;

    BEGIN_INTERFACE_MAP(TestPlugin)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IDispatcher)
    INTERFACE_AGGREGATE(Exchange::ITestPlugin, _testPlugin)
    END_INTERFACE_MAP

public:
    // IPlugin methods
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

private:
    void Deactivated(RPC::IRemoteConnection* connection);

private:
    uint32_t _connectionId;
    PluginHost::IShell* _service;
    Exchange::ITestPlugin* _testPlugin;
    Core::Sink<Notification> _notification;
};
```


```c++ linenums="1" title="TestPlugin.cpp" hl_lines="12 32 62-73"
const string TestPlugin::Initialize(PluginHost::IShell* service)
{
    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);

    string result = {};

    _service = service;
    _service->AddRef();

    // Register for COM-RPC connection/disconnection notifications
    _service->Register(&_notification);

    // Instantiate the ITestPlugin interface (which will spawn the OOP side if running in OOP mode)
    // Store connection ID in _connectionId
    _testPlugin = _service->Root<Exchange::ITestPlugin>(_connectionId, 2000, _T("TestPluginImplementation"));

    if (!_testPlugin) {
        // Error occurred, return non-empty string
        result = "Failed to create ITestPlugin";
    }

    return result;
}

void TestPlugin::Deinitialize(PluginHost::IShell* service)
{
    if (service != nullptr) {
        ASSERT(_service == service);

        // Unsubscribe from connection notification first to prevent any false-positives
        _service->Unregister(&_notification);

        if (_testPlugin != nullptr) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // This should release the last reference and destruct the object. If not,
            // there's something else holding on to it and we have a leak
            VARIABLE_IS_NOT_USED uint32_t result = _testPlugin->Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            _testPlugin = nullptr;

            // Shut down the out-of-process connection if still running
            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }

        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }
}

string TestPlugin::Information() const
{
    // No additional info to report
    return string();
}

void TestPlugin::Deactivated(RPC::IRemoteConnection* connection)
{
    // Gracefully handle an unexpected termination from the other side of the
    // connection (for example if the remote process crashed) and deactivate
    // ourselves as we cannot continue safely
    if (connection->Id() == _connectionId) {
        ASSERT(_service != nullptr);
        Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
            PluginHost::IShell::DEACTIVATED,
            PluginHost::IShell::FAILURE));
    }
}
```

### Client Crashes

If your plugin provides the ability for client applications to register for notifications, then if the client crashes the plugin should remove any notification registrations that belong to that client.

Whilst this is not strictly necessary (calling a method on a dead client's notification proxy will not cause a crash), it is a good practice to avoid holding on to dead proxy objects. This ensures memory is correctly freed and you don't waste time firing notifications to dead clients.

In the below example, `ITestPlugin` has a notification called `INotification` and allows client applications can register/unregister for that notification (see `TestPluginImplementation.cpp`). 

In normal operation, the client will call `Register()` when it starts, and `Unregister()` when it exits. However, if the client crashes it might not have chance to call the `Unregister()` method. Therefore it is up to the plugin to remove the registration manually.

To do this, the plugin should register for `ICOMLink::INotification` events from the framework. When the `Dangling()` event occurs (indicating we have an interface that is not connected on both ends), the plugin should check to see which interface was revoked. If the interface belongs to the plugin's notification, then unregister that client.

!!! note
	The below example only demonstrates the `ICOMLink::INotification`. In the real world, this should be implemented alongside the `RPC::IRemoteConnection::INotification` notification shown in the previous example


```cpp title="TestPlugin.h" linenums="1" hl_lines="3-40 47 69 75"
class TestPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC {
private:
    class Notification : public PluginHost::IShell::ICOMLink::INotification {
    public:
        explicit Notification(TestPlugin* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~Notification() override = default;

        Notification(Notification&&) = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(Notification&&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        void Dangling(const Core::IUnknown* remote, const uint32_t interfaceId) override
        {
            ASSERT(remote != nullptr);
            if (interfaceId == Exchange::ITestPlugin::INotification::ID) {
                const auto revokedInterface = remote->QueryInterface<Exchange::ITestPlugin::INotification>();
                if (revokedInterface) {
                    _parent.CallbackRevoked(revokedInterface);
                    revokedInterface->Release();
                }
            }
        }

        void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId) override
        {
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::IShell::ICOMLink::INotification)
        END_INTERFACE_MAP

    private:
        TestPlugin& _parent;
    };

public:
    TestPlugin()
    	: _connectionId(0)
    	, _service(nullptr)
    	, _testPlugin(nullptr)
    	, _notification(this)
	{
	}
    ~TestPlugin() override = default;

    // Do not allow copy constructors
    TestPlugin(const TestPlugin&) = delete;
    TestPlugin& operator=(const TestPlugin&) = delete;

    BEGIN_INTERFACE_MAP(TestPlugin)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IDispatcher)
    INTERFACE_AGGREGATE(Exchange::ITestPlugin, _testPlugin)
    END_INTERFACE_MAP

public:
    // IPlugin methods
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

private:
    void CallbackRevoked(const Exchange::ITestPlugin::INotification* remote);

private:
    uint32_t _connectionId;
    PluginHost::IShell* _service;
    Exchange::ITestPlugin* _testPlugin;
    Core::Sink<Notification> _notification;
};
```

```cpp title="TestPlugin.cpp" linenums="1" hl_lines="12 32 62-66"
const string TestPlugin::Initialize(PluginHost::IShell* service)
{
    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);

    string result = {};

    _service = service;
    _service->AddRef();

    // Register for COM-RPC connection/disconnection notifications
    _service->Register(&_notification);

    // Instantiate the ITestPlugin interface (which could spawn the OOP side if running in OOP mode)
    // Store connection ID in _connectionId
    _testPlugin = _service->Root<Exchange::ITestPlugin>(_connectionId, 2000, _T("TestPluginImplementation"));

    if (!_testPlugin) {
        // Error occurred, return non-empty string
        result = "Failed to create ITestPlugin";
    }

    return result;
}

void TestPlugin::Deinitialize(PluginHost::IShell* service)
{
    if (service != nullptr) {
        ASSERT(_service == service);

        // Unsubscribe from connection notification first to prevent any false-positives
        _service->Unregister(&_notification);

        if (_testPlugin != nullptr) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // This should release the last reference and destruct the object. If not,
            // there's something else holding on to it and we have a leak
            VARIABLE_IS_NOT_USED uint32_t result = _testPlugin->Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            _testPlugin = nullptr;

            // Shut down the out-of-process connection if still running
            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }

        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }
}

string TestPlugin::Information() const
{
    // No additional info to report
    return string();
}

void TestPlugin::CallbackRevoked(const Exchange::ITestPlugin::INotification* remote)
{
    // Unregister the notification
    _testPlugin->Unregister(remote);
}
```

```cpp title="TestPluginImplementation.cpp" linenums="1" hl_lines="22-50 54"
class TestPluginImplementation : public Exchange::ITestPlugin {

public:
    TestPluginImplementation() = default;
    ~TestPluginImplementation() = default;

    // Do not allow copy/move constructors
    TestPluginImplementation(const TestPluginImplementation&) = delete;
    TestPluginImplementation& operator=(const TestPluginImplementation&) = delete;

    BEGIN_INTERFACE_MAP(TestPluginImplementation)
    INTERFACE_ENTRY(Exchange::ITestPlugin)
    END_INTERFACE_MAP

public:
    Core::hresult Test(string& result /* @out */) override
    {
        result = "Hello World";
        return Core::ERROR_NONE;
    }

    uint32_t Register(Exchange::ITestPlugin::INotification* notification) override
    {
        _adminLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(_notificationCallbacks.begin(), _notificationCallbacks.end(), notification) == _notificationCallbacks.end()) {
            _notificationCallbacks.emplace_back(notification);
            notification->AddRef();
        }

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    uint32_t Unregister(const Exchange::ITestPlugin::INotification* notification) override
    {
        _adminLock.Lock();

        auto itr = std::find(_notificationCallbacks.begin(), _notificationCallbacks.end(), notification);
        if (itr != _notificationCallbacks.end()) {
            (*itr)->Release();
            _notificationCallbacks.erase(itr);
        }

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

private:
    Core::CriticalSection _adminLock;
    std::list<Exchange::ITestPlugin::INotification*> _notificationCallbacks;
};

SERVICE_REGISTRATION(TestPluginImplementation, 1, 0);
```