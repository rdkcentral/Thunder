The PluginSkeletonGenerator (PSG) is a tool that can be used to generate a skeleton for a new plugin, giving you a quick start when developing a new plugin but also preventing common mistakes to be made in the generic plugin code.

Please note that the tool is in beta release at the moment. Although it was very carefully designed and reviewed of course it cannot be ruled out that there are any issues with it, certainly seen the number of different configurations it can generate. If you find any issues with the generated code please contact the Thunder team.

Some remarks:

* Although it already supports quite a number of different plugin scenario’s/configurations it will not support everything you can think of in a plugin, that it will never do. So if you cannot do it with the PSG, it does not mean Thunder cannot do it, you just probably need to add/change code by hand. On the other hand the PSG will be extended and will support more common plugin options in the future.
* At the moment the PSG will generate Thunder 5 compliant code.
* Start the PSG in the folder where you want a subfolder to be created for your new plugin (so do not start it from the PSG folder itself).
* The PSG does already support a plugin implementing more than one interface.
* The generator now has the ability to parse the header file for the interface(s) you want your plugin to implement.
* The parser checks for tags such as @json to generate the correct code required to make the JSONRPC interface work automatically (add required libraries, interface, registration code, notification listeners of events are used etc.). If the interface is found to be JSONRPC, the PSG will also generate the code for COMRPC (Register and Unregister) if appropriate.
* The PSG will generate a default text for the License (Apache License 2.0) in all the code files with a placeholder for the organization. Please update this after generating the code.

How to use the Plugin Skeleton Generator:

* Start the PSG from the folder in which you want your plugin to be generated (it will be created in its own subfolder of course) or start it from the ThunderDevTools menu.
* You first will be asked how you want to name your plugin (this will be used for the subfolder, classnames, callsign etc.).
* Then you will be asked if the plugin needs to be able to run OOP, select the desired option.
* Next, it will ask if the plugin has plugin specific configuration. If answered positive, example code will be added on how that can be provided to the plugin configuration file from the build, and also how it can be parsed and handled in the plugin itself (in an OOP plugin it is assumed it needs to be dealt with in the OOP part as that is the most complex case).
* Now you will be given the option to define the interface(s) your plugin will implement, the PSG already supports generating a skeleton for a plugin that does implement more than one. The PSG also supports a plugin that does not implement an interface at all (except IPlugin of course), but this is logically only available for an in process plugin. Add the full path to your interface and press enter to define an interface. Press ENTER on an empty string to stop defining interfaces.
* If your file contains more than one interface at the root level, you will be asked to specify which interface(s) you would like to use.
* Following this, it will ask whether your plugin relies on Thunder subsystems (Preconditions, Terminations, Controls).
* Afterwards, it will ask in which subfolder of the include path the interfaces your plugin will implement are located (default is interfaces, <interfaces/IExample.h>) so in that case you just can press ENTER or otherwise include the custom location you want to use in the generated code. Please note that this will only influence the path the PSG will use in the generated code to include the interface from, it might be needed to adjust the plugin CMake file if your interfaces are not located in ThunderInterfaces.
* Now an overview of the desired plugin options is displayed, and the plugin code is generated.