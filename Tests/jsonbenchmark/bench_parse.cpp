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
// bench_parse.cpp
//
// Parse-performance benchmarks for all three JSON libraries.
//
// Each test:
//   1. Warm up the parser (avoid JIT / cache cold-start distortion).
//   2. Run N timed iterations, collecting per-iteration wall-clock time.
//   3. Compute min / mean / p50 / p95 / p99 / throughput.
//   4. Register the result with ResultCollector for the final summary.
//
// Iteration counts are chosen so the total wall time is roughly 1-2 s per test
// on a modern embedded SoC (~1 GHz Cortex-A class).  Adjust ITER_* constants
// for your hardware.
//
// Libraries under test:
//   Thunder     — Core::JSON::Container (schema-bound, no dynamic DOM)
//                 Core::JSON::VariantContainer / JsonObject (dynamic DOM)
//   nlohmann    — nlohmann::json::parse()  (DOM)
//   RapidJSON   — rapidjson::Document::Parse()  (DOM)
//               — rapidjson SAX handler  (zero-allocation, event-driven)
// ============================================================================

#include <gtest/gtest.h>

#include <core/JSON.h>

#include <nlohmann/json.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>

#include "BenchmarkUtils.h"
#include "TestFixtures.h"

using namespace JsonBenchmark;
using namespace JsonBenchmark::Fixtures;

// ---------------------------------------------------------------------------
// Iteration counts — tune per hardware
// ---------------------------------------------------------------------------
static constexpr uint64_t ITER_SMALL  = 200'000;
static constexpr uint64_t ITER_MEDIUM =  20'000;
static constexpr uint64_t ITER_LARGE  =     200;
static constexpr uint64_t ITER_DEEP   =  50'000;
static constexpr uint64_t ITER_REAL   =  50'000;
static constexpr uint64_t WARMUP_PCT  =      10; // 10 % of ITER_* as warmup

// ---------------------------------------------------------------------------
// RapidJSON SAX: discard handler — counts events, discards all values.
// This measures the theoretical floor of RapidJSON parse throughput.
// ---------------------------------------------------------------------------
struct DiscardHandler {
    bool Null()                        { return true; }
    bool Bool(bool)                    { return true; }
    bool Int(int)                      { return true; }
    bool Uint(unsigned)                { return true; }
    bool Int64(int64_t)                { return true; }
    bool Uint64(uint64_t)              { return true; }
    bool Double(double)                { return true; }
    bool String(const char*, rapidjson::SizeType, bool) { return true; }
    bool StartObject()                 { return true; }
    bool Key(const char*, rapidjson::SizeType, bool)    { return true; }
    bool EndObject(rapidjson::SizeType) { return true; }
    bool StartArray()                  { return true; }
    bool EndArray(rapidjson::SizeType)  { return true; }
    bool RawNumber(const char*, rapidjson::SizeType, bool) { return true; }
};

// ============================================================================
// TEST FIXTURE — provides per-test-suite setup
// ============================================================================
class ParseBench : public ::testing::Test {
protected:
    // Prevent optimizer from eliminating parse calls
    volatile int sink = 0;
};

// ============================================================================
// SMALL_CONFIG (~230 bytes)
// ============================================================================

TEST_F(ParseBench, Thunder_Typed_SmallConfig)
{
    const std::string json(SMALL_CONFIG());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        PluginConfig cfg;
        cfg.FromString(json);
        sink += static_cast<int>(cfg.AutoStart.Value());
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Parse", "Small", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Parse", "Small", stats, mem});
}

TEST_F(ParseBench, Thunder_Dynamic_SmallConfig)
{
    const std::string json(SMALL_CONFIG());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        Thunder::Core::JSON::VariantContainer obj;
        obj.FromString(json);
        sink += obj.HasLabel("callsign") ? 1 : 0;
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-dyn", "Parse", "Small", stats, mem);
    ResultCollector::Instance().Record({"Thunder-dyn", "Parse", "Small", stats, mem});
}

TEST_F(ParseBench, Nlohmann_SmallConfig)
{
    const std::string json(SMALL_CONFIG());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        auto j = nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
        sink += j.is_discarded() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("nlohmann", "Parse", "Small", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Parse", "Small", stats, mem});
}

TEST_F(ParseBench, RapidJson_DOM_SmallConfig)
{
    const std::string json(SMALL_CONFIG());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
        sink += doc.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-DOM", "Parse", "Small", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Parse", "Small", stats, mem});
}

