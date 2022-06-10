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
#include "TextMessage.h"

namespace WPEFramework {
namespace Messaging {

    template <typename CATEGORY, const char** MODULENAME, WPEFramework::Core::Messaging::MetaData::MessageType TYPE>
    class ControlLifetime {
    private:
        template <typename CONTROLCATEGORY, const char** CONTROLMODULE, Core::Messaging::MetaData::MessageType CONTROLTYPE>
        class Control : public Core::Messaging::IControl {
        public:
            Control(const Control<CONTROLCATEGORY, CONTROLMODULE, CONTROLTYPE>&) = delete;
            Control<CONTROLCATEGORY, CONTROLMODULE, CONTROLTYPE>& operator=(const Control<CONTROLCATEGORY, CONTROLMODULE, CONTROLTYPE>&) = delete;

            Control()
                : _enabled(0x02)
                , _metaData(CONTROLTYPE,
                      Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text(), *CONTROLMODULE)
            {
                // Register Our trace control unit, so it can be influenced from the outside
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
        ControlLifetime(const ControlLifetime<CATEGORY, MODULENAME, TYPE>&) = delete;
        ControlLifetime<CATEGORY, MODULENAME, TYPE>& operator=(const ControlLifetime<CATEGORY, MODULENAME, TYPE>&) = delete;

    public:
        inline static bool IsEnabled()
        {
            return (_messageControl.IsEnabled());
        }
        inline static void Enable(const bool enable)
        {
            _messageControl.Enable(enable);
        }

    private:
        static Control<CATEGORY, MODULENAME, TYPE> _messageControl;
    };

    template <typename CATEGORY, const char** MODULENAME, WPEFramework::Core::Messaging::MetaData::MessageType TYPE>
    typename ControlLifetime<CATEGORY, MODULENAME, TYPE>::template Control<CATEGORY, MODULENAME, TYPE> ControlLifetime<CATEGORY, MODULENAME, TYPE>::_messageControl;
}

}
