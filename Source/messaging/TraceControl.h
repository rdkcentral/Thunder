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

#if defined(_THUNDER_PRODUCTION) && defined(__CORE_MESSAGING__)

#define TRACE(CATEGORY, PARAMETERS)
#define TRACE_GLOBAL(CATEGORY, PARAMETERS)
#define TRACE_DURATION(CODE, ...)
#define TRACE_DURATION_GLOBAL(CODE, ...)

#elif defined(__CORE_MESSAGING__)

#define TRACE_CONTROL(CATEGORY) Thunder::Messaging::LocalLifetimeType<CATEGORY, &Thunder::Core::System::MODULE_NAME, Thunder::Core::Messaging::Metadata::type::TRACING>

#define TRACE_ENABLED(CATEGORY) TRACE_CONTROL(CATEGORY)::IsEnabled()

#define TRACE(CATEGORY, PARAMETERS)                                                     \
    do {                                                                                \
        static_assert(std::is_base_of<Thunder::Core::Messaging::BaseCategoryType<Thunder::Core::Messaging::Metadata::type::TRACING>, CATEGORY>::value, "TRACE() only for Tracing controls"); \
        using __control__ = TRACE_CONTROL(CATEGORY);                                    \
        if (__control__::IsEnabled() == true) {                                         \
            CATEGORY __data__ PARAMETERS;                                               \
            Thunder::Core::Messaging::MessageInfo __info__(                             \
                __control__::Metadata(),                                                \
                Thunder::Core::Time::Now().Ticks()                                      \
            );                                                                          \
            Thunder::Core::Messaging::IStore::Tracing __trace__(                        \
                __info__,                                                               \
                __FILE__,                                                               \
                __LINE__,                                                               \
                Thunder::Core::ClassNameOnly(typeid(*this).name()).Text()               \
            );                                                                          \
            Thunder::Core::Messaging::TextMessage __message__(__data__.Data());         \
            Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__, __control__::Routing());  \
        }                                                                               \
    } while(false)

#define TRACE_GLOBAL(CATEGORY, PARAMETERS)                                              \
    do {                                                                                \
        static_assert(std::is_base_of<Thunder::Core::Messaging::BaseCategoryType<Thunder::Core::Messaging::Metadata::type::TRACING>, CATEGORY>::value, "TRACE_GLOBAL() only for Tracing controls"); \
        using __control__ = TRACE_CONTROL(CATEGORY);                                    \
        if (__control__::IsEnabled() == true) {                                         \
            CATEGORY __data__ PARAMETERS;                                               \
            Thunder::Core::Messaging::MessageInfo __info__(                             \
                __control__::Metadata(),                                                \
                Thunder::Core::Time::Now().Ticks()                                      \
            );                                                                          \
            Thunder::Core::Messaging::IStore::Tracing __trace__(                        \
                __info__,                                                               \
                __FILE__,                                                               \
                __LINE__,                                                               \
                __FUNCTION__                                                            \
            );                                                                          \
            Thunder::Core::Messaging::TextMessage __message__(__data__.Data());         \
            Thunder::Messaging::MessageUnit::Instance().Push(__trace__, &__message__, __control__::Routing());  \
        }                                                                               \
    } while(false)

#define TRACE_DURATION(CODE, ...)                                                       \
    do {                                                                                \
        Thunder::Core::Time start = Thunder::Core::Time::Now();                         \
        { CODE }                                                                        \
        TRACE(Thunder::Trace::Duration, (start, ##__VA_ARGS__));                        \
    } while(false)

#define TRACE_DURATION_GLOBAL(CODE, ...)                                                \
    do {                                                                                \
        Thunder::Core::Time start = Thunder::Core::Time::Now();                         \
        { CODE }                                                                        \
        TRACE_GLOBAL(Thunder::Trace::Duration, (start, ##__VA_ARGS__));                 \
    } while(false)

#else

#define TRACE_ENABLED(CATEGORY) true

#define TRACE(CATEGORY, PARAMETERS)                                                                                                                                                          \
    do {                                                                                                                                                                                     \
        static_assert(std::is_base_of<Thunder::Core::Messaging::BaseCategoryType<Thunder::Core::Messaging::Metadata::type::TRACING>, CATEGORY>::value, "TRACE() only for Tracing controls"); \
        CATEGORY __data__ PARAMETERS;                                                                                                                                                        \
        TRACE_L1("%s: %s", Thunder::Core::ClassNameOnly(typeid(CATEGORY).name()).Text().c_str(), __data__.Data());                                                                           \
    } while(false)

#define TRACE_GLOBAL(CATEGORY, PARAMETERS) TRACE(CATEGORY, PARAMETERS)

#define TRACE_DURATION(CODE, ...)                                \
    do {                                                         \
        Thunder::Core::Time start = Thunder::Core::Time::Now();  \
        { CODE }                                                 \
        TRACE(Thunder::Trace::Duration, (start, ##__VA_ARGS__)); \
    } while(false)

#define TRACE_DURATION_GLOBAL(CODE, ...) TRACE_DURATION(CODE, ##__VA_ARGS__)

#endif // __CORE_MESSAGING__
