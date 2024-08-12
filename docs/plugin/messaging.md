In the past, prior to the R3 version of Thunder, logging, tracing and warning reporting were completely separate issues and were handled individually. However, even though these message types have distinct characteristics, we recognized the advantages of consolidating them into a unified framework, which is now referred to as `Messaging`. In the early versions of R3, we still had tracing enabled by default and messaging was in the early development. It is the R4 version of Thunder where `Messaging` is finally the default framework to handle all types of messages.

## Advantages of using messaging

There are of course several advantages of using this framework to deliver the messages over simply using something like `printf`:

* There are not only different types of messages, but also categories within these types, and each of them can be enabled/disabled at runtime
* No serialization penalty between different processes
* Always getting full lines one after another since all messages are timestamped
* It is possible to redirect the messages to another output like, e.g., a console, syslog, a file or a network stream (in time to come, this will be particularly useful in the case of containers, and this is something we are working towards)
* In the near future, *standard out* and *standard error* will also be redirectable (also something that is in development)

We are convinced that `Messaging` is much better suited for the development, and with some recent changes which will be described in the following sections, it is fully operational and enabled by default. We strongly believe that it is a good time to shed some more light on that functionality, since it is simple to use and yet very effective.

## Differences between the message types

Logging, tracing and warning reporting are important techniques often used in software development to gather information and provide insight into the behavior of an application. However, there are some fundamental differences between them within Thunder, in a nutshell:

* Tracing is meant for the developers and is dropped in production
* Logging is not dropped in production and is used to indicate information vital to the user
* Warning Reporting is only available if Thunder is compiled with the `WARNING_REPORTING` option and sends warnings only if a condition is met

!!! note
	In `Production`, so when building with the `Min_Size_Rel` flag, the `TRACE` and `TRACE_GLOBAL` macros are declared empty and there is no way to enable tracing. This version of Thunder is meant to be used only by operators who want the smallest footprint on memory possible. In both `Debug` and `Release` versions, tracing is enabled. `Debug` is used by the developers, so all macros are enabled and it has no code optimization at all, whereas `Release` is used by most of our operators and the QA team - asserts off and some code optimization.

## Viewing logs

### MessageControl plugin

The `MessageControl` plugin not only consolidates all of the various message types but also offers the flexibility to redirect these messages to different outputs. These outputs include the Console, Syslog, a file, or even a Network Stream through UDP. This can be configured by assigning appropriate values to JSON objects in the configuration file located in `/etc/Thunder/plugins/MessageControl.json`. These object's names can be found in the constructor of the `Config` class in `ThunderNanoServicesRDK/MessageControl/MessageControl.h`:

```c++
Config()
    : Core::JSON::Container()
    , Console(false)
    , SysLog(false)
    , FileName()
    , Abbreviated(true)
    , MaxExportConnections(Publishers::WebSocketOutput::DefaultMaxConnections)
    , Remote()
{
    Add(_T("console"), &Console); // (1)
    Add(_T("syslog"), &SysLog); // (2)
    Add(_T("filepath"), &FileName); // (3)
    Add(_T("abbreviated"), &Abbreviated); // (4)
    Add(_T("maxexportconnections"), &MaxExportConnections); // (5)
    Add(_T("remote"), &Remote); // (6)
}
```

1. Boolean value indicating if console output is enabled/disabled
2. Boolean value indicating if syslog output is enabled/disabled
3. Path to the file in which the messages should be stored. If the path is not empty, the file output is enabled
4. Reducing the amount of information in the messages coming from the `MessageControl` plugin (e.g. timestamp reduced to the time of day instead of the full date; removing file name, line number and class name for tracing type messages)
5. Specifying to how many WebSockets can the messages be outputted
6. An object that should have two properties: `binding` which corresponds to a binding address and `port` on which the UDP connection will be established

!!! note
	Even though by default no output is set to either true or false in this config file, the plugin will output the messages to a console or syslog depending on whether Thunder is running in the background or not.

### DirectOutput

It is important to note that when the `MessageControl` plugin is not actively running or in scenarios where it is disabled, there is an option to directly print messages onto the console using the `DirectOutput()` method. This allows for immediate display of messages on the console without going through the buffering process.  In such a situation, the `Output()` method of the `DirectOutput` class is used. Furthermore, the `DirectOutput` class can be configured using the `Mode()` method to send the messages to the system logger instead of simply printing them on the console.

```c++
void DirectOutput::Output(const Core::Messaging::MessageInfo& messageInfo, const Core::Messaging::IEvent* message) const
{
    ASSERT(message != nullptr);
    ASSERT(messageInfo.Type() != Core::Messaging::Metadata::type::INVALID);

    string result = messageInfo.ToString(_abbreviate).c_str() +
                    Core::Format("%s\n", message->Data().c_str());

#ifndef __WINDOWS__
    if (_isSyslog == true) {
        //use longer messages for syslog
        syslog(LOG_NOTICE, "%s\n", result.c_str());
    }
    else
#endif
    {
        std::cout << result << std::endl;
    }
}
```

## How to adjust messaging

### Configuration

The main config file (` /etc/Thunder/config.json`) can be used to enable/disable the default messaging categories used for logging, tracing and warning reporting.

