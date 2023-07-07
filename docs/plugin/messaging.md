# Messaging

In the past, logging, tracing and warning reporting were completely separate issues and were handled individually. However, even though these message types have distinct characteristics, we recognized the advantages of consolidating them into a unified framework, which is now referred to as `Messaging`. We are convinced it is much better suited for the development, and with some recent fixes that will be described in the following sections, `Messaging` is fully operational. We strongly believe that it is a good time to shed some more light on that functionality, since it is simple to use and yet very effective.

Before we begin to describe what the `MessageControl` plugin does and where logging, tracing and warning reporting can be directed, it is probably a good idea to start from the ground up. In this case, what precisely are these message types, what are their differences in general and in Thunder, how exactly the messages are created, and what is going on with them before they end up at their final destination.

We can all agree that logging, tracing and warning reporting are important techniques often used in software development to gather information and provide insight into the behavior of an application. However, there are some fundamental differences between them.

## Tracing

First, let us briefly discuss the part responsible for the tracing. In theory, tracing is the process of monitoring the flow of a request through an application. It is used to identify performance bottlenecks and understand the interactions between different components of a distributed system. A trace typically consists of a series of events, each of which corresponds to a particular stage in the processing of a request. Tracing provides a much broader and more continuous perspective of the application compared to logging. The goal of tracing is to track the flow and evolution of data within a program so that we can be proactive instead of just reactive and increase overall performance.

```c++
TRACE(Trace::Information, (_T("Not an A2DP audio sink device!")));

TRACE_GLOBAL(Trace::Error, ("Is this not a descriptor to a DRM Node... =^..^= "));
```

In Thunder, there are two main macros that can be used to wrap messages that will be treated as traces. These macros, namely `TRACE` and `TRACE_GLOBAL`, can be found in `Thunder/Source/messaging/TraceControl.h`. Upon examining the code, we can notice that the only distinction between them is that `TRACE` includes the class name where it is used, whereas the global version refers to the function name instead of a class.

```c++
TRACE_L1("Failed to load library: %s, error %s", filename, _error.c_str());
```

On a side note, if you browse the Thunder code, you may come across another macro called `TRACE_L1`. You might wonder how it differs from the previously mentioned macros. To understand its purpose, it is important to know that tracing is disabled in certain layers of Thunder. This is necessary because `Messaging` relies on both `COM` and `Core`, and we surely want to avoid circular dependencies. As a result, `TRACE_L1` is restricted to lower layers only. Within the framework, you can utilize it in `Core` and `Messaging`, but it should not be used in `COM`. Moreover, once you reach a certain point, refrain from using `TRACE_L1` altogether, and instead use the `TRACE` macro.

```c++
#define TRACE(CATEGORY, PARAMETERS)
    do {
        using __control__ = TRACE_CONTROL(CATEGORY);
        if (__control__::IsEnabled() == true) {
            CATEGORY __data__ PARAMETERS;
            WPEFramework::Core::Messaging::MessageInfo __info__(
                __control__::Metadata(),
                WPEFramework::Core::Time::Now().Ticks()
            );
            WPEFramework::Core::Messaging::IStore::Tracing __trace__(
                __info__,
                __FILE__,
                __LINE__,
                WPEFramework::Core::ClassNameOnly(typeid(*this).name()).Text()
            );
            WPEFramework::Messaging::TextMessage __message__(__data__.Data());
            WPEFramework::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);
        }
    } while(false)
```

In the code fragment above, you can observe the internal structure of the macro. While more detailed explanations will be provided in subsequent paragraphs, let us cover the general process briefly. Firstly, we need to verify if the corresponding category is enabled. If it is, we proceed to create metadata for the message and send it alongside the message itself to the plugin, which will be discussed in greater detail in the plugin section. Before continuing, it is also worth noting that there is a way to print the trace messages directly on the console in case the `MessageControl` plugin is disabled. In such a situation, the `Output()` method of the `DirectOutput` class can be used, but this will be described further on in the following section.

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

## Logging

Logging, on the other hand, is the process of recording events that occur during the execution of an application. These events could be error messages, warnings, or informational messages that provide details about the application’s behavior. Logging is typically used for debugging and troubleshooting purposes. The primary goal of logging is to provide a historical record of events that can be used to analyze and diagnose problems. This means that in theory, we want to save the logs for later use.

```c++
SYSLOG(Logging::Startup, (_T("Failure in setting Key:Value:[%s]:[%s]\n"), index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str()));
```

