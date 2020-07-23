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

namespace WPEFramework {
namespace Exchange {

    // This is an example to show the workings and how to develope a COMRPC/JSONRPC method/interface
    struct EXTERNAL IMath : virtual public Core::IUnknown {

        enum { ID = ID_MATH };

        virtual ~IMath() {}

        virtual uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const = 0;
        virtual uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const = 0;
    };
}
}
