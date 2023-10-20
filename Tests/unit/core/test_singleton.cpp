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

using namespace WPEFramework;
using namespace WPEFramework::Core;

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
        SingletonTypeTwo()
        {
        }
        virtual ~SingletonTypeTwo()
        {
        }
};
class SingletonTypeThree {
    public:
        SingletonTypeThree ()
        {
        }
        virtual ~SingletonTypeThree()
        {
        }
};

TEST(test_singleton, simple_singleton)
{
    static SingletonTypeOne& object1 = SingletonType<SingletonTypeOne>::Instance();
    static SingletonTypeOne& object_sample = SingletonType<SingletonTypeOne>::Instance();
    EXPECT_EQ(&object1,&object_sample);

    static SingletonTypeTwo& object2 = SingletonType<SingletonTypeTwo>::Instance();
    static SingletonTypeThree& object3 = SingletonType<SingletonTypeThree>::Instance();
    EXPECT_NE(static_cast<void*>(&object2), static_cast<void*>(&object3));

    SingletonType<SingletonTypeTwo>* x = static_cast<SingletonType<SingletonTypeTwo>*>(&object2);
    SingletonType<SingletonTypeThree>* y = static_cast<SingletonType<SingletonTypeThree>*>(&object3);
    // Implementation defined names
    EXPECT_STRNE(x->ImplementationName().c_str(), y->ImplementationName().c_str());

    Singleton::Dispose();
}