This relates to the main difference between the `TRACE` and `SYSLOG` macros in Thunder, which is that `SYSLOG` is present in any build, and `TRACE` is dropped in production, the same way as, for example, an `ASSERT` macro. It concludes the main distinction between these two message categories in Thunder: tracing should be used when we want to indicate a vital information during the development, and we should use logging to record any important data that could be useful in the future.

```c++
SYSLOG_GLOBAL(Logging::Fatal, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
```

Similarly as in the case of tracing, there is also a global version of the `SYSLOG` macro, and the difference is exactly the same as with tracing macros. Furthermore, the `DirectOutput` class can be configured using the `Mode()` method to send the messages to the system logger instead of simply printing them on the console.

In the piece of code below we can see that the first noticeable difference between tracing and logging macros is that there is an assert which ensures that the macro parameter `CATEGORY` is an actual logging category, so there is no way to create a custom logging category, unlike in the case of tracing or warning reporting. Apart from this difference, everything looks very similar besides the omission of file, line and class name, since these are not particularly useful when it comes to logging.

```c++
#define SYSLOG(CATEGORY, PARAMETERS)
    do {
        static_assert(std::is_base_of<WPEFramework::Logging::BaseLoggingType<CATEGORY>, CATEGORY>::value, "SYSLOG() only for Logging controls");
        if (CATEGORY::IsEnabled() == true) {
            CATEGORY __data__ PARAMETERS;
            WPEFramework::Core::Messaging::MessageInfo __info__(
                CATEGORY::Metadata(),
                WPEFramework::Core::Time::Now().Ticks()
            );
            WPEFramework::Core::Messaging::IStore::Logging __log__(__info__);
            WPEFramework::Messaging::TextMessage __message__(__data__.Data());
            WPEFramework::Messaging::MessageUnit::Instance().Push(__log__, &__message__);
        }
    } while(false)
```

In addition, you might be wondering why we have some `Messaging` components such as `Metadata` in `Core`, and why everything is not simply inside
`Source/messaging`. From an architectural point of view, there is a reason for this, which revolves around the need for reporting capabilities within `Core`. For instance, we would like to measure how long it takes to lock and then unlock. To accomplish this, we need some of the messaging features to be accessible in `Core`, because otherwise we would get, as you may have guessed, circular dependencies.

## Warning Reporting

Last but not least, it is about time to describe warning reporting. It is a crucial aspect of software systems that aims to alert developers and users about potential issues or anomalies within the system's operation. Unlike logging and tracing, which primarily focus on capturing and storing detailed information for diagnostic purposes, warning reporting specifically targets situations where certain conditions might lead to unexpected behavior or errors. While logging records events and activities to provide a comprehensive record of system activity, and tracing follows the flow of execution across different components or services, warning reporting is designed to raise flags about specific conditions that could lead to failures, performance degradation, or security vulnerabilities.

By emphasizing the significance of these conditions, warning reporting enables proactive identification and resolution of potential problems, enhancing system reliability and user experience. It acts as an early detection mechanism, signaling the need for attention and potential action before the situation escalates into a critical failure or incident. Overall, warning reporting complements logging and tracing by focusing on identifying and communicating conditions that require immediate attention, facilitating effective troubleshooting and maintenance of software systems.

```c++
REPORT_OUTOFBOUNDS_WARNING(WarningReporting::SinkStillHasReference, _referenceCount);
```

### Macros

Within Thunder, there are five distinct macros dedicated to warning reporting, each with a slightly different use case scenario. You can find these macros inside `Source/core/WarningReportingControl.h`. The first two macros, `REPORT_WARNING` and `REPORT_WARNING_GLOBAL`, only require the `CATEGORY` parameter. These macros should be used when reporting a warning without without specific values within the category. While all categories in Thunder currently use actual values and compare them with the reporting bounds. While all categories in Thunder currently utilize actual values and compare them against reporting bounds, these macros have been implemented with future use cases in mind. If someone decides to add their own simple category without reporting values, they can effortlessly make use of these macros.

The following two macros, namely `REPORT_OUTOFBOUNDS_WARNING` and `REPORT_OUTOFBOUNDS_WARNING_EX`, serve a different purpose compared to the macros described in the previous paragraph. As their names suggest, these macros are utilized when we want to generate a warning only if a specific parameter value exceeds the defined reporting bound. To provide a clearer understanding, let us examine an example code snippet. Please note that for improved readability, the majority of the namespace has been intentionally omitted, and the `REPORT_OUTOFBOUNDS_WARNING_EX` macro includes an additional user-provided callsign parameter.

