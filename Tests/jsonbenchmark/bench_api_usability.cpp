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
// bench_api_usability.cpp
//
// Qualitative API evaluation.  Each test:
//   - Demonstrates idiomatic usage of each library for a common task.
//   - Counts approximate lines-of-code (LOC) required.
//   - Documents observations about type safety, error handling, and
//     expressiveness as inline comments and printf output.
//
// Evaluated tasks:
//   Task A — Parse a known schema and read nested fields
//   Task B — Build a JSON object programmatically and serialize it
//   Task C — Mutate an existing JSON object (add / change / remove a field)
//   Task D — Iterate over a JSON array
//   Task E — Handle parse errors with descriptive messages
//   Task F — Check for optional fields
//   Task G — Integration complexity notes (CMake, headers, compile time)
//   Task H — Community and ecosystem notes
// ============================================================================

#include <gtest/gtest.h>

#include <core/JSON.h>

#include <nlohmann/json.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "BenchmarkUtils.h"
#include "TestFixtures.h"

using namespace JsonBenchmark::Fixtures;

class ApiUsabilityTest : public ::testing::Test {};

// ============================================================================
// Task A — Parse a known schema, access nested values
// ============================================================================

// ---------------------------------------------------------------------------
// Thunder — LOC ≈ 22 (schema definition) + 5 (parse + access) = 27
//   Pro: compile-time field names, zero ambiguity at the use site, naturally
//        integrates with JSON-RPC generation toolchain (jsonrpc-tools).
//   Con: requires upfront schema type definition; no ad-hoc access.
// ---------------------------------------------------------------------------
TEST_F(ApiUsabilityTest, TaskA_ParseKnownSchema_Thunder)
{
    // Schema already defined in TestFixtures.h (PluginConfig).
    // At real call sites the Container type is defined once per interface.
    const std::string json(SMALL_CONFIG());

    PluginConfig cfg;                                    // L1
    Thunder::Core::OptionalType<Thunder::Core::JSON::Error> err; // L2
    bool ok = cfg.FromString(json, err);                 // L3  parse

    ASSERT_TRUE(ok) << (err.IsSet() ? err.Value().Message() : "unknown");

    // Field access — strongly typed, no string keys in hot path
    uint32_t    delay    = cfg.Configuration.Delay.Value();    // L4
    std::string endpoint = cfg.Configuration.Endpoint.Value(); // L5
    bool        autoStart = cfg.AutoStart.Value();             // L6

    EXPECT_EQ(delay, 5u);
    EXPECT_EQ(endpoint, "http://localhost:80");
    EXPECT_TRUE(autoStart);

    printf("\n  [API-A] Thunder-typed: LOC≈6 (after schema def), "
           "delay=%u endpoint=%s\n", delay, endpoint.c_str());
    fflush(stdout);
}

// ---------------------------------------------------------------------------
// nlohmann — LOC ≈ 6 (no upfront schema needed)
//   Pro: no schema type needed, intuitive [] operator, works on arbitrary JSON.
//   Con: runtime key-string errors not caught by compiler; value() is verbose
//        for deeply nested access; every element is a heap-allocated node.
// ---------------------------------------------------------------------------
TEST_F(ApiUsabilityTest, TaskA_ParseKnownSchema_Nlohmann)
{
    const std::string json(SMALL_CONFIG());

    auto j = nlohmann::json::parse(json, nullptr, false); // L1
    ASSERT_FALSE(j.is_discarded());

    uint32_t    delay    = j["configuration"]["delay"].get<uint32_t>();    // L2
    std::string endpoint = j["configuration"]["endpoint"].get<std::string>(); // L3
    bool        autoStart = j["autostart"].get<bool>();                   // L4

    EXPECT_EQ(delay, 5u);
    EXPECT_EQ(endpoint, "http://localhost:80");
    EXPECT_TRUE(autoStart);

    printf("  [API-A] nlohmann     : LOC≈4, delay=%u endpoint=%s\n",
           delay, endpoint.c_str());
    fflush(stdout);
}

