Interface Definition
====================

Table of Contents
=================

1. [Introduction](#Introduction)
2. [Rule, Recommendation or guideline](#RuleRecommendationOrGuideline)
3. [Rules](#Rules)
    1. [Interfaces should not leak implementation details](#InterfacesShouldNotLeakImplementationDetails)
    2. [Comply to the Single Responsibility Principle (SPR)](#ComplyToSingleResponsibilityPrinciple)
    3. [Interfaces published cannot change, only extend](#InterfacesPublishedCannotChangeOnlyExtend)
4. [Guidelines](#Guidlines)
    1. [Interface methods that are implementation invariant should be marked const](#InterfaceMethodsThatAreImplementationInvariantShouldBeMarkedConst)
    2. [Use nested INotification name for one-to-many dependency](#UseNestedInotificationNameForOnetomanyDependency)
    3. [Use nested ICallback name for one-to-one dependency](#UseNestedIcallbackNameForOnetooneDependency)
5. [Recommendations](#Recommendations)
    1. [State complete interfaces](#StateCompleteInterfaces)
    2. [Return "core::hresult” on methods](#ReturnCorehresultOnMethods)
    3. [Specifically define the length of the enum to be used](#SpecificallyDefineTheLenthgOfTheEnumToBeUsed)
    4. [Be verbose even if there is functionally no need](#BeVerboseEvenIfThereIsFunctionallyNoNeed)
6. [Mandatory keywords in an interface definition](#MandatoryKeywordsInAnInterfaceDefinition)
7. [Tags/annotations available for optimizing the generated code](#TagsAnnotationsAvailableForOptimizingTheGeneratedCode)
8. [Practical Considerations](#PracticalConsiderations)
    1. [Less is more]{#LessIsMore}
    2. [Frequency versus Quantity]{#FrequencyVersusQuantity}
    3. [Prefer multiple smaller interfaces over one big one]{#PreferMultipleInterfaces}

Introduction {#Introduction}
============

Interface-based development offers several advantages:

1. **Modularity**: By defining interfaces, different modules of a system can interact with each other without necessarily knowing about the implementation details. This promotes a modular design which makes the system easier to understand, maintain, and extend.
2. **Flexibility**: Interfaces allow for the decoupling of components within a system. This means that components can be swapped out or updated without affecting other parts of the system, as long as they adhere to the defined interface contract.
3. **Multiple Inheritance**: In languages that support multiple inheritance through interfaces, a class can implement more than one interface, allowing it to inherit behavior from multiple sources. This provides flexibility in defining the functionality of a class.
4. **Abstraction**: Interfaces allow developers to focus on what needs to be done rather than how it is done. By defining a clear contract in the form of an interface, developers can work independently on different parts of a system without worrying about the internal complexities of other components.
5. **Interoperability**: Interfaces facilitate interoperability between different components or even different systems. By adhering to a common interface, different parts of a system or different systems altogether can communicate effectively.
6. **Testing**: Interfaces help in unit testing by allowing developers to create mock objects that implement interfaces. This enables easier testing of individual components in isolation, leading to more robust and reliable code.

Overall, interface-based development promotes a modular, flexible, and maintainable codebase that is easier to work with and extend over time.

To exploit these advantages to the fullest, Thunder uses Interfaces definitions between plugins and internally. The interface definitions created for Thunder should adhere to certain rules/recommendations and guidelines. This document describes these Rules/Recommendations and guidelines for designing a good interface and the syntax to be used to describe the interface.

Rule, Recommendation or guideline {#RuleRecommendationGuideline}
=================================

Here are the distinctions between a rule, a recommendation, and a guideline:

1. **Rule**:
    * A rule is a prescribed directive or principle that must be followed strictly. It is typically mandatory and non-negotiable.
    * Rules often have legal or formal implications and violations may lead to consequences or penalties.
    * Rules are usually clear, specific, and binding, leaving little room for interpretation or deviation.
2. **Recommendation**:
    * A recommendation is a suggestion or advice on the best course of action, but it is not mandatory.
    * Recommendations are based on best practices, standards, or guidelines, but individuals or organizations have the freedom to choose whether or not to follow them.
    * While recommendations are meant to guide decision-making, they allow for flexibility and individual judgment in applying them.
3. **Guideline**:
    * A guideline is a set of general principles or rules that provide guidance or direction on how to achieve a particular objective.
    * Guidelines are more flexible than rules, as they offer suggestions on the most effective way to accomplish a task or goal.
    * They serve as a framework for good practice and can be adapted or customized based on specific circumstances or requirements.

In summary, rules are strict, mandatory, and non-negotiable directives, recommendations are suggestions or advice that are not obligatory, and guidelines offer flexible principles for achieving a specific objective.

Rules {#Rules}
=====

Interfaces should not leak implementation details {#InterfacesShouldNotLeakImplementationDetails}
-------------------------------------------------

Interfaces should be designed to provide a clear and concise abstraction of functionality without exposing the underlying implementation details. Here are some reasons why interfaces should not leak implementation details:

* **Encapsulation**: By hiding implementation details, interfaces ensure that the internal workings of a system or component are not exposed to the outside world. This promotes encapsulation, which is a core principle of object-oriented design.
* **Flexibility and Maintainability**: When interfaces do not expose implementation details, the underlying code can be changed, optimized, or refactored without affecting the code that depends on the interface. This makes the system more adaptable and easier to maintain.
* **Abstraction**: Interfaces should provide a high-level abstraction that defines what operations can be performed without detailing how they are performed. This allows developers to focus on the "what" rather than the "how," making the system easier to understand and use.
* **Reduced Coupling**: If implementation details are hidden, components that depend on the interface are less tightly coupled to specific implementations. This helps in reducing dependencies and makes it easier to swap out one implementation for another.
* **Security**: Hiding implementation details can also enhance security by preventing unauthorized access to sensitive information or potential manipulation of internal processes.
* **Interchangeability**: When interfaces do not leak implementation details, it becomes easier to interchange different implementations of the same interface. This is particularly useful in scenarios where multiple implementations might be needed, such as testing with mock objects or switching between different service providers.
* **Simpler API**: A clean interface that hides complexity makes the API simpler and more intuitive to use. Users of the interface do not need to understand the complexities behind the scenes, leading to a better developer experience.

To achieve these benefits, interfaces should be carefully designed to define clear and minimal contracts for interaction, focusing on what needs to be done rather than how it is done.

Comply to the Single Responsibility Principle (SPR) {#ComplyToSingleResponsibilityPrinciple}
---------------------------------------------------

The Single Responsibility Principle (SRP) is a fundamental principle in software development that states that an interface should have only one reason to change, which should also adhere to the idea of having a single, well-defined responsibility. Here are some reasons why interfaces should comply with the Single Responsibility Principle:

* **Clear and focused functionality**: Interfaces that adhere to the SRP are easier to understand and work with because they have a clear and focused purpose. Developers can quickly grasp what the interface is meant to do and how it should be implemented.
* **Encourages better design**: By adhering to the SRP, interfaces are more likely to be well-designed and follow best practices in software development. Separating different responsibilities into distinct interfaces helps to create a more modular and maintainable codebase.
* **Easier to maintain and extend**: When an interface has a single responsibility, changes to that responsibility are less likely to impact other parts of the codebase. This makes it easier to maintain and extend the code without causing unintended consequences or introducing bugs.
* **Promotes code reusability**: Interfaces that adhere to the SRP are more likely to be reusable in different contexts. Since each interface represents a single responsibility, it can be easily reused in various parts of the codebase without introducing unnecessary complexity.
* **Improved testability**: Interfaces with a single responsibility are often easier to test because their behavior is well-defined and focused. This makes it simpler to write unit tests for classes that implement the interface and ensures that each component of the software can be tested effectively in isolation.
* **Enhanced flexibility**: Interfaces that follow the SRP are more flexible and adaptable to changes in requirements. If a new responsibility needs to be added or an existing one modified, it can be done more easily without affecting other parts of the codebase.

In conclusion, interfaces that comply with the Single Responsibility Principle are easier to understand, maintain, and extend. By keeping interfaces focused on a single responsibility, developers can create more modular, reusable, and robust software systems.

Interfaces published cannot change, only extend {#InterfacesPublishedCannotChangeOnlyExtend}
-----------------------------------------------

When it comes to interfaces, it's important to extend them rather than change them directly to maintain compatibility with existing code. By extending an interface, you can add new functionality without breaking the code that already uses the interface. This allows for a more flexible and scalable design, as different classes can implement the new interface without affecting the existing codebase. Additionally, extending interfaces promotes the principle of "open-closed" in software design, where classes are open for extension but closed for modification. This helps in creating more maintainable and modular code.
 
Guidelines {#Guidelines}
==========

Interface methods that are implementation invariants should be marked const {#InterfaceMethodsThatAreImplementationInvariantShouldBeMarkedConst} 
---------------------------------------------------------------------------

If a method on an interface is expected to \*not\* functionally change anything in the implementation of the interface, mark the method as const. This allows for passing const interfaces in case the interface needs to be passed to users that should not "control" the implementation.

Use nested INotification name for one-to-many dependency {#UseNestedInotificationNameForOnetomanyDependency}
--------------------------------------------------------

The Observer pattern, is a behavioral design pattern where there is a one-to-many dependency between the parent interface and the INotification (nested child interface) so that when one parent interface implementation changes state, all its dependents INotification child interface implementations are notified and updated automatically.

In software development, this pattern is commonly used to establish communication between different parts of a system without them being directly coupled. The Publisher (or Subject) maintains a list of Subscribers (or Observers) interested in being notified of changes, and when an event occurs or state changes, the Publisher notifies all Subscribers by invoking specific methods on them. This allows for a loosely coupled architecture where changes in one component trigger updates in other components without them having explicit knowledge of each other.

As there is a one-to-many dependency the parent interface requires methods to add and remove the INotification. It is a guideline to call these ```core::hresult Register(INotifcation*)``` and ```core::hresult Unregister(const INotification*);```

``` c++
// @json 1.0.0
struct EXTERNAL IObject : virtual public Core::IUnknown {
    enum { ID = ID_OBJECT };

    // @event
    struct EXTERNAL INotification : virtual public Core::IUnknown {
        enum { ID = ID_OBJECT_NOTIFICATION };
        /* @brief Object visibility changes */
        /* @param hidden: Denotes if application is currently hidden */
        virtual void Changed(const bool hidden) = 0;
        };

    virtual core::hresult Register(INotification* sink) = 0;
    virtual core::hresult Unregister(const INotification* sink) = 0;
};
```

Use nested ICallback name for one-to-one dependency {#UseNestedIcallbackNameForOnetooneDependency}
---------------------------------------------------

The callback pattern in software development is a programming pattern that allows a function to call another function, typically provided as an argument, at a specified point during its execution. It is different from [Use nested INotification name for one-to-many dependency](#UseNestedInotificationNameForOnetomanyDependency) with respect to cardinality. The callback is for a one-to-one dependency.

In this pattern, a callback function is passed as an argument to a higher-order function, which then invokes the callback function at the appropriate time. Callbacks are commonly used in asynchronous programming to handle responses to events that may not occur immediately. They enable non-blocking behavior by allowing a function to continue executing while waiting for a response, and then invoke the callback function with the result when it becomes available.

Callbacks are widely used in event-driven architectures, such as in web development for handling user interactions, network requests, and other asynchronous tasks. They provide a flexible way to customize and extend the behavior of functions and enable efficient handling of asynchronous operations in software systems.

``` c++
// @json 1.0.0
struct EXTERNAL IObject : virtual public Core::IUnknown {
    enum { ID = ID_OBJECT };
    // @event
    struct EXTERNAL ICallback : virtual public Core::IUnknown {
        enum { ID = ID_OBJECT_CALLBACK };
        /* @brief Called if the job has done its work */
        virtual void Completed() = 0;
    };
    Virtual core::hresult Process(ICallback* sink) = 0;
    Virtual core::hresult Abort() = 0;
};
```

Recommendations {#Recommendations} 
===============

State complete interfaces {#StatetCompleteInterfaces}
-------------------------
If a method on the interface can change an implementation into a different state, make sure the interface also has methods to revert the entered state. A good example is an interface that can ```core::hresult Start() = 0``` the executions of a job. As the execution of the job has an expected lifespan which is greater that the calls lifespan, make sure there is a method on the interface as well to bring back the interface to the initial, not started state. E.g. an ```core::hresult::Abort() = 0``` method. 

Return ```core::hresult``` on methods {#ReturnCorehresultOnMethods}
---------------------------------
As most interfaces are designed to work over process boundaries, it is recommended to cater for issues during the passing of the process boundary, e.g. the other process might have crashed. This means that method calls on interfaces that seem triial but do change the state of an implementation, like ```SetTime()```, may fail due to cross process communication. If the return value of a method is defined as core::hresult, the Thunder COMRPC framework will signal issues with the communication by returning a negative value. The whole positive range from 0-2147483647 (0-0x7FFFFFFF) is free for the implementer to return values.

Specifically define the length of the enum to be used {#SpecificallyDefineTheLenthgOfTheEnumToBeUsed}
-----------------------------------------------------
To allow for the code generator to optimize for the communication frame, it is recommended to always define the length of an enum if it is used as a parameter. If the enum length is not defined, it is assumed to be 32 bits. If an enum only has less than 256 values, it means 3 bytes are wasted on the line.

Be verbose even if there is functionally no need {#BeVerboseEvenIfThereIsFunctionallyNoNeed}
------------------------------------------------

Describe/define the interface as verbose as possible. This is the contract between two parties, it is better to clearly state what the intention of a parameter is than to leave the intent floating. This means that if a parameter is expected ***not*** to be changed by the implementer define it as const, even if from a C/C++ language point of view it does not add value.

``` c++
core::hresult Logging(const string& cat, bool& enabled /* @out */) const = 0;
core::hresult Logging(const string& cat, const bool enabled) = 0;
```

Although the const bool in the second method has no added value, it is a recommendation to use it, just to indicate clearly that the intend is that the implementation should not and can not change the parameter.

!!! note "TODO"
    Do we want to promote the use of properties in JSONRPC, requirements for the interface specifications.

 Mandatory keywords in an interface definition {#MandatoryKeywordsInAnInterfaceDefinition}
==============================================

The following keywords are mandatory in a COMRPC interface definition, which is an interface that can be used over process boundaries. The interface definition complies with the C/C++ standard rules for header file definitions and thus these interfaces can (and will) be used directly in C/C++ code.

``` c++
struct EXTERNAL IObject : public virtual Core::IUnknown {
    enum { ID = ID_OBJECT };
};
```

A ***struct*** in the ***C programming language*** (and many derivatives) is a ***composite data type*** (or ***record***) declaration that defines a physically grouped list of variables under one name in a block of memory, allowing the different variables to be accessed via a single ***pointer*** or by the struct declared name which returns the same address. The struct data type can contain other data types so is used for mixed-data-type records such as a hard-drive directory entry (file length, name, extension, physical address, etc.), or other mixed-type records (name, address, telephone, balance, etc.).

The ```EXTERNAL``` keyword is used for different platforms to define the link type to be used when trying to access this interface. This typically refers to the default constructor/destructors if generated for a struct.

The ```Core::IUnknown``` must always be the base of the interface to allow for interface navigation (```QueryInterface```) and life-time management of the interface (reference counting). As typical implementations of an interface might implement multiple interface by one implementation it is mandatory to all route back to a single lifetime management/Navigational base class (diamond structure) of the core::IUnknown implementation. This is realized by making sure that all interface inherit ```virtual``` from the ```Core::IUnknown```. 

As small-as-it-gets is what Thunder stands for, so identifying an interface by a name (or using a GUID, as Microsoft did) is a bit of overkill. Thunder uses an ID which should be unique. The ID to be assigned to the interface is mandatory and it is defines as ```enum { ID = <NUMBER> };``` the number should be unique and thus all ID’s should be registered in ```ids.h``` to guarantee uniqueness.

The ID’s are split up in ranges. 

|begin|end|Owner|ID storage|
|--|-|-|-|
|0x00000000|0x00000080|Thunder internal|```Thunder/Source/plugin/ids.h```|
|0x00000080|0x80000000|Public Plugin interfaces|```ThunderInterfaces/interfaces/ids.h```|
|0x80000000|0xA0000000|Custom Plugin interfaces|```<Custom>```|
|0xA0000000|0xFFFFFFFF|QA Interfaces|```ThunderInterfaces/interfaces/ids.h```|

Tags/annotations available for optimizing the generated code {#TagsAnnotationsAvailableForOptimizingTheGeneratedCode}
============================================================
 
Tags/annotations that can be used to optimize the generated code, influence the generated JSON-RPC interface from the interface (if desired at all), or document the interface can be found here:
 
See: [https://rdkcentral.github.io/Thunder/plugin/interfaces/tags](https://rdkcentral.github.io/Thunder/plugin/interfaces/tags/)

Practical Considerations {#PracticalConsiderations}
========================

Less is more {#LessIsMore}
------------

When designing an interface spend some time thinking on how the interface can be as easy to use, understand and be consistent without the need of adding a lot of documentation and the need of explaining all the cornercases that can happen when using the interface in a certain way.

As an interface is in principle immutable it is better to spend more time on this before releasing the interface than later on trying to improve the interface or writing a lot of documentation explaining on how to use it in certain situations. 

When designing the interface it is good to use the "less it more" principle when adding methods to it to make sure it can do what should be possible with it. 
Less methods will make it easier to understand the interface, less documentation will be needed, it is easier to keep it consistent and there is less need to think or document what will happen if you use the methods in an unexpected order. 

Let's design an Player Interface as an example. At first one would expect all the following should be possible for a player:
* Start playback
* Stop Playback
* Pause Playback
* Rewind (with a certain speed perhaps)
* Forward (with a certain speed perhaps)

So a naive interface could look like this:

``` c++
struct EXTERNAL IPlayer : public virtual Core::IUnknown {
    enum { ID = ID_PLAYER };

    core::hresult Play() const = 0;
    core::hresult Stop() = 0;
    core::hresult Pause() const = 0;
    core::hresult Rewind(const uint32_t speed) = 0;
    core::hresult Forward(const uint32_t speed) = 0;
};
```
At first this might seem logical but how would you expect the implementation to respond, if you would do Pause(), before Play()? Or Play() after Rewind()? You would need to think about all the different options and specify how the interface should respond in that situation. 
But if you think of it what all the different methods do is specify a certain playback speed. So if you have one method with which you could set the player speed (and allow a negative number) it could do all the above while at the same time be consistent, easy to understand and no need to explain what will happen in certain situation as just the last set speed is the one that is current.
So this is how this was specified in the ISteamer interface [here](https://github.com/rdkcentral/ThunderInterfaces/blob/a229ea291aa52dc99e9c27839938f4f2af4a6190/interfaces/IStream.h#L105C32-L105C53)


Frequency versus Quantity {#FrequencyVersusQuantity}
------------------------

When designing an interface in an embedded environment where memory, processing power and bandwith might be limited it is also advisable to think about the frequency and quantity of the data exchange. Even if bandwith itself is not an issue, sending big chuncks of data, or small ones with a high frequency will lead to an increase in resource usage (memory and CPU) for handling this. 

So when thinking of the data passed to/from a component via a method on the interface take into consideration if all the data is really needed for handling that call or notification. On the other hand sending very small chunks per call while it is expected up front that multiple calls will be needed to combine enough data that is only useful for processing is also not very efficient (certainly for an environment where latency is high). So don't make the amount of data per call an afterthought but pass enough data that can be processed in one call but not more.

As an example when designing an interface to send stored EPG data to a client it probably does not make much sense of sending a month of EPG data for all channels in one call. Probably the client will only display a certain number of channels and a week of data so it would make sense to only send this amount. It also would not make sense to sent it per channel per hour only as that would require multiple calls to combine all information the client is interested in (and might cause additional consistency issues as the data is not retrieved over multiple calls and could change in the mean time).

A technique that can be considered in these kind of situations are iterators that allow retrieval of only part of the data per call, certainly if the whole dataset needs to be kept consistent (but then might come at the cost of keeping a (partial) copy of the data).

Also when sending notifications on certain events keep in mind how often these happen and if the client is always interested in receiving all of these (also see the "Static vs Dynamic" section above). 
If these events happen a lot the component might become very chatty and it might be a better idea think of solutions on how to reduce the number of events sent (e.g. reduce the number of possible events, make it possible to subscribe to subevents etc.).

Prefer multiple smaller interfaces over one big one {#PreferMultipleInterfaces}
---------------------------------------------------

COM-RPC makes it rather easy to have one component implement multiple interfaces instead of just one and have the user of the interface query if a certain interface is implemented by a component, see the IUnknown QueryInterface method on how to do that.

This feature of come can be used in multiple ways:
* When designing an interface only have one responsibility per interface. This makes it easier to understand and maintain the interface but now also gives component the opportunity to only implement one responsibility and not the other if it does not make sense. The client can using QueryInterface check if also the extended functionality is available. For example a Player interface where you could also change the Volume you would create an IPlayer and an IVolume instead of only an IPlayer. This will also make reuse of an interface easier as it can be used for any component implementing that responsibility , no reason to say it does not fit the component as only a part of the interface applies to what it is trying to achieve.
* When an interface needs to be extended instead of adding new methods to the existing one one could add the extended functionality to a new interface. Now the client can also work with components that do not yet implement the new extended functionality (as it can find out with QueryInterface if it is available) while on the other hand not forcing an update of all the components that do implement the initial interface, you only update the one(s) for which the extended new functionality makes sense.
* When implementing an interface in a plugin meant to be used out of process it can be used to distinguish between the public part (what really is the interface to be used by clients of the interface) and the private part (what is needed to facilitate the exchange between the in process and out of process part of the plugin). Most probably this would already be in contradiction with the first bullet point above but an example of this would be the Configure method to forward the configuration from the in process part of the plugin to the out of process part. It does not make sense to add a Configuration method to your public interface for this, it does not belong to this and of course it is far from ideal that you just document "do not call this from the outside, it is internal only. It would be better to have the Configuration call in its own interface and have the out of process part of the plugin implement both the public interface as well as The IConfiguration interface. The in process part can then via QueryInterface get access to IConfiguration and call the Configuration method. The in process part would then of course not expose the IConfiguration via the interface map making it impossible to be called externally. As this is of course a real wold use case the IConfiguration interface is already available [here](https://github.com/rdkcentral/ThunderInterfaces/blob/a229ea291aa52dc99e9c27839938f4f2af4a6190/interfaces/IConfiguration.h#L26).
























