When designing a COM-RPC interface, it is possible to add specific comments (known as annotations or tags) that will drive the code generation. 

All tags follow the same format of `@` followed by the tag name. They can be inserted into the interface using either inline `//` comments or `/* */` block comments. Tags can influence the generation of the COM-RPC ProxyStubs and the generated JSON-RPC interfaces. They can also be used to configure the documentation generation

## Summary

### General purpose tags

| Tag|Short Description|Deprecated|StubGen|JsonGen|Scope|
|--|--|:--:|:--:|:--:|:--:|
|[@stubgen:skip](#stubgenskip)|Stop processing current file. Prefer `@omit`| Yes | Yes|Yes|File|
|[@stop](#stop)|Equivalent to `@stubgen:skip`. Prefer `@omit`. ||Yes| Yes|File|
|[@stubgen:omit](#stubgenomit)|Omit processing of the next class or method | | Yes| No (but has side-effects)| Class, Method|
|[@omit](#omit)|Same as `@stubgen:omit` | | Yes| No (but has side-effects)| Class, Method|
|[@stubgen:include](#stubgen_include)|Insert another C++ file ||Yes| Yes|File|
|[@stubgen:stub](#stubgen_stub)|Emit empty function stub instead of full proxy implementation | | Yes| No| Method|
|[@stub](#stub)|Same as `@stubgen:stub` | | Yes| No|Method|
|[@insert](#insert)|Same as `@stubgen:include` | | Yes|Yes|File|
|[@define](#define)|Defines a literal as a known identifier | | Yes |Yes|File|

<hr/>

#### @stubgen:skip
!!! warning
	This tag is deprecated. [@omit](#omit) is preferred over this tag.

The remaining portion of the file below this tag will be skipped from processing. If placed on top of the file, the complete file is skipped from processing.

##### Example
[IDRM.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IDRM.h#L41) file is skipped from processing placing this tag at the top of this file.

<hr/>

#### @stop
Prefer `@omit` to this tag.

This tag is equivalent to `@stubgen:skip` but can be used as a last resort when the generator is technically unable to parse the file. However in such case it is recommended that the complicated part is moved to a separate header file instead.

<hr/>

#### @stubgen:omit
This tag is applied to structs, class or functions. When the struct/class marked as omit, proxy stubs will not be created for those. Includes inner classes/structs/enums. But [@json](#json) tag will still be applicable to the struct/class. 

##### Example
[ITimeSync.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITimeSync.h#L27) uses omit flag to skip the content of the whole file.  This is the preferred way to skip the content.

<hr/>

#### @omit
Same as `@stubgen:omit`

<hr/>

#### @stubgen:include
This tag is used to include definitions from another file. This tag imports the contents of the file while creating json generation and as well as in stub generation. 

Like `#include` preprocessor the contents of the files are included before processing any other tags. As with #include, it supports two formats:

* `"file"` - include a C++ header file, relative to the directory of the current file
* `<file>` - include a C++ header file, relative to the defined include directories
	* Note: this is intended for resolving unknown types, classes defined in included headers are not considered for stub generation (except for template classes)

##### Example
IDeviceInfo.h [includes](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IDeviceInfo.h#L24) com/IIteratorType.h to get the definition for RPC::IIteratorType

```
// @stubgen:include <com/IIteratorType.h>
```

<hr/>

#### @stubgen:stub
To avoid proxy implementation for a function, mark it with this tag.

##### Example
In [IShell](https://github.com/rdkcentral/Thunder/blob/master/Source/plugins/IShell.h#L232) Submit function is marked as stub as it does not want that function to be called beyond Thunder process

<hr/>

#### @stub
Same as `@stubgen:stub`

<hr/>

#### @insert
Same as `@stubgen:include`

<hr/>

#### @define
Ddefines a literal as a known identifier (equivalent of `#define` in C++ code)

##### Example
```
// @define EXTERNAL
```

### Parameter Related Tags

| Tag|Short Description|Deprecated|StubGen|JsonGen|Scope|
|--|--|:--:|:--:|:--:|:--:|
|[@in](#in)|Marks an input parameter | | Yes| Yes| Method Parameter|
|[@out](#out)|Marks an output parameter | | Yes|Yes|Method Parameter|
|[@inout](#inout)|Marks as input and output parameter (equivalent to `@in @out`) | | Yes|Yes| Method Parameter|
|[@restrict](#restrict)|Specifies valid range for a parameter | | Yes |Yes| Method Parameter |
|[@interface](#interface)| Specifies a parameter holding interface ID value for void* interface passing |  | Yes | No |Method paramter|
|[@length](#length)|Specifies the expression to evaluate length of an array parameter (can be other parameter name, or constant, or math expression)|  | No | Yes | Method Parameter|
|[@maxlength](#maxlength)|Specifies a maximum buffer length value |  | No | Yes |Method parameter|
|[@default](#default)|Provides a default value for an unset variable |  | Yes | Yes |Method parameter|
|[@encode:base64](#encode:base64)|Encodes C-Style arrays as JSON-RPC Base64 arrays |  | Yes | Yes |Method parameter|

#### @in
This tag will mark a parameter in a function as an input parameter. By default, all parameters in a function are treated as input paramter. 

All input paramters are expected to be const. If not, warning will be thrown during JSON-RPC code generation.

##### Example

In [IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IDolby.h#L77) enable parameter is marked as an input parameter.

<hr/>

#### @out
This tag will mark a parameter in a function as an output parameter. By default, all parameters in a function are treated as input parameter.

Output parameters should either be a reference or a pointer and should not be constant.
If these conditions are not met, Error will be thrown during JSON-RPC code generation.

##### Example
In [IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IDolby.h#L67) supported parameter is marked as output paramter.

<hr/>

#### @inout
In few methods, a parameter will act both as input as well as output parameter. Such parameters are marked using this tag. 

##### Example
In [IDisplayInfo.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDisplayInfo.h#L98) the parameter length acts both as input as well as the output. 

While calling this API, application will fill the buffer size in the length paramenter.
When the function returns, the parameter will have the modified length value. Thus acts as both input and output parameter

<hr/>

#### @restrict

Specifies a valid range for a parameter (e.g. for buffers and strings it could specify a valid size). Ranges are inclusive.

If a parameter is outside the valid range, then there are two possibilities:

* If running a debug build, an ASSERT will be triggered if the value is outside the allowed range
* If the stub generator is invoked with the `--secure` flag, then the range will be checked on all builds and an error (`ERROR_INVALID_RANGE`) will be returned if the value is outside the range

##### Example

* `@restrict:1..32` - Value must be between 1 and 32
* `@restrict:256..1K` - Value must be between 256B and 1K in size
* `@restrict:1M` - Value must be <= 1M in size

<hr/>

#### @interface
This tag specifies a parameter holding interface ID value for `void*` interface passing. 

Functions like [Acquire](https://github.com/rdkcentral/Thunder/blob/master/Source/com/ICOM.h#L45) will return the pointer to the queried interface. For such functions, this tag will specify which field to look for to get the corresponding interface id.

##### Example

In [ICOM.h](https://github.com/rdkcentral/Thunder/blob/master/Source/com/ICOM.h#L45) specifies parameter 3 interfaceId holds the interface id for the returned interface.

<hr/>

#### @length
This tag should be associated with an array. It specifies the expresion to evaluate length of an array parameter (can be other parameter name, or constant, or math expression)

Use round parenthesis for expressions, e.g.  `@length:bufferSize` `@length:(width * height * 4)`

##### Example
**From another parameter**

```
/* @length:param1 */
```

**From a constant.**

```
/* @length:32 */
```

**From an expression**

```
/* @length:(param1+param2+16) */
```

In `IOCDM.h`: 

* function [StoreLicenseData](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IOCDM.h#L159) @length param is marked as constant.

* function [SelectKeyId](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IOCDM.h#L171) @length tag is marked as another parameter.

<hr/>

#### @maxlength
Used with the `@out` or `@inout` tag. It specifies a maximum buffer length value (a constant, a parameter name or a math expression). If not specified, `@length` is considered as maximum length

When used with `@inout` it will use different buffer for output depending upon this tag. If not specified it will reuse the same input buffer for output as well.

##### Example
In [IPerformance.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IPerformance.h#L32) it specifies, the maximum length for the buffer.

<hr/>

#### @default
This tag should be associated with optional types. It provides a default value for JSON-RPC, even if no explicit value was set.

Note: Unless a parameter is optional, it must be set.
Note: Currently, OptionalType does not work with C-Style arrays.

##### Example

In this example class, the OptionalType members have a default value that would be transmitted incase their values have not been set.

```cpp
struct Store {
	enum sizetype: uint8_t {
		S, M, L
	};

	Core::OptionalType<string> Brand /* @default:"unknown" */;
	Core::OptionalType<sizetype> Size /* @default: M */;
};

virtual Core::hresult Shirt(const Core::OptionalType<Store>& London) = 0;
```
<hr/>

In [IController.h](https://github.com/rdkcentral/Thunder/blob/master/Source/plugins/IController.h#L78) it specifies, the default value assigned incase the parameter is not set.

<hr/>

#### @encode:base64

This tag encodes C-Style arrays into JSON-RPC arrays as Base64 strings, on the condition that the array base is type uint8_t. 

##### Example

In this example, C-Style uint8_t arrays are encoded as Base64.

```cpp
struct Fixated {
	struct BlueToothInfo {
		uint8_t Addr[6] /* encode:base64 */;
		uint32_t UUIDs[8];
		string Descriptions[8];
	};

Bluetooth BtAddr[5];
uint8_t Edid[128] /*encode:base64 */;
};

```

<hr/>

In [IDisplayInfo.h](https://github.com/rdkcentral/ThunderInterfaces/blob/a229ea291aa52dc99e9c27839938f4f2af4a6190/interfaces/IDisplayInfo.h#L101), the EDID data parameter is encoded.


<hr/>

### JSON-RPC Related Tags

| Tag|Short Description|Deprecated|StubGen|JsonGen|Scope|
|--|--|:--:|:--:|:--:|:--:|
|[@json](#json)|Marks a class as JsonGenerator input | | No| Yes| Class|
|[@json:omit](#json_omit)|Marks a method/property/notification to omit | | No| Yes|Method|
|[@uncompliant:extended](#uncompliantextended)|Indicates the generated JSON-RPC code should use the old "extended" format for parameters| Yes | No| Yes|Class|
|[@uncompliant:collapsed](#uncompliantcollapsed)|Indicates the generated JSON-RPC code should use the old "collapsed" format for parameters| Yes | No | Yes |Class|
|[@compliant](#compliant)|Indicates the generated JSON-RPC code should be strictly JSON-RPC compliant (default)|  | No | Yes |Class|
|[@event](#event)|Marks a class as JSON notification | | No| Yes|Class|
|[@property](#property)|Marks a method as a property ||No|Yes| Method|
|[@iterator](#iterator)|Marks a class as an iterator | | Yes| Yes|Class|
|[@bitmask](#bitmask)| Indicates that enumerator lists should be packed into into a bit mask | | No | Yes |Method parameter|
|[@index](#index)|Marks an index parameter to a property or notification | | No| Yes|Method paramter|
|[@opaque](#opaque)| Indicates that a string parameter is an opaque JSON object | | No | Yes |Method parameter|
|[@alt](#alt)| Provides an alternative name a method can by called by | | No | Yes |Method|
|[@text](#text)| Renames identifier Method, Parameter, PoD Member, Enum, Interface | | No | Yes |Enum, Method parameter, Method name, PoD member, Interface |
|[@prefix](#prefix)| Prepends identifier for all JSON-RPC methods and properties in a class | | No | Yes | Class |
|[@statuslistener](#statuslistener)| Notifies when a JSON-RPC client registers/unregisters from an notification | | No | Yes | Method |
|[@lookup](#lookup)| Creates JSON-RPC interface to access dynamically created sessions | | No | Yes | Method |

#### @json
This tag helps to generate JSON-RPC files for the given Class/Struct/enum.

It will creates 3 files automatically:

* `JsonData_<structname>Output.h` will have definitions for structs that are used in JsonMethods.
* `Jsonenum_<structname>Output.cpp` will have definition for enums
* `J<InterfaceFilename>Output.h` will have definition for the methods for JSON-RPC.

##### Example
[IDisplayInfo.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDisplayInfo.h#L121) uses this tag to generate JSON-RPC files. 

It will create the following files:

* Jsonenum_HDRProperties.cpp
* JHDRProperties.h

<hr/>

#### @json:omit
This tag is used to leave out any Class/Struct/enum from generating JSON-RPC file.

##### Example
[IBrowser.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IBrowser.h#L117) uses this tag to remove HeaderList function from JSON-RPC file generation.

<hr/>

####  @uncompliant:extended

!!! warning
	This tag is deprecated

When a JSON-RPC method is marked as a property (and therefore can only have a single parameter), allow providing that parameter value directly without enclosing it in a surrounding JSON object. For example:

```json
params: "foobar"
```

This should not be used for new interfaces as does not comply strictly with the JSON-RPC specification.

<hr/>

####  @uncompliant:collapsed

!!! warning
	This tag is deprecated

When any JSON-RPC method/property/notification only has a single parameter, allow that parameter value to be directly provided without enclosing it in a surrounding JSON object. For example:

```json
params: "foobar"
```

This should not be used for new interfaces as does not comply strictly with the JSON-RPC specification.

<hr/>

####  @compliant

All JSON-RPC methods, notifications and properties should strictly comply to the JSON-RPC specification - meaning all parameters must be enclosed in a surrounding object with the name of the parameter and value:

```json
params: { 
	"name":  "abcd"
}
```

This is the default behaviour so does not normally need adding to interfaces (unless the generator is being run with non-standard options)

<hr/>


#### @event
This tag is used in JSON-RPC file generation. This tag is used to mark a struct/class that will be called back as an notification by the framework.

##### Example

[IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDolby.h#L53) Whenever the audio mode changes AudioModeChanged API will be called. 

<hr/>

#### @property
Mark a method to be a property when the intention is to perform simple get and set. It cannot have more than one parameter. 

* A method which does more than get and set should not be marked as property even if it is having a single parameter.
* A property is said to be write only if its parameter is const and there no ther method definition with non const is given for reading.
* A property is said to be read only if its parameter is non-const and there no ther method definition with const is given for setting.
* A property is said to be both if it has both const and non-const version present.

##### Example

* [IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDolby.h#L64) is a read only property as it does not have a const version for setting the property.

* [IBrowser.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IBrowser.h#L140) is a write only property as it has only const version and not non const version is available.

* [IBrowser.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IBrowser.h#L100) is a read write property as it has both const and non const version defined.

<hr/>

#### @iterator
This is a helper tag. This helps in generating helper functions to iterate through an array. 
The helper functions are defined in IIteratorType.h. 

The interfaces which needs iteration functionality should include that header using [@stubgen:include](#stubgen_include) tag.

* In Json Generator,it will help to convert the class to JsonArray.
* In Proxy generation, it will help in generating the helper function like Current, Next, Previous for iterating through the array.


##### Example

[IDeviceInfo.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDeviceInfo.h#L24) uses @stubgen:include to insert the IIteratorType to that file.

Which in turn used to iterate through iterators in function [AudioOutputs](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDeviceInfo.h#L83), [VideoOutputs](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDeviceInfo.h#L84), [Resolutions](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDeviceInfo.h#L85)

<hr/>

#### @bitmask
Indicates that enumerator lists should be packed into into a bit mask. 

#####  Example

[IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/3f38ee11f4396b791dc1e75fe575779de3ff1611/interfaces/IBluetoothAudio.h#L159) uses @bitmask to indicate the supported audio codecs should be encoded as a bitmask.

<hr/>

#### @index
Used in conjunction with @property. Allows a property list to be accessed at a given index.

Index should be the first parameter in the function. 

##### Example
[IController.h](https://github.com/rdkcentral/Thunder/blob/6fa31a946314fdbad05792a216b33891584fb4b5/Source/plugins/IController.h#L125) sets the `@index` tag on the `index` parameter.

This allows the status method to be called as normal to return all plugin configs:

:arrow_right: Request

```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"method": "Controller.1.status"
}
```

Or for a specific plugin callsign to be provided after an `@` symbol to retrieve just the status of that plugin

:arrow_right: Request
```json
{
	"jsonrpc": "2.0",
	"id": 1,
	"method": "Controller.1.status@TestPlugin"
}
```

<hr/>

#### @opaque
Indicates the string parameter contains a JSON document that should not be deserialised and just treated as a string

##### Example
[IController](https://github.com/rdkcentral/Thunder/blob/R4.3/Source/plugins/IController.h#L80) uses @opaque to indicate the plugin configuration should be treated as an opaque JSON object that does not need to be deserialised and should just be treated as a string

<hr/>

#### @alt
Provide an alternative name for the method. JSON-RPC methods will be generated for both the actual function name and the alternative name

Note: @text, @alt, @deprecated can be used together.

##### Example

Methods, properties and notifications can be marked by using:
```cpp
// @alt <alternative_name>
// @alt:deprecated <alternative_deprecated_name>
// @alt:obsolete <alternative_obsolete_name>
```
<hr/>

[IController.h](https://github.com/rdkcentral/Thunder/blob/R4.3/Source/plugins/IController.h#L38) uses @alt for the `Reboot()` method to generate an alternatively named method called `Harakiri` (for legacy reasons)

<hr/>

#### @text
This tag is applicable to enums, method names, method parameters, PoD members and whole interfaces. 

* When used for an enum value, it will associate the enum values to the given text in the JSON code while keeping the case of the given text as is.
* When used for a method name, it will replace the actual method name with the text that is given in the tag while keeping the case of the text as is.
* When used for a method parameter, it will replace the parameter name with the text that is given in the tag while keeping the case of the text as is.
* When used for a PoD member, it will replace the PoD member name with the text that is given in the tag while keeping the case of the text as is. Please note that the tag must be placed before the ';'  following the PoD member name (see the examples below).
* When used for a whole interface it must be used in the form '@text:keep'. It will in this case make the JSON generator use the name of the above items in JSON code exactly as specified in the interface, including the case of the text. Please note that of course the interface designer is responsible for making the interface compliant and consistent with the interface guideliness in use (even more perhaps then with the other uses of @text as now the whole interface is influenced). Please see an example on how to apply this form of @text below in the examples.

##### Example
<hr>

[IBrowser.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IBrowser.h#L61) uses this tag for enum. The generated code for this header will map the text for these enums as allowed and not as Allowed, blocked and not as Blocked. 

Without these tags the string equivalent of the enums will be first letter caps followed by all small. This tag has changed it to all small.

</hr>
<hr>

[IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/ac6cd5e8fe6b1735d811ec85633d145e1c1d4945/interfaces/IDolby.h#L62) uses this tag for method names. The generated code for this header will map the text for this method to the name 'soundmodechanged'. As can be seen @text can be combined with @alt if needed so in this case in the JSON-RPC interface this method can be called with both 'soundmodechanged' as well as 'dolby_audiomodechanged'

</hr>
<hr>

In this example the names for PoD member 'One' will be mapped to 'First' and 'Two' will be mapped to 'Second' in the generated code. So in the JSON-RPC interface the JSON container names will be 'First' and 'Second' for these PoD members.
```cpp hl_lines="1"
	struct EXTERNAL Example {
		uint32_t One /* @text First */;
		uint32_t Two /* @text Second */;
	};
```
</hr>
<hr>

In this example now for all enums, method names, method parameters and PoD member names the exact name as in the IExample interface will be used in the JSON generated code.

```cpp hl_lines="1"
  /* @json 1.0.0 @text:keep */
  struct EXTERNAL IExample : virtual public Core::IUnknown {
```
</hr>
<hr>

#### @prefix
Use this tag on a class to prepend all JSON-RPC methods with a prefix identifier. This is an alternative to using multiple @text tags.

In this example, all registered methods would contain the prefix source::.
This is particularly useful for avoiding clashes, especially if several interfaces are implemented by the same plugin.

```cpp
// @json 1.0.0
// @prefix source
struct EXTERNAL ISource: virtual public Core::IUnknown {
```

</hr>
<hr>

### JSON-RPC Documentation Related Tags

| Tag|Short Description|Deprecated|StubGen|JsonGen|Scope|
|--|--|:--:|:--:|:--:|:--:|
|[@sourcelocation](#sourcelocation)|Sets source location link to be used in the documentation | | No | Yes |Class|
|[@deprecated](#deprecated)|Marks a method/property/notification deprecated (i.e. obsolete and candidate for removal) | | No| Yes|Method|
|[@obsolete](#obsolete)|Marks a method/property/notification as obsolete | | No| Yes|Method|
|[@brief](#brief)|Specifies brief description method/property/notification or parameter or POD structure member | | No| Yes|Method, Method parameter, POD member |
|[@details](#details)|Specifies detaild description of a method/property/notification | | No| Yes|Method|
|[@param](#param)|Provide description for method/notification parameter or property/notification index |  | No | Yes|Method|
|[@retval](#retval)|Specifies possible return error codes for method/property (can be many) | | No|Yes|Method|

#### @sourcelocation
By default, the documentation generator will add links to the implemented interface definitions. 

The link used by default is `https://github.com/rdkcentral/ThunderInterfaces/blob/{revision}/jsonrpc/{interfacefile}` (as set in `ThunderInterfaces/jsonrpc/common.json`)

The @sourcelocation tag allows changing this to a custom URL

##### Example
```
@sourcelocation http://example.com
```

<hr/>

#### @deprecated
This tag is used to mark a Method, Property as deprecated in the generated document.

##### Example
When a method is marked with this tag, in the generated .md documentation, it will be marked with the below Message 

>This API is **deprecated** and may be removed in the future. It is no longer recommended for use in new implementations.
>
<hr/>

#### @obsolete
This tag is used to mark a Method, Property as obolete in the generated document.

##### Example
When a method is marked with this tag, in the generated .md documentation, it will be marked with the below Message 
> This API is **obsolete**. It is no longer recommended for use in new implementations

<hr/>

#### @brief
This is a short description about the function. This description will be appeneded to the method description in the JSON-RPC generated file.

##### Example
[IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDolby.h#L65) mentions a brief descrption using this tag.

JsonGenerator.py will create a file JDolbyOutput.h. In that file the method AtmosMetadata. It adds that description.

```cpp hl_lines="1"
// Property: dolby_atmosmetadata - Atmos capabilities of Sink (r/o)
module.Register<void, Core::JSON::Boolean>(_T("dolby_atmosmetadata"),
    [_destination](Core::JSON::Boolean& Result) -> uint32_t {
        uint32_t _errorCode;

        // read-only property get
        bool result{};
        _errorCode = _destination->AtmosMetadata(result);

        if (_errorCode == Core::ERROR_NONE) {
            Result = result;
        }
        return (_errorCode);
    });
```
<hr />

#### @details
Just like @brief starts a brief description, @details starts the detailed description. This tag will be used while creating markdown documents for the header.

There it will be captured in the description section for the method. This description will not be added in the code generation. 

It will be added only in the document generation.

<hr />

#### @param
The syntax for this tag is `@param <PARAMETER>`. It is associated with a function/property.
This tag adds the description about the specified parameter in the generated code and in the generated document.

##### Example
[IDolby.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IDolby.h#L76) add description about enable parameter using this tag.

<hr />

#### @retval
This tag is used in document creation.

The syntax for this tag is `@retval <ErrorCode>: <Description>`. It is associated with function/property

This tag adds description about each return codes specified in the generated markdown document.

##### Example
In [IVolumeControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/5fa166bd17c6b910696c6113c5520141bcdea07b/interfaces/IVolumeControl.h#L54), it uses this tag to add description about the returned error code.

<hr/>

### Advanced Topics

#### @statuslistener

Tagging a notification with @statuslistener will emit additional code that will allow you to be notified when a JSON-RPC client has registered (or unregistered) from this notification.

As a result, an additional IHandler interface is generated, providing the callbacks.

##### Example

In [IMessenger.H](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IMessenger.h#L95-L111), the methods use this tag to enable notification changes.

<hr/>

In this example, a method with the tag @lookup is used, which will allow for the generation of an IHandler interface.

```cpp
// @json 1.0.0
struct EXTERNAL IMessenger {
	virtual ~IMessenger() = default;

	/* @event */
    struct EXTERNAL INotification {
		virtual ~INotification() = default;

		// @statuslistener
		virtual void RoomUpdate() = 0;
	}
}	
```
An example of a generated IHandler interface providing the callbacks from the RoomUpdate() function.

```cpp 
struct IHandler {
    virtual ~IHandler() = default;

    virtual void OnRoomUpdateEventRegistration(const string& client, const   
		PluginHost::JSONRPCSupportsEventStatus::Status status) = 0;
}
```
<hr/>

#### @lookup

This tag can be used on a method, to create a JSON-RPC interface that is accessing dynamically created objects (or sessions). This object interface is brought into JSON-RPC scope with a prefix.

The format for this tag is '@lookup:[prefix]'.

Note: If 'prefix' is not set, then the name of the object interface is used instead.


##### Example

The signature of the lookup function must be:

```cpp 
virtual <Interface>* Method(const uint32_t id) = 0;
```

An example of an IPlayer interface containing a playback session. Using tag @lookup, you are able to have multiple sessions, which can be differentiated by using JSON-RPC.

```cpp 
struct IPlayer {
	struct IPlaybackSession {
		virtual Core::hresult Play() = 0;
		virtual Core::hresult Stop() = 0;
	};

	// @lookup:session
	virtual IPlaySession* Session(const uint32_t id) = 0;

	virtual Core::hresult Create(uint32_t& id /* @out */) = 0;
	virtual Core::hresult Configure(const string& config) = 0;
}
```
An example of possible calls to the interface: the lookup method is used to convert the id to the object.

```
Player.1.configure
Player.1.create
Player.1.session#1::play
Player.1.session#1::stop
```

