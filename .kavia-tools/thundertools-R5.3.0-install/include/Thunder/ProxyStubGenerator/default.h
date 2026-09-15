/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

//
// Forward declarations of types used by the interfaces.
// Included by default by the ProxyStubGenerator.
// (Fundamental types, stdint and string types are built into the generator)
//

#pragma once

/* @define EXTERNAL */
/* @define DEPRECATED */
/* @define VARIABLE_IS_NOT_USED */

typedef char TCHAR;
typedef wchar_t WCHAR;

namespace std {
  typedef __stubgen_undetermined_integer uintmax_t;
  typedef __stubgen_undetermined_integer intmax_t;
  typedef __stubgen_undetermined_integer uintptr_t;
  typedef __stubgen_undetermined_integer intptr_t;
  typedef __stubgen_undetermined_integer size_t;
  typedef __stubgen_undetermined_integer ssize_t;
  typedef __stubgen_undetermined_integer time_t;
  typedef __stubgen_undetermined_integer clock_t;

  template<typename T>
  class vector;
}

namespace __FRAMEWORK_NAMESPACE__ {

  namespace ProxyStub {
    class UnknownProxy;
  }

  namespace Core {
    typedef __stubgen_instance_id instance_id;
    typedef __stubgen_time Time;
    typedef __stubgen_macaddress MACAddress;

    typedef uint32_t hresult;

    template<typename T>
    struct OptionalType;

    struct IUnknown {
      enum {
        ID_OFFSET_INTERNAL = 0
      };
    };

    namespace JSONRPC {
      struct Context;
    }
  }

  namespace PluginHost {
    class IShell;
    class ISubSystem;
    class IPlugin {
      class INotification;
    };
  }
}
