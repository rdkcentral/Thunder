Thunder uses its own JSON parser for serialising and deserialising JSON objects. The parser is built around the idea of strongly-typed JSON objects, where every JSON document/object has a corresponding C++ class that represents it. This design has a few advantages:

* Increased parsing performance - the parser can quickly discard parts of the document that are not relevant and the data types are known ahead of time
* Compile-time checks - each JSON document is a strongly typed object so the types of all fields are known by the compiler. This removes the chance to make a mistake when accessing document fields at runtime.


The only downside of this approach it that all JSON documents must have a known structure at compile time. However, for embedded systems this is considered an acceptable trade-off.

## Define a JSON document

All JSON documents should be defined as instances of `Core::JSON::Container`.

The class will contain public variables for each JSON property that should be accessible. In the constructor of the class, the `Add()` method should be called for each property to map the JSON object name to the variable. The class initialiser list should define sensible default values for each variable.

If a key exists in the JSON document that does not exist in the C++ class, then it is silently ignored.

For example, the following JSON document:

```json
{
    "name": "Emily Smith",
    "age": 36,
    "gender": "Female",
    "address": {
        "line1": "1 Example Way",
        "town": "Sample Town",
        "city": "Test City",
        "postcode": "AB1 2CD"
    }
}
```

Would be represented by the following two C++ classes - one for the main object and one for the address

```c++
class Address : public Core::JSON::Container {
public:
    Address(const Address&) = delete;
    Address& operator=(const Address&) = delete;

    Address()
        : Core::JSON::Container()
        , LineOne()
        , Town()
        , City()
        , Postcode()
    {
        Add(_T("line1"), &LineOne);
        Add(_T("town"), &Town);
        Add(_T("city"), &City);
        Add(_T("postcode"), &Postcode);
    }

    Core::JSON::String LineOne;
    Core::JSON::String Town;
    Core::JSON::String City;
    Core::JSON::String Postcode;
};

class Person : public Core::JSON::Container {
public:
    Person(const Person&) = delete;
    Person& operator=(const Person&) = delete;

    Person()
        : Core::JSON::Container()
        , Name()
        , Age(0)
        , Gender()
        , Location()
    {
        Add(_T("name"), &Name);
        Add(_T("age"), &Age);
        Add(_T("gender"), &Gender);
        Add(_T("address"), &Location);
    }

    Core::JSON::String Name;
    Core::JSON::DecUInt16 Age;
    Core::JSON::String Gender;
    Address Location;
};
```

### JSON Data Types

The following JSON data type representations are available:

* `Core::JSON::String`
* `Core::JSON::Boolean`
* `Core::JSON::NumberType<Type, Signed, Base>`
    * This represents an integer of given width. For convenience all common integer types are provided as typedefs. For example:
        * `Core::JSON::DecUInt8` - represent a base-10 integer of width uint8_t
        * `Core::JSON::HexSInt16` - represent a base-16 integer of width uint16_t
        * `Core::JSON::OctSInt32` - represent a base-8 integer of width uint32_t
* `Core::JSON::Float`
* `Core::JSON::Double`
* `Core::JSON::ArrayType<T>` - An array containing objects of type `T`, where T is either a primative JSON type (e.g. Core::JSON::String) or a Core::JSON::Container object
* `Core::JSON::EnumType<T>` - A string that will be converted to a C++ enum

#### Enums

Thunder supports deserialising JSON strings directly to a C++ enum. This is useful when you need to restrict the possible values of a string to a known set.

To do this, an enum conversion must be defined first to map the enum to string

```cpp
enum class Colour {
    RED,
    GREEN,
    BLUE
};

// Define how to convert strings to enum values
ENUM_CONVERSION_BEGIN(Colour)
{Color::RED, _TXT("red")},
{Color::GREEN, _TXT("green")},
{Color::BLUE, _TXT("blue")},
ENUM_CONVERSION_END(Colour)
```

The enum can now be used in a `Core::JSON::EnumType<>` object inside a JSON document.

## Streaming

Unlike many other parsers, Thunder's JSON parser does not require the full JSON document to be read into memory before it can parse it. Instead, it parses JSON documents in chunks (as small as 1 byte). 

This is very useful when working with data over the network, as Thunder can begin to deserialise the message incrementally the instant the first bytes are received instead of having to wait for the entire message to be read into memory. This improves both performance and reduces memory usage.

## Examples

