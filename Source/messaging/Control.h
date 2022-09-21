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

    using MessageType = Core::Messaging::MessageType;

    template <typename CATEGORY, const char** MODULENAME, MessageType TYPE> class ControlLifetime;

    template <typename CATEGORY, const char** MODULENAME, MessageType TYPE>
    class ControlLifetimeBaseType {
    protected:
        template <typename CONTROLCATEGORY, const char** CONTROLMODULENAME, MessageType CONTROLTYPE>
        class ControlType : public Core::Messaging::IControl {
        public:
            ControlType(const ControlType&) = delete;
            ControlType& operator=(const ControlType&) = delete;

            ControlType()
                : _enabled(0x02)
                , _metaData(CONTROLTYPE, Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text(), *CONTROLMODULENAME)
            {
                // Register Our trace control unit, so it can be influenced from the outside
                // if nessecary..
                Core::Messaging::MessageUnit::Instance().Announce(this);
            }
            ~ControlType() override
            {
                Destroy();
            }

        public:
            const Core::Messaging::MetaData& MessageMetaData() const override
            {
                return (_metaData);
            }

            //non virtual method, so it can be called faster
            bool IsEnabled() const
            {
                return ((_enabled & 0x01) != 0);
            }

            bool Enable() const override
            {
                return (IsEnabled());
            }

            void Enable(const bool enabled) override
            {
                _enabled = (_enabled & 0xFE) | (enabled ? 0x01 : 0x00);
            }

            void Destroy() override
            {
                if ((_enabled & 0x02) != 0) {
                    Core::Messaging::MessageUnit::Instance().Revoke(this);
                    _enabled = 0;
                }
            }

        private:
            uint8_t _enabled;
            Core::Messaging::MetaData _metaData;
        };

    public:
        ControlLifetimeBaseType() = default;
        ~ControlLifetimeBaseType() = default;
        ControlLifetimeBaseType(const ControlLifetimeBaseType&) = delete;
        ControlLifetimeBaseType& operator=(const ControlLifetimeBaseType&) = delete;

    public:
        using ControlClass = ControlType<CATEGORY, MODULENAME, TYPE>;
        using ControlLifetimeClass = ControlLifetime<CATEGORY, MODULENAME, TYPE>;

        inline static void Announce()
        {
            IsEnabled();
        }
        inline static bool IsEnabled()
        {
            return (ControlLifetimeClass::_messageControl.IsEnabled());
        }
        inline static void Enable(const bool enable)
        {
            ControlLifetimeClass::_messageControl.Enable(enable);
        }
        inline static const Core::Messaging::MetaData& MetaData()
        {
            return (ControlLifetimeClass::_messageControl.MessageMetaData());
        }
    };

    template <typename CATEGORY, const char** MODULENAME, MessageType TYPE>
    class ControlLifetime : public ControlLifetimeBaseType<CATEGORY, MODULENAME, TYPE> {
    public:
        ControlLifetime() = default;
        ~ControlLifetime() = default;
        ControlLifetime(const ControlLifetime&) = delete;
        ControlLifetime& operator=(const ControlLifetime&) = delete;

        using ControlLifetimeBaseClass = ControlLifetimeBaseType<CATEGORY, MODULENAME, TYPE>;

    private:
        friend ControlLifetimeBaseClass;
        static typename ControlLifetimeBaseClass::ControlClass _messageControl;
    };

    template <typename CATEGORY, const char** MODULENAME>
    class ControlLifetime<CATEGORY, MODULENAME, MessageType::LOGGING>
        : public ControlLifetimeBaseType<CATEGORY, MODULENAME, MessageType::LOGGING> {
    public:
        ControlLifetime() = default;
        ~ControlLifetime() = default;
        ControlLifetime(const ControlLifetime&) = delete;
        ControlLifetime& operator=(const ControlLifetime&) = delete;

        using ControlLifetimeBaseClass = ControlLifetimeBaseType<CATEGORY, MODULENAME, MessageType::LOGGING>;

    private:
        friend ControlLifetimeBaseClass;
        static typename ControlLifetimeBaseClass::ControlClass _messageControl;
    };

    // Controls should not be exported out of defining modules...
    template <typename CATEGORY, const char** MODULENAME, MessageType TYPE>
    EXTERNAL_HIDDEN typename ControlLifetime<CATEGORY, MODULENAME, TYPE>::ControlLifetimeBaseClass::ControlClass
    ControlLifetime<CATEGORY, MODULENAME, TYPE>::_messageControl;

    // ...but logging controls have to be visible outside of the Messaging lib
    template <typename CATEGORY, const char** MODULENAME>
    EXTERNAL typename ControlLifetime<CATEGORY, MODULENAME, MessageType::LOGGING>::ControlLifetimeBaseClass::ControlClass
    ControlLifetime<CATEGORY, MODULENAME, MessageType::LOGGING>::_messageControl;

} // namespace Messaging
}