```c++
#define REPORT_OUTOFBOUNDS_WARNING(CATEGORY, ACTUALVALUE, ...)
	if(...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) {
		...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>> __message__;
        if(__message__.Analyze(...MODULE_NAME, ...Callsign(), ACTUALVALUE, ##__VA_ARGS__) == true) {
            ...WarningReportingUnitProxy::Instance().ReportWarningEvent(
                ...CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),
                    __message__);
		}
    }
```

The initial section is identical across all macros, including warning reporting, tracing, and logging. It begins by verifying if the corresponding category is enabled. It's important to note that in Thunder, the tracing categories are disabled by default, while all logging and warning reporting categories are enabled. However, these settings can be modified either through the ThunderUI or the configuration options, which will be explained further in the upcoming paragraph.

Returning to the provided code snippet, we observe that if the category is enabled, an object is created, and the `Analyze()` method is invoked. This represents the primary distinction between the warning reporting macros and those used for tracing and logging.

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

### Warning Reporting proxy

Moving forward, the next step involves sending the message to a proxy called `WarningReportingUnit`. This distinction highlights one of the key differences between warning reporting and other message types. While warning reporting can be used in `Core`, which is dependent on `messaging`, it necessitated the creation of a proxy to enable the use of warning reporting macros in `Core` while still routing the messages to `MessageUnit` located in `Source/messaging`. This entire process is accomplished through the utilization of the `ReportWarningEvent()` method:

```c++
void WarningReportingUnit::ReportWarningEvent(const char identifier[], const IWarningEvent& information)
{        
    WPEFramework::Core::Messaging::Metadata metadata(WPEFramework::Core::Messaging::Metadata::type::REPORTING,
                                                     information.Category(), WPEFramework::Core::Messaging::MODULE_REPORTING);
    WPEFramework::Core::Messaging::MessageInfo messageInfo(metadata, WPEFramework::Core::Time::Now().Ticks());
    WPEFramework::Core::Messaging::IStore::WarningReporting report(messageInfo, identifier);

    string text;
    information.ToString(text);
    WPEFramework::Messaging::TextMessage data(text);

    WPEFramework::Messaging::MessageUnit::Instance().Push(report, &data);
}
```

The code above is very similar to the one that can be found directly in tracing and logging macros, but for warning reporting the functionality of sending messages through the `Push()` method of `MessageUnit` has to be outside of the `Core`.

In tracing and logging macros, the user directly enters the message as a macro parameter. However, this differs for warning reporting. As shown in the provided code snippet, the actual message content is obtained from the `ToString()` method. This method needs to be implemented in each warning reporting category class and should return the desired string to be printed when a warning occurs.

The `REPORT_DURATION_WARNING` macro, as its name implies, serves the purpose of measuring the execution time of a specific code segment and generating a warning if the duration exceeds the expected threshold. In the provided code snippet, we observe a key distinction compared to the previous macros: the first parameter of the macro represents the code segment to be measured, and the timing is captured prior to invoking `Analyze()`. It is important to note that even if warning reporting is not enabled in the code or if a specific category provided as a parameter is not enabled, this code segment will still be executed.

```c++
#define REPORT_DURATION_WARNING(CODE, CATEGORY, ...)
	if (...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) {
        WPEFramework::Core::Time start = WPEFramework::Core::Time::Now();
        CODE
        uint32_t duration = static_cast<uint32_t>((Core::Time::Now().Ticks() - start.Ticks()) / Core::Time::TicksPerMillisecond);
		...WarningReportingType<...WarningReportingBoundsCategory<CATEGORY>> __message__;
        if (__message__.Analyze(WPEFramework::Core::System::MODULE_NAME, ...CallsignAccess<&...MODULE_NAME>::Callsign(),
                                duration, ##__VA_ARGS__) == true) {
            ...WarningReportingUnitProxy::Instance().ReportWarningEvent(
                ...CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),__message__);
        }
    } else {
        CODE
    }
```

The inclusion of the `REPORT_DURATION_WARNING` macro facilitates the efficient monitoring of code execution times and allows for the prompt identification of potential performance issues. By incorporating this macro at strategic points in the code, developers can gain valuable insights into the duration of specific code segments and receive warnings when execution times exceed the defined thresholds.

Even though it is nice to learn some theory of how things work, it is probably best to learn in practice and from examples, so before moving to the next paragraph, let us have a quick look at a piece of code in which the warning reporting macro is actually utilized. The code fragment below can be found in `Source/core/WorkerPool.h`, and the idea behind this reporting is to automatically trigger a warning in case a job has been dispatched for way too long.

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

## MessageControl plugin

