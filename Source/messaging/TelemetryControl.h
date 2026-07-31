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
#ifdef __CORE_MESSAGING__
#include "Control.h"
#include "MessageUnit.h"
#endif

namespace Thunder {

namespace Telemetry {

#ifdef __CORE_MESSAGING__
    template <typename CATEGORY>
    class BaseTelemetryType : public Core::Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TELEMETRY> {
    public:
        using BaseClass = Core::Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TELEMETRY>;
        using Control = Messaging::ControlType<CATEGORY, &Core::System::MODULE_NAME, Core::Messaging::Metadata::type::TELEMETRY>;

        BaseTelemetryType(const BaseTelemetryType&) = delete;
        BaseTelemetryType& operator=(const BaseTelemetryType&) = delete;

        BaseTelemetryType() = default;
        ~BaseTelemetryType() = default;

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

        inline static Core::Messaging::OutputMode Routing() {
            return (_control.Routing());
        }

        inline static const Core::Messaging::Metadata& Metadata() {
            return (_control.Metadata());
        }

    private:
        static Control _control;
    };

    // Controls should not be exported out of defining modules...
    template <typename CATEGORY>
    EXTERNAL_HIDDEN typename BaseTelemetryType<CATEGORY>::Control
        BaseTelemetryType<CATEGORY>::_control(false);
#else
    inline void ConsoleTelemetry(const string& category, const string& value)
    {
        TRACE_L1("%s: %s", category.c_str(), value.c_str());
    }

    template <typename VALUE, std::enable_if_t<std::is_integral_v<std::decay_t<VALUE>> && std::is_signed_v<std::decay_t<VALUE>>, int> = 0>
    inline void ConsoleTelemetry(const string& category, const VALUE value)
    {
        TRACE_L1("%s: %lld", category.c_str(), static_cast<int64_t>(value));
    }

    template <typename VALUE, std::enable_if_t<std::is_integral_v<std::decay_t<VALUE>> && std::is_unsigned_v<std::decay_t<VALUE>>, int> = 0>
    inline void ConsoleTelemetry(const string& category, const VALUE value)
    {
        TRACE_L1("%s: %llu", category.c_str(), static_cast<uint64_t>(value));
    }

    template <typename VALUE, std::enable_if_t<std::is_floating_point_v<std::decay_t<VALUE>>, int> = 0>
    inline void ConsoleTelemetry(const string& category, const VALUE value)
    {
        TRACE_L1("%s: %g", category.c_str(), static_cast<double>(value));
    }

    template <typename CATEGORY>
    class BaseTelemetryType : public Core::Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TELEMETRY> {
    public:
        using BaseClass = Core::Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TELEMETRY>;

        BaseTelemetryType(const BaseTelemetryType&) = delete;
        BaseTelemetryType& operator=(const BaseTelemetryType&) = delete;

        BaseTelemetryType() = default;
        ~BaseTelemetryType() = default;

        using BaseClass::BaseClass;
    };
#endif

} // namespace Telemetry
}

#define DEFINE_TELEMETRY_CATEGORY(CATEGORY) \
    DEFINE_MESSAGING_CATEGORY(Thunder::Telemetry::BaseTelemetryType<CATEGORY>, CATEGORY)

#ifdef __CORE_MESSAGING__
#define TELEMETRY_ANNOUNCE(CATEGORY) \
    Thunder::Telemetry::BaseTelemetryType<CATEGORY>::Announce();

#define TELEMETRY(CATEGORY, VALUE)                                                                                                                  \
    do {                                                                                                                                            \
        static_assert(std::is_base_of<Thunder::Telemetry::BaseTelemetryType<CATEGORY>, CATEGORY>::value, "TELEMETRY() only for Telemetry controls");\
        if (CATEGORY::IsEnabled() == true) {                                                                                                        \
            Thunder::Core::Messaging::MessageInfo __info__(                                                                                         \
                CATEGORY::Metadata(),                                                                                                               \
                Thunder::Core::Time::Now().Ticks()                                                                                                  \
            );                                                                                                                                      \
            Thunder::Core::Messaging::IStore::Telemetry __telemetry__(__info__);                                                                    \
            Thunder::Core::Messaging::TelemetryMessage __message__(VALUE);                                                                          \
            Thunder::Messaging::MessageUnit::Instance().Push(__telemetry__, &__message__, CATEGORY::Routing());                                     \
        }                                                                                                                                           \
    } while(false)
#else
#define TELEMETRY_ANNOUNCE(CATEGORY)

#define TELEMETRY(CATEGORY, VALUE)                                                                                                                  \
    do {                                                                                                                                            \
        static_assert(std::is_base_of<Thunder::Telemetry::BaseTelemetryType<CATEGORY>, CATEGORY>::value, "TELEMETRY() only for Telemetry controls");\
        Thunder::Telemetry::ConsoleTelemetry(Thunder::Core::ClassNameOnly(typeid(CATEGORY).name()).Text(), VALUE);                                  \
    } while(false)
#endif