!!! warning
	Thunder does not support comments in JSON documents since comments are not part of the formal [JSON specification](https://datatracker.ietf.org/doc/html/rfc8259)

### Deserialise

#### From File

```c++
Core::File sampleFile("/path/to/sample.json");

if (sampleFile.Open(true)) {
    Person person;
    if (person.IElement::FromFile(sampleFile)) {
        printf("Successfully parsed JSON from file\n");
        if (person.Name.IsNull()) {
            printf("Name is null\n");
        } else {
        	printf("Name: %s\n", person.Name.Value().c_str());
        }
    } else {
        printf("Failed to parse JSON\n");
    }
}

/* Output:
Successfully parsed JSON from file
Name: Emily Smith
*/
```

Since the JSON parser works on streams, the file is read in 1KB chunks and each chunk is parsed individually to build the final document. As a result, it does not need to read the entire file into memory before it can parse it.

If you need to get an error description for why the parsing failed, provide an Error object to the FromFile() method. The error will contain the error and the character at which the error occurred.

```cpp
Core::File sampleFile("/path/to/invalid.json");

if (sampleFile.Open(true)) {
    Person person;
    Core::OptionalType<Core::JSON::Error> error;
    if (person.IElement::FromFile(sampleFile, error)) {
        printf("Successfully parsed JSON from file\n");
        printf("Name: %s\n", person.Name.Value().c_str());
    } else {
        if (error.IsSet()) {
            printf("Failed to parse JSON with error %s\n", ErrorDisplayMessage(error.Value()).c_str());
        }
    }
}

/* Output:
Failed to parse JSON with error Expected either "," or "}", """ found.
At character 33: {
    "name": "Emily Smith"
    "
*/
```

#### From String

```cpp
std::string input = R"({"name": "Emily Smith", "age": 36, "gender": "Female", "address":
                        {"line1": "1 Example Way", "town": "Sample Town",
                        "city": "Test City", "postcode": "AB1 2CD"}})";

Person person;
if (person.FromString(input)) {
    printf("Successfully parsed JSON from string\n");
    printf("Name: %s\n", person.Name.Value().c_str());
} else {
    printf("Failed to parse JSON\n");
}
```

As with deserialising from file, an optional Error object can be provided to store an error message if the JSON could not be parsed

#### From Stream

This (slightly contrived) example simulates reading data in small chunks and parsing each chunk as it arrives. 

In the real-world, this could be data read over a slow connection (such as a serial port or bluetooth) or a large file that you do not want to read entirely into memory before parsing. Note the `FromFile`/`ToFile` chunk their read/writes in 1KB buffers in the same way.

```c++
std::string input = R"({"name": "Emily Smith", "age": 36, "gender": "Female", "address":
                        {"line1": "1 Example Way", "town": "Sample Town",
                        "city": "Test City", "postcode": "AB1 2CD"}})";

// Create object to deserialise into
Person person;

Core::OptionalType<Core::JSON::Error> error;
uint16_t bytesRead = 0;
uint32_t offset = 0;

// Read the string in one byte at a time
for (const char& c : input) {
    char buffer[1];
    buffer[0] = c;

    bytesRead += person.Deserialize(buffer, sizeof(buffer), offset, error);
}

// Check that we read all the data correctly
if (offset != 0 || bytesRead < input.size() && !error.IsSet()) {
    error = Core::JSON::Error { "Malformed JSON. Missing closing quotes or brackets" };
}

// Check if an error occured during parsing
if (error.IsSet()) {
    printf("Failed to read JSON with error %s\n", ErrorDisplayMessage(error.Value()).c_str());
    person.Clear();
} else {
    printf("Parsed JSON. Name is %s\n", person.Name.Value().c_str());
}
```



### Serialise

#### To File

```cpp
Person person;
person.Name = "John Smith";
person.Age = 50;
person.Gender = "Male";

Core::File outputFile("/tmp/result.json");
if (outputFile.Create()) {
    if (person.IElement::ToFile(outputFile)) {
        printf("Successfully wrote json to file\n");
    } else {
        printf("Failed to write JSON to file");
    }
}
```

#### To String

```c++
Person person;
person.Name = "John Smith";
person.Age = 50;
person.Gender = "Male";

std::string outputBuffer;
person.ToString(outputBuffer);

printf("Result: %s\n", outputBuffer.c_str());

/* Output:
Result: {"name":"John Smith","age":50,"gender":"Male"}
*/
```

### Arrays

For the following examples, consider the following JSON document and corresponding class

```json
{
    "fruits": ["Apple", "Orange", "Banana"]
}
```

```c++
class Fruit : public Core::JSON::Container {
public:
    Fruit(const Fruit&) = delete;
    Fruit& operator=(const Fruit&) = delete;

    Fruit()
        : Core::JSON::Container()
        , Fruits()
    {
        Add(_T("fruits"), &Fruits);
    }

    Core::JSON::ArrayType<Core::JSON::String> Fruits;
};
```

#### Iterator

```c++
Fruit fruit;
fruit.FromString(input);

auto index = fruit.Fruits.Elements();
printf("There are %d fruits in the array:\n", index.Count());

while (index.Next()) {
    printf("* %s\n", index.Current().Value().c_str());
}

/* Output:
There are 3 fruits in the array:
* Apple
* Orange
* Banana
*/
```

#### Add Item

```cpp
std::string input = R"({"fruits": ["Apple", "Orange", "Banana"]})";

Fruit fruit;
fruit.FromString(input);

printf("There are %d fruits in the array\n", fruit.Fruits.Elements().Count());

fruit.Fruits.Add(Core::JSON::String("Grape"));
printf("There are %d fruits in the array\n", fruit.Fruits.Elements().Count());

/* Output:
There are 3 fruits in the array
There are 4 fruits in the array
*/
```

!!! note
	There is currently no API for removing array items - it will be added in a future release.

## Variant JSON Type

!!! danger
	Whilst the Variant JSON type does exist and can be used, it offers worse performance, and loses all type-safety that comes with using formally defined JSON documents. It should be avoided wherever possible.


	The below documentation is for reference only, use the strongly-typed JSON Container if you can!

The JSON variant type behaves closer to other C++ json parsers, and does not require defining a JSON document structure up front. Instead, it allows accessing the JSON properties dynamically at runtime.

### Example

Some typedef's are provided to make the variant types easier to work with

```cpp
using JsonObject = Thunder::Core::JSON::VariantContainer;
using JsonValue = Thunder::Core::JSON::Variant;
using JsonArray = Thunder::Core::JSON::ArrayType<JsonValue>;
```

#### Deserialise

```cpp
std::string input = R"({"name": "Emily Smith", "age": 36, "gender": "Female", "address":
                    {"line1": "1 Example Way", "town": "Sample Town",
                    "city": "Test City", "postcode": "AB1 2CD"}})";

JsonObject jsonObject;
if (jsonObject.IElement::FromString(input)) {
    JsonValue name = jsonObject["name"];
    JsonValue age = jsonObject["age"];

    if (name.IsSet() && !name.IsNull()) {
        printf("Name is %s\n", name.String().c_str());
    }

    if (age.IsSet() && !age.IsNull()) {
        printf("Age is %ld\n", age.Number());
    }
}

/* Output:
Name is Emily Smith
Age is 36
*/
```

#### Serialise

```cpp
Thunder::Core::JSON::VariantContainer jsonObject;

JsonArray sampleArray;
sampleArray.Add(JsonValue("apple"));
sampleArray.Add(JsonValue("banana"));
sampleArray.Add(JsonValue("orange"));

jsonObject["fruit"] = sampleArray;

// Print debug string
printf("%s\n", jsonObject.GetDebugString().c_str());

// Print actual JSON document
string jsonString;
jsonObject.ToString(jsonString);
printf("%s\n", jsonString.c_str());

/* Output:
name=fruit type=Array value=[
    [0] type=String value=apple
    [1] type=String value=banana
    [2] type=String value=orange
]

{"fruit":["apple","banana","orange"]}
*/
```

## MessagePack support

For scenarios where small message sizes are imperative and the size of the message on the wire needs to be reduced as much as possible, Thunder supports serialising/deserialising JSON containers to [MessagePack](https://msgpack.org/index.html) encoding (aka MsgPack) instead.

> MessagePack is an efficient binary serialization format. It lets you exchange data among multiple languages like JSON. But it's faster and smaller. Small integers are encoded into a single  byte, and typical short strings require only one extra byte in addition  to the strings themselves.
>
> *Source: https://msgpack.org/index.html*

This is a fairly niche feature, and the normal JSON format will be more suitable the majority of the time, but the feature is available if required.

The below example uses the same `Person` class defined earlier in this page without modification and turns it into a MsgPack encoded string

```c++
Person person;
person.Name = "John Smith";
person.Age = 50;
person.Gender = "Male";

std::vector<uint8_t> outputBuffer;
person.IMessagePack::ToBuffer(outputBuffer);

for (const auto& value : outputBuffer) {
    printf("%x ", value);
}
printf("\n");

/* Output:
83 a4 6e 61 6d 65 aa 4a 6f 68 6e 20 53 6d 69 74 68 a3 61 67 65 32 a6 67 65 6e 64 65 72 a4 4d 61 6c 65
*/
```

The `IMessagePack` interface supports the following methods:

* `ToBuffer(std::vector<uint8_t>& stream)`
* `FromBuffer(const std::vector<uint8_t>& stream)`
* `ToFile(Core::File& fileObject)`
* `FromFile(Core::File& fileObject)`