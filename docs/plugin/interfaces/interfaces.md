Thunder plugins are built around the concept of interfaces. An interface acts as a contract between the plugin and the outside world (this could be external client applications or other plugins). 

In Thunder, plugins can expose their interfaces over two communication channels:

* JSON-RPC ([https://www.jsonrpc.org/specification](https://www.jsonrpc.org/specification))
* COM-RPC (Custom Thunder RPC protocol)

In older Thunder versions (<R4), it was common for plugins to define two interface files, one for each RPC format. In Thunder R4 and later, it is strongly recommended to use the code-generation tools and only write a single interface file. This makes it much easier to maintain and reduces the amount of boilerplate code that must be written to create a plugin. This is the approach documented in this page.

Examples of existing Thunder interfaces can be found in the ThunderInterfaces repository: [https://github.com/rdkcentral/ThunderInterfaces/](https://github.com/rdkcentral/ThunderInterfaces/).

## Designing a good Interface
Before writing any plugin code, it is essential to define the interface that plugin will implement. A good interface defines a set of methods that a plugin can support, without dictating anything about the actual code that will implement the interface. It is possible that many plugins could implement that same interface if they provide overlapping or equivalent functionality (for example an `IBrowser` interface could be implemented by different web browser engines such as WebKit or Chromium).

The interface provides a clear boundary between the code that invokes methods on the plugin and the plugin code that implements the functionality. Typically, interface definitions are stored in a separate repository to the plugin implementations to reflect this boundary.

Well-designed interfaces should also be easy to read and self explanatory. Methods names should be descriptive and have doxygen-style comments that describe their functionality, inputs and outputs. All methods should return a standardised error code to indicate whether they completed successfully, and any data that should be returned by the method should be stored in an output parameter.

It is possible to build an interface from smaller sub-interfaces using composition. This is preferred to having a large monolithic interface, as it encourages reuse and modularity.

## Interface Definitions

### COM-RPC Interfaces

When designing a plugin interface, it is always best to start from the COM-RPC interface definition. A COM-RPC interface is defined in a C++ header file, and is a struct that inherits virtually from `Core::IUnknown`. This is very similar to interfaces in Microsoft COM. The plugin code will then provide an implementation of one or more COM-RPC interfaces.

During the build, code-generation tools automatically generate ProxyStub classes for each interface. These ProxyStubs handle the serialisation/deserialisation of messages when crossing process boundaries (this code is not necessary for in-process COM-RPC communication, since that just resolves down to local function calls and no data marshalling occurs).

#### IUnknown

All COM-RPC interfaces inherit virtually from `IUnknown`. As with Microsoft COM, this contains 3 vital methods:

* **QueryInterface()** - provides interface navigation. Allows clients to dynamically discover (at run time) whether or not an interface is supported. Given an interface ID, if the plugin supports that interface than a pointer to that interface will be returned, otherwise will return nullptr.
* **AddRef()** - lifetime management. Increase the reference count on the object
* **Release()** -  lifetime management. Decrement the reference count on the object. When 0, it is safe to destroy the object

#### Interface Characteristics

* **Interfaces are not plugins**: A Thunder plugin can implement 0 or more COM-RPC interfaces, but an interface cannot be instantiated by itself because it does not define an implementation. It is possible for many Thunder plugins to implement the same interface
* **COM-RPC clients interact with pointers to interfaces**: A client application communicating with a Thunder plugin over COM-RPC will receive nothing more than an opaque pointer through which it can access the interface methods. It cannot access any data from the plugin that implements the interface. This encapsulation ensures the client can only communicate with the plugin over an agreed interface
* **Interfaces are immutable**: COM-RPC interfaces are not versioned, so if an interface needs changing it must only add new methods. It must never break existing methods
* **Interfaces are strongly typed**: Each COM-RPC interface has a unique ID, used to identify it in the system. When a client requests a pointer to an interface, it does so using the ID of that interface. 

#### Guidelines

* Each COM-RPC interface must inherit **virtually** from `Core::IUnknown` and have a unique ID
    * Virtual inheritance is important to prevent the [diamond problem](https://isocpp.org/wiki/faq/multiple-inheritance#mi-diamond) and ensure multiple interfaces are implemented on a single, reference counted object. Without this, each interface might have its own reference count and not be destroyed correctly.

* Ensure API compatibility is maintained when updating interfaces to avoid breaking consumers
* Methods should be pure virtual methods that can be overridden by the plugin that implements the interface
* Methods should return **`Core::hresult`** which will store the error code from the method
    * If a method succeeds, it should return `Core::ERROR_NONE`
    * If a method fails, it should return a suitable error code that reflects the failure (e.g `Core::ERROR_READ_ERROR`)
    * If an error occurs over the COM-RPC transport the `COM_ERROR` bit will be set. This allows consumers to determine where the failure occurred
* Ensure all enums have explicit data types set (if not set, the code generators will fall back to `uint32_t` as a safe default)
    * If you know you will be communicating over different architectures, then you can define `INSTANCE_ID_BITS`  to specify a specific default integer width

* C++ types such as `std::vector` and `std::map` are **not** compatible with COM-RPC
    * Only "plain-old data" (POD) types can be used (e.g. scalar values, interface pointers)
    * COM-RPC can auto-generate iterators for returning multiple results
* Ensure integer widths are explicitly set (e.g. use `uint16_t` or `uint8_t` instead of just `int`) to prevent issues if crossing architecture boundaries
* Prefer asynchronous APIs for long running tasks (and use notifications to signal completion)

#### Notifications & Sinks

A COM-RPC interface not only allows for defining the methods exposed by a plugin, but can also be used to define notifications that plugins can raise and clients can subscribe to.

As with Microsoft COM, this is done by allowing clients to create implementations of notification interfaces as sinks, and register that sink with the plugin. When a notification occurs, the plugin will call the methods on the provided sink.


### JSON-RPC Interfaces

Once a COM-RPC interface is defined, if a JSON-RPC interface is also required then the interface should have the **`@json`** tag added. This signals to the code-generator that a corresponding JSON-RPC interface should be generated alongside the COM-RPC one. 

By default, when the `@json` tag is added, all methods in the COM-RPC interface will have corresponding JSON-RPC methods. It is possible to ignore/skip specific methods from the JSON-RPC generation by adding tags in the interface definition.

In older Thunder versions (<R4), JSON-RPC interfaces were defined using separate JSON schema files. These would then need to be manually wired up in the plugin. By using the code-generators, we can eliminate this step, making it much faster and easier to write plugins. It is no longer recommended to create JSON schema files for JSON-RPC interfaces.

### Code Generation

The code generation tooling for Thunder lives in the [ThunderTools](https://github.com/rdkcentral/ThunderTools) repository. These tools are responsible for the generation of ProxyStub implementations for COM-RPC, JSON-RPC interfaces, JSON data types, and documentation.

When building the interfaces from the ThunderInterfaces repository, the code generation is automatically triggered as part of the CMake configuration.

```
[cmake] -- ProxyStubGenerator ready /home/stephen.foulds/Thunder/host-tools/sbin/ProxyStubGenerator/StubGenerator.py
[cmake] ProxyStubGenerator: IAVNClient.h: created file ProxyStubs_AVNClient.cpp
[cmake] ProxyStubGenerator: IAVSClient.h: created file ProxyStubs_AVSClient.cpp
[cmake] ProxyStubGenerator: IAmazonPrime.h: created file ProxyStubs_AmazonPrime.cpp
[cmake] ProxyStubGenerator: IApplication.h: created file ProxyStubs_Application.cpp
...
```

Each interface definition will result in up to 4 auto-generated files depending on whether a JSON-RPC interface is required.

* `ProxyStubs_<Interface>.cpp` - COM-RPC marshalling/unmarshalling code
* `J<Interface>.h` - If the `@json` tag is set, this will contain boilerplate code for wiring up the JSON-RPC interface automatically
* `JsonData_<Interface>.h` - If the `@json` tag is set, contains C++ classes for the serialising/deserialising JSON-RPC parameters
* `JsonEnum_<Interface>.cpp` - If the `@json` tag is set, and the interface contains enums, this contains code to convert between strings and enum values

The resulting generated code is then compiled into 2 libraries:

* `/usr/lib/thunder/proxystubs/libThunderMarshalling.so`
    * This contains all the generated proxy stub code responsible for handling COM-RPC serialisation/deserialisation

* `/usr/lib/thunder/libThunderDefinitions.so`
    * This contains all generated data types (e.g. json enums and conversions) that can be used by plugins

!!! note
	There will also be a library called `libThunderProxyStubs.so` installed in the `proxystubs` directory as part of the main Thunder build - this contains the generated ProxyStubs for the internal Thunder interfaces (such as Controller and Dispatcher).


The installation path can be changed providing the `proxystubpath` option in the Thunder config.json file is updated accordingly so Thunder can find the libraries. When Thunder starts, it will load all available libraries in the `proxystubpath` directory. If an observable proxystubpath is set in the config, then Thunder will monitor that directory and automatically load any new libraries in that directory. This makes it possible to load new interfaces at runtime.

If you are building the interface as part of a standalone repository instead of ThunderInterfaces, it is possible to manually invoke the code generation tools from that repository's CMake file. The CMake commands drive the following Python scripts:

* ThunderTools/ProxyStubGenerator/StubGenerator.py
* ThunderTools/JsonGenerator/JsonGenerator.py

The below CMakeLists.txt file is an example of how to invoke the code generators, you may need to tweak the build/install steps according to your specific project requirements

```cmake title="CMakeLists.txt" linenums="1"
project(SampleInterface)

find_package(Thunder)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(CompileSettingsDebug REQUIRED)
find_package(ProxyStubGenerator REQUIRED)
find_package(JsonGenerator REQUIRED)

# Interfaces we want to build
set(INTERFACES_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/ITest.h"
)

# Invoke the code generators
ProxyStubGenerator(
    INPUT ${INTERFACES_HEADERS}
    OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

JsonGenerator(
    CODE
    INPUT ${INTERFACES_HEADERS}
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# Find the generated sources
file(GLOB PROXY_STUB_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs*.cpp")
file(GLOB JSON_ENUM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/JsonEnum*.cpp")
file(GLOB JSON_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/generated/J*.h")

# Build the proxystub lib
add_library(ExampleProxyStubs SHARED
    ${PROXY_STUB_SOURCES}
    Module.cpp
)

target_include_directories(ExampleProxyStubs
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(ExampleProxyStubs
    PRIVATE
    ${NAMESPACE}Core::${NAMESPACE}Core
    CompileSettingsDebug::CompileSettingsDebug
)

# Build the definitions lib if applicable
if(JSON_ENUM_SOURCES)
    add_library(ExampleDefinitions SHARED
        ${JSON_ENUM_SOURCES}
    )

    target_link_libraries(ExampleDefinitions
        PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug
    )
endif()

# Install libs & headers
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)
install(TARGETS ExampleProxyStubs
    EXPORT ExampleProxyStubsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Runtime
)

if(JSON_ENUM_SOURCES)
    install(TARGETS ExampleDefinitions
        EXPORT ExampleDefinitionsTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/ COMPONENT ${NAMESPACE}_Runtime
    )
endif()

install(FILES ${INTERFACES_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/interfaces
)

install(FILES ${JSON_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/interfaces/json
)
```



## Worked Example

!!! tip
	This example only focuses on defining an interface. Writing the plugin that implements an interface is beyond the scope of this example. Refer to the "Hello World" walkthrough elsewhere in the documentation for a full end-to-end example.

For this example, we will define a simple interface that defines a way to search for WiFi access points (APs) and connect to them. 

Our WiFi interface should define 3 methods:

* **Scan** - start a scan for nearby APs. This will be an asynchronous method since a WiFi scan could take some time
    * This will take no arguments

* **Connect** - connect to an AP discovered during the scan
    * This will take one argument - the SSID to connect to

* **Disconnect** - disconnect from the currently connected AP
    * This will take no arguments


It should also define one notification/event that will be fired when the scan is completed. This notification will contain the list of APs discovered during the scan.

As with C++, COM-RPC interfaces are prefixed with the letter "I". Therefore the name of this example interface will be `IWiFi`.

### Define Interface ID

!!! warning
	Ensure your interface has a unique ID! If the ID is in use by another interface it will be impossible for Thunder to distinguish between them.


All interfaces must have a unique ID number that must never change for the life of the interface. From this ID, Thunder can identify which ProxyStub is needed to communicate over a process boundary.

Each ID is a `uint32_t` value. This was chosen to reduce the complexity and minimise the size of the data on the wire when compared to a GUID. 

Interface IDs for the core Thunder interfaces (such as Controller) are defined in the Thunder source code at `Source/com/Ids.h`.  For existing Thunder interfaces in the *ThunderInterfaces* repository, IDs are defined in the [`interfaces/Ids.h`](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/Ids.h) file.

For this example we will define 3 unique IDs for our interface represented by the following enum values:

* ID_WIFI
* ID_WIFI_NOTIFICATION
* ID_WIFI_AP_ITERATOR

### Define COM-RPC Interface

Each COM-RPC interface should be defined in a C++ header file with the same name as the interface, so in this case `IWiFi.h`

```cpp title="IWiFi.h" linenums="1"
#pragma once
#include "Module.h"

// @stubgen:include <com/IIteratorType.h> // (1)

namespace Thunder {
namespace Exchange {
    struct EXTERNAL IWiFi : virtual public Core::IUnknown {
        enum {
            ID = ID_WIFI // (2)
        };

        /**
         * @brief Represent a single WiFi access point
         */
        struct AccessPoint {
            string ssid;
            uint8_t channel;
            uint32_t frequency;
            int32_t signal;
        };
        using IAccessPointIterator = RPC::IIteratorType<AccessPoint, ID_WIFI_AP_ITERATOR>; // (3)

        /* @event */
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum {
                ID = ID_WIFI_NOTIFICATION // (4)
            };

            /**
             * @brief Signal that a previously requested WiFi AP scan has completed
             */
            virtual void ScanComplete(IAccessPointIterator* accessPoints /* @in */) = 0; // (5)
        };

        /**
         * @brief Start a WiFi scan
         */
        virtual Core::hresult Scan() = 0;

        /**
         * @brief Connect to an access point
         *
         * @param   ssid      SSID to connect to
         */
        virtual Core::hresult Connect(const string& ssid) = 0;

        /**
         * @brief Disconnect from the currently connected access point
         */
        virtual Core::hresult Disconnect() = 0;

        /**
         *  @brief Register for COM-RPC notifications
         */
        virtual uint32_t Register(IWiFi::INotification* notification) = 0; // (6)

        /**
         * @brief Unregister for COM-RPC notifications
         */
        virtual uint32_t Unregister(const IWiFi::INotification* notification) = 0;
    };
}
}
```

1. Use the include tag to include the COM-RPC iterator code
2. Each interface must have a unique ID value
3. We need to return a list of detected access points. Since we can't use a standard container such as `std::vector`, use the supported COM-RPC iterators. This iterator must have a unique ID
4. All interfaces must have a unique ID, so the `INotification` interface must also have an ID
5. This method will be invoked when the AP scan completes, and the `accessPoints` variable will hold a list of all the discovered APs.
6. Provide register/unregister methods for COM-RPC clients to subscribe to notifications

### Enable JSON-RPC Generation

To enable the generation of the corresponding JSON-RPC interface, add the @json tag above the interface definition.

Here, the `@json` tag was passed a version number `1.0.0`, which can be used to version JSON-RPC interfaces. If not specified, it will default to `1.0.0`

```cpp title="IWiFi.h" linenums="1" hl_lines="8"
#pragma once
#include "Module.h"

// @stubgen:include <com/IIteratorType.h>

namespace Thunder {
namespace Exchange {
    // @json 1.0.0
    struct EXTERNAL IWiFi : virtual public Core::IUnknown {
        enum {
            ID = ID_WIFI
        };
        //...
```

When JSON-RPC support is enabled, the code generator will create code to for the plugin to register the JSON-RPC methods, and code to convert between JSON and C++ classes.

!!! danger
	The below code samples are auto-generated and provided as an example. As the code-generation tools change, the actual output you see may look different than the below. Do not copy the below code for your own use

The generated `JWiFi.h` file contains two methods - `Register` and `Unregister`, which are used by the plugin to connect the JSON-RPC interface to the underlying implementation.

??? example "Auto-generated code (click to expand/collapse)"
    ```cpp title="JWiFi.h" linenums="1"
	// Generated automatically from 'IWiFi.h'. DO NOT EDIT.
    #pragma once

    #include "Module.h"
    #include "JsonData_WiFi.h"
    #include <interfaces/IWiFi.h>
    
    namespace Thunder {
    
    namespace Exchange {
    
        namespace JWiFi {
    
            namespace Version {
    
                constexpr uint8_t Major = 1;
                constexpr uint8_t Minor = 0;
                constexpr uint8_t Patch = 0;
    
            } // namespace Version
    
            using JSONRPC = PluginHost::JSONRPC;
    
            static void Register(JSONRPC& _module_, IWiFi* _impl_)
            {
                ASSERT(_impl_ != nullptr);
    
                _module_.RegisterVersion(_T("JWiFi"), Version::Major, Version::Minor, Version::Patch);
    
                // Register methods and properties...
    
                // Method: 'scan' - Start a WiFi scan
                _module_.Register<void, void>(_T("scan"), 
                    [_impl_]() -> uint32_t {
                        uint32_t _errorCode;
    
                        _errorCode = _impl_->Scan();
    
                        return (_errorCode);
                    });
    
                // Method: 'connect' - Connect to an access point
                _module_.Register<JsonData::WiFi::ConnectParamsData, void>(_T("connect"), 
                    [_impl_](const JsonData::WiFi::ConnectParamsData& params) -> uint32_t {
                        uint32_t _errorCode;
    
                        const string _ssid{params.Ssid};
    
                        _errorCode = _impl_->Connect(_ssid);
    
                        return (_errorCode);
                    });
    
                // Method: 'disconnect' - Disconnect from the currently connected access point
                _module_.Register<void, void>(_T("disconnect"), 
                    [_impl_]() -> uint32_t {
                        uint32_t _errorCode;
    
                        _errorCode = _impl_->Disconnect();
    
                        return (_errorCode);
                    });
    
            }
    
            static void Unregister(JSONRPC& _module_)
            {
                // Unregister methods and properties...
                _module_.Unregister(_T("scan"));
                _module_.Unregister(_T("connect"));
                _module_.Unregister(_T("disconnect"));
            }
    
            namespace Event {
    
                PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)
    
                // Event: 'scancomplete' - Signal that a previously requested WiFi AP scan has completed
                static void ScanComplete(const JSONRPC& _module_, const JsonData::WiFi::ScanCompleteParamsData& params)
                {
                    _module_.Notify(_T("scancomplete"), params);
                }
    
                // Event: 'scancomplete' - Signal that a previously requested WiFi AP scan has completed
                static void ScanComplete(const JSONRPC& _module_,
                         const Core::JSON::ArrayType<JsonData::WiFi::ScanCompleteParamsData::AccessPointData>& accessPoints)
                {
                    JsonData::WiFi::ScanCompleteParamsData _params_;
                    _params_.AccessPoints = accessPoints;
    
                    ScanComplete(_module_, _params_);
                }
    
                // Event: 'scancomplete' - Signal that a previously requested WiFi AP scan has completed
                static void ScanComplete(const JSONRPC& _module_, const std::list<Exchange::IWiFi::AccessPoint>& accessPoints)
                {
                    JsonData::WiFi::ScanCompleteParamsData _params_;
                    _params_.AccessPoints = accessPoints;
    
                    ScanComplete(_module_, _params_);
                }
    
                POP_WARNING()
    
            } // namespace Event
    
        } // namespace JWiFi
    
    } // namespace Exchange
    
    } // namespace Thunder
       
    ```

The auto-generated `JsonData_WiFi.h` file contains code that can convert from the parameters object in the incoming JSON-RPC request to a C++ object. This is used by both the plugin and client apps to read incoming parameters and build responses.

??? example "Auto-generated code (click to expand/collapse)"
    ```cpp title="JsonData_WiFi.h" linenums="1"
    // C++ classes for WiFi API JSON-RPC API.
    // Generated automatically from 'IWiFi.h'. DO NOT EDIT.
    
    // Note: This code is inherently not thread safe. If required, proper synchronisation must be added.
    
    #pragma once
    
    #include <core/JSON.h>
    #include <interfaces/IWiFi.h>
    
    namespace Thunder {
    
    namespace JsonData {
    
        namespace WiFi {
    
            // Method params/result classes
            //
    
            class ConnectParamsData : public Core::JSON::Container {
            public:
                ConnectParamsData()
                    : Core::JSON::Container()
                {
                    Add(_T("ssid"), &Ssid);
                }
    
                ConnectParamsData(const ConnectParamsData&) = delete;
                ConnectParamsData& operator=(const ConnectParamsData&) = delete;
    
            public:
                Core::JSON::String Ssid; //      SSID to connect to
            }; // class ConnectParamsData
    
            class ScanCompleteParamsData : public Core::JSON::Container {
            public:
                class AccessPointData : public Core::JSON::Container {
                public:
                    AccessPointData()
                        : Core::JSON::Container()
                    {
                        _Init();
                    }
    
                    AccessPointData(const AccessPointData& _other)
                        : Core::JSON::Container()
                        , Ssid(_other.Ssid)
                        , Channel(_other.Channel)
                        , Frequency(_other.Frequency)
                        , Signal(_other.Signal)
                    {
                        _Init();
                    }
    
                    AccessPointData& operator=(const AccessPointData& _rhs)
                    {
                        Ssid = _rhs.Ssid;
                        Channel = _rhs.Channel;
                        Frequency = _rhs.Frequency;
                        Signal = _rhs.Signal;
                        return (*this);
                    }
    
                    AccessPointData(const Exchange::IWiFi::AccessPoint& _other)
                        : Core::JSON::Container()
                    {
                        Ssid = _other.ssid;
                        Channel = _other.channel;
                        Frequency = _other.frequency;
                        Signal = _other.signal;
                        _Init();
                    }
    
                    AccessPointData& operator=(const Exchange::IWiFi::AccessPoint& _rhs)
                    {
                        Ssid = _rhs.ssid;
                        Channel = _rhs.channel;
                        Frequency = _rhs.frequency;
                        Signal = _rhs.signal;
                        return (*this);
                    }
    
                    operator Exchange::IWiFi::AccessPoint() const
                    {
                        Exchange::IWiFi::AccessPoint _value{};
                        _value.ssid = Ssid;
                        _value.channel = Channel;
                        _value.frequency = Frequency;
                        _value.signal = Signal;
                        return (_value);
                    }
    
                private:
                    void _Init()
                    {
                        Add(_T("ssid"), &Ssid);
                        Add(_T("channel"), &Channel);
                        Add(_T("frequency"), &Frequency);
                        Add(_T("signal"), &Signal);
                    }
    
                public:
                    Core::JSON::String Ssid;
                    Core::JSON::DecUInt8 Channel;
                    Core::JSON::DecUInt32 Frequency;
                    Core::JSON::DecSInt32 Signal;
                }; // class AccessPointData
    
                ScanCompleteParamsData()
                    : Core::JSON::Container()
                {
                    Add(_T("accesspoints"), &AccessPoints);
                }
    
                ScanCompleteParamsData(const ScanCompleteParamsData&) = delete;
                ScanCompleteParamsData& operator=(const ScanCompleteParamsData&) = delete;
    
            public:
                Core::JSON::ArrayType<ScanCompleteParamsData::AccessPointData> AccessPoints;
            }; // class ScanCompleteParamsData
    
        } // namespace WiFi
    
    } // namespace JsonData
    
    }
    
    ```

Since there are no enums in this example interface, no `JsonEnums_WiFi.cpp` file was generated.