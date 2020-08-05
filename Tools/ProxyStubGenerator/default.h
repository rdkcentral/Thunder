/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
// Included by default by the stub generator.
// (fundamental types, time_t, stdint, string and bool types are built into the generator)
//

#pragma once

typedef char TCHAR;
typedef wchar_t WCHAR;

namespace WPEFramework {

  namespace Core {
    class IUnknown;
  }

  namespace PluginHost {
    class IShell;
    class ISubSystem;
    class IPlugin {
      class INotification;
    };
  }

  namespace RPC {
    class IStringIterator;
    class IValueIterator;
  }

} // namespace WPEFramework

