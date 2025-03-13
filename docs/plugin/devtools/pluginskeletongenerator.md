The PluginSkeletonGenerator (PSG) is a tool that can be used to generate a skeleton for a new plugin, giving you a quick start when developing a new plugin but also preventing common mistakes to be made in the generic plugin code.

Please note that the tool is in beta release at the moment. Although it was very carefully designed and reviewed of course it cannot be ruled out that there are any issues with it, certainly seen the number of different configurations it can generate. If you find any issues with the generated code please contact the Thunder team.

Some remarks:

* Although it already supports quite a number of different plugin scenario’s/configurations it will not support everything you can think of in a plugin, that it will never do. So if you cannot do it with the PSG does not mean Thunder cannot do it, you just probably need to add/change code by hand. On the other hand the PSG will be extended and will support more common plugin options in the future.
* At the moment the PSG will generate Thunder 5 compliant code.
* Start the PSG in the folder where you want a subfolder to be created for your new plugin (so do not start it from the PSG folder itself).
* The PSG does already support a plugin implementing more than one interface.
* At the moment the PSG does not yet parse the header file for the interface(s) you want your plugin to implement, this is planned for a future release. So it will not generate the correct methods etc, but for now only some example methods and you will have to update manually to reflect your interface(s).
* The PSG will generate a default text for the License in all the code files with a placeholder for the organization. Please update this after generating the code.

How to use the Plugin Skeleton Generator:

* Start the PSG from the folder in which you want your plugin to be generated (it will be created in its own subfolder of course) or start it from the ThuderDevTools menu.
* You first will be asked how you want to name your plugin (this will be used for the subfolder, classnames, callsign etc.).
* Then you will be asked if the plugin needs to be able to run OOP, select the desired option.
* Following it will ask where the interfaces your plugin will implement are located (default is ThunderInterfaces/interfaces so in that case you just can press ENTER)
* After that it will in a loop ask for the Interfaces the plugin needs to implement (as mentioned it does support multiple interfaces).
* For an interface first it will ask for the header file of the interface the plugin will implement. This is on purpose because if we in a future version of the PSG actually scan the file we do no longer have to ask a number of the question that will follow and also the generated code will match the interface better (as we then know exactly what methods the interface expects to be overridden. But at the moment it will not parse the header file yet).
* Then it will guess the name of the Interface the header file provides (at the moment only one interface per header file supported) but you can override it if it is different.
* Then it will ask if the interface does also have notifications (note not only relevant for JSONRPC it will also generate code for COMRPC, e.g. register and unregister code, and required containers). But if the answer is yes it will assume the JSONRPC interface will have an @event for this notification and the PSG will generate the code for that as well
* Following that it will ask if this interface also exposes a JSONRPC interface (so if it has a @json tag). Of course if applicable it will generate all code required to make the JSONRPC interface work automatically (add required libraries, interface, registration code, notification listeners of events are used etc.).
* After that it will ask for more subsequent interfaces, just press enter to stop adding interfaces.
* Finally it will ask if the plugin has plugin specific configuration. If answered positive (example)code will be added on how that can be provided to the plugin configuration file from the build and also how it can be parsed and handled in the plugin itself (in an OOP plugin it is assumed it need to be dealt with in the OOP part as that is the most complex case).
* Now an overview of the desired plugin options is generated and the plugin code generated.
