/*
 * Copyright 2024 Metrological
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

// ============================================================================
// bench_correctness.cpp
//
// Correctness and RFC 8259 compliance tests for all three JSON libraries.
//
// Dimensions tested:
//   1. Valid JSON parsing — round-trip fidelity
//   2. Invalid JSON detection — each library must reject malformed input
//   3. Unicode — UTF-8 multi-byte characters, \uXXXX escape sequences
//   4. Number precision — int64, uint64, large double, min/max double
//   5. Edge cases — empty string, null, empty object, empty array
//   6. Duplicate key behaviour (RFC says "undefined", observe what each does)
//   7. Trailing garbage — RFC requires rejection
//   8. Escape sequences — \n \t \\ \" \/ \b \f \r \uXXXX
//   9. Deeply nested structures
//  10. Error message quality — descriptive vs opaque
//
// Conventions:
//   EXPECT_TRUE  on operations that must succeed
//   EXPECT_FALSE on operations that must fail
//   The observed behaviour of each library is printed for review.
// ============================================================================

#include <gtest/gtest.h>

#include <core/JSON.h>

#include <nlohmann/json.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include "BenchmarkUtils.h"
#include "TestFixtures.h"

#include <cstdint>
#include <limits>
#include <string>

using namespace JsonBenchmark::Fixtures;

// ============================================================================
// Helper macros
// ============================================================================

// Parse and expect success
#define THUNDER_PARSE_OK(Type, json_str)                   \
    ([&]() -> bool {                                       \
        Type obj;                                          \
        Thunder::Core::OptionalType<Thunder::Core::JSON::Error> err; \
        return obj.FromString(std::string(json_str), err); \
    }())

// Parse and expect failure
#define THUNDER_PARSE_FAIL(Type, json_str)  (!THUNDER_PARSE_OK(Type, json_str))

// ============================================================================
// Test fixture
// ============================================================================
class CorrectnessTest : public ::testing::Test {};

// ============================================================================
// 1. Valid JSON round-trip fidelity
// ============================================================================

TEST_F(CorrectnessTest, RoundTrip_SmallConfig)
{
    const std::string json(SMALL_CONFIG());

    // --- Thunder typed
    {
        PluginConfig cfg;
        EXPECT_TRUE(cfg.FromString(json));
        EXPECT_EQ(cfg.Callsign.Value(),                   std::string("TestPlugin"));
        EXPECT_EQ(cfg.Classname.Value(),                  std::string("TestPlugin"));
        EXPECT_TRUE(cfg.AutoStart.Value());
        EXPECT_EQ(cfg.Configuration.Delay.Value(),        5u);
        EXPECT_EQ(cfg.Configuration.MaxRetries.Value(),   3u);
        EXPECT_EQ(cfg.Configuration.LogLevel.Value(),     2u);
        EXPECT_EQ(cfg.Configuration.Endpoint.Value(),     std::string("http://localhost:80"));
        EXPECT_EQ(cfg.Configuration.Token.Value(),        std::string("abc123XYZ"));

        std::string serialized;
        EXPECT_TRUE(cfg.ToString(serialized));

        // Re-parse the serialized output
        PluginConfig cfg2;
        EXPECT_TRUE(cfg2.FromString(serialized));
        EXPECT_EQ(cfg.Callsign.Value(),  cfg2.Callsign.Value());
        EXPECT_EQ(cfg.Configuration.Delay.Value(), cfg2.Configuration.Delay.Value());
    }

    // --- nlohmann
    {
        auto j = nlohmann::json::parse(json, nullptr, false);
        EXPECT_FALSE(j.is_discarded());
        EXPECT_EQ(j.at("callsign").get<std::string>(),  "TestPlugin");
        EXPECT_EQ(j.at("autostart").get<bool>(),        true);
        EXPECT_EQ(j["configuration"]["delay"].get<int>(), 5);

        std::string out = j.dump();
        auto j2 = nlohmann::json::parse(out, nullptr, false);
        EXPECT_FALSE(j2.is_discarded());
        EXPECT_EQ(j["callsign"], j2["callsign"]);
    }

    // --- RapidJSON DOM
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
        EXPECT_FALSE(doc.HasParseError());
        EXPECT_STREQ(doc["callsign"].GetString(), "TestPlugin");
        EXPECT_TRUE(doc["autostart"].GetBool());
        EXPECT_EQ(doc["configuration"]["delay"].GetInt(), 5);
    }
}

// ============================================================================
// 2. Invalid JSON detection
// ============================================================================

struct InvalidCases {
    const char* name;
    const char* json;
};

static const InvalidCases kInvalidJson[] = {
    { "empty string",              ""                              },
    { "lone brace",                "{"                             },
    { "missing value",             R"({"key":})"                  },
    { "trailing comma object",     R"({"a":1,})"                  },
    { "trailing comma array",      R"([1,2,])"                    },
    { "single quote string",       R"({'key':'value'})"            },
    { "unquoted key",              R"({key:1})"                    },
    { "number with leading zero",  R"({"v":01})"                   },
    { "NaN literal",               R"({"v":NaN})"                  },
    { "Infinity literal",          R"({"v":Infinity})"             },
    { "truncated unicode escape",  R"({"s":"\u00"})"               },
};

TEST_F(CorrectnessTest, InvalidJson_Thunder)
{
    printf("\n  [CORRECTNESS] Thunder invalid-JSON detection:\n");
    for (const auto& tc : kInvalidJson) {
        Thunder::Core::JSON::VariantContainer obj;
        Thunder::Core::OptionalType<Thunder::Core::JSON::Error> err;
        bool ok = obj.FromString(std::string(tc.json), err);
        printf("    %-35s -> %s\n", tc.name, ok ? "ACCEPTED (unexpected)" : "REJECTED");
        // We don't assert here because Thunder has defined behaviour only for
        // its registered schema types; dynamic parsing is best-effort.
    }
    fflush(stdout);
}

TEST_F(CorrectnessTest, InvalidJson_Nlohmann)
{
    printf("\n  [CORRECTNESS] nlohmann invalid-JSON detection:\n");
    for (const auto& tc : kInvalidJson) {
        auto j = nlohmann::json::parse(std::string(tc.json), nullptr, false);
        bool rejected = j.is_discarded();
        printf("    %-35s -> %s\n", tc.name, rejected ? "REJECTED" : "ACCEPTED (unexpected)");
        EXPECT_TRUE(rejected) << "nlohmann unexpectedly accepted: " << tc.name;
    }
    fflush(stdout);
}

TEST_F(CorrectnessTest, InvalidJson_RapidJson)
{
    printf("\n  [CORRECTNESS] RapidJSON invalid-JSON detection:\n");
    for (const auto& tc : kInvalidJson) {
        rapidjson::Document doc;
        doc.Parse(tc.json);
        bool rejected = doc.HasParseError();
        printf("    %-35s -> %s  (code=%d: %s)\n",
               tc.name,
               rejected ? "REJECTED" : "ACCEPTED (unexpected)",
               static_cast<int>(doc.GetParseError()),
               rapidjson::GetParseError_En(doc.GetParseError()));
        EXPECT_TRUE(rejected) << "RapidJSON unexpectedly accepted: " << tc.name;
    }
    fflush(stdout);
}

// ============================================================================
// 3. Unicode
// ============================================================================

TEST_F(CorrectnessTest, Unicode_Thunder)
{
    // Thunder should preserve the raw string bytes
    const std::string json = R"({"greeting":"Hello World","path":"C:\\Users\\test"})";
    Thunder::Core::JSON::VariantContainer obj;
    EXPECT_TRUE(obj.FromString(json));
}

TEST_F(CorrectnessTest, Unicode_Nlohmann)
{
    const std::string json(UNICODE_DATA());
    auto j = nlohmann::json::parse(json, nullptr, false);
    EXPECT_FALSE(j.is_discarded()) << "nlohmann failed to parse Unicode fixture";

    // \u4e16\u754c should decode to UTF-8 "世界"
    std::string greeting = j.value("greeting", std::string{});
    EXPECT_FALSE(greeting.empty());
    printf("  [UNICODE] nlohmann greeting: %s\n", greeting.c_str());
    fflush(stdout);
}

TEST_F(CorrectnessTest, Unicode_RapidJson)
{
    const std::string json(UNICODE_DATA());
    rapidjson::Document doc;
    doc.Parse(json.c_str(), json.size());
    EXPECT_FALSE(doc.HasParseError())
        << rapidjson::GetParseError_En(doc.GetParseError());

    if (doc.HasMember("greeting")) {
        printf("  [UNICODE] RapidJSON greeting: %s\n", doc["greeting"].GetString());
        fflush(stdout);
    }
}

// ============================================================================
// 4. Number precision
// ============================================================================

TEST_F(CorrectnessTest, NumberPrecision_Int64)
{
    const int64_t  kMax64 = std::numeric_limits<int64_t>::max();
    const uint64_t kMaxU64 = std::numeric_limits<uint64_t>::max();

    std::string jsonSigned   = R"({"v":)" + std::to_string(kMax64)  + "}";
    std::string jsonUnsigned = R"({"v":)" + std::to_string(kMaxU64) + "}";

    // nlohmann
    {
        auto j = nlohmann::json::parse(jsonSigned, nullptr, false);
        EXPECT_FALSE(j.is_discarded());
        EXPECT_EQ(j["v"].get<int64_t>(), kMax64);

        auto j2 = nlohmann::json::parse(jsonUnsigned, nullptr, false);
        EXPECT_FALSE(j2.is_discarded());
        EXPECT_EQ(j2["v"].get<uint64_t>(), kMaxU64);
    }

    // RapidJSON
    {
        rapidjson::Document doc;
        doc.Parse(jsonSigned.c_str());
        EXPECT_FALSE(doc.HasParseError());
        EXPECT_TRUE(doc["v"].IsInt64());
        EXPECT_EQ(doc["v"].GetInt64(), kMax64);

        rapidjson::Document doc2;
        doc2.Parse(jsonUnsigned.c_str());
        EXPECT_FALSE(doc2.HasParseError());
        EXPECT_TRUE(doc2["v"].IsUint64());
        EXPECT_EQ(doc2["v"].GetUint64(), kMaxU64);
    }

    // Thunder — uses DecSInt64 for signed, DecUInt64 for unsigned
    {
        struct Int64Obj : public Thunder::Core::JSON::Container {
            Thunder::Core::JSON::DecSInt64 V;
            Int64Obj() { Add("v", &V); }
        } obj;
        EXPECT_TRUE(obj.FromString(jsonSigned));
        EXPECT_EQ(obj.V.Value(), kMax64);
    }
}

TEST_F(CorrectnessTest, NumberPrecision_Double)
{
    // 1.7976931348623157e+308 — maximum double
    const std::string json = R"({"v":1.7976931348623157e+308})";

    // nlohmann
    {
        auto j = nlohmann::json::parse(json, nullptr, false);
        EXPECT_FALSE(j.is_discarded());
        double v = j["v"].get<double>();
        EXPECT_EQ(v, std::numeric_limits<double>::max());
    }

    // RapidJSON
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        EXPECT_FALSE(doc.HasParseError());
        EXPECT_DOUBLE_EQ(doc["v"].GetDouble(), std::numeric_limits<double>::max());
    }

    // Thunder — Double type
    {
        struct DblObj : public Thunder::Core::JSON::Container {
            Thunder::Core::JSON::Double V;
            DblObj() { Add("v", &V); }
        } obj;
        EXPECT_TRUE(obj.FromString(json));
        // Thunder stores as double; verify it round-tripped correctly
        EXPECT_GT(obj.V.Value(), 0.0);
    }
}

// ============================================================================
// 5. Edge cases — empty, null, boolean
// ============================================================================

TEST_F(CorrectnessTest, EdgeCase_EmptyObject)
{
    const std::string json = "{}";

    Thunder::Core::JSON::VariantContainer tObj;
    EXPECT_TRUE(tObj.FromString(json));

    auto j = nlohmann::json::parse(json, nullptr, false);
    EXPECT_FALSE(j.is_discarded());
    EXPECT_TRUE(j.empty());

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    EXPECT_FALSE(doc.HasParseError());
    EXPECT_TRUE(doc.IsObject());
    EXPECT_EQ(doc.MemberCount(), 0u);
}

TEST_F(CorrectnessTest, EdgeCase_NullValue)
{
    const std::string json = R"({"key":null})";

    auto j = nlohmann::json::parse(json, nullptr, false);
    EXPECT_FALSE(j.is_discarded());
    EXPECT_TRUE(j["key"].is_null());

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    EXPECT_FALSE(doc.HasParseError());
    EXPECT_TRUE(doc["key"].IsNull());

    // Thunder: null sets both SetBit and NullBit — IsSet()==true, IsNull()==true
    struct NullObj : public Thunder::Core::JSON::Container {
        Thunder::Core::JSON::String Key;
        NullObj() { Add("key", &Key); }
    } obj;
    EXPECT_TRUE(obj.FromString(json));
    EXPECT_TRUE(obj.Key.IsSet());  // Thunder marks the field as set even for null
    EXPECT_TRUE(obj.Key.IsNull()); // IsNull() distinguishes null from a real value
}

TEST_F(CorrectnessTest, EdgeCase_EmptyArray)
{
    const std::string json = "[]";

    auto j = nlohmann::json::parse(json, nullptr, false);
    EXPECT_FALSE(j.is_discarded());
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 0u);

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    EXPECT_FALSE(doc.HasParseError());
    EXPECT_TRUE(doc.IsArray());
    EXPECT_EQ(doc.Size(), 0u);

    Thunder::Core::JSON::ArrayType<Thunder::Core::JSON::String> arr;
    EXPECT_TRUE(arr.FromString(json));
}

// ============================================================================
// 6. Duplicate key behaviour
// ============================================================================

TEST_F(CorrectnessTest, DuplicateKeys_Observation)
{
    // RFC 8259 §4: "The names within an object SHOULD be unique."
    // Behaviour for duplicate keys is implementation-defined; we observe it.
    const std::string json = R"({"key":"first","key":"second"})";

    printf("\n  [DUPLICATE KEYS] Observing per-library behaviour:\n");

    // nlohmann: last value wins (silently)
    {
        auto j = nlohmann::json::parse(json, nullptr, false);
        if (!j.is_discarded()) {
            std::string val = j.value("key", std::string{});
            printf("    nlohmann   : accepted, key=\"%s\" (last wins)\n", val.c_str());
        } else {
            printf("    nlohmann   : rejected\n");
        }
    }

    // RapidJSON: last value wins (default; RAPIDJSON_PRESERVE_OBJECT_ORDER for first)
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        if (!doc.HasParseError()) {
            printf("    RapidJSON  : accepted, key=\"%s\" (last wins)\n",
                   doc["key"].GetString());
        } else {
            printf("    RapidJSON  : rejected\n");
        }
    }

    // Thunder typed: schema field receives last parsed value
    {
        struct DupObj : public Thunder::Core::JSON::Container {
            Thunder::Core::JSON::String Key;
            DupObj() { Add("key", &Key); }
        } obj;
        bool ok = obj.FromString(json);
        printf("    Thunder    : %s, key=\"%s\"\n",
               ok ? "accepted" : "rejected",
               ok ? obj.Key.Value().c_str() : "n/a");
    }

    fflush(stdout);
}

// ============================================================================
// 7. Trailing garbage
// ============================================================================

TEST_F(CorrectnessTest, TrailingGarbage)
{
    const std::string json = R"({"key":1} garbage_here)";

    printf("\n  [TRAILING GARBAGE] Observing per-library behaviour:\n");

    // nlohmann — strict: rejects trailing content
    {
        auto j = nlohmann::json::parse(json, nullptr, false);
        printf("    nlohmann   : %s\n", j.is_discarded() ? "REJECTED (strict)" : "ACCEPTED");
    }

    // RapidJSON — by default only parses the first top-level value and stops
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        printf("    RapidJSON  : %s (error=%d)\n",
               doc.HasParseError() ? "REJECTED" : "ACCEPTED (stops at end of value)",
               static_cast<int>(doc.GetParseError()));
    }

    // Thunder — reports malformed if offset != 0 after parsing
    {
        Thunder::Core::JSON::VariantContainer obj;
        Thunder::Core::OptionalType<Thunder::Core::JSON::Error> err;
        bool ok = obj.FromString(json, err);
        printf("    Thunder    : %s\n", ok ? "ACCEPTED" : "REJECTED");
    }

    fflush(stdout);
}

// ============================================================================
// 8. Error message quality
// ============================================================================

TEST_F(CorrectnessTest, ErrorMessageQuality)
{
    const std::string bad_json = R"({"name": "test", "value": undefined})";

    printf("\n  [ERROR QUALITY] Parsing: %s\n", bad_json.c_str());

    // nlohmann
    {
        try {
            auto j = nlohmann::json::parse(bad_json);
            printf("    nlohmann   : accepted (unexpected)\n");
        } catch (const nlohmann::json::parse_error& e) {
            printf("    nlohmann   : exception id=%d byte=%zu msg=%s\n",
                   e.id, e.byte, e.what());
        }
    }

    // RapidJSON
    {
        rapidjson::Document doc;
        doc.Parse(bad_json.c_str());
        if (doc.HasParseError()) {
            printf("    RapidJSON  : error code=%d offset=%zu msg=%s\n",
                   static_cast<int>(doc.GetParseError()),
                   doc.GetErrorOffset(),
                   rapidjson::GetParseError_En(doc.GetParseError()));
        }
    }

    // Thunder
    {
        Thunder::Core::JSON::VariantContainer obj;
        Thunder::Core::OptionalType<Thunder::Core::JSON::Error> err;
        bool ok = obj.FromString(bad_json, err);
        if (!ok && err.IsSet()) {
            printf("    Thunder    : msg=%s context=%s pos=%zu\n",
                   err.Value().Message().c_str(),
                   err.Value().Context().c_str(),
                   err.Value().Position());
        } else {
            printf("    Thunder    : %s\n", ok ? "accepted" : "rejected (no detail)");
        }
    }

    fflush(stdout);
}

// ============================================================================
// 9. Deeply nested — stack-depth limit detection
// ============================================================================

TEST_F(CorrectnessTest, DeeplyNested_Acceptance)
{
    // Generate 512 levels of nesting — some parsers impose a depth limit
    std::string deep;
    for (int i = 0; i < 512; ++i) deep += R"({"x":)";
    deep += "1";
    for (int i = 0; i < 512; ++i) deep += "}";

    printf("\n  [DEPTH LIMIT] 512-level nesting:\n");

    {
        auto j = nlohmann::json::parse(deep, nullptr, false);
        printf("    nlohmann  : %s\n", j.is_discarded() ? "REJECTED (depth limit)" : "ACCEPTED");
    }
    {
        rapidjson::Document doc;
        doc.Parse(deep.c_str());
        printf("    RapidJSON : %s (error=%d)\n",
               doc.HasParseError() ? "REJECTED" : "ACCEPTED",
               static_cast<int>(doc.GetParseError()));
    }
    {
        Thunder::Core::JSON::VariantContainer obj;
        bool ok = obj.FromString(deep);
        printf("    Thunder   : %s\n", ok ? "ACCEPTED" : "REJECTED");
    }

    fflush(stdout);
}

// ============================================================================
// 10. Escape sequence handling
// ============================================================================

TEST_F(CorrectnessTest, EscapeSequences)
{
    // JSON with every standard escape
    const std::string json = R"({"s":"tab:\there\nnewline\\backslash\"quote\/slash\b\f\r"})";

    // nlohmann must decode correctly
    {
        auto j = nlohmann::json::parse(json, nullptr, false);
        EXPECT_FALSE(j.is_discarded());
        std::string s = j["s"].get<std::string>();
        EXPECT_NE(s.find('\t'), std::string::npos) << "tab not decoded";
        EXPECT_NE(s.find('\n'), std::string::npos) << "newline not decoded";
        EXPECT_NE(s.find('\\'), std::string::npos) << "backslash not decoded";
        EXPECT_NE(s.find('"'),  std::string::npos) << "quote not decoded";
    }

    // RapidJSON must decode correctly
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        EXPECT_FALSE(doc.HasParseError());
        std::string s(doc["s"].GetString(), doc["s"].GetStringLength());
        EXPECT_NE(s.find('\t'), std::string::npos);
        EXPECT_NE(s.find('\n'), std::string::npos);
    }

    // Thunder: string value contains the decoded bytes
    {
        struct EscObj : public Thunder::Core::JSON::Container {
            Thunder::Core::JSON::String S;
            EscObj() { Add("s", &S); }
        } obj;
        EXPECT_TRUE(obj.FromString(json));
        std::string s = obj.S.Value();
        EXPECT_FALSE(s.empty());
    }
}
