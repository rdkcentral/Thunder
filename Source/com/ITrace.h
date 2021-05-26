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

#pragma once

#include "Ids.h"

namespace WPEFramework {
    namespace Trace {
        struct EXTERNAL ITraceIterator : virtual public Core::IUnknown {
            enum { ID = RPC::ID_TRACEITERATOR };

            virtual void Reset() = 0;
            virtual bool Info(bool& enabled /* @out */, string& module /* @out */, string& category /* @out */) const = 0;
        };
        struct EXTERNAL ITraceController : virtual public Core::IUnknown {
            enum { ID = RPC::ID_TRACECONTROLLER };

            virtual void Enable(const bool enabled, const string& module, const string& category) = 0;
        };
    }
}
