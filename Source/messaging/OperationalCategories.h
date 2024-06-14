/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#include "BaseCategory.h"
#include "Control.h"

#ifdef __WINDOWS__

#define DEFINE_OPERATIONAL_CATEGORY(CATEGORY)                                                           \
    DEFINE_MESSAGING_CATEGORY(Thunder::OperationalStream::BaseOperationalType<CATEGORY>, CATEGORY)

#else

#define DEFINE_OPERATIONAL_CATEGORY(CATEGORY)                                                           \
    DEFINE_MESSAGING_CATEGORY(Thunder::OperationalStream::BaseOperationalType<CATEGORY>, CATEGORY) \
    template<>                                                                                          \
    EXTERNAL typename Thunder::OperationalStream::BaseOperationalType<CATEGORY>::Control           \
                      Thunder::OperationalStream::BaseOperationalType<CATEGORY>::_control;

#endif

#define OPERATIONAL_STREAM_ANNOUNCE(CATEGORY)                                                           \
        template<> Thunder::OperationalStream::BaseOperationalType<CATEGORY>::Control              \
        Thunder::OperationalStream::BaseOperationalType<CATEGORY>::_control(true)

namespace Thunder {

namespace OperationalStream {

    template <typename CATEGORY>
    class BaseOperationalType : public Messaging::BaseCategoryType<Core::Messaging::Metadata::type::OPERATIONAL_STREAM> {
    public:
        using BaseClass = Messaging::BaseCategoryType<Core::Messaging::Metadata::type::OPERATIONAL_STREAM>;
        using Control = Messaging::ControlType<CATEGORY, &Core::Messaging::MODULE_OPERATIONAL_STREAM, Core::Messaging::Metadata::type::OPERATIONAL_STREAM>;

        BaseOperationalType(const BaseOperationalType&) = delete;
        BaseOperationalType& operator=(const BaseOperationalType&) = delete;

        BaseOperationalType() = default;
        ~BaseOperationalType() = default;

        using BaseClass::BaseClass;

    public:
        inline static void Announce() {
            IsEnabled();
        }

        inline static bool IsEnabled() {
            return (_control.IsEnabled());
        }

        inline static void Enable(const bool enable) {
            _control.Enable(enable);
        }
        
        inline static const Core::Messaging::Metadata& Metadata() {
            return (_control.Metadata());
        }

    private:
        static Control _control;
    };

    DEFINE_OPERATIONAL_CATEGORY(StandardOut)
    DEFINE_OPERATIONAL_CATEGORY(StandardError)

}
}
