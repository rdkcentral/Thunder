# Container

!!! note
	Container support is not enabled by default.
	
	To enable it, build Thunder with the `-DPROCESSCONTAINERS=ON` CMake option and specify the desired backend using the `-DPROCESSCONTAINERS_XXX=ON` where `XXX` corresponds to the backend you wish to use.

In container mode which is an extension to the out-of-process mode, ThunderPlugin host will run within a containerised environment.  

Thunder supports multiple container integrations through the `ProcessContainer` abstraction mechanism in Thunder core.

If you have only one container integration, that will be the default one. However, if you have enabled multiple container integrations, you must specify the one you want to use.

There are two ways to do this:

- Specify it in the plugin configuration:

	This method allows you to specify which integration to use for the plugin.
```
"configuration":{
   "root":{
      "mode":"Container",
      "configuration" : {
         "containertype":"lxc"
      }
   }
}
```

- Specify the default integration in the Thunder configuration.
	
	This method allows you to set a default integration for plugins that do not have a specified container type in their configuration; they will use this default integration.
```
"processcontainers" : {
    "default" : "runc"
}
```

## Supported Containers

- [LXC ](https://linuxcontainers.org/)
- [runc](https://github.com/opencontainers/runc)
- [crun](https://github.com/containers/crun)
- [Dobby](https://github.com/rdkcentral/Dobby) (Maintained by RDK)
- AWC (Maintained Externally)

To run a plugin in a container, a suitable container configuration must already exist. Thunder does not create container configurations dynamically. Thunder will search for existing container configurations in the following locations, in order of priority:

1. `<volatile path>/<callsign>/Container`
1. `<persistent path>/<callsign>/Container`
1. `<data path>/<classname>/Container`

For instance, the configuratoin for a plugin with the callsign SamplePlugin might be stored in `/opt/thunder/SamplePlugin/Container` if the persistent path is set to `/opt/thunder` in Thunder configuration.

**Advantages**

* Retains all the benefits of OOP plugins.
* Enhanced security - Containers have restricted access to the host system including filesystem and device access.
* Improved resource management - Containers can use cgroups to tightly control and monitor resource usage.

**Disadvantages**

* Maintenance - Requires managing container configurations and appropriately configuring access to resources. (e.g. device nodes)
* Startup time - Plugin activation may take longer due to the additional overhead of setting up the container environment.