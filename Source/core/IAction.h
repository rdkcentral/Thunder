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

#ifndef __IACTION_H
#define __IACTION_H

#include "Module.h"
#include "Trace.h"
#include "TextFragment.h"

namespace Thunder {
namespace Core {
    template <typename ELEMENT>
    struct IDispatchType {
        virtual ~IDispatchType() = default;
        virtual void Dispatch(ELEMENT& element) = 0;
    };

    template <>
    struct IDispatchType<void> {
        virtual ~IDispatchType() = default;
        virtual void Dispatch() = 0;
    };

    struct EXTERNAL IDispatch : public IDispatchType<void> {
        ~IDispatch() override = default;

        virtual string Identifier() const {
            return (ClassName(typeid(*this).name()).Text());
        }
    };
}
} // namespace Core

#endif // __IACTION_H
