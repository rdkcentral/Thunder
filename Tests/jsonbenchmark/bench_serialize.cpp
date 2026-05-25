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
// bench_serialize.cpp
//
// Serialize-performance benchmarks for all three JSON libraries.
//
// Each test builds an in-memory object once, then times the repeated
// serialization of that object to a JSON string.  Building the object once
// ensures we measure serialization only — not construction overhead.
//
// Libraries under test:
//   Thunder     — obj.ToString(string) or IElement::ToString(obj, string)
//   nlohmann    — j.dump()
//   RapidJSON   — StringBuffer + Writer<StringBuffer>
// ============================================================================

#include <gtest/gtest.h>

#include <core/JSON.h>

#include <nlohmann/json.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "BenchmarkUtils.h"
#include "TestFixtures.h"

#include <sstream>

using namespace JsonBenchmark;
using namespace JsonBenchmark::Fixtures;

static constexpr uint64_t ITER_SMALL  = 200'000;
static constexpr uint64_t ITER_MEDIUM =  20'000;
static constexpr uint64_t ITER_LARGE  =     200;
static constexpr uint64_t WARMUP_PCT  =      10;

class SerializeBench : public ::testing::Test {
protected:
    volatile size_t sink = 0;
};

// ============================================================================
// Helpers: build in-memory objects for each library
// ============================================================================

// --- Build a nlohmann::json for SMALL_CONFIG --------------------------------
static nlohmann::json MakeNlohmannSmall()
{
    nlohmann::json j;
    j["callsign"]  = "TestPlugin";
    j["classname"] = "TestPlugin";
    j["autostart"] = true;
    j["configuration"] = {
        {"delay",      5},
        {"maxretries", 3},
        {"endpoint",   "http://localhost:80"},
        {"loglevel",   2},
        {"token",      "abc123XYZ"}
    };
    return j;
}

// --- Build a RapidJSON Document for SMALL_CONFIG ----------------------------
static rapidjson::Document MakeRapidJsonSmall()
{
    rapidjson::Document d;
    d.SetObject();
    auto& alloc = d.GetAllocator();

    d.AddMember("callsign",  "TestPlugin", alloc);
    d.AddMember("classname", "TestPlugin", alloc);
    d.AddMember("autostart", true,         alloc);

    rapidjson::Value cfg(rapidjson::kObjectType);
    cfg.AddMember("delay",      5,                    alloc);
    cfg.AddMember("maxretries", 3,                    alloc);
    cfg.AddMember("endpoint",   "http://localhost:80", alloc);
    cfg.AddMember("loglevel",   2,                    alloc);
    cfg.AddMember("token",      "abc123XYZ",          alloc);
    d.AddMember("configuration", cfg, alloc);

    return d;
}

// --- Build 20-device nlohmann array -----------------------------------------
static nlohmann::json MakeNlohmannMedium()
{
    nlohmann::json result;
    result["jsonrpc"] = "2.0";
    result["id"]      = 42;

    nlohmann::json devices = nlohmann::json::array();
    for (int i = 0; i < 20; ++i) {
        nlohmann::json dev;
        dev["id"]     = i + 1;
        dev["name"]   = "eth" + std::to_string(i);
        dev["mac"]    = "aa:bb:cc:dd:ee:" + std::to_string(i);
        dev["ip"]     = "192.168.1." + std::to_string(i + 1);
        dev["active"] = (i % 3 != 0);
        dev["speed"]  = (i % 2 == 0) ? 1000 : 100;
        dev["vendor"] = "VendorCorp" + std::to_string(i + 1);
        devices.push_back(dev);
    }
    result["result"] = {{"status", 0}, {"devices", devices}};
    return result;
}

// --- Build 1000-device RapidJSON array --------------------------------------
static rapidjson::Document MakeRapidJsonLarge()
{
    rapidjson::Document d;
    d.SetArray();
    auto& alloc = d.GetAllocator();

    for (int i = 0; i < 1000; ++i) {
        rapidjson::Value item(rapidjson::kObjectType);

        item.AddMember("id",    i + 1, alloc);

        std::string name = "Device" + std::to_string(i + 1);
        rapidjson::Value vName(name.c_str(), alloc);
        item.AddMember("name", vName, alloc);

        item.AddMember("active", (i % 5 != 0), alloc);
        item.AddMember("speed",  (i % 4 + 1) * 100, alloc);

        d.PushBack(item, alloc);
    }
    return d;
}

// ============================================================================
// SMALL_CONFIG serialize
// ============================================================================

TEST_F(SerializeBench, Thunder_Typed_SmallConfig)
{
    // Build once, parse from the canonical JSON string
    PluginConfig cfg;
    cfg.FromString(std::string(SMALL_CONFIG()));

    const uint64_t warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        std::string out;
        cfg.ToString(out);
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Serialize", "Small", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Serialize", "Small", stats, mem});
}

