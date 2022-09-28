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

    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<Startup>,      Startup);
    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<Shutdown>,     Shutdown);
    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<Notification>, Notification);
    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<Error>,        Error);
    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<ParsingError>, ParsingError);
    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<Fatal>,        Fatal);
    DEFINE_MESSAGING_CATEGORY(BaseLoggingType<Crash>,        Crash);

} // namespace Logging
}