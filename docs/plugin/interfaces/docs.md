Documentation for both an Interface as well as a whole plugin can be generated using the DocumentGenerator.

When generated for a plugin the json meta file describing the plugin, found in the plugin folder and usually called [pluginname]Plugin.json, should be used as input to the DocumentGenerator. The DocumentGenerator will generate a document (md file) describing the plugin, the configuration options possible in the plugin configuration json file and a complete overview of the the different JSON-RPC interfaces the plugin implements.
See [Here](https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK/blob/master/PlayerInfo/doc/PlayerInfoPlugin.md) for an example of the generated documentation for a plugin (normally in the ThunderNanoServices and ThunderNanoServicesRDK repositories the generated documentation for a plugin is added to the doc folder of the plugin).

The DocumentGenerator can also generate the documentation for a specific interface. In this case the input for the DocumentGenerator is just the IDL header file (like [this](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBrowser.h) one for example) or a json meta file describing the JSON-RPC interface (like [this](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Netflix.json) one).
Please note that for a Thunder release the documentation for all the interfaces in the ThunderInterfaces repository is generated and published [here](https://webplatformforembedded.github.io/ServicesInterfaceDocumentation/latest/), the specific Thunder release can be selected in a drop down list on the top of the page.

The DocumentGenerator can be found in the ThunderTools repository, [here](https://github.com/rdkcentral/ThunderTools/tree/master).
Of course as the options supported in the IDL file increases, the document generator is adopted accordingly and therefore the branch for the ThunderTools repository from which the DocumentGenerator is used must be taken into account compared to the branch where the interfaces and/or plugins are from that you use as input for generation.

DocumentGeneration can be triggered by starting JsonGenerator.py with the --docs parameter (found [here](https://github.com/rdkcentral/ThunderTools/tree/master/JsonGenerator)). 
As you probably need to specify a number of additional parameters to JsonGenerator.py for the generation to be successful, a helper shell script is created that sets the parameters correctly when generating the documentation for the Thunder interfaces and plugin repositories. 
This script can be found [here](https://github.com/rdkcentral/ThunderTools/tree/master/JsonGenerator), and is called GenerateDocs.sh (there is also a GenerateDocs.bat in case you want to do this on a Windows OS).

Follow these simple steps to generate the documentation for an interface or plugin from one of the Thunder repositories:

1. Clone the ThunderTools repository
2. Clone the ThunderInterfaces repository (inside the same folder where you cloned ThunderTools)
3. In case you want to generate plugin documentation, also clone the applicable Thunder plugin repository (again inside the same folder where you cloned ThunderTools)
4. Open a console window
5. Go into the folder [path to where you cloned the ThunderTools]/ThunderTools/JsonGenerator
6. now call ./GenerateDocs.sh [path to where you cloned the ThunderInterfaces]/ThunderInterfaces/interfaces/IYourInterface.h (or specify a path to a plugin json file in case you want the plugin documentation to be generated). Example call for generating the Plugin documentation would be: ./GenerateDocs.sh ../../ThunderNanoServices/Dictionary/DictionaryPlugin.json
7. the document generator will print where it created the documentation .md file (for an interface .h file it will be ThunderInterfaces/interfaces/doc/YourInterfaceAPI.md and for a plugin it will be inside the doc folder of the applicable plugin)