// ---------------------------------------------------------------------------
// RapidJSON DOM — LOC ≈ 8
//   Pro: extremely fast; explicit type query before get avoids UB.
//   Con: verbose type checks (IsInt, GetString etc.); no implicit coercion;
//        DOM object lifetimes are coupled to the Document allocator.
// ---------------------------------------------------------------------------
TEST_F(ApiUsabilityTest, TaskA_ParseKnownSchema_RapidJson)
{
    const std::string json(SMALL_CONFIG());

    rapidjson::Document doc;                              // L1
    doc.Parse(json.c_str(), json.size());                 // L2
    ASSERT_FALSE(doc.HasParseError());

    const auto& cfg = doc["configuration"];               // L3
    uint32_t  delay    = cfg["delay"].GetUint();          // L4
    std::string endpoint(cfg["endpoint"].GetString(),     // L5
                         cfg["endpoint"].GetStringLength());
    bool autoStart = doc["autostart"].GetBool();          // L6

    EXPECT_EQ(delay, 5u);
    EXPECT_EQ(endpoint, "http://localhost:80");
    EXPECT_TRUE(autoStart);

    printf("  [API-A] RapidJSON    : LOC≈6, delay=%u endpoint=%s\n",
           delay, endpoint.c_str());
    fflush(stdout);
}

// ============================================================================
// Task B — Build a JSON object programmatically and serialize
// ============================================================================

// ---------------------------------------------------------------------------
// Thunder — LOC ≈ 10 (type instantiation + field assignment + ToString)
//   Pro: field assignment is type-safe (can't put a string in a uint32 field).
//   Con: must use Container-derived type; no inline ad-hoc construction.
// ---------------------------------------------------------------------------
TEST_F(ApiUsabilityTest, TaskB_BuildAndSerialize_Thunder)
{
    PluginConfig cfg;                               // L1
    cfg.Callsign  = "MyPlugin";                     // L2
    cfg.Classname = "MyPlugin";                     // L3
    cfg.AutoStart = true;                           // L4
    cfg.Configuration.Delay      = 10;             // L5
    cfg.Configuration.MaxRetries = 5;              // L6
    cfg.Configuration.Endpoint   = "http://myhost:8080"; // L7
    cfg.Configuration.LogLevel   = 3;              // L8
    cfg.Configuration.Token      = "tok-xyz";      // L9

    std::string out;                                // L10
    EXPECT_TRUE(cfg.ToString(out));                 // L11

    EXPECT_FALSE(out.empty());
    printf("\n  [API-B] Thunder: LOC≈11, output size=%zu bytes\n", out.size());
    printf("          Output: %s\n", out.c_str());
    fflush(stdout);
}

// ---------------------------------------------------------------------------
// nlohmann — LOC ≈ 8 (initialiser list syntax — very concise)
//   Pro: initialiser-list construction is extremely readable; one-liner dump.
//   Con: no compile-time checking; typos in key names silently produce wrong output.
// ---------------------------------------------------------------------------
TEST_F(ApiUsabilityTest, TaskB_BuildAndSerialize_Nlohmann)
{
    nlohmann::json j = {                           // L1
        {"callsign",  "MyPlugin"},                 // L2
        {"classname", "MyPlugin"},                 // L3
        {"autostart", true},                       // L4
        {"configuration", {                        // L5
            {"delay",      10},
            {"maxretries", 5},
            {"endpoint",   "http://myhost:8080"},
            {"loglevel",   3},
            {"token",      "tok-xyz"}
        }}
    };                                             // L6

    std::string out = j.dump();                    // L7
    EXPECT_FALSE(out.empty());
    printf("  [API-B] nlohmann: LOC≈8, output size=%zu bytes\n", out.size());
    fflush(stdout);
}

