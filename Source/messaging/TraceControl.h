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

#ifdef _THUNDER_PRODUCTION

#define TRACE_CONTROL(CATEGORY)
#define TRACE_ENABLED(CATEGORY)
#define TRACE(CATEGORY, PARAMETERS)
#define TRACE_GLOBAL(CATEGORY, PARAMETERS)
#define TRACE_DURATION(CODE, ...)
#define TRACE_DURATION_GLOBAL(CODE, ...)

#else // _THUNDER_PRODUCTION

#define TRACE_CONTROL(CATEGORY)                                                              \
    Thunder::Messaging::LocalLifetimeType<CATEGORY, &Thunder::Core::System::MODULE_NAME, Thunder::Core::Messaging::Metadata::type::TRACING>

#define TRACE_ENABLED(CATEGORY)                                                              \
    TRACE_CONTROL(CATEGORY)::IsEnabled()

#define TRACE(CATEGORY, PARAMETERS)                                                          \
    do {                                                                                     \
        using __control__ = TRACE_CONTROL(CATEGORY);                                         \
        if (__control__::IsEnabled() == true) {                                              \
            CATEGORY __data__ PARAMETERS;                                                    \
            Thunder::Core::Messaging::MessageInfo __info__(                             \
                __control__::Metadata(),                                                     \
                Thunder::Core::Time::Now().Ticks()                                      \
            );                                                                               \
            Thunder::Core::Messaging::IStore::Tracing __trace__(                        \
                __info__,                                                                    \
                __FILE__,                                                                    \
                __LINE__,                                                                    \
                Thunder::Core::ClassNameOnly(typeid(*this).name()).Text()               \
            );                                                                               \
            Thunder::Messaging::TextMessage __message__(__data__.Data());               \
            Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);  \
        }                                                                                    \
    } while(false)

#define TRACE_GLOBAL(CATEGORY, PARAMETERS)                                                   \
    do {                                                                                     \
        using __control__ = TRACE_CONTROL(CATEGORY);                                         \
        if (__control__::IsEnabled() == true) {                                              \
            CATEGORY __data__ PARAMETERS;                                                    \
            Thunder::Core::Messaging::MessageInfo __info__(                             \
                __control__::Metadata(),                                                     \
                Thunder::Core::Time::Now().Ticks()                                      \
            );                                                                               \
            Thunder::Core::Messaging::IStore::Tracing __trace__(                        \
                __info__,                                                                    \
                __FILE__,                                                                    \
                __LINE__,                                                                    \
                __FUNCTION__                                                                 \
            );                                                                               \
            Thunder::Messaging::TextMessage __message__(__data__.Data());               \
            Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__);  \
        }                                                                                    \
    } while(false)

#define TRACE_DURATION(CODE, ...)                                                            \
    do {                                                                                     \
        Thunder::Core::Time start = Thunder::Core::Time::Now();                    \
        { CODE }                                                                             \
        TRACE(Thunder::Trace::Duration, (start, ##__VA_ARGS__));                        \
    } while(false)

#define TRACE_DURATION_GLOBAL(CODE, ...)                                                     \
    do {                                                                                     \
        Thunder::Core::Time start = Thunder::Core::Time::Now();                    \
        { CODE }                                                                             \
        TRACE_GLOBAL(Thunder::Trace::Duration, (start, ##__VA_ARGS__));                 \
    } while(false)

#endif // _THUNDER_PRODUCTION