Now that we have explored the concepts of logging, tracing, and warning reporting in theory and their implementation within Thunder, we can delve into the significance of the `MessageControl` plugin. This plugin plays a vital role as it manages all message types seamlessly. In the subsequent sections, we will provide a more detailed explanation of how the plugin operates and the specific steps it undertakes. However, it is important to highlight that the plugin not only consolidates various message types but also offers the flexibility to direct these messages to different specific outputs. These outputs include the Console, Syslog, a file, or even a Network Stream through UDP.

### Content of the message

Before examining the workings of the plugin, it is crucial to provide a brief overview of the components that make up a message, as they directly impact the message processing. A message consists of three main parameters: module, category, and the actual content of the message. Understanding these parameters is essential for effective message handling.

#### Categories of each message type

When it comes to tracing, the module parameter represents the name of the plugin responsible for sending the message, for example `Plugin_Cobalt`. However, when the `SYSLOG` macro is used, the module name will simply be `Syslog`.

Choosing the appropriate category is vital when working with macros. The category can be described as the class that either formats the message string or best represents the nature of the message. While it is possible to create custom categories, which will be explained in the following paragraph, it is generally recommended to utilize the predefined categories available in Thunder to avoid redundancy. Thunder already offers six default categories for tracing:

-   Text

-   Initialization

-   Information

-   Warning

-   Error

-   Fatal

Each of them is created in `Thunder/Source/messaging/TraceCategories.h` using the `DEFINE_MESSAGING_CATEGORY` macro:

```c++
DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Text);
```

As you can see in the code, these classes inherit from a templated class called `BaseCategoryType`. The association of these categories with tracing is established by passing the `TRACING` metadata type as a template parameter. Similarly, logging includes several categories, such as:

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
    DEFINE_MESSAGING_CATEGORY(WPEFramework::Logging::BaseLoggingType<CATEGORY>, CATEGORY)
    template<>
    EXTERNAL typename ...BaseLoggingType<CATEGORY>::Control ...BaseLoggingType<CATEGORY>::_control;
```

The `DEFINE_LOGGING_CATEGORY` macro functions by invoking the `DEFINE_MESSAGING_CATEGORY` macro. However, in this case, the classes created inherit from the templated class `BaseLoggingType`. The definition of this class can be found directly above the macro definition. Notably, the `BaseLoggingType` class itself inherits from the `BaseCategoryType` class, with the `LOGGING` metadata type passed as a template parameter.

Furthermore, Thunder allows for the creation of customized categories within plugins by simply creating classes within the `Tracing` namespace that handle message formatting. An example of this can be found in `ThunderNanoServices/BluetoothControl/Tracing.h`.

Last but not least, the warning reporting categories in Thunder can be found in either `Source/WPEFramework/WarningReportingCategories.h` or `Source/core/WarningReportingCategories.h`. These categories, located within the `WPEFramework::WarningReporting` namespace, include:

- TooLongWaitingForLock
- SinkStillHasReference
- TooLongInvokeRPC
- JobTooLongToFinish
- JobTooLongWaitingInQueue
- TooLongDecrypt
- JobActiveForTooLong
- TooLongPluginState
- TooLongInvokeMessage

#### Creating a warning reporting category

Unlike the tracing and logging categories, these warning reporting categories are not declared using macros. Instead, they are directly declared within the designated header files mentioned above.

The code snippet below demonstrates the simplicity of creating a custom warning reporting category. If an additional `Analyze()` method is not necessary, it is sufficient to declare the `Serialize()` and `Deserialize()` methods to return `0`. However, attention must be given to the implementation of the `ToString()` method, along with the variables `DefaultWarningBound` and `DefaultReportBound`.

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

### From a macro to an output

Now, let us delve into how the MessageControl plugin manages the messages from logging, tracing, and warning reporting. To begin, let us revisit the macros discussed earlier. When handling tracing and logging messages, the first step involves checking if the corresponding category is enabled. Subsequently, both tracing and logging macros create an object of the `CATEGORY` class, which encapsulates the actual contents of the message provided as `PARAMETERS` within the macro. This enables the convenient storage and processing of the message content.

However, the approach differs for warning reporting messages. As we previously discussed, the content of a warning reporting message is derived from the `ToString()` method implemented in the respective warning reporting category class. Instead of creating an object for the message content, the MessageControl plugin retrieves the message content from the `ToString()` method, which returns the formatted string representation of the warning.

```c++
#define TRACE_GLOBAL(CATEGORY, PARAMETERS)
    do {
        using __control__ = TRACE_CONTROL(CATEGORY);
        if (__control__::IsEnabled() == true) {
            CATEGORY __data__ PARAMETERS;
            WPEFramework::Core::Messaging::MessageInfo __info__(
                __control__::Metadata(),
                WPEFramework::Core::Time::Now().Ticks()
            );
            WPEFramework::Core::Messaging::IStore::Tracing __trace__(
                __info__,
                __FILE__,
                __LINE__,
                __FUNCTION__
            );
            WPEFramework::Messaging::TextMessage __message__(__data__.Data());
            WPEFramework::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);
        }
    } while(false)