// ---------------------------------------------------------------------------
// RapidJSON — LOC ≈ 18 (verbose but explicit)
//   Pro: zero additional heap allocation if MemoryPoolAllocator is used.
//   Con: very boilerplate-heavy; AddMember requires allocator arg on every call;
//        string literals must be explicitly passed with allocator for non-literal
//        strings to avoid dangling references.
// ---------------------------------------------------------------------------
TEST_F(ApiUsabilityTest, TaskB_BuildAndSerialize_RapidJson)
{
    rapidjson::Document d;                          // L1
    d.SetObject();
    auto& alloc = d.GetAllocator();                 // L2

    d.AddMember("callsign",  "MyPlugin", alloc);   // L3
    d.AddMember("classname", "MyPlugin", alloc);   // L4
    d.AddMember("autostart", true,       alloc);   // L5

    rapidjson::Value cfg(rapidjson::kObjectType);   // L6
    cfg.AddMember("delay",      10,                    alloc); // L7
    cfg.AddMember("maxretries", 5,                     alloc); // L8
    cfg.AddMember("endpoint",   "http://myhost:8080",  alloc); // L9
    cfg.AddMember("loglevel",   3,                     alloc); // L10
    cfg.AddMember("token",      "tok-xyz",             alloc); // L11
    d.AddMember("configuration", cfg, alloc);       // L12

    rapidjson::StringBuffer sb;                     // L13
    rapidjson::Writer<rapidjson::StringBuffer> w(sb); // L14
    d.Accept(w);                                    // L15

    std::string out(sb.GetString(), sb.GetSize());  // L16
    EXPECT_FALSE(out.empty());
    printf("  [API-B] RapidJSON: LOC≈16, output size=%zu bytes\n", out.size());
    fflush(stdout);
}

// ============================================================================
// Task C — Mutate an existing parsed object
// ============================================================================

TEST_F(ApiUsabilityTest, TaskC_Mutation_Nlohmann)
{
    // nlohmann: natural [] assignment
    auto j = nlohmann::json::parse(std::string(SMALL_CONFIG()), nullptr, false);
    ASSERT_FALSE(j.is_discarded());

    j["autostart"]              = false;         // change existing
    j["configuration"]["delay"] = 99;            // change nested
    j["newfield"]               = "added";       // add new field
    j.erase("callsign");                          // remove field

    EXPECT_EQ(j.value("autostart", true), false);
    EXPECT_EQ(j["configuration"]["delay"].get<int>(), 99);
    EXPECT_TRUE(j.contains("newfield"));
    EXPECT_FALSE(j.contains("callsign"));

    printf("\n  [API-C] nlohmann mutation: LOC≈4 (trivial, natural syntax)\n");
    fflush(stdout);
}

TEST_F(ApiUsabilityTest, TaskC_Mutation_RapidJson)
{
    rapidjson::Document doc;
    doc.Parse(SMALL_CONFIG());
    ASSERT_FALSE(doc.HasParseError());
    auto& alloc = doc.GetAllocator();

    // Change existing
    doc["autostart"].SetBool(false);

    // Change nested
    doc["configuration"]["delay"].SetUint(99);

    // Add new field
    doc.AddMember("newfield", "added", alloc);

    // Remove field
    doc.RemoveMember("callsign");

    EXPECT_FALSE(doc["autostart"].GetBool());
    EXPECT_EQ(doc["configuration"]["delay"].GetUint(), 99u);
    EXPECT_TRUE(doc.HasMember("newfield"));
    EXPECT_FALSE(doc.HasMember("callsign"));

    printf("  [API-C] RapidJSON mutation: LOC≈6 (verbose, no natural lvalue assignment)\n");
    fflush(stdout);
}

