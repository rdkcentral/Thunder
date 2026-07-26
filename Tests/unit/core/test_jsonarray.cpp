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

#include <string>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    using JsonArray = ::Thunder::Core::JSON::ArrayType<::Thunder::Core::JSON::String>;

    TEST(JSONARRAY, InsertAtHead)
    {
        JsonArray array;
        ::Thunder::Core::JSON::String head;

        array.Add() = _T("middle");
        array.Add() = _T("tail");
        head = _T("head");

        array.Insert(0, head);

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("head", array[0].Value().c_str());
        EXPECT_STREQ("middle", array[1].Value().c_str());
        EXPECT_STREQ("tail", array[2].Value().c_str());
    }

    TEST(JSONARRAY, InsertAtTail)
    {
        JsonArray array;
        ::Thunder::Core::JSON::String three;

        array.Add() = _T("one");
        array.Add() = _T("two");
        three = _T("three");

        array.Insert(array.Length(), three);

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("one", array[0].Value().c_str());
        EXPECT_STREQ("two", array[1].Value().c_str());
        EXPECT_STREQ("three", array[2].Value().c_str());
    }

    TEST(JSONARRAY, InsertMidArray)
    {
        JsonArray array;
        ::Thunder::Core::JSON::String center;

        array.Add() = _T("left");
        array.Add() = _T("right");
        center = _T("center");

        array.Insert(1, center);

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("left", array[0].Value().c_str());
        EXPECT_STREQ("center", array[1].Value().c_str());
        EXPECT_STREQ("right", array[2].Value().c_str());
    }

    TEST(JSONARRAY, AppendAndCheckLength)
    {
        JsonArray array;

        array.Add() = _T("A");
        array.Add() = _T("B");
        array.Add() = _T("C");

        EXPECT_EQ(3u, array.Length());
    }

    TEST(JSONARRAY, RemoveAtHead)
    {
        JsonArray array;

        array.Add() = _T("head");
        array.Add() = _T("middle");
        array.Add() = _T("tail");

        array.Remove(0);

        EXPECT_EQ(2u, array.Length());
        EXPECT_STREQ("middle", array[0].Value().c_str());
        EXPECT_STREQ("tail", array[1].Value().c_str());
    }

    TEST(JSONARRAY, RemoveAtTail)
    {
        JsonArray array;

        array.Add() = _T("head");
        array.Add() = _T("middle");
        array.Add() = _T("tail");

        array.Remove(array.Length() - 1);

        EXPECT_EQ(2u, array.Length());
        EXPECT_STREQ("head", array[0].Value().c_str());
        EXPECT_STREQ("middle", array[1].Value().c_str());
    }

    TEST(JSONARRAY, RemoveMidArray)
    {
        JsonArray array;

        array.Add() = _T("left");
        array.Add() = _T("center");
        array.Add() = _T("right");

        array.Remove(1);

        EXPECT_EQ(2u, array.Length());
        EXPECT_STREQ("left", array[0].Value().c_str());
        EXPECT_STREQ("right", array[1].Value().c_str());
    }

    TEST(JSONARRAY, SerializeAfterMixedMutations)
    {
        JsonArray array;
        std::string serialized;
        ::Thunder::Core::JSON::String beta;

        array.Add() = _T("alpha");
        array.Add() = _T("gamma");
        beta = _T("beta");

        array.Insert(1, beta);
        array.Remove(0);

        array.Add() = _T("delta");

        array.ToString(serialized);

        EXPECT_EQ(3u, array.Length());
        EXPECT_EQ(R"(["beta","gamma","delta"])", serialized);
    }

    TEST(JSONARRAY, RemoveAllElements)
    {
        JsonArray array;

        array.Add() = _T("first");
        array.Add() = _T("second");
        array.Add() = _T("third");

        array.Remove(0);
        array.Remove(0);
        array.Remove(0);

        EXPECT_EQ(0u, array.Length());
        EXPECT_FALSE(array.IsSet());
    }

    TEST(JSONARRAY, ClearAfterRemove)
    {
        JsonArray array;

        array.Add() = _T("first");
        array.Add() = _T("second");
        array.Add() = _T("third");

        array.Remove(1);
        array.Clear();

        EXPECT_EQ(0u, array.Length());
        EXPECT_FALSE(array.IsSet());
    }

    TEST(JSONARRAY, SerializeAfterRemove)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("A");
        array.Add() = _T("B");
        array.Add() = _T("C");

        array.Remove(1);
        array.ToString(serialized);

        EXPECT_EQ(2u, array.Length());
        EXPECT_EQ(R"(["A","C"])", serialized);
    }

    TEST(JSONARRAY, AddAfterRemove)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("A");
        array.Add() = _T("B");
        array.Add() = _T("C");

        array.Remove(1);
        array.Add() = _T("D");
        array.ToString(serialized);

        EXPECT_EQ(3u, array.Length());
        EXPECT_EQ(R"(["A","C","D"])", serialized);
    }

    TEST(JSONARRAY, AddOperatorIndexAndGetRegression)
    {
        JsonArray array;

        array.Add() = _T("A");
        array.Add() = _T("B");
        array.Add() = _T("C");

        EXPECT_STREQ("A", array[0].Value().c_str());
        EXPECT_STREQ("B", array.Get(1).Value().c_str());
        EXPECT_STREQ("C", array[2].Value().c_str());
    }

    TEST(JSONARRAY, MoveThroughInsertAndRemove)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("first");
        array.Add() = _T("second");
        array.Add() = _T("third");

        ::Thunder::Core::JSON::String moved = array[0];
        array.Remove(0);
        array.Insert(1, moved);

        array.ToString(serialized);

        EXPECT_EQ(3u, array.Length());
        EXPECT_EQ(R"(["second","first","third"])", serialized);
    }

} // Core
} // Tests
} // Thunder