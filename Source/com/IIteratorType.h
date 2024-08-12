/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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
#include <core/core.h>

namespace Thunder {

namespace RPC {

    // @iterator
    template<typename ELEMENT, const uint32_t INTERFACE_ID>
    struct IIteratorType : virtual public Core::IUnknown {

        typedef ELEMENT Element;

        enum { ID = INTERFACE_ID };

        ~IIteratorType() override = default;

        virtual bool Next(ELEMENT& info /* @out */) = 0;
        virtual bool Previous(ELEMENT& info /* @out */) = 0;
        virtual void Reset(const uint32_t position) = 0;
        virtual bool IsValid() const = 0;
        virtual uint32_t Count() const = 0;
        virtual ELEMENT Current() const = 0;
    };
}
}
