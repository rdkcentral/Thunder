# Thunder Custom (Error) Codes

## Introduction

Error codes used in Thunder are normally predefined, which suffices up to now. These error codes (due to the "JSON-RPC in terms of COM-RPC" feature) are also translated into JSON-RPC error codes. The custom code feature supports a more flexible error scheme to be used (mainly to have more flexibility in the error codes reported in the JSON-RPC error object).
There is already a feature for "custom JSON-RPC error messages" which allows for context dependent JSON-RPC messages but also allows to override the error code returned in some form. 
This however leads to inconsistent error codes between COM-RPC and JSON-RPC for the same handler and is not designed to be used on a large scale but for exceptional cases only.
So with Custom Codes we support a way to have custom error codes that are consistent for both COM-RPC and JSON-RPC (or any other future protocol that will be implemented in terms of COM-RPC), allow for custom (not hardcoded inside Thunder) code to string translation and allow for direct influence on the error code returned by JSON-RPC.
(the "custom JSON-RPC error messages" feature will continue to be supported for context dependent JSON-RPC messages).

## Custom Error Code hresults solution and range provided

Every COM-RPC method returns a Core::hresult error code. This is an unsigned double word value. 
Custom Codes uses the 25th bit of the hresult to indicate the hresult contains a "custom error code" that is not one of the Thunder predefined error codes and should be treated differently.
When this bit is set the 24th bit will be considered a sign bit so that the custom error codes can also be negative.
This leads to an available range of -8.388.608 to 8.388.607 for the custom error codes. 
One custom code will however have special meaning, code 0, (so with "custom code bit" set, hresult code 0x1000000) will mean an invalid custom error code was set, more on this later. 

## Thunder Core Code support

The Thunder Core header file (Errors.h) adds the following helper function:

### Core::CustomCode

```cpp
  Core::hresult CustomCode(const int24_t customCode);
```

This function can be used to transform a custom code into a Core::hresult where the correct bit is set to indicate this.

If the customCode input for CustomCode is Core::ERROR_NONE it will create a Core::hresult of Core::ERROR_NONE (so no Custom Code bit set). This as existing (Thunder and plugin) code do at certain locations check if a call failed with:

```cpp
  Core::hresult error = DoSomething();

  if (error != Core::ERROR_NONE) {
     //handle the error situation
  }
```

and with the above this code will continue to work without adopting (which would also cause additional overhead and it will be hard to identify all locations where code might be checking like this). Note that this is why returning 0x1000000 directly is not recommended (custom error bit set with code 0) as that will cause issues (and would mean you are returning the error code "invalid custom error code" not "no error")

And with this behaviour of the CustomCode the following code will work correctly and not lead to unexpected behaviour:

```cpp
Core::hresult A()
{
  int24_t error = Core::ERROR_NONE;
  
  if(something_bad_happened == true) {
    error = -12345; // set a custom code error
  }

  return Core::CustomCode(error);
}
```

or if preferred this alternative will work as well of course:

```cpp
Core::hresult A()
{
  Core::hresult error = Core::ERROR_NONE;
  
  if(something_bad_happened == true) {
    error = Core::CustomCode(-12345); // set a custom code error
  }

  return error;
}
```

In case an error code is passed to CustomCode that would overflow the reserved range this will ASSERT in debug and in Release (or any situations asserts are off) to a flexible error code of 0 (so custom code bit set with code 0) so this info is still carried in the hresult and can be extracted later.

Some examples:

```cpp
{

 Core::hresult error = Core::CustomCode(-12345); // will return an hresult with the "custom code bit" set and the custom code will be -12345

 Core::hresult error = Core::CustomCode(Core::ERROR_NONE); // will set the hresult to Core::ERROR_NONE ("custom code bit" not set)

 Core::hresult error = Core::CustomCode(9000000); // as this overflows the allowed range it will return an hresult with the "custom code bit" set and the flex error code will be 0, so 0x1000000 (or an assert in debug)

}
```

### Core::IsCustomCode

With IsCustomCode one can find out if a Core::hresult is a custom code and what the code is:

