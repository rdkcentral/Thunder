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
#include <utility>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace WPEFramework {
namespace Tests {
namespace Core {

    using JsonArray = ::WPEFramework::Core::JSON::ArrayType<::WPEFramework::Core::JSON::String>;

    TEST(JSONARRAY, InsertEmptyAtHead)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("middle");
        array.Add() = _T("tail");
        array.Insert(0) = _T("head");

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("head", array[0].Value().c_str());
        EXPECT_STREQ("middle", array[1].Value().c_str());
        EXPECT_STREQ("tail", array[2].Value().c_str());

        array.ToString(serialized);
        EXPECT_EQ(R"(["head","middle","tail"])", serialized);
    }

    TEST(JSONARRAY, InsertAtHead)
    {
        JsonArray array;
        ::WPEFramework::Core::JSON::String head;

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
        ::WPEFramework::Core::JSON::String three;

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
        ::WPEFramework::Core::JSON::String center;

        array.Add() = _T("left");
        array.Add() = _T("right");
        center = _T("center");

        array.Insert(1, center);

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("left", array[0].Value().c_str());
        EXPECT_STREQ("center", array[1].Value().c_str());
        EXPECT_STREQ("right", array[2].Value().c_str());
    }

    TEST(JSONARRAY, InsertEmptyMidAndSerialize)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("A");
        array.Add() = _T("C");
        array.Insert(1) = _T("B");

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("B", array[1].Value().c_str());

        array.ToString(serialized);
        EXPECT_EQ(R"(["A","B","C"])", serialized);
    }

    TEST(JSONARRAY, InsertRvalueAtHead)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("B");
        array.Add() = _T("C");

        ::WPEFramework::Core::JSON::String element;
        element = _T("A");
        array.Insert(0, std::move(element));

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("A", array[0].Value().c_str());
        EXPECT_STREQ("B", array[1].Value().c_str());
        EXPECT_STREQ("C", array[2].Value().c_str());

        array.ToString(serialized);
        EXPECT_EQ(R"(["A","B","C"])", serialized);
    }

    TEST(JSONARRAY, InsertRvalueMidAndSerialize)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("first");
        array.Add() = _T("third");

        ::WPEFramework::Core::JSON::String element;
        element = _T("second");
        array.Insert(1, std::move(element));

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("second", array[1].Value().c_str());

        array.ToString(serialized);
        EXPECT_EQ(R"(["first","second","third"])", serialized);
    }

    TEST(JSONARRAY, InsertEmptyAtTailAndSerialize)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("X");
        array.Add() = _T("Y");
        array.Insert(array.Length()) = _T("Z");

        EXPECT_EQ(3u, array.Length());
        EXPECT_STREQ("Z", array[2].Value().c_str());

        array.ToString(serialized);
        EXPECT_EQ(R"(["X","Y","Z"])", serialized);
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
        ::WPEFramework::Core::JSON::String beta;

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

    TEST(JSONARRAY, RemoveReturnsNextElement)
    {
        JsonArray array;
        std::string serialized;

        array.Add() = _T("A");
        array.Add() = _T("B");
        array.Add() = _T("C");

        ::WPEFramework::Core::JSON::String* next = array.Remove(0);
        ASSERT_NE(nullptr, next);
        EXPECT_STREQ("B", next->Value().c_str());

        next = array.Remove(1);
        EXPECT_EQ(nullptr, next);

        EXPECT_EQ(1u, array.Length());
        EXPECT_STREQ("B", array[0].Value().c_str());

        array.ToString(serialized);
        EXPECT_EQ(R"(["B"])", serialized);
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

        ::WPEFramework::Core::JSON::String moved = array[0];
        array.Remove(0);
        array.Insert(1, moved);

        array.ToString(serialized);

        EXPECT_EQ(3u, array.Length());
        EXPECT_EQ(R"(["second","first","third"])", serialized);
    }

} // Core
} // Tests
} // WPEFramework
