---
applyTo: '**'
---

# 03 — Code Style

> These rules are mechanical and concrete. The agent must apply them to every C++ file it writes or modifies.

## Formatter

Thunder uses **clang-format** with a WebKit-based style. The agent must treat every C++ edit as incomplete until clang-format has been applied.

```bash
clang-format -i <file.h> <file.cpp>
```

The `.clang-format` file at the root of the repository is authoritative. Key settings:

| Setting | Value | Meaning |
|---------|-------|---------|
| `BasedOnStyle` | `WebKit` | Inherits WebKit defaults |
| `IndentWidth` | `4` | 4 spaces — **no tabs** |
| `PointerAlignment` | `Left` | `int* ptr`, not `int *ptr` or `int * ptr` |
| `ColumnLimit` | `0` | No line-length limit enforced |
| `BreakBeforeBraces` | `WebKit` | Function/class/namespace braces on a **new line**; control-flow (`if`/`for`/`while`) braces on **same line** |
| `AccessModifierOffset` | `-4` | `public:`, `private:`, `protected:` dedented to column 0 relative to class body |
| `NamespaceIndentation` | `Inner` | Inner namespaces are indented |
| `BreakConstructorInitializers` | `BeforeComma` | Comma-first style for constructor init lists |
| `AllowShortFunctionsOnASingleLine` | `All` | Short functions may be written on one line |

## Naming Conventions

| Construct | Convention | Examples |
|-----------|-----------|---------|
| Private / protected member variable | `_camelCase` | `_adminLock`, `_handler`, `_refCount`, `_skipURL` |
| Class / struct | `PascalCase` | `CriticalSection`, `ServiceMap`, `WorkerPoolImplementation` |
| COM interface | `I` + `PascalCase` | `IPlugin`, `IShell`, `IWorkerPool`, `IMyInterface` |
| Method (any visibility) | `PascalCase` | `Initialize()`, `AddRef()`, `QueryInterface()` |
| Template parameter | `UPPER_CASE` | `IMPLEMENTATION`, `CONTEXT`, `ELEMENT`, `FORWARDER` |
| Enum value | `UPPER_CASE` | `ACTIVATED`, `DEACTIVATED`, `ERROR_NONE` |
| Macro | `UPPER_CASE` | `EXTERNAL`, `ASSERT`, `TRACE_L1`, `BEGIN_INTERFACE_MAP` |
| Namespace | `PascalCase` | `Thunder`, `Core`, `PluginHost`, `Plugin`, `RPC`, `Exchange` |
| `using` type alias | `PascalCase` | `using Clients = std::map<string, Entry>;` |
| Include guard macro | `__UPPER_CASE_H__` | `#ifndef __IPLUGIN_H__`, `#ifndef __MYPLUGIN_H__` |
| Local variable | `camelCase` | `entryCount`, `socketPath`, `currentState` |
| Function parameter | `camelCase` | `interfaceId`, `callsign`, `dataPath` |

**Legacy note**: older files (e.g. `Source/core/Thread.h`) use `m_PascalCase` for member variables. This pattern **must not** be used in new code — use `_camelCase`.

## Include Guards

Thunder does **not** use `#pragma once`. Every header must use the double-underscore guard pattern:

```cpp
#ifndef __MYPLUGIN_H__
#define __MYPLUGIN_H__

// ... file content ...

#endif // __MYPLUGIN_H__
```

The macro name must be `__` + `UPPER_CASE_FILENAME` + `_H__`. Never deviate from this pattern.

## String Literals

All string literals passed to Thunder APIs must use the `_T()` macro for wide-char portability:

```cpp
Add(_T("retries"), &Retries);
Register(_T("mymethod"), &MyPlugin::MyMethod, this);
service->DataPath(_T("myfile.json"));
```

## Constructor Initializer Lists

Use **comma-first** (BeforeComma) style — one member per line, comma at the start of each continuation line:

```cpp
MyPlugin()
    : _adminLock()
    , _skipURL(0)
    , _clients()
    , _sink(this)
    , _currentState(Exchange::IPower::PCState::On)
{
}
```

## Non-Copyable / Non-Movable Classes

Declare all four deleted special members explicitly in the `public:` section, **before** any other members or methods:

```cpp
class MyClass {
public:
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;
    MyClass(MyClass&&) = delete;
    MyClass& operator=(MyClass&&) = delete;

    // ... other private members ...
};
```

Most plugin classes, COM classes, and framework classes must be non-copyable and non-movable.

## Virtual Methods and `override`

- Every method that overrides a virtual base **must** have the `override` specifier. Never rely on implicit override.
- Interface destructors must be declared `= default` with `override` — never define a body:
  ```cpp
  ~IMyInterface() override = default;
  ```
- Concrete final classes must be marked `final`:
  ```cpp
  class MyPluginImpl final : public PluginHost::IPlugin, public Exchange::IMyInterface { ... };
  ```

## `public` Default Constructor Deletion

If a class must not be default-constructed, explicitly delete it in the `public:` section:

```cpp
class MySink {
public:
    MySink() = delete;
    // ...
};
```

## Warning Suppression

Use Thunder's portable macros — **never** raw `#pragma warning` or platform-specific `#pragma`:

```cpp
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
MyPlugin()
    : _sink(this)   // safe: sink does not use this in its constructor
{
}
POP_WARNING()
```

Common suppressors available:
- `DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST`
- `DISABLE_WARNING_DEPRECATED_USE`
- `DISABLE_WARNING_MULTIPLE_INHERITANCE_OF_BASE_CLASS`
- `DISABLE_WARNING_SHORTEN_64_32`

## License Header

Every new `.h` and `.cpp` file must begin with the Apache 2.0 license header:

```cpp
/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
```
Existing files with Copyright pointing to Metrological need not be changed.

## `Module.h` and `Module.cpp`

Every Thunder library/plugin **must** have a `Module.h` defining `MODULE_NAME` and a `Module.cpp` registering it. `Module.h` is always the **first** internal include in every `.cpp` file in that module:

```cpp
// Module.h
#ifndef __MODULE_MYPLUGIN_H__
#define __MODULE_MYPLUGIN_H__
#ifndef MODULE_NAME
#define MODULE_NAME MyPlugin
#endif
#include <core/core.h>
#include <plugins/plugins.h>
#endif // __MODULE_MYPLUGIN_H__

// Every .cpp in the plugin:
#include "Module.h"
// ... other includes follow
```

## `EXTERNAL` Macro

Apply `EXTERNAL` to every class and free function that crosses a shared library boundary:

```cpp
class EXTERNAL MyPlugin
    : public PluginHost::IPlugin {
    // ...
};
```

On Linux: expands to `__attribute__((visibility("default")))`.
On Windows: expands to `__declspec(dllexport)` / `__declspec(dllimport)`.
Missing `EXTERNAL` causes linker failures in OOP mode and incorrect `dynamic_cast` behavior on macOS for non-template classes.

## Namespace Conventions

- All framework code lives in `Thunder::` or one of its sub-namespaces (`Core::`, `PluginHost::`, `RPC::`, `Exchange::`).
- Plugin implementations use `Plugin::` namespace.
- Do **not** use `using namespace` in header files — only in `.cpp` files at function scope.
- Anonymous namespaces (`namespace { }`) are used for file-local definitions (e.g. `ANNOUNCE` registration in generated stubs).