Messages are split into 3 types: logging, tracing and warning reporting. Each type has a list of categories which can be marked as enabled or disabled. There is also a similar list for tracing when it comes to enabling or disabling certain modules (e.g. plugins). By default, all categories are enabled for logging and warning reporting, but in terms of tracing, if a category or a module is not present in the config, it will be disabled.

Below is an example of the messaging section in the config:

```json
{
	"messaging": {
		"logging": {
			"abbreviated": true, // (1)
            "settings":[
                {
                	"category": "Notification", // (2)
                	"enabled": false
                }
            ]
		},
		"tracing": {
			"settings": [
				{
					"category": "Fatal", // (3)
					"enabled": true
				},
                {
                    "module": "Plugin_SamplePlugin", // (4)
                    "enabled": true
                }
			]
		},
		"reporting": {
			"abbreviated": true,
			"settings": [
				{
					"category": "TooLongWaitingForLock", // (5)
					"enabled": true,
                    "excluded": {
                        "callsigns": [
                            "com.example.SamplePlugin" // (6)
                        ],
                        "modules": [
                            "Plugin_SamplePlugin" // (7)
                        ]
                    },
                    "config": {
                        "reportbound": 1000, // (8)
                        "warningbound": 2000 // (9)
                    }
				}
			]
		}
	}
}
```

1. Reducing the amount of information in the messages coming from the `DirectOutput` (e.g. timestamp reduced to the time of day instead of the full date; removing file name, line number and class name for tracing type messages). This setting can be included separately in each message type.

2. Disabling logging messages from the `Notification` category

3. Enabling trace messages from the `Fatal` category

4. Enabling **all** tracing categories used in the `SamplePlugin` plugin. The module name reflects the `MODULE_NAME` definition in the plugin `Module.h`

5. Name of the category to configure

6. Callsigns of plugins to exclude from the warning reporting category

7. Module names to exclude from this warning reporting category

8. Report bound indicating a value that must be exceeded for a report to be generated

9. Warning bound indicating a value that must be exceeded for a warning to be generated (considered higher severity than a report). Note: warning bound should be >= report bound

!!! tip
	It is also possible to supply a filepath instead of an object to allow storing messaging configuration in a separate file
	```json
	{
		"messaging": "/path/to/messagingconfig.json"
	}
	```

Warning Reporting enables various runtime checks for potentially erroneous conditions and can be enabled on a per-category basis. These are typically time-based - i.e. a warning will be reported if something exceeded an allowable time. Each category can also have its own configuration to tune the thresholds for triggering the warning.

