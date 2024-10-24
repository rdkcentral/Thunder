# Out-Of-Process (OOP) Plugins

If a plugin is configured to run out-of-process, it will run in its own individual hosting process called `ThunderPlugin` instead of in the main Thunder process. Each out-of-process plugin will run in a separate ThunderPlugin instance.

When the plugin is activated, Thunder will automatically spawn a ThunderPlugin instance as a child process. The ThunderPlugin host will load the plugin library and establish a COM-RPC connection between itself and the main Thunder process. This COM-RPC connection is how clients communicating with the main Thunder process are still able to invoke methods on out-of-process plugins. The ThunderPlugin process will be stopped when the plugin is deactivated.

!!! tip
	For larger, more complex out-of-process plugins, it is often useful to split a plugin into two separate libraries - see [here](../split-implementation) for more details.

**Advantages**

* Reliability - if a plugin crashes, it will only bring down the ThunderPlugin instance and therefore not affect any other plugin. It can then be restarted as necessary
* Monitoring - by running a plugin in its own process, it becomes easier to monitor the resource usage (memory, CPU etc) of that plugin
* Security - out of process plugins can be run in a different user/group to the main Thunder process (which might be running as root) to reduce the privileges of the plugin and increase security
* Resource control - the size of the thread pool and process priority can be set for the ThunderPlugin host, allowing more custom tuning for specific plugin requirements

**Disadvantages**

* Performance - since there is now an additional RPC hop over COM-RPC to invoke methods on the plugin, this can introduce some latency (although COM-RPC is efficient so this is very minimal)
    * It may also take slightly longer to activate the plugin due to the overhead of starting the ThunderPlugin host
* Resource usage - potentially increased resource usage from spawning and running a new process