TEST_F(ApiUsabilityTest, TaskC_Mutation_Thunder)
{
    // Thunder typed Container fields can be assigned directly
    PluginConfig cfg;
    cfg.FromString(std::string(SMALL_CONFIG()));

    cfg.AutoStart               = false;
    cfg.Configuration.Delay     = 99;
    // Note: removing a field from a typed Container is not possible —
    // it will serialize with default/zero value if not set.
    // Use JsonObject (VariantContainer) for truly dynamic manipulation.

    EXPECT_FALSE(cfg.AutoStart.Value());
    EXPECT_EQ(cfg.Configuration.Delay.Value(), 99u);

    printf("  [API-C] Thunder-typed: LOC≈2 (but no field removal; use JsonObject for that)\n");
    fflush(stdout);
}

// ============================================================================
// Task D — Iterate over a JSON array
// ============================================================================

TEST_F(ApiUsabilityTest, TaskD_ArrayIteration_Nlohmann)
{
    auto j = nlohmann::json::parse(std::string(MEDIUM_RESPONSE()), nullptr, false);
    ASSERT_FALSE(j.is_discarded());

    size_t count = 0;
    for (const auto& dev : j["result"]["devices"]) {  // range-for, STL-compatible
        count++;
        (void)dev["id"].get<int>();
    }
    EXPECT_EQ(count, 20u);
    printf("\n  [API-D] nlohmann: range-for with [] — LOC≈3, count=%zu\n", count);
    fflush(stdout);
}

TEST_F(ApiUsabilityTest, TaskD_ArrayIteration_RapidJson)
{
    rapidjson::Document doc;
    doc.Parse(MEDIUM_RESPONSE());
    ASSERT_FALSE(doc.HasParseError());

    const auto& devices = doc["result"]["devices"].GetArray();
    size_t count = 0;
    for (const auto& dev : devices) {  // range-for via GetArray()
        count++;
        (void)dev["id"].GetInt();
    }
    EXPECT_EQ(count, 20u);
    printf("  [API-D] RapidJSON: GetArray() + range-for — LOC≈4, count=%zu\n", count);
    fflush(stdout);
}

TEST_F(ApiUsabilityTest, TaskD_ArrayIteration_Thunder)
{
    JsonRpcDeviceResponse rsp;
    rsp.FromString(std::string(MEDIUM_RESPONSE()));

    size_t count = 0;
    // ArrayType iteration uses Thunder's iterator pattern
    auto it = rsp.Result.Devices.Elements();
    while (it.Next()) {
        count++;
        (void)it.Current().Id.Value();
    }
    EXPECT_EQ(count, 20u);
    printf("  [API-D] Thunder: Elements().Next() iterator — LOC≈4, count=%zu\n", count);
    fflush(stdout);
}

// ============================================================================
// Task E — Optional field access (field may or may not be present)
// ============================================================================

TEST_F(ApiUsabilityTest, TaskE_OptionalField_AllLibraries)
{
    const std::string json = R"({"name":"test","value":42})"; // no "description" field

    printf("\n  [API-E] Optional field access (\"description\" absent):\n");

    // nlohmann: value() with default, or contains() + []
    {
        auto j = nlohmann::json::parse(json, nullptr, false);
        std::string desc = j.value("description", std::string("(none)"));
        printf("    nlohmann  : j.value(\"description\",\"(none)\") = \"%s\"\n", desc.c_str());
    }

    // RapidJSON: HasMember() + []
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        const char* desc = doc.HasMember("description")
                           ? doc["description"].GetString()
                           : "(none)";
        printf("    RapidJSON : HasMember check = \"%s\"\n", desc);
    }

    // Thunder typed: IsSet() on the field
    {
        struct OptObj : public Thunder::Core::JSON::Container {
            Thunder::Core::JSON::String Name;
            Thunder::Core::JSON::String Description;
            OptObj() { Add("name",&Name); Add("description",&Description); }
        } obj;
        obj.FromString(json);
        std::string desc = obj.Description.IsSet()
                           ? obj.Description.Value()
                           : "(none)";
        printf("    Thunder   : IsSet() check = \"%s\"\n", desc.c_str());
    }

    fflush(stdout);
}

