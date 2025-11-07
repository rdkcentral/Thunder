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

#include <functional>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif
 
#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

class TestObjectString : public :: Thunder::Core::JSON::Container {
public :
    TestObjectString(const TestObjectString&) = delete;
    TestObjectString(TestObjectString&&) = delete;

    TestObjectString& operator=(const TestObjectString&) = delete;
    TestObjectString& operator=(TestObjectString&&) = delete;

    TestObjectString()
        : Model(true)
    {
        Add(_T("model"), &Model);
    }

    ~TestObjectString() override = default;

    ::Thunder::Core::JSON::String Model;
};


class TestOpaqueString : public :: Thunder::Core::JSON::Container {
public :
    TestOpaqueString(const TestOpaqueString&) = delete;
    TestOpaqueString(TestOpaqueString&&) = delete;

    TestOpaqueString& operator=(const TestOpaqueString&) = delete;
    TestOpaqueString& operator=(TestOpaqueString&&) = delete;

    TestOpaqueString()
        : Model(false)
    {
        Add(_T("model"), &Model);
    }

    ~TestOpaqueString() override = default;

    ::Thunder::Core::JSON::String Model;
};


TEST(JSONString, Failure)
{
    std::string textOut{};

    Thunder::Core::JSON::String opaque;

    opaque.FromString(R"json({"payload":"This is test\" message"})json");
    opaque.ToString(textOut);

    EXPECT_STREQ(textOut.c_str(), R"("{\"payload\":\"This is test\\\" message\"}")");
}

TEST(METROL_1201, PR1976)
{
    std::string json_single_begin  = R"({"model":"\"This is the single"})";
    std::string json_single_middle = R"({"model":"This is the \" single"})";
    std::string json_single_end    = R"({"model":"This is the single\""})";
    std::string json_multi_begin   = R"({"model":"\\\\\\\"This is the multi"})";
    std::string json_multi_middle  = R"({"model":"This is the \\\\\\\" multi"})";
    std::string json_multi_end     = R"({"model":"This is the multi\\\\\\\""})";
    std::string json_multi_faulthy = R"({"model":"This is \\\\\\" the failure"})";

    ::Thunder::Core::OptionalType<::Thunder::Core::JSON::Error> result;

    std::string textIn = R"json({   "payload"   :    "This is test\" message"})json";
    std::string textOut = "";

    ::Thunder::Core::JSON::String json;
    ::Thunder::Core::JSON::String opaque;

    // Text Conversion
    json.FromString(textIn, result);
    json.ToString(textOut);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(textOut.c_str(), R"("{\"payload\":\"This is test\\\" message\"}")");

    opaque.FromString(textIn, result);
    opaque.ToString(textOut);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(textOut.c_str(), R"("{\"payload\":\"This is test\\\" message\"}")");

    ::Thunder::Core::JSON::VariantContainer testVariant;

	  TestObjectString testObject;
    TestOpaqueString testOpaque;

    testVariant.FromString(json_single_begin, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("\"This is the single")");

    testObject.FromString(json_single_begin, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testObject.Model.Value().c_str(), R"("This is the single)");

    testOpaque.FromString(json_single_begin, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testOpaque.Model.Value().c_str(), R"("\"This is the single")");

    testVariant.Clear();
    testOpaque.Clear();
    testObject.Clear();

    testVariant.FromString(json_single_middle, result);

    EXPECT_FALSE(result.IsSet());

    testObject.FromString(json_single_middle, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testObject.Model.Value().c_str(), R"(This is the " single)");

    testOpaque.FromString(json_single_middle, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testOpaque.Model.Value().c_str(), R"("This is the \" single")");

    testVariant.Clear();
    testOpaque.Clear();
    testObject.Clear();

    testVariant.FromString(json_single_end, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("This is the single\"")");

    testObject.FromString(json_single_end, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testObject.Model.Value().c_str(), R"(This is the single")");

    testOpaque.FromString(json_single_end, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testOpaque.Model.Value().c_str(), R"("This is the single\"")");

    testVariant.Clear();
    testOpaque.Clear();
    testObject.Clear();

    testVariant.FromString(json_multi_begin, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("\\\\"This is the multi")");

    testObject.FromString(json_multi_begin, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testObject.Model.Value().c_str(), R"(\\\"This is the multi)");

    testOpaque.FromString(json_multi_begin, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testOpaque.Model.Value().c_str(), R"("\\\\"This is the multi")");

    testVariant.Clear();
    testOpaque.Clear();
    testObject.Clear();

    testVariant.FromString(json_multi_middle, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("This is the \\\\" multi")");

    testObject.FromString(json_multi_middle, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testObject.Model.Value().c_str(), R"(This is the \\\" multi)");

    testOpaque.FromString(json_multi_middle, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testOpaque.Model.Value().c_str(), R"("This is the \\\\" multi")");

    testVariant.Clear();
    testOpaque.Clear();
    testObject.Clear();

    testVariant.FromString(json_multi_end, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("This is the multi\\\\"")");

    testObject.FromString(json_multi_end, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testObject.Model.Value().c_str(), R"(This is the multi\\\")");

    testOpaque.FromString(json_multi_end, result);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testOpaque.Model.Value().c_str(), R"("This is the multi\\\\"")");

    testVariant.Clear();
    testOpaque.Clear();
    testObject.Clear();

	  // The following should fail
    // No error set

    testObject.FromString(json_multi_faulthy);

    EXPECT_FALSE(result.IsSet());
    EXPECT_STRNE(testObject.Model.Value().c_str(), R"(This is \\\\\" the failure)");
}

} } } // Thunder::Tests::Core
