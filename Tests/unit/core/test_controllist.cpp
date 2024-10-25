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

#include <algorithm>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include <messaging/messaging.h>

#define PRINT_MODULES(NAME, MODULES) {std::cout << NAME << " ---> "; std::for_each(MODULES.begin(), MODULES.end(), [](std::string MODULE){std::cout << " " << MODULE;}); std::cout << std::endl;};

namespace Thunder {
namespace Tests {
namespace Core {

    class Control : public ::Thunder::Core::Messaging::IControl {
    public :

        class Handler : public ::Thunder::Core::Messaging::IControl::IHandler {
        public :

            Handler() = default;
            ~Handler() = default;
            void Handle(IControl* element) override
            {
                element->Enable();
            }
        };
    
        Control() = delete;

        Control(const ::Thunder::Core::Messaging::Metadata& metaData)
            : _metaData{ metaData }
            , _enabled{ false }
        {
        }

        ~Control() override = default;

        void Enable(bool enable) override
        {
            _enabled = enable;
        }

        bool Enable() const override
        {
            return _enabled;
        }

        void Destroy() override
        {
            Enable(false);
        }

        const ::Thunder::Core::Messaging::Metadata& Metadata() const override
        {
            return _metaData;
        }

    private :

        ::Thunder::Core::Messaging::Metadata _metaData;
        bool _enabled;
    };


    // ACTIVATE ONLY ONE TEST AT A TIME !


    TEST(Core_Controllist, DISABLED_WrongUseButAllowed)
    {
        const string basePath{ _T("/tmp/") };
        constexpr bool background{ false };
        const ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        constexpr ::Thunder::Messaging::MessageUnit::flush flushMode { ::Thunder::Messaging::MessageUnit::OFF };

        ::Thunder::Messaging::MessageUnit::Instance().Open(basePath, configuration, background, flushMode);

        const ::Thunder::Core::Messaging::Metadata toBeAdded(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

        Control::Handler handler;

        Control control(toBeAdded);

        ::Thunder::Core::Messaging::IControl::Announce(&control);

        ::Thunder::Core::Messaging::IControl::Iterate(handler);

//        ::Thunder::Core::Messaging::IControl::Iterate(handler);

        ::Thunder::Messaging::MessageUnit::Instance().Close();

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_Controllist, DISABLED_PossiblePureVirtualMethodCalled)
    {
        const string basePath{ _T("/tmp/") };
        constexpr bool background{ false };
        const ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        constexpr ::Thunder::Messaging::MessageUnit::flush flushMode { ::Thunder::Messaging::MessageUnit::OFF };

        ::Thunder::Messaging::MessageUnit::Instance().Open(basePath, configuration, background, flushMode);

        const ::Thunder::Core::Messaging::Metadata toBeAdded(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

        Control::Handler handler;

        {
            Control control(toBeAdded);

            ::Thunder::Core::Messaging::IControl::Announce(&control);
            ::Thunder::Core::Messaging::IControl::Iterate(handler);
//            ::Thunder::Core::Messaging::IControl::Revoke(&control);
        }

        // The scope triggers a pure virtual method called message  - at Close() (MessageUnit.cpp) if Revoke() is not executed - which is something that could happen as the behavior of the pointer after the scope is undefined
        // It is up to the compiler. It might well have triggert a segfault.

        ::Thunder::Messaging::MessageUnit::Instance().Close();

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_Controllist, DISABLED_PossibleSegfault)
    {
        const string basePath{ _T("/tmp/") };
        constexpr bool background{ false };
        const ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        constexpr ::Thunder::Messaging::MessageUnit::flush flushMode { ::Thunder::Messaging::MessageUnit::OFF };

//        ::Thunder::Messaging::MessageUnit::Instance().Open(basePath, configuration, background, flushMode);

        const ::Thunder::Core::Messaging::Metadata toBeAdded(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

        Control::Handler handler;

        {
            Control control(toBeAdded);

            ::Thunder::Core::Messaging::IControl::Announce(&control);

            ::Thunder::Core::Messaging::IControl::Iterate(handler);

//            ::Thunder::Core::Messaging::IControl::Revoke(&control);
        }

        // The scope triggers a segfault in ~Controls (MessageStore.cpp) at program end when statics are destroyed

//        ::Thunder::Messaging::MessageUnit::Instance().Close();

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