TEST_F(ParseBench, RapidJson_SAX_SmallConfig)
{
    const std::string json(SMALL_CONFIG());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_SMALL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_SMALL, warmup, [&] {
        rapidjson::StringStream ss(json.c_str());
        DiscardHandler          handler;
        rapidjson::Reader       reader;
        reader.Parse(ss, handler);
        sink += reader.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-SAX", "Parse", "Small", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-SAX", "Parse", "Small", stats, mem});
}

// ============================================================================
// MEDIUM_RESPONSE (~3.5 KB)
// ============================================================================

TEST_F(ParseBench, Thunder_Typed_MediumResponse)
{
    const std::string json(MEDIUM_RESPONSE());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        JsonRpcDeviceResponse rsp;
        rsp.FromString(json);
        sink += static_cast<int>(rsp.Id.Value());
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Parse", "Medium", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Parse", "Medium", stats, mem});
}

TEST_F(ParseBench, Thunder_Dynamic_MediumResponse)
{
    const std::string json(MEDIUM_RESPONSE());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        Thunder::Core::JSON::VariantContainer obj;
        obj.FromString(json);
        sink += obj.HasLabel("result") ? 1 : 0;
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-dyn", "Parse", "Medium", stats, mem);
    ResultCollector::Instance().Record({"Thunder-dyn", "Parse", "Medium", stats, mem});
}

TEST_F(ParseBench, Nlohmann_MediumResponse)
{
    const std::string json(MEDIUM_RESPONSE());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        auto j = nlohmann::json::parse(json, nullptr, false);
        sink += j.is_discarded() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("nlohmann", "Parse", "Medium", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Parse", "Medium", stats, mem});
}

TEST_F(ParseBench, RapidJson_DOM_MediumResponse)
{
    const std::string json(MEDIUM_RESPONSE());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
        sink += doc.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-DOM", "Parse", "Medium", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Parse", "Medium", stats, mem});
}

TEST_F(ParseBench, RapidJson_SAX_MediumResponse)
{
    const std::string json(MEDIUM_RESPONSE());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_MEDIUM / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_MEDIUM, warmup, [&] {
        rapidjson::StringStream ss(json.c_str());
        DiscardHandler          handler;
        rapidjson::Reader       reader;
        reader.Parse(ss, handler);
        sink += reader.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-SAX", "Parse", "Medium", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-SAX", "Parse", "Medium", stats, mem});
}

// ============================================================================
// LARGE_ARRAY (~100 KB)
// ============================================================================

TEST_F(ParseBench, Thunder_Typed_LargeArray)
{
    const std::string& json  = LARGE_ARRAY();
    const size_t       bytes = json.size();
    const uint64_t     warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        DeviceArray arr;
        arr.FromString(json);
        sink += arr.IsSet() ? 1 : 0;
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Parse", "Large", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Parse", "Large", stats, mem});
}

TEST_F(ParseBench, Thunder_Dynamic_LargeArray)
{
    const std::string& json  = LARGE_ARRAY();
    const size_t       bytes = json.size();
    const uint64_t     warmup = ITER_LARGE / WARMUP_PCT + 1;

    // Thunder's VariantContainer is an object container, not an array container.
    // Use ArrayType<Variant> for dynamic array parsing.
    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        Thunder::Core::JSON::ArrayType<Thunder::Core::JSON::Variant> arr;
        arr.FromString(json);
        sink += arr.IsSet() ? 1 : 0;
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-dyn", "Parse", "Large", stats, mem);
    ResultCollector::Instance().Record({"Thunder-dyn", "Parse", "Large", stats, mem});
}

TEST_F(ParseBench, Nlohmann_LargeArray)
{
    const std::string& json  = LARGE_ARRAY();
    const size_t       bytes = json.size();
    const uint64_t     warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        auto j = nlohmann::json::parse(json, nullptr, false);
        sink += j.is_discarded() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("nlohmann", "Parse", "Large", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Parse", "Large", stats, mem});
}

TEST_F(ParseBench, RapidJson_DOM_LargeArray)
{
    const std::string& json  = LARGE_ARRAY();
    const size_t       bytes = json.size();
    const uint64_t     warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
        sink += doc.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-DOM", "Parse", "Large", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Parse", "Large", stats, mem});
}