```

In tracing, you may recall that the module name indicates the originating plugin from which the traces originate. To obtain the module name for tracing purposes, the `TRACE_CONTROL` macro is invoked. This macro plays a crucial role in capturing the module name associated with the trace. However, in logging, the module name is simply set as `Syslog` without the need for a dedicated macro, and similarly, in warning reporting, the module name is set as `Reporting`. This provides a standardized module name for all logging and warning reporting messages.

```c++
const char* MODULE_LOGGING = _T("SysLog");
const char* MODULE_REPORTING = _T("Reporting");
```

Next, an object of `MessageInfo` class is created that stores the metadata and time of the message. This provides essential information for further handling and analysis of the message. Then, the last and the most complex part of the message is built, and it is slightly different for tracing, logging and warning reporting. File, line and a name of a class or a function for global trace version are passed as the members of the `Tracing` class. On the other hand, there are no additional data like this for logging, and for warning reporting there is only a callsign.

In addition to the part of the macros where the messages are formed, the crucial part to which we want to pay extra attention are these two lines:

```c++
WPEFramework::Messaging::TextMessage __message__(__data__.Data());
WPEFramework::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);
```

#### MessageUnit::Push()

This is how the communication between Thunder and the `MessageControl` plugin takes place. For tracing and logging it is within the macros, but for warning reporting it is in the separate method `ReportWarningEvent()` of the reporting proxy `WarningReportingUnit`.  The first step involves constructing the content of the message. Once the message content is prepared, it is then pushed to a buffer or special queue in the second line of code. This buffer serves as a centralized storage for messages, ensuring that they are properly organized and ready for further processing.

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

When the `MessageControl` plugin is used, each message is buffered and added to a queue. The ultimate destination of these messages depends on the specific configuration settings applied to the plugin. The code segment above is responsible for pushing messages of any type and their associated information to the buffer.

It is important to note that when the `MessageControl` plugin is not actively running or in scenarios where it is disabled, there is an option to directly print the message onto the console using the `DirectOutput()` method. This allows for immediate display of the message on the console without going through the buffering process.

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

Once the `MessageControl` plugin receives a notification through its interface after receiving a doorbell ring (after the `WaitForUpdated()` function),  it proceeds to retrieve the messages from the buffers by calling the `PopMessagesAndCall()` method from the `MessageClient` component. After gathering the messages, the `MessageControl` plugin proceeds to send them to their designated destinations, which is accomplished by invoking the `Message()` method provided by the plugin.

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

It is important to note that if the `MessageControl` plugin is disabled, the message queue will eventually reach its capacity. As a result, the framework will issue warnings indicating that there is no more space available in the queue. In this situation, older messages will be overwritten by newer ones as they continue to arrive. Within the `Message()` method, as shown below, the plugin is responsible for sending the message to all designated outputs selected in the plugin configuration.

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

#### Output configuration

Let us take a closer look at how outputs are configured and how this configuration can be modified. The configuration is generated as a JSON file, where specific JSON values correspond to the actual objects in the code. The association between an object and its corresponding JSON value is established within the constructor of the `Config` class in `ThunderNanoServicesRDK/MessageControl/MessageControl.h` utilizing the `Add()` method. This connection ensures that the configuration values in the JSON file are correctly linked to the corresponding objects in the code. To understand the configuration process in more detail, it is important to examine the `Initialize()` method in `MessageControl/MessageControl.cpp`.

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

Let us examine the `Announce()` method and then investigate the `ConsoleOutput` class. In the code listing above, we can notice that the method’s body is inside a lock, so that we can be sure that there will be no concurrency issues. The crucial part is the `_outputDirector` vector of pointers for the `Publishers::IPublish` objects. First, we make sure that an object passed as a method parameter is not already present in this vector, and then add it to the list. In the above list, the `Announce()` method takes a new object of the `Publishers::ConsoleOutput` class as a parameter.

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

The final step before the message is sent to the output involves invoking the `Convert()` method. As demonstrated in the above code listing, this method is responsible for constructing a string that combines the metadata, formatted according to the specific message type, with the actual text of the message. The resulting string represents a comprehensive representation of the message ready for output. For instance, this is how the `ToString()` method looks like for tracing type messages:

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
