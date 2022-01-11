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

#ifndef __TRACECONTROL_H
#define __TRACECONTROL_H

// ---- Include system wide include files ----
#include <inttypes.h>

// ---- Include local include files ----
#include "Module.h"
#include "TraceMessage.h"
// ---- Referenced classes and types ----

// ---- Helper types and constants ----
#define TRACE(CATEGORY, PARAMETERS)                                                                                      \
    if (WPEFramework::Trace::ControlLifetime<CATEGORY, &WPEFramework::Core::System::MODULE_NAME>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                                                    \
        WPEFramework::Core::MessageInformation __info__(WPEFramework::Core::MessageMetaData::MessageType::TRACING,       \
            Core::ClassNameOnly(typeid(CATEGORY).name()).Text(),                                                         \
            WPEFramework::Core::System::MODULE_NAME,                                                                     \
            __FILE__,                                                                                                    \
            __LINE__,                                                                                                    \
            Core::Time::Now().Ticks());                                                                                  \
        WPEFramework::Trace::TraceMessage __message__(__data__.Data());                                                  \
        WPEFramework::Core::MessageUnit::Instance().Push(__info__, &__message__);                                        \
    }

#define TRACE_GLOBAL(CATEGORY, PARAMETERS)                                                                               \
    if (WPEFramework::Trace::ControlLifetime<CATEGORY, &WPEFramework::Core::System::MODULE_NAME>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                                                    \
        WPEFramework::Core::MessageInformation __info__(WPEFramework::Core::MessageMetaData::MessageType::TRACING,       \
            __FUNCTION__,                                                                                                \
            WPEFramework::Core::System::MODULE_NAME,                                                                     \
            __FILE__,                                                                                                    \
            __LINE__,                                                                                                    \
            Core::Time::Now().Ticks());                                                                                  \
        WPEFramework::Trace::TraceMessage __message__(__data__.Data());                                                  \
        WPEFramework::Core::MessageUnit::Instance().Push(__info__, &__message__);                                        \
    }

#define TRACE_DURATION(CODE, ...)                                     \
    WPEFramework::Core::Time start = WPEFramework::Core::Time::Now(); \
    CODE                                                              \
        TRACE(WPEFramework::Trace::Duration, (start, ##__VA_ARGS__));

#define TRACE_DURATION_GLOBAL(CODE, ...)                              \
    WPEFramework::Core::Time start = WPEFramework::Core::Time::Now(); \
    CODE                                                              \
        TRACE_GLOBAL(WPEFramework::Trace::Duration, (start, ##__VA_ARGS__));
namespace WPEFramework {
namespace Trace {

    template <typename CATEGORY, const char** MODULENAME>
    class ControlLifetime {
    private:
        template <typename CONTROLCATEGORY, const char** CONTROLMODULE>
        class Control : public Core::IControl {
        public:
            Control(const Control<CONTROLCATEGORY, CONTROLMODULE>&) = delete;
            Control<CONTROLCATEGORY, CONTROLMODULE>& operator=(const Control<CONTROLCATEGORY, CONTROLMODULE>&) = delete;

            Control()
                : _enabled(0x02)
                , _metaData(Core::MessageMetaData::MessageType::TRACING,
                      Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text(), *CONTROLMODULE)
            {
                // Register Our trace control unit, so it can be influenced from the outside
                // if nessecary..
                Core::MessageUnit::Instance().Announce(this);


                bool isEnabled = Core::MessageUnit::Instance().IsControlEnabled(this);

                if (isEnabled) {
                    _enabled = _enabled | 0x01;
                }
            }
            ~Control() override
            {
                Destroy();
            }

        public:
            const Core::MessageMetaData& MetaData() const override
            {
                return _metaData;
            }

            //non virtual method, so it can be called faster
            inline bool IsEnabled() const
            {
                return ((_enabled & 0x01) != 0);
            }

            bool Enable() const override
            {
                return IsEnabled();
            }

            void Enable(const bool enabled) override
            {
                _enabled = (_enabled & 0xFE) | (enabled ? 0x01 : 0x00);
            }

            void Destroy() override
            {
                if ((_enabled & 0x02) != 0) {
                    // Register Our trace control unit, so it can be influenced from the outside
                    // if nessecary..
                    Core::MessageUnit::Instance().Revoke(this);
                    _enabled = 0;
                }
            }

        private:
            uint8_t _enabled;
            Core::MessageMetaData _metaData;
        };

    public:
        ControlLifetime() = default;
        ~ControlLifetime() = default;
        ControlLifetime(const ControlLifetime<CATEGORY, MODULENAME>&) = delete;
        ControlLifetime<CATEGORY, MODULENAME>& operator=(const ControlLifetime<CATEGORY, MODULENAME>&) = delete;

    public:
        inline static bool IsEnabled()
        {
            return (_messageControl.IsEnabled());
        }

    private:
        static Control<CATEGORY, MODULENAME> _messageControl;
    };

    template <typename CATEGORY, const char** MODULENAME>
    EXTERNAL_HIDDEN typename ControlLifetime<CATEGORY, MODULENAME>::template Control<CATEGORY, MODULENAME> ControlLifetime<CATEGORY, MODULENAME>::_messageControl;

}

} // namespace Trace

#endif // __TRACECONTROL_H