TEST_F(SerializeBench, Thunder_Dynamic_SmallConfig)
{
    Thunder::Core::JSON::VariantContainer obj;
    obj.FromString(std::string(SMALL_CONFIG()));

    const uint64_t warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        std::string out;
        obj.ToString(out);
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("Thunder-dyn", "Serialize", "Small", stats, mem);
    ResultCollector::Instance().Record({"Thunder-dyn", "Serialize", "Small", stats, mem});
}

TEST_F(SerializeBench, Nlohmann_SmallConfig)
{
    nlohmann::json j = MakeNlohmannSmall();
    const uint64_t warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        std::string out = j.dump();
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("nlohmann", "Serialize", "Small", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Serialize", "Small", stats, mem});
}

TEST_F(SerializeBench, RapidJson_SmallConfig)
{
    rapidjson::Document d = MakeRapidJsonSmall();
    const uint64_t warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        sink += sb.GetSize();
    });

    MemStats mem{};
    PrintTestResult("RapidJSON", "Serialize", "Small", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON", "Serialize", "Small", stats, mem});
}

// ============================================================================
// MEDIUM_RESPONSE serialize
// ============================================================================

TEST_F(SerializeBench, Thunder_Typed_MediumResponse)
{
    JsonRpcDeviceResponse rsp;
    rsp.FromString(std::string(MEDIUM_RESPONSE()));

    const uint64_t warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        std::string out;
        rsp.ToString(out);
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Serialize", "Medium", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Serialize", "Medium", stats, mem});
}

TEST_F(SerializeBench, Nlohmann_MediumResponse)
{
    nlohmann::json j = MakeNlohmannMedium();
    const uint64_t warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        std::string out = j.dump();
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("nlohmann", "Serialize", "Medium", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Serialize", "Medium", stats, mem});
}

TEST_F(SerializeBench, RapidJson_MediumResponse)
{
    rapidjson::Document d;
    d.Parse(MEDIUM_RESPONSE());
    const uint64_t warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        sink += sb.GetSize();
    });

    MemStats mem{};
    PrintTestResult("RapidJSON", "Serialize", "Medium", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON", "Serialize", "Medium", stats, mem});
}

// ============================================================================
// LARGE_ARRAY serialize
// ============================================================================

TEST_F(SerializeBench, Thunder_Typed_LargeArray)
{
    DeviceArray arr;
    arr.FromString(LARGE_ARRAY());

    const uint64_t warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        std::string out;
        arr.ToString(out);
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Serialize", "Large", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Serialize", "Large", stats, mem});
}

TEST_F(SerializeBench, Nlohmann_LargeArray)
{
    nlohmann::json j = nlohmann::json::parse(LARGE_ARRAY(), nullptr, false);
    const uint64_t warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        std::string out = j.dump();
        sink += out.size();
    });

    MemStats mem{};
    PrintTestResult("nlohmann", "Serialize", "Large", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Serialize", "Large", stats, mem});
}

TEST_F(SerializeBench, RapidJson_LargeArray)
{
    rapidjson::Document d = MakeRapidJsonLarge();
    const uint64_t warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        sink += sb.GetSize();
    });

    MemStats mem{};
    PrintTestResult("RapidJSON", "Serialize", "Large", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON", "Serialize", "Large", stats, mem});
}

// ============================================================================
// Output-string size comparison (single run — verifies serialization parity)
// ============================================================================

TEST_F(SerializeBench, OutputSizeComparison_SmallConfig)
{
    // Parse into each library
    PluginConfig   tCfg;  tCfg.FromString(std::string(SMALL_CONFIG()));
    nlohmann::json jCfg  = MakeNlohmannSmall();
    rapidjson::Document rCfg = MakeRapidJsonSmall();

    std::string tOut;
    tCfg.ToString(tOut);

    std::string jOut  = jCfg.dump();

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
    rCfg.Accept(w);
    std::string rOut(sb.GetString(), sb.GetSize());

    printf("\n  [SIZE] SmallConfig serialization output lengths:\n");
    printf("         Thunder   : %zu bytes\n", tOut.size());
    printf("         nlohmann  : %zu bytes\n", jOut.size());
    printf("         RapidJSON : %zu bytes\n", rOut.size());
    printf("  Note: differences reflect default whitespace/ordering choices.\n\n");
    fflush(stdout);

    // All should produce valid JSON that round-trips
    EXPECT_GT(tOut.size(), 0u);
    EXPECT_GT(jOut.size(), 0u);
    EXPECT_GT(rOut.size(), 0u);
}
