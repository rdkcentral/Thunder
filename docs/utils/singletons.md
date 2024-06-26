## What is singleton and why it is used
The Singleton pattern is a design pattern that restricts the instantiation of a class to a single instance. It ensures that only one instance of the class exists throughout the runtime of the program and provides a global point of access to that instance. In addition, they allow for lazy allocation and initialization, whereas global variables will always consume resources. Examples of using singletons can be found in ThunderClientLibraries repository.

## Singleton and SingletonType
The `SingletonType` class can be used to make a class a singleton get access to an instance (which creates one if that was not done before):
```cpp
Core::SingletonType<MessageUnit>::Instance()
```
If a parameter is required for the construction of the class that should be a singleton, Create can be used (and after that access to the singleton instance can be done using the Instance method):
```cpp
Core::SingletonType<OpenCDMAccessor>::Create(connector.c_str());
```
`SingletonType` is a templated class designed to streamline the implementation of singleton objects in Thunder. It is intended to be used as a base class for concrete singleton classes and implements constructors and destructors to handle registration and unregistration of singleton instances within a `SingletonList`.

## disposing singletons

In general a singleton can be disposed using the Dispose method. Please note was will be highlighted below that calling Instance after Dispose might create a new instance of the Singleton.
All created Singletons cann be disposed at once with:
```cpp
Thunder::Core::Singleton::Dispose();
```
This should typically done at the end of an application fully based on the Thunder framework to cleanup all Singletons. As will be highlighted below if it is in a Library based on the Thunder framework you need to be more careful and not call the general Dispose but only cleanup what you created.

Singletons that use sockets should connect to them before they are registered in the singletons list. This is therefore best done in their constructor. If the connection occurs too late, the singleton may not be cleaned up before the Resource Monitor destructor is called.
Here is an example of making sure a socket connection is opened in the OpenCDMAccessor singleton constructor.
```cpp
OpenCDMAccessor(const TCHAR domainName[])
        : _refCount(1)
        , _domain(domainName)
        , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create())
        , _client()
        , _remote(nullptr)
        , _adminLock()
        , _signal(false, true)
        , _interested(0)
        , _sessionKeys()
{
    TRACE_L1("Trying to open an OCDM connection @ %s\n", domainName);
    Reconnect(); // make sure ResourceMonitor singleton is created before OpenCDMAccessor so the destruction order is correct
}
```

When a singleton uses itself (via the instance method) or any other singleton during destruction be very careful to make sure it does not create a new singleton by accident.
    
A client library dispose method should not call `Core::Singleton::Dispose()` but only clean up its own internal singletons. 
!!!warning
    `Core::Singleton::Dispose()` dispose of all singletons in the singletons list.

Calling this method on a particular singleton will dispose of them all. Instead, we should use `Core::SingletonType<>::Dispose()` with the appropriate type of singleton we want to dispose of specified, or write our own `Dispose()` which will not dispose of all remaining singletons.

Below is an example of how you SHOULD NOT dispose of a singleton!
```cpp
void opencdm_dispose() {
    Core::Singleton::Dispose();
}
```
And here an example of the RIGHT way to dispose of a singleton.
```cpp
void opencdm_dispose() {
    Core::SingletonType<OpenCDMAccessor>::Dispose();
}
```
