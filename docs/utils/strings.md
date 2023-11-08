Thunder has a few useful utilities for common string manipulation and encoding tasks. Below are some examples of common functionality that can be implemented using these utilities.

!!! hint
	Throughout most Thunder code, the `string` type is used instead of `std::string` or `std::wstring`. This is recommended since Thunder will automatically pick the correct string type depending on platform (e.g. std::wstring on Windows).


## Trim string

Remove characters (e.g. whitespace, quotes) from the start or end of the string in-place.

```c++
Core::TextFragment sampleString("   Test string");
sampleString.TrimBegin(" "); // Here we specify we want to trim whitespace

printf("%s\n", sampleString.Text().c_str());

/* Output:
Test string
*/
```

## Split string by delimiter

Split a given string on a specified delimiter and generate an iterator to loop over the segments.

```c++
Core::TextFragment sampleString("The;quick;brown;fox");

// Setting the second argument to true will automatically ignore empty segments
Core::TextSegmentIterator segments(sampleString, true, ";");
while (segments.Next()) {
       printf("%s\n", segments.Current().Text().c_str());
}

/* Output:
The
quick
brown
fox
*/
```

## Formatting

The `Core::Format` API can be used to format strings using printf syntax as a safe alternative to traditional C `sprintf`-style APIs.

```c++
string formattedValue = Core::Format(_T("Hello %s"), "World");
printf("%s\n", formattedValue.c_str());

/* Output:
Hello World
*/
```

## String Conversion

The `Core::ToString()` and `Core::FromString()` APIs can be used to safely convert to/from various different types

### Numbers

```c++
uint32_t number = 32;
string text = Core::ToString(number);
printf("%s\n", test.c_str());

/* Output:
32
*/
```

### Base64

Convert from string to base64
```c++
std::string sample = "The quick brown fox jumped over the lazy dog";

std::string base64result;
Core::ToString(reinterpret_cast<const uint8_t*>(sample.c_str()), sample.length(), true, base64result);

printf("%s\n", base64result.c_str());

/* 
Output:
VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wZWQgb3ZlciB0aGUgbGF6eSBkb2c=
*/
```

Convert from base64 to string

```cpp
std::string base64 = "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wZWQgb3ZlciB0aGUgbGF6eSBkb2c=";

uint8_t buffer[128];
uint16_t length = sizeof(buffer);

Core::FromString(base64, buffer, length);

std::string result = reinterpret_cast<char *>(buffer);
printf("%s\n", result.c_str());

/* 
Output:
The quick brown fox jumped over the lazy dog
*/
```

### Hex
This is useful for debugging and printing the contents of arrays in hexadecimal format

```cpp
uint8_t data[] = {0x01, 0xAB, 0x23, 0x10};

string dataString;
Core::ToHexString(data, sizeof(data), dataString);
printf("%s\n", dataString.c_str());

/* Output:
01ab2310
*/
```