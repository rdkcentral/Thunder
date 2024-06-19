The architecture of Thunder is designed to provide a modular, scalable, and efficient framework for building and managing software services and applications. At its core, Thunder employs a plugin-based architecture, where each functionality is encapsulated within a plugin that can be dynamically loaded, activated, and managed at runtime. This modularity ensures ease of maintenance, flexibility, and scalability, allowing developers to extend and customize the framework with minimal impact on existing components.

Thunder leverages robust communication mechanisms, including JSON-RPC and COM-RPC protocols, to facilitate seamless interaction between plugins and external clients. The framework also emphasizes secure service management, with mechanisms for authentication, authorization, and secure communication.

!!! note
	Overall, Thunder's architecture is geared towards providing a high-performance, reliable platform that can adapt to various use cases and deployment environments.



## Resource-Constrained Focus

Thunder is meticulously designed with resource constraints in mind, making it exceptionally suitable for embedded systems and environments where memory and processing power are limited. The architecture prioritizes efficiency and optimization to ensure that the framework operates smoothly even under severe resource limitations.

This focus on resource constraints drives several key design decisions, including the use of lightweight communication protocols, modular plugin systems, and efficient resource management strategies.

### Execution Architecture

The execution architecture of Thunder is a critical aspect that ensures optimal performance and resource utilization. Execution architecture refers to the structural organization of computational elements and their interactions within a system. In the context of Thunder, this architecture is geared towards maximizing efficiency and maintaining stability in resource-constrained environments.

