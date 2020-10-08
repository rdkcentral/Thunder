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

#pragma once
#include "Module.h"

 // @stubgen:skip

namespace WPEFramework {
namespace Exchange {

    // This interface allows for retrieval of memory usage specific to the implementor
    // of the interface
    struct EXTERNAL IMemory : virtual public Core::IUnknown {
        enum { ID = ID_MEMORY };

        virtual uint64_t Resident() const = 0;
        virtual uint64_t Allocated() const = 0;
        virtual uint64_t Shared() const = 0;
        virtual uint8_t Processes() const = 0;
        virtual const bool IsOperational() const = 0;
    };
}
}