TEST_F(ParseBench, RapidJson_SAX_LargeArray)
{
    const std::string& json  = LARGE_ARRAY();
    const size_t       bytes = json.size();
    const uint64_t     warmup = ITER_LARGE / WARMUP_PCT + 1;

    auto stats = BenchmarkRunner::Run(ITER_LARGE, warmup, [&] {
        rapidjson::StringStream ss(json.c_str());
        DiscardHandler          handler;
        rapidjson::Reader       reader;
        reader.Parse(ss, handler);
        sink += reader.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-SAX", "Parse", "Large", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-SAX", "Parse", "Large", stats, mem});
}

// ============================================================================
// DEEPLY_NESTED (30 levels)
// ============================================================================

TEST_F(ParseBench, Thunder_Dynamic_DeeplyNested)
{
    const std::string json(DEEPLY_NESTED());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_DEEP / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_DEEP, warmup, [&] {
        Thunder::Core::JSON::VariantContainer obj;
        obj.FromString(json);
        sink += obj.IsSet() ? 1 : 0;
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-dyn", "Parse", "DeepNested", stats, mem);
    ResultCollector::Instance().Record({"Thunder-dyn", "Parse", "DeepNested", stats, mem});
}

TEST_F(ParseBench, Nlohmann_DeeplyNested)
{
    const std::string json(DEEPLY_NESTED());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_DEEP / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_DEEP, warmup, [&] {
        auto j = nlohmann::json::parse(json, nullptr, false);
        sink += j.is_discarded() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("nlohmann", "Parse", "DeepNested", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Parse", "DeepNested", stats, mem});
}

TEST_F(ParseBench, RapidJson_DOM_DeeplyNested)
{
    const std::string json(DEEPLY_NESTED());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_DEEP / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_DEEP, warmup, [&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
        sink += doc.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-DOM", "Parse", "DeepNested", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Parse", "DeepNested", stats, mem});
}

TEST_F(ParseBench, RapidJson_SAX_DeeplyNested)
{
    const std::string json(DEEPLY_NESTED());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_DEEP / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_DEEP, warmup, [&] {
        rapidjson::StringStream ss(json.c_str());
        DiscardHandler          handler;
        rapidjson::Reader       reader;
        reader.Parse(ss, handler);
        sink += reader.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-SAX", "Parse", "DeepNested", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-SAX", "Parse", "DeepNested", stats, mem});
}

// ============================================================================
// REAL_WORLD (~1 KB)
// ============================================================================

TEST_F(ParseBench, Thunder_Typed_RealWorld)
{
    const std::string json(REAL_WORLD());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_REAL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_REAL, warmup, [&] {
        ControllerStatus cs;
        cs.FromString(json);
        sink += static_cast<int>(cs.Uptime.Value());
    }, bytes);

    MemStats mem{};
    PrintTestResult("Thunder-typed", "Parse", "RealWorld", stats, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Parse", "RealWorld", stats, mem});
}

TEST_F(ParseBench, Nlohmann_RealWorld)
{
    const std::string json(REAL_WORLD());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_REAL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_REAL, warmup, [&] {
        auto j = nlohmann::json::parse(json, nullptr, false);
        sink += j.is_discarded() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("nlohmann", "Parse", "RealWorld", stats, mem);
    ResultCollector::Instance().Record({"nlohmann", "Parse", "RealWorld", stats, mem});
}

TEST_F(ParseBench, RapidJson_DOM_RealWorld)
{
    const std::string json(REAL_WORLD());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_REAL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_REAL, warmup, [&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
        sink += doc.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-DOM", "Parse", "RealWorld", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Parse", "RealWorld", stats, mem});
}

TEST_F(ParseBench, RapidJson_SAX_RealWorld)
{
    const std::string json(REAL_WORLD());
    const size_t      bytes = json.size();
    const uint64_t    warmup = ITER_REAL / WARMUP_PCT;

    auto stats = BenchmarkRunner::Run(ITER_REAL, warmup, [&] {
        rapidjson::StringStream ss(json.c_str());
        DiscardHandler          handler;
        rapidjson::Reader       reader;
        reader.Parse(ss, handler);
        sink += reader.HasParseError() ? 0 : 1;
    }, bytes);

    MemStats mem{};
    PrintTestResult("RapidJSON-SAX", "Parse", "RealWorld", stats, mem);
    ResultCollector::Instance().Record({"RapidJSON-SAX", "Parse", "RealWorld", stats, mem});
}
