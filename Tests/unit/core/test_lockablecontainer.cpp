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

using namespace Thunder;
using namespace Thunder::Core;

class Container
{
    public:
        Container()
            : _length(0)
            , _data(nullptr)
        {
        }

        ~Container()
        {
        }

    private:
        int _length;
        int* _data;
};

TEST(test_lockableContainer, lockContainer_test)
{
    LockableContainerType<Container> containerObj1;
    LockableContainerType<Container> containerObj2(containerObj1);
    LockableContainerType<Container> containerObj3;
    containerObj3 = containerObj2;

    EXPECT_TRUE(containerObj1.ReadLock());
    containerObj1.ReadUnlock();
    EXPECT_TRUE(containerObj2.WriteLock());
    containerObj2.WriteUnlock();
}
