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
#include "MessageUnit.h"

namespace Thunder {

namespace Messaging {

    using MessageType = Core::Messaging::Metadata::type;

    template <typename CONTROLCATEGORY, const char** CONTROLMODULENAME, MessageType CONTROLTYPE>
    class ControlType : public Core::Messaging::IControl {
    public:
        ControlType() = delete;
        ControlType(const ControlType&) = delete;
        ControlType& operator=(const ControlType&) = delete;

        ControlType(const bool enabled)
            : _enabled(0x02 | (enabled ? 0x01 : 0x00))
            , _metaData(CONTROLTYPE, Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text(), *CONTROLMODULENAME) {
            // Register Our trace control unit, so it can be influenced from the outside
            // if nessecary..
            Core::Messaging::IControl::Announce(this);
        }
        ~ControlType() override {
            Destroy();
        }

    public:
        const Core::Messaging::Metadata& Metadata() const override {
            return (_metaData);
        }

        //non virtual method, so it can be called faster
        bool IsEnabled() const {
            return ((_enabled & 0x01) != 0);
        }

        bool Enable() const override {
            return (IsEnabled());
        }

        void Enable(const bool enabled) override {
            _enabled = (_enabled & 0xFE) | (enabled ? 0x01 : 0x00);
        }

        void Destroy() override
        {
            if ((_enabled & 0x02) != 0) {
                Core::Messaging::IControl::Revoke(this);
                _enabled = 0;
            }
        }

    private:
        uint8_t _enabled;
        Core::Messaging::Metadata _metaData;
    };

    template <typename CATEGORY, const char** MODULENAME, MessageType TYPE>
    class LocalLifetimeType {
    public:
        LocalLifetimeType(const LocalLifetimeType&) = delete;
        LocalLifetimeType& operator=(const LocalLifetimeType&) = delete;

        LocalLifetimeType() = default;
        ~LocalLifetimeType() = default;

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
        
        inline static const Core::Messaging::Metadata& Metadata() {
            return (_control.Metadata());
        }

    private:
        static ControlType<CATEGORY, MODULENAME, TYPE> _control;
    };

    // Controls should not be exported out of defining modules...
    template <typename CATEGORY, const char** MODULENAME, MessageType TYPE>
    EXTERNAL_HIDDEN ControlType<CATEGORY, MODULENAME, TYPE>
        LocalLifetimeType<CATEGORY, MODULENAME, TYPE>::_control(false);

} // namespace Messaging
}
