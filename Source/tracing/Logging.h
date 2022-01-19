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

#pragma once

#include "LoggingCategories.h"
#include "Module.h"
#include "TextMessage.h"

#include <stdarg.h>

namespace WPEFramework {
namespace Logging {

#define SYSLOG(CATEGORY, PARAMETERS)                                                                                       \
    if (WPEFramework::Logging::ControlLifetime<CATEGORY>::IsEnabled() == true) {                                           \
        CATEGORY __data__ PARAMETERS;                                                                                      \
        WPEFramework::Core::Messaging::Information __info__(WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING, \
            Core::ClassNameOnly(typeid(CATEGORY).name()).Text(),                                                           \
            _T("SysLog"),                                                                                                  \
            __FILE__,                                                                                                      \
            __LINE__,                                                                                                      \
            Core::Time::Now().Ticks());                                                                                    \
        WPEFramework::Messaging::TextMessage __message__(__data__.Data());                                                 \
        WPEFramework::Core::Messaging::MessageUnit::Instance().Push(__info__, &__message__);                               \
    }

    void EXTERNAL DumpException(const string& exceptionType);
    void EXTERNAL DumpSystemFiles(const Core::process_t pid);

    template <typename CATEGORY>
    class ControlLifetime {
    private:
        template <typename CONTROLCATEGORY>
        class Control : public Core::Messaging::IControl {
        public:
            Control(const Control<CONTROLCATEGORY>&) = delete;
            Control<CONTROLCATEGORY>& operator=(const Control<CONTROLCATEGORY>&) = delete;

            Control()
                : _enabled(0x02)
                , _metaData(Core::Messaging::MetaData::MessageType::LOGGING,
                      Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text(), _T("SysLog"))
            {
                // Register Our logging control unit, so it can be influenced from the outside
                // if nessecary..
                Core::Messaging::MessageUnit::Instance().Announce(this);
            }
            ~Control() override
            {
                Destroy();
            }

        public:
            const Core::Messaging::MetaData& MessageMetaData() const override
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
                    Core::Messaging::MessageUnit::Instance().Revoke(this);
                    _enabled = 0;
                }
            }

        private:
            uint8_t _enabled;
            Core::Messaging::MetaData _metaData;
        };

    public:
        ControlLifetime() = default;
        ~ControlLifetime() = default;
        ControlLifetime(const ControlLifetime<CATEGORY>&) = delete;
        ControlLifetime<CATEGORY>& operator=(const ControlLifetime<CATEGORY>&) = delete;

    public:
        inline static bool IsEnabled()
        {
            return (_messageControl.IsEnabled());
        }

    private:
        static Control<CATEGORY> _messageControl;
    };

    template <typename CATEGORY>
    EXTERNAL_HIDDEN typename ControlLifetime<CATEGORY>::template Control<CATEGORY> ControlLifetime<CATEGORY>::_messageControl;

}
} // namespace Logging
