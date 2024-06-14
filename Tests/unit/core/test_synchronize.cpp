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

class SynchronizeClass {
public:
    SynchronizeClass()
        : _msg("")
    {
    }

    ~SynchronizeClass()
    {
    }

public:
    bool Copy(const string message)
    {
        bool result = false;
        _msg = message;
        if (!_msg.empty())
            result = true;
        return result;
    }

private:
    string _msg;
};

TEST(test_synchronize, synchronize_test)
{
    string MESSAGE = "SynchronizeType request";
    SynchronizeClass syncObject1;
    SynchronizeType<string> syncObject2;
    SynchronizeType<SynchronizeClass> syncObject3;

    syncObject2.Load();
    syncObject2.Evaluate();

    syncObject3.Load(syncObject1);
    EXPECT_EQ(syncObject3.Acquire(unsigned(5)), unsigned(11));
    syncObject3.Load(syncObject1);
    EXPECT_TRUE(syncObject3.Evaluate<string>(MESSAGE));

    syncObject2.Lock();
    syncObject2.Flush();
    syncObject2.Unlock();

    syncObject3.Lock();
    syncObject3.Flush();
    syncObject3.Unlock();
}
