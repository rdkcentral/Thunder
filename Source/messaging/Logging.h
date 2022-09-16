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
#include "Control.h"
#include "LoggingCategories.h"
#include "TextMessage.h"

namespace WPEFramework {
namespace Logging {

    extern EXTERNAL const char* MODULE_LOGGING;

    void EXTERNAL DumpException(const string& exceptionType);
    void EXTERNAL DumpSystemFiles(const Core::process_t pid);

} // namespace Logging
}

#define SYSLOG_CONTROL(CATEGORY) \
    WPEFramework::Messaging::ControlLifetime<CATEGORY, &WPEFramework::Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>

#define SYSLOG_ANNOUNCE(CATEGORY) \
    SYSLOG_CONTROL(CATEGORY)::Announce()

#define SYSLOG_ENABLED(CATEGORY) \
    SYSLOG_CONTROL(CATEGORY)::IsEnabled()

#define SYSLOG(CATEGORY, PARAMETERS) \
    do {                                                                                                                                              \
        if (SYSLOG_ENABLED(CATEGORY) == true) {                                                                                                       \
            static_assert(std::is_base_of<WPEFramework::Logging::BaseCategory, CATEGORY>::value, "SYSLOG() only for Logging controls");               \
            CATEGORY __data__ PARAMETERS;                                                                                                             \
            WPEFramework::Core::Messaging::Information __info__(WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING,                        \
                WPEFramework::Core::ClassNameOnly(typeid(CATEGORY).name()).Text(),                                                                    \
                WPEFramework::Logging::MODULE_LOGGING,                                                                                                \
                __FILE__,                                                                                                                             \
                __LINE__,                                                                                                                             \
                typeid(*this).name(),                                                                                                                 \
                WPEFramework::Core::Time::Now().Ticks());                                                                                             \
            WPEFramework::Messaging::TextMessage __message__(__data__.Data());                                                                        \
            WPEFramework::Core::Messaging::MessageUnit::Instance().Push(__info__, &__message__);                                                      \
        }                                                                                                                                             \
    } while(false)

#define SYSLOG_GLOBAL(CATEGORY, PARAMETERS) \
    do {                                                                                                                                              \
        if (SYSLOG_ENABLED(CATEGORY) == true) {                                                                                                       \
            static_assert(std::is_base_of<WPEFramework::Logging::BaseCategory, CATEGORY>::value, "SYSLOG() only for Logging controls");               \
            CATEGORY __data__ PARAMETERS;                                                                                                             \
            WPEFramework::Core::Messaging::Information __info__(WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING,                        \
                WPEFramework::Core::ClassNameOnly(typeid(CATEGORY).name()).Text(),                                                                    \
                WPEFramework::Logging::MODULE_LOGGING,                                                                                                \
                __FILE__,                                                                                                                             \
                __LINE__,                                                                                                                             \
                __FUNCTION__,                                                                                                                         \
                WPEFramework::Core::Time::Now().Ticks());                                                                                             \
            WPEFramework::Messaging::TextMessage __message__(__data__.Data());                                                                        \
            WPEFramework::Core::Messaging::MessageUnit::Instance().Push(__info__, &__message__);                                                      \
        }                                                                                                                                             \
    } while(false)
