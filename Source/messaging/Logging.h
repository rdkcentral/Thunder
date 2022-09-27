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
#include "TextMessage.h"

namespace WPEFramework {
namespace Logging {

    extern EXTERNAL const char* MODULE_LOGGING;

    void EXTERNAL DumpException(const string& exceptionType);
    void EXTERNAL DumpSystemFiles(const Core::process_t pid);

} // namespace Logging
}

#define SYSLOG_CONTROL(CATEGORY) \
    WPEFramework::Messaging::GlobalLifetimeType<CATEGORY,&WPEFramework::Logging::MODULE_LOGGING,WPEFramework::Messaging::MessageType::LOGGING>

#define SYSLOG_ANNOUNCE(CATEGORY) \
    SYSLOG_CONTROL(CATEGORY)::Announce()

#define SYSLOG_ENABLED(CATEGORY) \
    SYSLOG_CONTROL(CATEGORY)::IsEnabled()

#define _SYSLOG_INTERNAL(CATEGORY, CLASSNAME, PARAMETERS) \
    do {                                                                                                                                     \
        using __control__ = SYSLOG_CONTROL(CATEGORY);                                                                                        \
        if (__control__::IsEnabled() == true) {                                                                                              \
            static_assert(CATEGORY::Type == WPEFramework::Core::Messaging::MessageType::LOGGING, "SYSLOG() only for Logging controls");      \
            CATEGORY __data__ PARAMETERS;                                                                                                    \
            WPEFramework::Core::Messaging::Information __info__(                                                                             \
                __control__::MetaData(),                                                                                                     \
                __FILE__,                                                                                                                    \
                __LINE__,                                                                                                                    \
                (CLASSNAME),                                                                                                                 \
                WPEFramework::Core::Time::Now().Ticks()                                                                                      \
            );                                                                                                                               \
            WPEFramework::Messaging::TextMessage __message__(__data__.Data());                                                               \
            WPEFramework::Core::Messaging::MessageUnit::Instance().Push(__info__, &__message__);                                             \
        }                                                                                                                                    \
    } while(false)

#define SYSLOG(CATEGORY, PARAMETERS) _SYSLOG_INTERNAL(CATEGORY, typeid(*this).name(), PARAMETERS)
#define SYSLOG_GLOBAL(CATEGORY, PARAMETERS) _SYSLOG_INTERNAL(CATEGORY, __FUNCTION__, PARAMETERS)