!!! note
	A core component of Thunder’s execution architecture is the `ResourceMonitor`, and a more detailed documentation about it can be found [here](https://rdkcentral.github.io/Thunder/utils/sockets/#resource-monitor).

In the nutshell, the `ResourceMonitor` operates as a separate thread dedicated to the aggregation, allocation, and management of resources. It plays a pivotal role in monitoring system resources, ensuring that each component receives the necessary resources while preventing over-allocation that could lead to system instability. Providing a centralized management system helps in identifying potential issues early and taking corrective actions before they escalate. The `ResourceMonitor`'s responsibilities include:

- **Resource Aggregation**: Collecting data on resource usage from various components and plugins.
- **Resource Allocation**: Dynamically allocating resources to plugins and services based on current demands and availability.
- **Communication Handling**: Managing the sending and receiving of data between components, optimizing communication paths to reduce latency and overhead.

This resource-centric approach is complemented by Thunder’s use of efficient communication protocols, such as COM-RPC, which minimizes overhead and maximizes throughput. By employing COM-RPC within the execution architecture, Thunder ensures that inter-component communication is both fast and resource-efficient, which is crucial for maintaining performance in embedded systems.

### Benefits of Execution Architecture in Thunder

- **Efficiency**: The architecture is optimized for minimal resource usage, ensuring that the framework can run on devices with limited processing power and memory.
- **Stability**: By isolating components and managing resources dynamically, Thunder can prevent the failure of one component from affecting the entire system.
- **Scalability**: The modular design allows for easy addition and management of plugins without significant reconfiguration, making it scalable for various applications.

To sum up, Thunder’s focus on resource constraints and its robust execution architecture make it a powerful framework for embedded systems. The role  of `ResourceMonitor` in dynamically managing resources and optimizing communication ensures that Thunder remains lean, mean, and highly efficient, catering to the needs of modern embedded devices.



## Interface-Based Development

Interface-based development is a software design principle that emphasizes defining clear, stable interfaces between components. This approach decouples the implementation details from the interface, allowing for greater flexibility, maintainability, and scalability.

!!! warning
	Interfaces serve as contracts that define the methods and properties a component must implement, enabling different components to interact seamlessly without being tightly coupled to each other's implementations.

### Main Benefits

1. **Modularity:** Interfaces allow different parts of the system to be developed, tested, and maintained independently. This modularity simplifies updates and improvements without disrupting the entire system.
2. **Interchangeability:** Implementations can be changed with minimal changes to the system. This is particularly useful for testing and upgrading components.
3. **Scalability:** As the system grows, new functionalities can be added through new interfaces, ensuring that existing components remain unaffected.
4. **Maintainability:** Clear interfaces make the codebase easier to understand and maintain, as they provide a clear contract for what a component should do.

### Implementation in Thunder

In Thunder, interfaces are typically named with a capital `I` prefix, and the actual classes implementing these interfaces follow a consistent naming convention. The `IPlugin` interface can be a good example of that principle. This interface defines methods that any plugin must implement. Below you can see only a part of this interface and its methods with a detailed explanation. This code can be found [here](https://github.com/rdkcentral/Thunder/blob/e1b416eee5b057a8ffe5acfd68dfda04ad4f30eb/Source/plugins/IPlugin.h) in the Thunder repository.

```c++
struct EXTERNAL IPlugin : public virtual Core::IUnknown {

    enum { ID = RPC::ID_PLUGIN };

    struct INotification : virtual public Core::IUnknown {

        enum { ID = RPC::ID_PLUGIN_NOTIFICATION };

        ~INotification() override = default;

        //! @{
        //! ================================== CALLED ON THREADPOOL THREAD =====================================
        //! Whenever a plugin changes state, this is reported to an observer so proper actions could be taken
        //! on this state change.
        //! @}
        virtual void Activated(const string& callsign, IShell* plugin) = 0;
        virtual void Deactivated(const string& callsign, IShell* plugin) = 0;
        virtual void Unavailable(const string& callsign, IShell* plugin) = 0;
    };

...

    //! @{
    //! ==================================== CALLED ON THREADPOOL THREAD ======================================
    //! First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
    //! information and services for this particular plugin. The Service object contains configuration information that
    //! can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
    //! If there is an error, return a string describing the issue why the initialisation failed.
    //! The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
    //! The lifetime of the Service object is guaranteed till the deinitialize method is called.
    //! @}
    virtual const string Initialize(PluginHost::IShell* shell) = 0;

    //! @{
    //! ==================================== CALLED ON THREADPOOL THREAD ======================================
    //! The plugin is unloaded from framework. This is call allows the module to notify clients
    //! or to persist information if needed. After this call the plugin will unlink from the service path
    //! and be deactivated. The Service object is the same as passed in during the Initialize.
    //! After theis call, the lifetime of the Service object ends.
    //! @}
    virtual void Deinitialize(PluginHost::IShell* shell) = 0;

    //! @{
    //! ==================================== CALLED ON THREADPOOL THREAD ======================================
    //! Returns an interface to a JSON struct that can be used to return specific metadata information with respect
    //! to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
    //! @}
    virtual string Information() const = 0;
};

...
```

!!! note
	A class implementing this interface, for example, `MyPlugin`, must define all methods declared in `IPlugin`, which allows Thunder to manage plugins uniformly, relying on the interface rather than the specific implementation.

In the end, by adhering to an interface-based architecture, Thunder ensures that its components remain decoupled and maintainable. This approach not only benefits the framework’s development but also encourages developers using Thunder to adopt best practices in their own projects. The clear separation of interface and implementation simplifies upgrades, enhances flexibility, and makes the system easier to understand and extend.

Overall, interface-based development is a cornerstone of Thunder’s design philosophy, promoting a clean, modular, and scalable architecture. This ensures that Thunder remains robust and adaptable, capable of evolving alongside the ever-changing demands of embedded systems and the wider software development landscape.



## Abstraction and Portability

In the development journey, it's crucial for plugin developers to understand the breadth and depth of functionalities already integrated into the framework. While it may be tempting to implement custom features targeting specific operating systems, it's probably almost always more efficient to leverage the extensive capabilities already provided by Thunder. This not only aligns with the principles of portability and scalability but also ensures optimized performance of the plugin.

One of the core principles guiding the design of Thunder is portability. The framework is engineered to be platform-agnostic, allowing it to seamlessly run across various operating systems and hardware environments. Plugin developers are encouraged to embrace this portability by utilizing Thunder's abstraction layers for interacting with the underlying OS functionalities.

!!! note
	By leveraging Thunder's abstraction, developers can write platform-independent code that remains consistent across different environments, eliminating the need to manage OS-specific implementations and dependencies.

Thunder can be deployed on numerous platforms, including Linux, Windows, and macOS, with potential support for additional platforms in the future. The key benefit of using the features implemented within the framework rather than developing them locally within your plugin is that you don't have to worry about portability — the framework handles that for you. This is particularly advantageous as it allows developers to focus on the functionality of their plugins rather than the intricacies of different operating systems.

Moreover, if there are changes in the specifications or functionalities of any operating system, maintaining the system becomes much simpler. Instead of modifying each component (plugin) separately, changes need to be made only within the framework. This centralized approach significantly reduces the maintenance burden and ensures consistency across all plugins.

!!! warning
	As a general rule of thumb, in order to avoid reinventing the wheel, always do the research to make sure that the feature you want to implement is not already available in Thunder.

### Scalability and Efficient Resource Management

Thunder is designed to support the concurrent execution of multiple plugins, each with its own set of functionalities and resources. Utilizing Thunder's built-in capabilities ensures that plugins can seamlessly coexist and interact within the framework, without introducing unnecessary overhead or resource contention. This optimized approach not only improves the overall responsiveness of the system but also simplifies the management and deployment of plugins in large-scale environments.

A prime example of this is the usage of Thunder's `WorkerPool` to handle specific jobs on separate threads instead of creating threads within individual plugins. More detail information about this feature can be found [here](https://rdkcentral.github.io/Thunder/utils/threading/worker-pool/). In the nutshell, the main advantage of this approach is that there are a few threads that wait for a job, execute it, and then return to the waiting state for the next job. This eliminates the overhead associated with creating and destroying threads each time a task needs to be performed.

!!! note
	This is particularly beneficial both for smaller tasks where the time taken to create and destroy a thread could exceed the time required to execute the task itself, as well as for avoiding a scenario when each plugin spawns multiple threads which could lead to serious stability issues.

Simply put, thanks to the principles of abstraction and portability, Thunder ensures that plugins are robust, maintainable, and scalable. Developers are encouraged to utilize the framework's extensive capabilities, which not only simplifies development but also enhances the overall efficiency and performance of the system.



## Isolation: Out-of-process Plugins

Besides the fact that in-process plugins in Thunder offer significant advantages in terms of communication efficiency and simplicity, they also consume less memory due to the absence of extra processes. These plugins run within the same memory space as the core framework, allowing for direct function calls and reducing the overhead associated with inter-process communication.

However, despite these benefits, the use of out-of-process plugins remains crucial for maintaining the overall stability and resilience of the system.

!!! note
	By isolating each plugin in its own process, the framework ensures that any failure or crash within a plugin does not impact the core framework or other plugins.

This isolation is vital in preventing a single point of failure from cascading through the system, thereby enhancing the robustness and reliability of the entire environment.

### Ensuring Stability with Out-of-process Plugins

Out-of-process plugins provide an extra layer of security by running in a separate memory space. This separation means that if a plugin encounters a fatal error, the failure is contained within that process, allowing the core system to continue running uninterrupted.

!!! warning
	This architectural decision is especially important in complex systems with multiple plugins, as it prevents a single faulty plugin from compromising the entire system.

### Strategic Use of In-process Plugins

While the primary advantage of out-of-process plugins is their isolation, there are scenarios where in-process plugins are preferable due to their efficiency. An example within Thunder is the `MessageControl` plugin, which aggregates and handles all communication within the framework. Running such a plugin in-process minimizes latency and maximizes performance, which is crucial for high-throughput communication tasks.

However, incorporating in-process plugins requires meticulous attention to detail to ensure they are robust and free of bugs. This involves rigorous testing, code reviews, and adherence to best practices in software development to minimize the risk of crashes. The benefits of in-process plugins, such as reduced memory footprint and faster communication, justify this additional effort for components that are central to the framework’s operation.

### Balancing Efficiency and Stability

The architecture of Thunder exemplifies a balanced approach to plugin management, leveraging both in-process and out-of-process plugins as needed. 

!!! note
	In-process plugins are used sensibly for tasks that benefit from close integration and high efficiency, while out-of-process plugins are employed to safeguard the system’s stability. This strategic use of different plugin architectures ensures that Thunder remains both performant and resilient.

In conclusion, Thunder’s hybrid approach to plugin architecture, combining the strengths of both in-process and out-of-process plugins, ensures that the system remains efficient, stable, and flexible. This thoughtful design underscores Thunder's commitment to delivering a robust platform capable of handling the complexities and demands of modern embedded systems. By isolating potentially unstable components while optimizing critical ones, Thunder achieves a balance that maximizes performance without compromising reliability.



## Minimizing Layers

In any embedded systems, it's critical to minimize the addition of extra layers to maintain optimal performance and efficiency. Adding unnecessary layers can significantly impact both processing speed and resource utilization. For instance, when a plugin runs within the same process as Thunder, all interactions are handled through local virtual function calls, which are highly efficient. This approach avoids the overhead associated with Remote Procedure Calls (RPC), such as JSON-RPC, which introduce additional latency and processing requirements.

However, when plugins operate out-of-process, using proxies and inter-process communication becomes unavoidable. In these scenarios, choosing the most efficient communication method is paramount. COM-RPC, for example, offers lower latency and overhead compared to JSON-RPC, making it a preferable choice.

!!! danger
	While both in-process and out-of-process plugins have their own set of benefits and trade-offs, it is universally true that introducing unnecessary communication layers is going to degrade performance.

Therefore, developers should strive to use the most direct and efficient communication methods available without introducing additional unnecessary steps.

### Main Advantages

* **Reduced latency** - minimizing additional communication layers can significantly reduce latency in the system. Each additional layer introduces a delay, which can accumulate and become noticeable, especially in real-time applications. In Thunder, using direct virtual function calls within the same process ensures that interactions are swift and efficient, crucial for maintaining responsiveness.

* **Lower resource utilization** - extra layers often require additional resources to manage and process communications. By keeping the communication path as direct as possible, systems can avoid unnecessary consumption of CPU cycles and memory. This is particularly important in embedded systems where resources are limited. Thunder's architecture prioritizes this efficiency, ensuring that plugins can interact directly with the framework whenever feasible.

* **Simplified debugging and maintenance** - fewer layers mean simpler system architecture, making it easier to debug and maintain. When issues arise, having a straightforward communication path allows developers to trace problems more efficiently. In Thunder, minimizing layers means fewer points of potential failure, simplifying the process of identifying and resolving issues.

* **Enhanced security** - each layer added to the communication path can potentially introduce new security vulnerabilities. By minimizing these layers, the attack surface is reduced, enhancing the overall security of the system. Thunder's design philosophy embraces this principle, ensuring that communication paths are as direct and secure as possible.

In summary, minimizing unnecessary communication layers is a fundamental principle in Thunder’s architecture, critical for maintaining optimal performance, resource efficiency, and system simplicity. By leveraging Thunder’s direct function calls for in-process plugins and efficient RPC methods for out-of-process scenarios, developers can ensure that their applications are both robust and performant.



## Different RPC Protocols

Thunder offers two primary communication protocols: *JSON-RPC* and *COM-RPC*, each catering to different needs and use cases.

### JSON-RPC

On one hand we have JSON-RPC, which is a lightweight, remote procedure call protocol encoded in JSON. It is particularly useful for external communication with Thunder, as it provides a simple and easy-to-use interface for developers. JSON-RPC is ideal for scenarios where interoperability and ease of integration are paramount, enabling seamless communication with a wide range of clients and services. The use of JSON makes it human-readable and straightforward to debug, making it an excellent choice for development and testing environments.

In JSON-RPC, communication typically occurs over HTTP or another transport protocol. The client sends a request in the form of a JSON object to the server, specifying the method to be called and any parameters required for the method. The server processes the request, executes the specified method, and returns a JSON response containing the result or an error message if applicable.

In the context of Thunder, JSON-RPC serves as a fundamental communication protocol between the plugins developed by users and the outside world. Plugins can expose methods and functionalities through JSON-RPC interfaces, allowing to invoke these methods as needed.

### COM-RPC

On the other hand, COM-RPC is the cornerstone of Thunder's communication strategy, offering superior performance and efficiency, which are crucial for embedded devices where resources are limited.

!!! note
	COM-RPC is designed to be much faster and more efficient than JSON-RPC, as it involves less overhead and is optimized for high-speed, low-latency communication.

This efficiency is vital for embedded systems, where memory usage and processing power must be carefully managed. The lean and mean nature of COM-RPC allows Thunder to maintain high performance while running on devices with constrained resources. By leveraging COM-RPC, Thunder can ensure that its services are not only reliable but also capable of meeting the strict demands of embedded environments, making it a robust framework for a wide range of applications.

Despite the clear advantages of COM-RPC in terms of efficiency and performance, many developers working with Thunder still prefer using JSON-RPC due to its simplicity and ease of use. JSON-RPC's straightforward, human-readable format makes it an attractive choice for quick integrations and debugging, especially during the initial stages of development or when interoperability with various clients is required.

However, it's important to emphasize that for production environments, particularly in embedded systems where resources are limited, the performance benefits of COM-RPC cannot be overlooked. COM-RPC's optimized, low-overhead communication significantly enhances the framework's efficiency, leading to lower memory usage and faster response times.

!!! warning
	Therefore, we strongly urge developers to invest the effort in utilizing COM-RPC wherever feasible.

By doing so, they can achieve a much more optimized and performant system, ensuring that Thunder runs at its full potential even on resource-constrained devices. Adopting COM-RPC will ultimately lead to a more robust and scalable application, fully leveraging Thunder's architectural strengths.



### Conclusion

Thunder's architecture is designed with a strong emphasis on efficiency, stability, and modularity. By leveraging principles like interfaced-based development, efficient communication methods, and minimizing unnecessary layers, Thunder ensures a robust and performant framework for managing plugins in embedded systems. Developers are encouraged to fully utilize Thunder's built-in capabilities to create scalable, maintainable, and high-performance applications.