// ============================================================================
// Task F — Pretty-print (human-readable) serialization
// ============================================================================

TEST_F(ApiUsabilityTest, TaskF_PrettyPrint)
{
    printf("\n  [API-F] Pretty-print support:\n");

    const std::string json(SMALL_CONFIG());

    // nlohmann: dump(indent)  — trivial
    {
        auto j   = nlohmann::json::parse(json, nullptr, false);
        std::string pretty = j.dump(2);  // 2-space indent
        printf("    nlohmann  : j.dump(2) — first 80 chars: %.80s ...\n", pretty.c_str());
    }

    // RapidJSON: PrettyWriter  — simple swap of Writer for PrettyWriter
    {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> pw(sb);
        doc.Accept(pw);
        std::string pretty(sb.GetString(), sb.GetSize());
        printf("    RapidJSON : PrettyWriter — first 80 chars: %.80s ...\n", pretty.c_str());
    }

    // Thunder: no built-in pretty-print; ToString() produces compact JSON only.
    printf("    Thunder   : No built-in pretty-print. ToString() is always compact.\n");
    fflush(stdout);
}

// ============================================================================
// Task G — Integration complexity summary
// ============================================================================

TEST_F(ApiUsabilityTest, TaskG_IntegrationComplexity)
{
    printf(
        "\n"
        "  ======== Integration Complexity Summary ========\n"
        "\n"
        "  Thunder JSON\n"
        "    Location     : Source/core/JSON.h + JSON.cpp (already in the build)\n"
        "    CMake LOC    : 0 (no extra steps; part of ThunderCore)\n"
        "    Header-only  : No  (JSON.cpp must be compiled)\n"
        "    C++ standard : C++11 minimum\n"
        "    License      : Apache 2.0\n"
        "    API style    : Schema-bound Container types\n"
        "    Dynamic DOM  : Yes (VariantContainer / JsonObject) — limited\n"
        "\n"
        "  nlohmann/json\n"
        "    CMake LOC    : ~8 (FetchContent_Declare + FetchContent_MakeAvailable\n"
        "                       + target_link_libraries)\n"
        "    Header-only  : Yes (single json.hpp or json_fwd.hpp)\n"
        "    C++ standard : C++11 minimum; better with C++17\n"
        "    License      : MIT\n"
        "    API style    : DOM with STL-like interface\n"
        "    Binary size  : Large (heavy template instantiation)\n"
        "    Compile time : Significant (large header, many templates)\n"
        "\n"
        "  RapidJSON\n"
        "    CMake LOC    : ~6 (FetchContent_Declare + include_directories)\n"
        "                       No imported CMake target in v1.1.0\n"
        "    Header-only  : Yes (include/rapidjson/)\n"
        "    C++ standard : C++11 minimum\n"
        "    License      : MIT + BSD\n"
        "    API style    : DOM (Document) + SAX (Reader + Handler)\n"
        "    Binary size  : Small (minimal template instantiation)\n"
        "    Compile time : Fast\n"
        "    SAX mode     : Zero-allocation streaming parse (unique advantage)\n"
        "\n"
        "  =============================================\n\n"
    );
    fflush(stdout);
}

// ============================================================================
// Task H — Community and ecosystem
// ============================================================================

