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

#include "Control.h"
#include "Module.h"
#include "TextMessage.h"
#include <inttypes.h>

#define TRACE_ENABLED(CATEGORY)                                                                                                                                                             \
        WPEFramework::Messaging::ControlLifetime<CATEGORY, &WPEFramework::Core::System::MODULE_NAME, WPEFramework::Core::Messaging::MetaData::MessageType::TRACING>::IsEnabled()

#define TRACE(CATEGORY, PARAMETERS)                                                                                                                                                         \
    if (WPEFramework::Messaging::ControlLifetime<CATEGORY, &WPEFramework::Core::System::MODULE_NAME, WPEFramework::Core::Messaging::MetaData::MessageType::TRACING>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                                                                                                                       \
        WPEFramework::Core::Messaging::Information __info__(WPEFramework::Core::Messaging::MetaData::MessageType::TRACING,                                                                  \
            WPEFramework::Core::ClassNameOnly(typeid(CATEGORY).name()).Text(),                                                                                                              \
            WPEFramework::Core::System::MODULE_NAME,                                                                                                                                        \
            __FILE__,                                                                                                                                                                       \
            __LINE__,                                                                                                                                                                       \
            WPEFramework::Core::Time::Now().Ticks());                                                                                                                                       \
        WPEFramework::Messaging::TextMessage __message__(__data__.Data());                                                                                                                  \
        WPEFramework::Core::Messaging::MessageUnit::Instance().Push(__info__, &__message__);                                                                                                \
    }

#define TRACE_GLOBAL(CATEGORY, PARAMETERS)                                                                                                                                              \
    if (WPEFramework::Messaging::ControlLifetime<CATEGORY, &WPEFramework::Core::System::MODULE_NAME, WPEFramework::Core::Messaging::MetaData::MessageType::TRACING>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                                                                                                                   \
        WPEFramework::Core::Messaging::Information __info__(WPEFramework::Core::Messaging::MetaData::MessageType::TRACING,                                                              \
            __FUNCTION__,                                                                                                                                                               \
            WPEFramework::Core::System::MODULE_NAME,                                                                                                                                    \
            __FILE__,                                                                                                                                                                   \
            __LINE__,                                                                                                                                                                   \
            WPEFramework::Core::Time::Now().Ticks());                                                                                                                                                 \
        WPEFramework::Messaging::TextMessage __message__(__data__.Data());                                                                                                              \
        WPEFramework::Core::Messaging::MessageUnit::Instance().Push(__info__, &__message__);                                                                                            \
    }

#define TRACE_DURATION(CODE, ...)                                     \
    WPEFramework::Core::Time start = WPEFramework::Core::Time::Now(); \
    CODE                                                              \
        TRACE(WPEFramework::Trace::Duration, (start, ##__VA_ARGS__));

#define TRACE_DURATION_GLOBAL(CODE, ...)                              \
    WPEFramework::Core::Time start = WPEFramework::Core::Time::Now(); \
    CODE                                                              \
        TRACE_GLOBAL(WPEFramework::Trace::Duration, (start, ##__VA_ARGS__));