!!! warning
	Warning Reporting is only available if Thunder is compiled with the `WARNING_REPORTING` option, which can be found [here](https://github.com/rdkcentral/Thunder/blob/76e08e2e5eafa12272b9080d2680091824124d9c/Source/extensions/CMakeLists.txt#L26), and is disabled by default. Note that it should not be enabled in Production, since it not only leads to a higher CPU and memory usage, but also it does not add any value to have it turned on in Production.

### Runtime

It is also possible to use the `MessageControl` plugin to edit the configuration values at runtime. At the moment, it is possible to enable/disable any category at runtime, either globally for logging and warning reporting, or individually per plugin for the tracing messages. This can be achieved by either using the ThunderUI or doing a simple JSON-RPC call. In the example below, there is a request to enable traces from the `Information` category in the `BluetoothControl` plugin.

:arrow_right: Request

```json
{
	"jsonrpc": "2.0",
	"id": 42,
	"method": "MessageControl.1.enable",
	"params": {
		"type": "Tracing",
		"category": "Information",
		"module": "Plugin_BluetoothControl",
		"enabled": 1
    }
}
```

:arrow_left: Response

```json
{
	"jsonrpc": "2.0",
	"id": 42,
	"result": null
}
```

## Tracing

First, let us briefly discuss the part responsible for the tracing. In theory, tracing is the process of monitoring the flow of a request through an application. It is used to identify performance bottlenecks and understand the interactions between different components of a distributed system. A trace typically consists of a series of events, each of which corresponds to a particular stage in the processing of a request. Tracing provides a much broader and more continuous perspective of the application compared to logging. The goal of tracing is to track the flow and evolution of data within a program so that we can be proactive instead of just reactive and increase overall performance.

```c++
TRACE(Trace::Information, (_T("Not an A2DP audio sink device!")));

TRACE_GLOBAL(Trace::Error, ("Is this not a descriptor to a DRM Node... =^..^= "));
```

In Thunder, there are two main macros that can be used to wrap messages that will be treated as traces. These macros, namely `TRACE` and `TRACE_GLOBAL`, can be found in `Thunder/Source/messaging/TraceControl.h`. Upon examining the code, we can notice that the only distinction between them is that `TRACE` includes the class name where it is used, whereas the global version refers to the function name instead of a class.

!!! warning
	The `TRACE` macro should always be used if we have the `this` pointer available, so when tracing takes place inside a class, otherwise you have to use `TRACE_GLOBAL`.

```c++
#define TRACE(CATEGORY, PARAMETERS)
    do {
        using __control__ = TRACE_CONTROL(CATEGORY);
        if (__control__::IsEnabled() == true) {
            CATEGORY __data__ PARAMETERS;
            Thunder::Core::Messaging::MessageInfo __info__(
                __control__::Metadata(),
                Thunder::Core::Time::Now().Ticks()
            );
            Thunder::Core::Messaging::IStore::Tracing __trace__(
                __info__,
                __FILE__,
                __LINE__,
                Thunder::Core::ClassNameOnly(typeid(*this).name()).Text()
            );
            Thunder::Messaging::TextMessage __message__(__data__.Data());
            Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);
        }
    } while(false)
```

In the code fragment above, you can observe the internal structure of the macro. While more detailed explanations will be provided in subsequent paragraphs, let us cover the general process briefly. Firstly, we need to verify if the corresponding category is enabled. If it is, we proceed to create metadata for the message and send it alongside the message itself to the plugin, which will be discussed in greater detail in the plugin section.

### Internal tracing (TRACE_Lx)

!!! warning
	TRACE_Lx macros should only be used by the framework for low-level debug messages and not by plugins.

```c++
TRACE_L1("Failed to load library: %s, error %s", filename, _error.c_str());
```

On a side note, if you browse the Thunder code, you may come across another macro called `TRACE_L1`. You might wonder how it differs from the previously mentioned macros. To understand its purpose, it is important to know that tracing is disabled in certain layers of Thunder. This is necessary because `Messaging` relies on both `COM` and `Core`, and we surely want to avoid circular dependencies. As a result, `TRACE_L1` is restricted to lower layers only. Within the framework, you can utilize it in `Core` and `Messaging`, but it should not be used in `COM`. Moreover, once you reach a certain point, refrain from using `TRACE_L1` altogether, and instead use the `TRACE` macro.

### Defining custom trace categories

Furthermore, Thunder allows to create customized categories within plugins by simply creating classes that handle message formatting, which is pretty straightforward and you can find plenty examples of this across many plugins that all look very similar to one another. Here is an example which comes from the `AVS` plugin and is located in `ThunderNanoServices/AVS/Impl/TraceCategories.h`. This is a simple trace category for logs coming from the AVS plugin and implementation.

```c++
class AVSClient {
public:
    AVSClient() = delete;
    AVSClient(const AVSClient& a_Copy) = delete;
    AVSClient& operator=(const AVSClient& a_RHS) = delete;
    ~AVSClient() = default;

    explicit AVSClient(const string& text)
        : _text(Core::ToString(text))
    {
    }

    AVSClient(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        Trace::Format(_text, formatter, ap);
        va_end(ap);
    }

    inline const char* Data() const
    {
        return (_text.c_str());
    }

    inline uint16_t Length() const
    {
        return (static_cast<uint16_t>(_text.length()));
    }

private:
    std::string _text;
};
```

## Logging

Logging, on the other hand, is the process of recording events that occur during the execution of an application. These events could be error messages, warnings, or informational messages that provide details about the application’s behavior. Logging is typically used for debugging and troubleshooting purposes. The primary goal of logging is to provide a historical record of events that can be used to analyze and diagnose problems. This means that in theory, we want to save the logs for later use.

```c++
SYSLOG(Logging::Startup, (_T("Failure in setting Key:Value:[%s]:[%s]\n"), index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str()));
```

This relates to the main difference between the `TRACE` and `SYSLOG` macros in Thunder, which is that `SYSLOG` is present in any build, and `TRACE` is dropped in production, the same way as, for example, an `ASSERT` macro. It concludes the main distinction between these two message categories in Thunder: tracing should be used when we want to indicate a vital information during the development, and we should use logging to record any important data that could be useful in the future.

```c++
SYSLOG_GLOBAL(Logging::Fatal, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
```

Similarly as in the case of tracing, there is also a global version of the `SYSLOG` macro, but this is something that was used prior to when tracing and logging became separate issues.

!!! warning
	The `SYSLOG_GLOBAL` macro is marked as deprecated, because it is no longer necessary since the separation of logging and tracing (logging does not have either a class or a function name anymore), and thus it should not be used.

In the piece of code below we can see that the first noticeable difference between tracing and logging macros is that there is an assert, which ensures that the macro parameter `CATEGORY` is an actual logging category. Apart from this difference, everything looks very similar besides the omission of file, line and class name, since these are not particularly useful when it comes to logging.

!!! note
	It is not possible to create a custom logging category, unlike in the case of tracing or warning reporting.

```c++
#define SYSLOG(CATEGORY, PARAMETERS)
    do {
        static_assert(std::is_base_of<Thunder::Logging::BaseLoggingType<CATEGORY>, CATEGORY>::value, "SYSLOG() only for Logging controls");
        if (CATEGORY::IsEnabled() == true) {
            CATEGORY __data__ PARAMETERS;
            Thunder::Core::Messaging::MessageInfo __info__(
                CATEGORY::Metadata(),
                Thunder::Core::Time::Now().Ticks()
            );
            Thunder::Core::Messaging::IStore::Logging __log__(__info__);
            Thunder::Messaging::TextMessage __message__(__data__.Data());
            Thunder::Messaging::MessageUnit::Instance().Push(__log__, &__message__);
        }
    } while(false)
```

In addition, you might be wondering why we have some `Messaging` components such as `Metadata` in `Core`, and why everything is not simply inside
`Source/messaging`. From an architectural point of view, there is a reason for this, which revolves around the need for reporting capabilities to be accessible not only in the plugins and other Thunder components, but also within `Core`, where, for instance, we would like to measure how long it takes to lock and then unlock. To accomplish this, we need some of the messaging features to be accessible in `Core`, because otherwise we would get, as you may have guessed, circular dependencies.

## Warning Reporting

Last but not least, it is about time to describe warning reporting. It is a crucial aspect of software systems that aims to alert developers and users about potential issues or anomalies within the system's operation. Unlike logging and tracing, which primarily focus on capturing and storing detailed information for diagnostic purposes, warning reporting specifically targets situations where certain conditions might lead to unexpected behavior or errors. While logging records events and activities to provide a comprehensive record of system activity, and tracing follows the flow of execution across different components or services, warning reporting is designed to raise flags about specific conditions that could lead to failures, performance degradation, or security vulnerabilities.

By emphasizing the significance of these conditions, warning reporting enables proactive identification and resolution of potential problems, enhancing system reliability and user experience. It acts as an early detection mechanism, signaling the need for attention and potential action before the situation escalates into a critical failure or incident. Overall, warning reporting complements logging and tracing by focusing on identifying and communicating conditions that require immediate attention, facilitating effective troubleshooting and maintenance of software systems.

```c++
REPORT_OUTOFBOUNDS_WARNING(WarningReporting::SinkStillHasReference, _referenceCount);
```

### Macros

Within Thunder, at the moment there are four distinct macros dedicated to warning reporting, each with a slightly different use case scenario. You can find these macros inside `Source/core/WarningReportingControl.h`.

#### REPORT_WARNING

The first macro `REPORT_WARNING` only requires the `CATEGORY` parameter. This macros should be used when reporting a warning without without specific values within the category. While all categories in Thunder currently utilize actual values and compare them against reporting bounds, this macro has been implemented with future use cases in mind.

If someone decides to add their own simple category without reporting values, they can effortlessly make use of `REPORT_WARNING` macro. Below, there is an example of a potential macro use with a category that does not need any additional values to compare, thus the warning will always be triggered when entering this piece of code.

#### REPORT_OUTOFBOUNDS_WARNING

The following macro, namely `REPORT_OUTOFBOUNDS_WARNING`, serves a different purpose compared to the macros described in the previous paragraphs. As its name suggests, this macro is utilized when we want to generate a warning only if a specific parameter value exceeds the defined reporting bound. In the example below, a warning occurs while destructing a sink which still has a reference count greater than zero.

```c++
~Sink()
{
    REPORT_OUTOFBOUNDS_WARNING(WarningReporting::SinkStillHasReference, _referenceCount);

    if (_referenceCount != 0) {
        // This is probably due to the fact that the "other" side killed the connection, we need to
        // Remove our selves at the COM Administrator map.. no need to signal Releases on behalf of the dropped connection anymore..
        TRACE_L1("Oops this is scary, destructing a (%s) sink that still is being refered by something", typeid(ACTUALSINK).name());
    }
}
```

#### REPORT_OUTOFBOUNDS_WARNING_EX

The `REPORT_OUTOFBOUNDS_WARNING_EX` macro includes an additional user-provided callsign parameter. Below is an example of using the `REPORT_OUTOFBOUNDS_WARNING_EX` macro taken from `Source/core/WorkerPool.h`. Here a warning is triggered if a job has taken too long to complete (which could indicate a deadlock).

```c++
void AnalyseAndReportDispatchedJobs()
{
    _lock.Lock();

    if (_dispatchedJobList.size() > 0 && IsActive()) {
        for (auto &job : _dispatchedJobList) {
            ++job.ReportRunCount;
            REPORT_OUTOFBOUNDS_WARNING_EX(WarningReporting::JobActiveForTooLong, job.CallSign.c_str(),
            static_cast<uint32_t>((Time::Now().Ticks() - job.DispatchedTime) / Time::TicksPerMillisecond));
        }
    }
    _lock.Unlock();
}
```

#### REPORT_DURATION_WARNING

The `REPORT_DURATION_WARNING` macro, as its name implies, serves the purpose of measuring the execution time of a specific code segment and generating a warning if the duration exceeds the expected threshold. In the provided code snippet, we observe a key distinction compared to the previous macros: the first parameter of the macro represents the code segment to be measured, and the timing is captured prior to invoking `Analyze()`.

!!! note
	Even if warning reporting is not enabled in the code or if a specific category provided as a parameter is not enabled, this code segment will still be executed.

```c++
#define REPORT_DURATION_WARNING(CODE, CATEGORY, ...)
	if (...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) {
        Thunder::Core::Time start = Thunder::Core::Time::Now();
        CODE
        uint32_t duration = static_cast<uint32_t>((Core::Time::Now().Ticks() - start.Ticks()) / Core::Time::TicksPerMillisecond);
		...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>> __message__;
        if (__message__.Analyze(Thunder::Core::System::MODULE_NAME, ...CallsignAccess<&...MODULE_NAME>::Callsign(),
                                duration, ##__VA_ARGS__) == true) {
            ...WarningReportingUnitProxy::Instance().ReportWarningEvent(
                ...CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(),__message__);
        }
    } else {
        CODE
    }
```

The inclusion of the `REPORT_DURATION_WARNING` macro facilitates the efficient monitoring of code execution times and allows for the prompt identification of potential performance issues. By incorporating this macro at strategic points in the code, developers can gain valuable insights into the duration of specific code segments and receive warnings when execution times exceed the defined thresholds. For instance, there is an example below of how this macro was used in the `OpenCDMi` plugin to check whether the decryption is not taking too long:

```c++
REPORT_DURATION_WARNING(
    {
    cr = _mediaKeys->Decrypt(
        payloadBuffer,
        BytesWritten(),
        &clearContent,
        &clearContentSize,
        const_cast<CDMi::SampleInfo *>(&sampleInfo),
        dynamic_cast<const CDMi::IStreamProperties *>(&streamProperties));
    },
    WarningReporting::TooLongDecrypt
);
```

#### Creating a warning reporting category

Unlike the tracing and logging categories, the default warning reporting categories are not declared using macros. Instead, they are directly declared within the designated header files mentioned above.

The code snippet below demonstrates the simplicity of creating a custom warning reporting category. If an additional `Analyze()` method is not necessary, it is sufficient to declare the `Serialize()` and `Deserialize()` methods to return `0`. However, attention must be given to the implementation of the `ToString()` method, along with the variables `DefaultWarningBound` and `DefaultReportBound`.

!!! note
	In the near future, we want the `MessageControl` plugin to handle reports and warnings *separately* - a **report** means that we send the data to be stored, for example, in a file and then analyzed, while a **warning** is more severe, so we want to instantly output it to a different location, for instance, a console or a network stream. In the end, we are going to split it in the `MessageControl` plugin in such a way that it will be possible to filter out the warnings.

```c++
class EXTERNAL TooLongWaitingForLock {
public:
    TooLongWaitingForLock(const TooLongWaitingForLock&) = delete;
    TooLongWaitingForLock& operator=(const TooLongWaitingForLock&) = delete;
    TooLongWaitingForLock() = default;
    ~TooLongWaitingForLock() = default;

    //nothing to serialize/deserialize here
    uint16_t Serialize(uint8_t[], const uint16_t) const
    {
        return 0;
    }

    uint16_t Deserialize(const uint8_t[], const uint16_t)
    {
        return 0;
    }

    void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
    {
        visitor = (_T("It took suspiciously long to acquire a critical section"));
        visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
    };

    static constexpr uint32_t DefaultWarningBound = { 1000 };
    static constexpr uint32_t DefaultReportBound = { 1000 };
};
```

## MessageControl plugin

Now that we have explored the concepts of logging, tracing, and warning reporting in theory and their implementation within Thunder, we can delve into the significance of the `MessageControl` plugin. This plugin plays a vital role as it manages all message types seamlessly. In the subsequent sections, we will provide a more detailed explanation of how the plugin operates and the specific steps it undertakes.

### Content of the message

Before examining the workings of the plugin, it is crucial to provide a brief overview of the components that make up a message, as they directly impact the message processing. A message consists of three main parameters: module, category, and the actual content of the message. Understanding these parameters is essential for effective message handling.

#### Categories of each message type

When it comes to tracing, the module parameter represents the name of the plugin responsible for sending the message, for example `Plugin_Cobalt`. 

!!! note
	Module name is defined by each plugin in its `Module.h` file, and every plugin has a unique module name.

However, in logging, the module name is simply set as `Syslog`, and similarly, in warning reporting, the module name is set as `Reporting`. This provides a standardized module name for all logging and warning reporting messages.

```c++
const char* MODULE_LOGGING = _T("SysLog");
const char* MODULE_REPORTING = _T("Reporting");
```

Choosing the appropriate category is vital when working with macros. The category can be described as a class that either formats the message string or best represents the nature of the message. While it is possible to create custom categories, which will be explained in the following paragraph, it is generally recommended to utilize the predefined categories available in Thunder to avoid redundancy.

##### Tracing default categories

Thunder already offers several default categories for tracing:

-   Text
-   Initialization
-   Information
-   Warning
-   Error
-   Fatal
-   Constructor
-   Destructor
-   CopyConstructor
-   AssignmentOperator
-   MethodEntry
-   MethodExit
-   Duration

Each of them is created in `Thunder/Source/messaging/TraceCategories.h` either manually or by using the `DEFINE_MESSAGING_CATEGORY` macro:

```c++
DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Text);
```

As you can see in the code, these classes inherit from a templated class called `BaseCategoryType`. The association of these categories with tracing is established by passing the `TRACING` metadata type as a template parameter.

##### Logging categories

Similarly, logging includes several categories, such as:

-   Startup
-   Shutdown
-   Notification
-   Error
-   ParsingError
-   Fatal
-   Crash

These logging categories are created using the `DEFINE_LOGGING_CATEGORY` macro defined in `Thunder/Source/messaging/LoggingCategories.h`. The process of establishing these categories is slightly more complex and involves the utilization of the mentioned macro.

```c++
#define DEFINE_LOGGING_CATEGORY(CATEGORY)
    DEFINE_MESSAGING_CATEGORY(Thunder::Logging::BaseLoggingType<CATEGORY>, CATEGORY)
    template<>
    EXTERNAL typename ...BaseLoggingType<CATEGORY>::Control ...BaseLoggingType<CATEGORY>::_control;
```

The `DEFINE_LOGGING_CATEGORY` macro functions by invoking the `DEFINE_MESSAGING_CATEGORY` macro. However, in this case, the classes created inherit from the templated class `BaseLoggingType`. The definition of this class can be found directly above the macro definition. Notably, the `BaseLoggingType` class itself inherits from the `BaseCategoryType` class, with the `LOGGING` metadata type passed as a template parameter.

##### Warning reporting default categories

Last but not least, the warning reporting categories in Thunder can be found in either `Source/Thunder/WarningReportingCategories.h` or `Source/core/WarningReportingCategories.h`. These categories, located within the `Thunder::WarningReporting` namespace, include:

- TooLongWaitingForLock
- SinkStillHasReference
- TooLongInvokeRPC
- JobTooLongToFinish
- JobTooLongWaitingInQueue
- TooLongDecrypt
- JobActiveForTooLong
- TooLongPluginState
- TooLongInvokeMessage

### Internals - from a macro to an output

Now, let us delve into how the MessageControl plugin manages the messages from logging, tracing, and warning reporting. To begin, it is best to revisit the macros discussed earlier. When handling tracing and logging messages, the first step involves checking whether the corresponding category is enabled. Subsequently, both tracing and logging macros create an object of the `CATEGORY` class, which encapsulates the actual contents of the message provided as `PARAMETERS` within the macro. This enables the convenient storage and processing of the message content.

```c++
#define TRACE_GLOBAL(CATEGORY, PARAMETERS)
    do {
        using __control__ = TRACE_CONTROL(CATEGORY);
        if (__control__::IsEnabled() == true) {
            CATEGORY __data__ PARAMETERS;
            Thunder::Core::Messaging::MessageInfo __info__(
                __control__::Metadata(),
                Thunder::Core::Time::Now().Ticks()
            );
            Thunder::Core::Messaging::IStore::Tracing __trace__(
                __info__,
                __FILE__,
                __LINE__,
                __FUNCTION__
            );
            Thunder::Messaging::TextMessage __message__(__data__.Data());
            Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);
        }
    } while(false)
```

In tracing, you may recall that the module name indicates the originating plugin from which the traces emerged. To obtain the module name for tracing purposes, the `TRACE_CONTROL` macro is invoked. This macro plays a crucial role in capturing the module name associated with the trace. 

Next, an object of `MessageInfo` class is created that stores the metadata and time of the message. This provides essential information for further handling and analysis of the message. Then, the last and the most complex part of the message is built, and it is slightly different for tracing, logging and warning reporting. File, line and a name of a class or a function for global trace version are passed as the members of the `Tracing` class. On the other hand, there are no additional data like this for logging, and for warning reporting there is only a callsign.

#### Warning Reporting's differences

The initial section is identical across all macros, including warning reporting, tracing, and logging. However, the approach to creating a message differs for warning reporting messages. To provide a clearer understanding, let us examine an example code snippet. Please note that for improved readability, the majority of the namespace has been intentionally omitted.

```c++
#define REPORT_OUTOFBOUNDS_WARNING(CATEGORY, ACTUALVALUE, ...)
	if(...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) {
		...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>> __message__;
        if(__message__.Analyze(...MODULE_NAME, ...Callsign(), ACTUALVALUE, ##__VA_ARGS__) == true) {
            ...WarningReportingUnitProxy::Instance().ReportWarningEvent(
                ...CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(),
                    __message__);
		}
    }
```

##### Analyze()

The macro begins by verifying if the corresponding category is enabled. In the provided code snippets, we can observe that if the category is enabled, an object is created, and the `Analyze()` method is invoked. This represents the primary distinction between the warning reporting macros and those used for tracing and logging.

```        c++
templatec <typename... Args>
bool Analyze(const char moduleName[], const char identifier[], const uint32_t actualValue, Args&&... args)
{
    bool report = false;
    _actualValue = actualValue;
    if (actualValue > _reportingBound.load(std::memory_order_relaxed)) {
        report = CallAnalyze(moduleName, identifier, std::forward<Args>(args)...);
    }
    return report;
}
```

In the code above, we can notice that the main job of this method is to check whether the `actualValue` reported by the macro exceeds the `_reportingBound` set by the user.  If the condition is met, then the `CallAnalyze()` method is called, which will invoke an `Analyze()` method specific to the warning reporting category. However, if the category does not have such a method, then `CallAnalyze()` simply returns `true`. In such fashion, we can easily check additional conditions necessary for a warning to trigger.

##### Warning Reporting proxy

Moving forward, the next step involves sending the message to a proxy called `WarningReportingUnit`. This distinction highlights one of the key differences between warning reporting and other message types. Since warning reporting can be used in `Core`, on which `messaging` depends, it necessitated the creation of a proxy to enable the use of warning reporting macros in `Core` while still routing the messages to `MessageUnit` located in `Source/messaging`. This entire process is accomplished through the utilization of the `ReportWarningEvent()` method:

```c++
void WarningReportingUnit::ReportWarningEvent(const char identifier[], const IWarningEvent& information)
{        
    Thunder::Core::Messaging::Metadata metadata(Thunder::Core::Messaging::Metadata::type::REPORTING,
                                                     information.Category(), Thunder::Core::Messaging::MODULE_REPORTING);
    Thunder::Core::Messaging::MessageInfo messageInfo(metadata, Thunder::Core::Time::Now().Ticks());
    Thunder::Core::Messaging::IStore::WarningReporting report(messageInfo, identifier);

    string text;
    information.ToString(text);
    Thunder::Messaging::TextMessage data(text);

    Thunder::Messaging::MessageUnit::Instance().Push(report, &data);
}
```

The code above is very similar to the one that can be found directly in tracing and logging macros, but for warning reporting the functionality of sending messages through the `Push()` method of `MessageUnit` has to be outside of `Core`.

In tracing and logging macros, the user directly enters the message as a macro parameter. However, this differs for warning reporting. As shown in the provided code snippet, the actual message content is obtained from the `ToString()` method. This method needs to be implemented in each warning reporting category class and should return a desired string to be printed when a warning occurs.

#### MessageUnit::Push()

In addition to the part of the macros where the messages are formed, the crucial part to which we want to pay extra attention is this line of code:

```c++
Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);
```

This is how the communication between Thunder and the `MessageControl` plugin takes place. For tracing and logging it is within the macros, but for warning reporting it is in the separate method `ReportWarningEvent()` of the reporting proxy `WarningReportingUnit`. The first step involves the construction of message content. Once prepared, it is push to a buffer or special queue in the second line of code. This buffer serves as a centralized storage for messages, ensuring that they are properly organized and ready for further processing.

```c++
/* virtual */ void MessageUnit::Push(const Core::Messaging::MessageInfo& messageInfo, const Core::Messaging::IEvent* message)
{
    //logging messages can happen in Core, meaning, otherside plugin can be not started yet
    //those should be just printed
    if (_settings.IsDirect() == true) {
        _direct.Output(messageInfo, message);
    }

    if (_dispatcher != nullptr) {
        uint8_t serializationBuffer[DataSize];
        uint16_t length = 0;

        ASSERT(messageInfo.Type() != Core::Messaging::Metadata::type::INVALID);

        length = messageInfo.Serialize(serializationBuffer, sizeof(serializationBuffer));

        //only serialize message if the information could fit
        if (length != 0) {
            length += message->Serialize(serializationBuffer + length, sizeof(serializationBuffer) - length);

            if (_dispatcher->PushData(length, serializationBuffer) != Core::ERROR_NONE) {
                TRACE_L1("Unable to push message data!");
            }
        }
        else {
            TRACE_L1("Unable to push data, buffer is too small!");
        }
    }
```

When the `MessageControl` plugin is used, each message is buffered and added to a queue. The ultimate destination of these messages depends on the specific configuration settings applied to the plugin. The code segment above is responsible for pushing messages of any type and their associated metadata to the buffer.

#### MessageClient::PopMessagesAndCall()

```c++
using MessageHandler = std::function<void(const Core::ProxyType<Core::Messaging::MessageInfo>&, const Core::ProxyType<Core::Messaging::IEvent>&)>;

void MessageClient::PopMessagesAndCall(const MessageHandler& handler)
{
    _adminLock.Lock();

    for (auto& client : _clients) {
        uint16_t size = sizeof(_readBuffer);

        while (client.second.PopData(size, _readBuffer) != Core::ERROR_READ_ERROR) {
            ASSERT(size != 0);

            if (size > sizeof(_readBuffer)) {
                size = sizeof(_readBuffer);
            }

            const Core::Messaging::Metadata::type type = static_cast<Core::Messaging::Metadata::type>(_readBuffer[0]);
            ASSERT(type != Core::Messaging::Metadata::type::INVALID);

            uint16_t length = 0;

            ASSERT(handler != nullptr);

            auto factory = _factories.find(type);

            if (factory != _factories.end()) {
                Core::ProxyType<Core::Messaging::MessageInfo> metadata;
                Core::ProxyType<Core::Messaging::IEvent> message;

                metadata = factory->second->GetMetadata();
                message = factory->second->GetMessage();

                length = metadata->Deserialize(_readBuffer, size);
                length += message->Deserialize((&_readBuffer[length]), (size - length));

                handler(metadata, message);
            }

            if (length == 0) {
                client.second.FlushDataBuffer();
            }

            size = sizeof(_readBuffer);
        }
    }

    _adminLock.Unlock();
}
```

Once the `MessageControl` plugin is given a notification through its interface after receiving a doorbell ring (after the `WaitForUpdated()` function),  it proceeds to retrieve the messages from the buffers by calling the `PopMessagesAndCall()` method from the `MessageClient` component. After gathering the messages, the `MessageControl` plugin proceeds to send them to their designated destinations, which is accomplished by invoking the `Message()` method provided by the plugin.

```c++
void Dispatch()
{
    _client.WaitForUpdates(Core::infinite);

    _client.PopMessagesAndCall([this](const Core::ProxyType<Core::Messaging::MessageInfo>& metadata, const Core::ProxyType<Core::Messaging::IEvent>& message) {
        // Turn data into piecies to trasfer over the wire
        Message(*metadata, message->Data());
    });
}
```

!!! warning
	If the `MessageControl` plugin is disabled, the message queue will eventually reach its capacity. As a result, the framework will issue warnings indicating that there is no more space available in the queue. In this situation, older messages will be overwritten by newer ones as they continue to arrive. Of course it does not change the fact that the messages can still be generated by `DirectOutput`. 

Within the `Message()` method, as shown below, the plugin is responsible for sending the message to all designated outputs selected in the plugin configuration.

```c++
void Message(const Core::Messaging::MessageInfo& metadata, const string& message)
{
    // Time to start sending it to all interested parties...
    _outputLock.Lock();

    for (auto& entry : _outputDirector) {
        entry->Message(metadata, message);
    }

    _webSocketExporter.Message(metadata, message);

    _outputLock.Unlock();
}
```

#### Details of output configuration

Let us take a closer look at how outputs are configured and how this configuration can be modified. The configuration is generated as a JSON file, where specific JSON values correspond to the actual objects in the code. The association between an object and its corresponding JSON value is established within the constructor of the `Config` class in `ThunderNanoServicesRDK/MessageControl/MessageControl.h` utilizing the `Add()` method. This connection ensures that the configuration values in the JSON file are correctly linked to the corresponding objects in the code. However, to understand the configuration process in more detail, it is important to examine the `Initialize()` method in `MessageControl/MessageControl.cpp`.

```c++
if ((service->Background() == false) && (((_config.SysLog.IsSet() == false) && (_config.Console.IsSet() == false)) || (_config.Console.Value() == true))) {
    Announce(new Publishers::ConsoleOutput(abbreviate));
}
```

Following the configuration reading and the verification of a successful plugin start, there are four conditional statements where the `Announce()` methods are called. Let us consider an example where the system is running in the foreground, syslog and console outputs are not yet set, but console output has been enabled in the configuration. In this scenario, the `Announce()` method is invoked with a new object of the `Publishers::ConsoleOutput` class. This can be observed in the provided listing. Additionally, the class constructor takes as a parameter whether the message should be abbreviated or not.

```c++
void Announce(Publishers::IPublish* output)
{
    _outputLock.Lock();

    ASSERT(std::find(_outputDirector.begin(), _outputDirector.end(), output) == _outputDirector.end());

    _outputDirector.emplace_back(output);

    _outputLock.Unlock();
}
```

Let us examine the `Announce()` method and then investigate the `ConsoleOutput` class. In the code listing above, we can notice that the method’s body is inside a lock, so that we can be certain there will be no concurrency issues. The crucial part is the `_outputDirector` vector of pointers for the `Publishers::IPublish` objects. First, we make sure that an object passed as a method parameter is not already present in this vector, and then add it to the list. In our example, the `Announce()` method takes a new object of the `Publishers::ConsoleOutput` class as a parameter.

```c++
struct IPublish {
    virtual ~IPublish() = default;

    virtual void Message(const Core::Messaging::MessageInfo& metadata, const string& text) = 0;
};

class ConsoleOutput : public IPublish {
public:
    ConsoleOutput() = delete;
    ConsoleOutput(const ConsoleOutput&) = delete;
    ConsoleOutput& operator=(const ConsoleOutput&) = delete;

    explicit ConsoleOutput(const Core::Messaging::MessageInfo::abbreviate abbreviate)
        : _convertor(abbreviate)
    {
    }
    ~ConsoleOutput() override = default;

public:
    void Message(const Core::Messaging::MessageInfo& metadata, const string& text);

private:
    Text _convertor;
};
```

If you have noticed the connection between the `ConsoleOutput` and `IPublish` structures, you are absolutely correct. Within the `ThunderNanoServicesRDK/MessageControl/MessageOutput.h` file, you will find the `IPublish` structure along with several classes that inherit from it, including the `ConsoleOutput` class. The base structure `IPublish` has a virtual destructor, so that we can ensure that an instance of a derived class will not be potentially deleted through a pointer to the base class. In addition, it also has a virtual method `Message()` that is overridden in each of the derived classes.

#### Convert() - the final step

```c++
void ConsoleOutput::Message(const Core::Messaging::MessageInfo& metadata, const string& text) /* override */
{
    std::cout << _convertor.Convert(metadata, text);
}

string Text::Convert(const Core::Messaging::MessageInfo& metadata, const string& text) /* override */
{
    ASSERT(metadata.Type() != Core::Messaging::Metadata::type::INVALID);

    string output = metadata.ToString(_abbreviated).c_str() +
                    Core::Format("%s\n", text.c_str());

    return (output);
}
```

The final step before the message is sent to the output involves invoking the `Convert()` method. As demonstrated in the above code listing, this method is responsible for constructing a string that combines the metadata, formatted according to the specific message type, with an actual text of the message. The resulting string provides a comprehensive representation of the message, ready for output. For instance, this is how the `ToString()` method looks like for tracing type messages:

```c++
string IStore::Tracing::ToString(const abbreviate abbreviate) const
{
    string result;
    const Core::Time now(TimeStamp());

    if (abbreviate == abbreviate::ABBREVIATED) {
        const string time(now.ToTimeOnly(true));
        result = Core::Format("[%s]:[%s]:[%s]: ",
                time.c_str(),
                Module().c_str(),
                Category().c_str());
    }
    else {
        const string time(now.ToRFC1123(true));
        result = Core::Format("[%s]:[%s]:[%s:%u]:[%s]:[%s]: ",
                time.c_str(),
                Module().c_str(),
                Core::FileNameOnly(FileName().c_str()),
                LineNumber(),
                ClassName().c_str(),
                Category().c_str());
    }

    return (result);
}
```

In summary, the output of messages within Thunder is determined by the list of listeners stored in the `_outputDirector` container. This list is populated through successive calls to the `Announce()` method, which can occur multiple times during the initialization of the `MessageControl` plugin, depending on the configuration. Each call to the `Announce()` method adds a new output listener to the `_outputDirector` list, configuring the desired destinations for message delivery.
