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

    printf("------------------------------------------------------------------------------------------------\n");
    // Print the strings
    printf("String  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", textIn.c_str());
    printf("Reverse      => [%s]\n", textOut.c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(textOut.c_str(), R"("{\"payload\":\"This is test\\\" message\"}")");

    printf("------------------------------------------------------------------------------------------------\n");
    opaque.FromString(textIn, result);
    opaque.ToString(textOut);
    printf("String  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", textIn.c_str());
    printf("Reverse      => [%s]\n", textOut.c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(textOut.c_str(), R"("{\"payload\":\"This is test\\\" message\"}")");

    ::Thunder::Core::JSON::VariantContainer testVariant;
//	  TestObjectString testObject;
//    TestOpaqueString testOpaque;

    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.FromString(json_single_begin, result);
    printf("Variant [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testVariant[_T("model")].Value().c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("\"This is the single")");

//    testObject.FromString(json_single_begin, result);
//    printf("Object  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
//    testOpaque.FromString(json_single_begin, result);
//    printf("Opaque  <%s> => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testOpaque.Model.Value().c_str());

    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.Clear();
//    testOpaque.Clear();
//    testObject.Clear();
    testVariant.FromString(json_single_middle, result);

    EXPECT_FALSE(result.IsSet());

//    testObject.FromString(json_single_middle, result);
//    printf("Object  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
//    testOpaque.FromString(json_single_middle, result);
//    printf("Opaque  <%s> => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testOpaque.Model.Value().c_str());

    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.Clear();
//    testOpaque.Clear();
//    testObject.Clear();
    testVariant.FromString(json_single_end, result);
    printf("Variant [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testVariant[_T("model")].Value().c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("This is the single\"")");

//    testObject.FromString(json_single_end, result);
//    printf("Object  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
//    testOpaque.FromString(json_single_end, result);
//    printf("Opaque  <%s> => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testOpaque.Model.Value().c_str());

    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.Clear();
//    testOpaque.Clear();
//    testObject.Clear();
    testVariant.FromString(json_multi_begin, result);
    printf("Variant [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testVariant[_T("model")].Value().c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("\\\\"This is the multi")");

//    testObject.FromString(json_multi_begin, result);
//    printf("Object  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
//    testOpaque.FromString(json_multi_begin, result);
//    printf("Opaque  <%s> => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testOpaque.Model.Value().c_str());

    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.Clear();
//    testOpaque.Clear();
//    testObject.Clear();
    testVariant.FromString(json_multi_middle, result);
    printf("Variant [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testVariant[_T("model")].Value().c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("This is the \\\\" multi")");

//    testObject.FromString(json_multi_middle, result);
//    printf("Object  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
//    testOpaque.FromString(json_multi_middle, result);
//    printf("Opaque  <%s> => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testOpaque.Model.Value().c_str());

    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.Clear();
//    testOpaque.Clear();
//    testObject.Clear();
    testVariant.FromString(json_multi_end, result);
    printf("Variant [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testVariant[_T("model")].Value().c_str());

    EXPECT_FALSE(result.IsSet());
    EXPECT_STREQ(testVariant[_T("model")].Value().c_str(), R"("This is the multi\\\\"")");

//    testObject.FromString(json_multi_end, result);
//    printf("Object  [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
//    testOpaque.FromString(json_multi_end, result);
//    printf("Opaque  <%s> => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testOpaque.Model.Value().c_str());

	// The following should fail
    printf("------------------------------------------------------------------------------------------------\n");
    testVariant.Clear();
//    testOpaque.Clear();
//    testObject.Clear();
//    testObject.FromString(json_multi_faulthy);
//    printf("Result [%s] => [%s]\n", result.IsSet() ? result.Value().Message().c_str() : "No Error", testObject.Model.Value().c_str());
}

} } } // Thunder::Tests::Core
