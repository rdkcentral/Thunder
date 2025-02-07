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

#include "Module.h"
#include "MessageStore.h"
#include "Messaging.h"
#include "IAssertionControl.h"

namespace Thunder {

namespace Core {
    class CriticalSection;
}

namespace Assertion {

    class EXTERNAL AssertionUnitProxy {
    public:
        AssertionUnitProxy(const AssertionUnitProxy&) = delete;
        AssertionUnitProxy& operator=(const AssertionUnitProxy&) = delete;
        AssertionUnitProxy(AssertionUnitProxy&&) = delete;
        AssertionUnitProxy& operator=(AssertionUnitProxy&&) = delete;

        ~AssertionUnitProxy();

        static AssertionUnitProxy& Instance();

        void Handle(IAssertionUnit* handler);
        void AssertionEvent(Core::Messaging::IStore::Assert& metadata, const Core::Messaging::TextMessage& message);

    protected:
        AssertionUnitProxy();

    private:
        IAssertionUnit* _handler;
        Core::CriticalSection* _adminLock;
    };

    class EXTERNAL BaseAssertType : public Core::Messaging::BaseCategoryType<Core::Messaging::Metadata::type::ASSERT> {
    public:
        using BaseClass = Core::Messaging::BaseCategoryType<Core::Messaging::Metadata::type::ASSERT>;

    private:
        class EXTERNAL AssertionControl : public Core::Messaging::IControl {
        public:
            AssertionControl(const AssertionControl&) = delete;
            AssertionControl& operator=(const AssertionControl&) = delete;
            AssertionControl(AssertionControl&&) = delete;
            AssertionControl& operator=(AssertionControl&&) = delete;

            AssertionControl()
                : _categoryName(EXPAND_AND_QUOTE(ASSERT_CATEGORY))
                , _enabled(0x03)
                , _metadata(Core::Messaging::Metadata::type::ASSERT, _categoryName, Core::Messaging::MODULE_ASSERT)
            {
                Core::Messaging::IControl::Announce(this);
            }
            ~AssertionControl() override
            {
                Destroy();
            }

        public:
            inline bool IsEnabled() const
            {
                return (_enabled & 0x01) != 0;
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
                    Core::Messaging::IControl::Revoke(this);
                    _enabled = 0;
                }
            }
            const Core::Messaging::Metadata& Metadata() const override
            {
                return (_metadata);
            }

        protected:
            const string _categoryName;
            uint8_t _enabled;
            Core::Messaging::Metadata _metadata;
        };
    
    public:
        BaseAssertType(const BaseAssertType&) = delete;
        BaseAssertType& operator=(const BaseAssertType&) = delete;
        BaseAssertType(BaseAssertType&&) = delete;
        BaseAssertType& operator=(BaseAssertType&&) = delete;

        BaseAssertType() = default;
        ~BaseAssertType() = default;

        using BaseClass::BaseClass;

        static AssertionControl& Instance() {
            static AssertionControl control;
            return (control);
        }

        inline static bool IsEnabled() {
            return (Instance().IsEnabled());
        }

        inline static void Enable(const bool enable) {
            Instance().Enable(enable);
        }
        
        inline static const Core::Messaging::Metadata& Metadata() {
            return (Instance().Metadata());
        }
    };

} // namespace Assertion
}

#define DEFINE_ASSERT_CATEGORY \
    DEFINE_MESSAGING_CATEGORY(Thunder::Assertion::BaseAssertType, ASSERT_CATEGORY)

#define ANNOUNCE_ASSERT_CONTROL \
    Thunder::Assertion::BaseAssertType::Instance();

namespace Thunder {
namespace Assertion {

    // Define upfront the general Assert category..
    DEFINE_ASSERT_CATEGORY

}
}