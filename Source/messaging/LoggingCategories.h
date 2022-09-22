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

namespace WPEFramework {
namespace Logging {

    class BaseCategory : public Trace::BaseCategory {
    public:
        BaseCategory() = delete;
        ~BaseCategory() = default;
        BaseCategory& operator=(const BaseCategory&) = delete;
        using Trace::BaseCategory::BaseCategory;
        static constexpr bool isLoggingCategory = true;
    };

#define DEFINE_LOGGING_CATEGORY(CATEGORY) _INTERNAL_DEFINE_CATEGORY(BaseCategory, CATEGORY)

    DEFINE_LOGGING_CATEGORY(Startup);
    DEFINE_LOGGING_CATEGORY(Shutdown);
    DEFINE_LOGGING_CATEGORY(Notification);
    DEFINE_LOGGING_CATEGORY(Error);
    DEFINE_LOGGING_CATEGORY(ParsingError);
    DEFINE_LOGGING_CATEGORY(Fatal);
    DEFINE_LOGGING_CATEGORY(Crash);

#undef DEFINE_LOGGING_CATEGORY

} // namespace Logging
}