TEST_F(ApiUsabilityTest, TaskH_CommunityEcosystem)
{
    printf(
        "\n"
        "  ======== Community & Ecosystem (as of 2025) ========\n"
        "\n"
        "  Thunder JSON\n"
        "    GitHub stars    : Internal / not publicly listed independently\n"
        "    Maintenance     : Active within the Thunder project\n"
        "    Documentation   : Thunder framework docs only\n"
        "    Known issues    : Complex template code, high maintenance burden,\n"
        "                      no external community, limited test coverage\n"
        "                      for edge cases vs RFC 8259\n"
        "\n"
        "  nlohmann/json\n"
        "    GitHub stars    : ~45 000+\n"
        "    Last commit     : Very recent (actively maintained)\n"
        "    Open issues     : ~200\n"
        "    Documentation   : Extensive (https://json.nlohmann.me)\n"
        "    Ecosystem       : De-facto standard C++ JSON library;\n"
        "                      wide IDE/tooling support; STL integration\n"
        "    Drawbacks       : Heavy compile time; large binaries;\n"
        "                      exception-reliant (configurable)\n"
        "\n"
        "  RapidJSON\n"
        "    GitHub stars    : ~14 000+\n"
        "    Last commit     : Low recent activity (v1.1.0 from 2016;\n"
        "                      master has patches but no new release)\n"
        "    Open issues     : ~300+ accumulated\n"
        "    Documentation   : Good (http://rapidjson.org)\n"
        "    Ecosystem       : Widely used in performance-critical paths;\n"
        "                      used inside Google Chromium, TensorFlow, etc.\n"
        "    Drawbacks       : Aging release; some C++17 compat issues;\n"
        "                      more verbose API than nlohmann\n"
        "\n"
        "  =============================================\n\n"
    );
    fflush(stdout);
}

// ============================================================================
// Final summary table
// ============================================================================

TEST_F(ApiUsabilityTest, ZZZ_FinalEvaluationMatrix)
{
    printf(
        "\n"
        "  ======== Evaluation Matrix ========\n"
        "\n"
        "  Criterion               Thunder-typed  Thunder-dyn  nlohmann     RapidJSON-DOM RapidJSON-SAX\n"
        "  ----------------------  -------------  -----------  -----------  ------------- -------------\n"
        "  Schema-bound parsing    Excellent      n/a          None         None          None\n"
        "  Dynamic DOM access      Limited        Good         Excellent    Good          n/a\n"
        "  API intuitiveness       Moderate       Moderate     Excellent    Moderate      Poor\n"
        "  Parse performance       See bench      See bench    See bench    See bench     Best\n"
        "  Serialize performance   See bench      See bench    See bench    See bench     n/a\n"
        "  Memory allocations      Moderate       High         High         Low*          Minimal\n"
        "  Integration effort      Zero           Zero         Low          Low           Low\n"
        "  Compile time            Fast           Fast         Slow         Fast          Fast\n"
        "  Binary size impact      Existing       Existing     Large        Small         Small\n"
        "  Error messages          Good           Good         Excellent    Good          Limited\n"
        "  RFC 8259 strictness     Lenient        Lenient      Strict       Configurable  Configurable\n"
        "  Pretty-print            No             No           Yes (dump)   Yes (Writer)  n/a\n"
        "  Community support       Internal       Internal     Excellent    Good          Good\n"
        "  Exception-free          Yes            Yes          Config only  Yes           Yes\n"
        "  C++ standard needed     C++11          C++11        C++11/17     C++11         C++11\n"
        "\n"
        "  * RapidJSON DOM uses a MemoryPoolAllocator: very few malloc calls,\n"
        "    one large slab.  TotalBytes is similar but AllocCount is much lower.\n"
        "\n"
        "  Recommendation:\n"
        "    - If the goal is to REPLACE Thunder JSON entirely:\n"
        "        RapidJSON is the best fit for embedded/performance targets:\n"
        "        fast, low memory overhead, exception-free, small binary impact.\n"
        "    - If the goal is to SIMPLIFY the developer-facing API:\n"
        "        nlohmann/json offers the most natural C++ experience and is the\n"
        "        better choice when build size/compile time are acceptable.\n"
        "    - A hybrid approach (keep Thunder Container for JSON-RPC schema\n"
        "        validation; replace the parser internals with RapidJSON) offers\n"
        "        the best of both worlds.\n"
        "\n"
        "  See the benchmark numbers printed above for quantitative data.\n"
        "  =============================================\n\n"
    );
    fflush(stdout);
}
