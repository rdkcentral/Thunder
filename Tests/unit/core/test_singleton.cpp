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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    class SingletonTypeOne {
        public:
            SingletonTypeOne()
            {
            }
            virtual ~SingletonTypeOne()
            {
            }
            bool operator==(const SingletonTypeOne&) const
            {
                return true;
            }
    };

    class SingletonTypeTwo {
        public:
            SingletonTypeTwo() = delete;
            explicit SingletonTypeTwo(string)
            {
            }
            virtual ~SingletonTypeTwo()
            {
            }
    };
    class SingletonTypeThree {
        public:
            SingletonTypeThree() = delete;
            explicit SingletonTypeThree (string, string)
            {
            }
            virtual ~SingletonTypeThree()
            {
            }
    };

    TEST(test_singleton, simple_singleton)
    {
        // 'old' use
        ::Thunder::Tests::Core::SingletonTypeOne& objectTypeOne = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();

        // Multiple inheritance, SingletonType has base SINGLETON, eg, ::Thunder::Tests::Core::SingletonTypeOne, and base Singleton
        // Internally a ::Thunder::Core::SingletonType<SINGLETON> pointer is contructed and via upcasted version of a base avaiable via Instance
        // It is safe to downcast it
        ASSERT_NE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&objectTypeOne), nullptr);
        EXPECT_FALSE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&objectTypeOne)->ImplementationName().empty());
        EXPECT_STREQ(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&objectTypeOne)->ImplementationName().c_str(), "SingletonTypeOne");

        // Well-known lifetime!
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Create("My custom 2-string");
        // Deprecated use
        ::Thunder::Tests::Core::SingletonTypeTwo& objectTypeTwo = ::Thunder::Core::SingletonType<SingletonTypeTwo>::Instance("My other custom 2-string");
        EXPECT_FALSE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeTwo>*>(&objectTypeTwo)->ImplementationName().empty());
        EXPECT_STREQ(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeTwo>*>(&objectTypeTwo)->ImplementationName().c_str(), "SingletonTypeTwo");
        EXPECT_TRUE(::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose());

        // Well-known lifetime!
        ::Thunder::Core::SingletonType<SingletonTypeThree>::Create("My first custom 3-string", "My second custom 3-string");
        ::Thunder::Tests::Core::SingletonTypeThree& objectTypeThree = ::Thunder::Core::SingletonType<SingletonTypeThree>::Instance("My other first custom 3-string", "My other second custom 3 string");
        EXPECT_FALSE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeThree>*>(&objectTypeThree)->ImplementationName().empty());
        EXPECT_STREQ(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeThree>*>(&objectTypeThree)->ImplementationName().c_str(), "SingletonTypeThree");
        EXPECT_TRUE(::Thunder::Core::SingletonType<SingletonTypeThree>::Dispose());

        // SingletonTypeOne has not yet been destroyed
        // Assume equality operator has been defined
        EXPECT_EQ(::Thunder::Core::SingletonType<SingletonTypeOne>::Instance(), ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance());

        // Keep going with good practise
        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