```cpp
  int24_t IsCustomCode(const Core::hresult code);
```

 This will return the custom error code as signed 24 bit number when the "custom error bit" was set and 0 if the hresult was not a flexible error. 
 (if the hresult code would have the value 0x1000000 ("custom code bit" set with code 0) IsCustomCode will return an int24_t with overflow value set which can be checked with Core::Overflowed, see the example below.

```cpp
{

        ASSERT(Core::IsCustomCode(Core::CustomCode(-12345)) == -12345);

        ASSERT(Core::IsCustomCode(Core::CustomCode(Core::ERROR_NONE)) == 0);

        Core::hresult result = Core::ERROR_GENERAL;
        ASSERT(Core::IsCustomCode(result) == 0);

        if (Core::Overflowed(Core::IsCustomCode(Core::CustomCode(9000000))) == true) { printf("this will be printed"); } // note will when ASSERTS are enabled in the build already ASSERT on assigning the value to CustomCode

}
```
In example the above the ASSERTS will not fire and "this will be printed" will be printed (when ASSERTS are not turned on in the build)

### Core::ErrorToString (existing)

```cpp
    const TCHAR* ErrorToString(const Core::hresult code);
```

The existing Core::ErrorToString will also (so next to the string representation for non Custom Code hresult values) return the correct string representation for the custom error code (also see the string representation section below) although the message will not include the actual set Custom Code, see ErrorToStringExtended below
So you can feed any hresult to this function, whether it is a Custom Code hresult or not.

### Core::ErrorToStringExtended (new)

```cpp
    string ErrorToStringExtended(const Core::hresult code);
```

This function will return a correct string representation for any hresult value, whether it is a Custom Code hresult or not. If it is a Custom Code and the string cannot be retrieved from the conversion library the actual Custom Code value will be in the message.

As the full desired functionality with Core::ErrorToString can not be achieved, a second, ErrorToStringExtended, is available for this.
As the range for the custom code is too big to have pre created static texts for all of them, it is desirable to create a dynamic string representation when a static one is not provided by the conversion library (or the conversion library feature is not used at all) to include the actual custom code in the message.
This cannot be done in the ErrorToString as it returns a pointer to the text, therefore an extended version is created that will include the custom code number.
(changing the ErrorToString was not desirable as that would break backwards compatibility unnecessarily)

## String representation

To be able to provide for a custom error code to string conversion (e.g. used for the JSON-RPC error text in the error object or to return a correct string representation when Core::ErrorToString is called) the following infrastructure will be added to Thunder:

In Thunder sources/core a header file "ICustomErrorCodes.h" provides the interface to be implemented by an external library to support the custom error codes used in the plugin code for that Thunder instance.

For now there is only one function in this interface called "CustomCodeToString":

```cpp

/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
    This file contains the interface that a library can implement in case the "custom error codes" feature is used in Thunder and code to string conversion is desired
*/

#pragma once

#undef EXTERNAL

#if defined(WIN32) || defined(_WINDOWS) || defined(__CYGWIN__) || defined(_WIN64)
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifndef TCHAR
#ifdef _UNICODE
#define TCHAR wchar_t
#else
#define TCHAR char
#endif
#endif

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

// called from within Thunder to get the string representation for a custom code
// note parameter code is the pure (signed) Custom Code passed to Thunder. no additional bits set (so for signed numbers 32nd bit used as sign bit). 
// in case no special string representation is needed return nullptr (NULL), in that case Thunder will convert the Custom Code to a generic message itself
EXTERNAL const TCHAR* CustomCodeToString(const int32_t code);

#ifdef __cplusplus
} // extern "C"
#endif 

```

It should suffice to have static strings which prevents unnecessary string copying and keeps the memory management simple.

There is a Thunder config option called "customcodelibrary" that allows you to point Thunder to the library implementing the above interface.
If it is not provided, or in case CustomCodeToString returns a nullptr for the particular code and a string representation for a custom error code is required the error code will be translated into "Undefined Custom Error: XXXX"  where XXXX is the custom error code.
Note the existing ErrorToString will return a string without the number included when the string is not provided by the library as that cannot be supported without breaking backwards compatibility, ErrorToStringExtended will however (Thunder will of course use the new ErrorToStringExtended version to get the string that will be added to the JSON-RPC error text tag).
Note the custom error code 0 will not be routed through CustomCodeToString but always translate into "Invalid Custom ErrorCode set".

### JSON-RPC specifics for custom error codes

In case JSON-RPC is called in terms of COM-RPC (so an IDL C++ header file with a COM-RPC interface where the @json tag is used to generate the JSON-RPC interface) and the COM-RPC method returns a custom error code it will be dealt with in the following way:

* The custom error code will be placed as is in the JSON RPC error object "code" tag. (so code 0 means an invalid custom code (too big) was passed). Note it is the responsibility of developers setting the codes not to overlap with JSON RPC specification reserved codes or Thunder codes (1 to 100 in Thunder 4.4 or -31000 to -31999 in Thunder 5 and above) this on explicit request so that codes can be used for API backwards compatibility (a TRACE will be added to warn for these cases though)
* the JSON-RPC error object "message" tag will contain the string returned by the CustomCodeToString call from the conversion library if configured and does return a text for that specific code, otherwise it will be set to "Undefined Custom Error: XXXX" where XXXX will be the Custom Code set. (of course when the "custom JSON-RPC error messages" feature was used for this call that will override this default behaviour)
* if the value set with CustomCode() overflowed (too big for a 24bit number) and ASSERTS are not enabled in the build the error code will be set to 0 and the object "message" tag will be set to "Invalid Custom ErrorCode set"

### Example

An example on how the above can be used in a plugin, including creating a custom code to string library, can be found [here](https://github.com/rdkcentral/ThunderNanoServices/tree/R5_3/examples/CustomErrorCodes)