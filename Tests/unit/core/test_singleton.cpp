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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
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
    };

    class SingletonTypeTwo {
        public:
            SingletonTypeTwo(string)
            {
            }
            virtual ~SingletonTypeTwo()
            {
            }
    };
    class SingletonTypeThree {
        public:
            SingletonTypeThree (string, string)
            {
            }
            virtual ~SingletonTypeThree()
            {
            }
    };

    TEST(test_singleton, simple_singleton)
    {
        static SingletonTypeOne& object1 = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        static SingletonTypeOne& object_sample = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        EXPECT_EQ(&object1,&object_sample);
        static SingletonTypeTwo& object2 = ::Thunder::Core::SingletonType<SingletonTypeTwo>::Instance("SingletonTypeTwo");
        static SingletonTypeThree& object3 = ::Thunder::Core::SingletonType<SingletonTypeThree>::Instance("SingletonTypeThree","SingletonTypeThree");
        ::Thunder::Core::SingletonType<SingletonTypeTwo>* x = (::Thunder::Core::SingletonType<SingletonTypeTwo>*)&object2;
        EXPECT_STREQ(x->ImplementationName().c_str(),"SingletonTypeTwo");
        ::Thunder::Core::SingletonType<SingletonTypeThree>* y = (::Thunder::Core::SingletonType<SingletonTypeThree>*)&object3;
        EXPECT_STREQ(y->ImplementationName().c_str(),"SingletonTypeThree");
        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
