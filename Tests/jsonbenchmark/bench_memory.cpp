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
// bench_memory.cpp
//
// Memory allocation benchmarks for all three JSON libraries.
//
// Methodology:
//   - Enable the global operator new/delete tracking hooks (MemoryHooks.cpp)
//     via MemoryScope RAII guard.
//   - Parse/serialize each fixture exactly once inside the scope.
//   - Record: allocation count, total bytes allocated, peak live bytes.
//
// Because the global hooks capture ALL allocations while tracking is enabled,
// the measurement includes incidental std::string growth etc.  Run each test
// body with no other concurrent allocations for the most accurate picture.
//
// Output key:
//   AllocCnt   — number of operator new() calls (lower is better for caches)
//   TotalBytes — sum of all size arguments to operator new() (heap footprint)
//   PeakBytes  — max concurrent live bytes (working-set high watermark)
// ============================================================================

#include <gtest/gtest.h>

#include <core/JSON.h>

#include <nlohmann/json.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "BenchmarkUtils.h"
#include "TestFixtures.h"

using namespace JsonBenchmark;
using namespace JsonBenchmark::Fixtures;

// ============================================================================
// Helper: measure a single operation and return MemStats
// ============================================================================
template <typename Fn>
static MemStats MeasureAlloc(Fn&& fn)
{
    MemoryScope scope;
    fn();
    return scope.Stats();
}

class MemoryBench : public ::testing::Test {};

// ============================================================================
// Parse memory — SMALL_CONFIG
// ============================================================================

TEST_F(MemoryBench, Thunder_Typed_Parse_Small)
{
    const std::string json(SMALL_CONFIG());

    auto mem = MeasureAlloc([&] {
        PluginConfig cfg;
        cfg.FromString(json);
    });

    PrintTestResult("Thunder-typed", "Mem-Parse", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Mem-Parse", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, Thunder_Dynamic_Parse_Small)
{
    const std::string json(SMALL_CONFIG());

    auto mem = MeasureAlloc([&] {
        Thunder::Core::JSON::VariantContainer obj;
        obj.FromString(json);
    });

    PrintTestResult("Thunder-dyn", "Mem-Parse", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"Thunder-dyn", "Mem-Parse", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, Nlohmann_Parse_Small)
{
    const std::string json(SMALL_CONFIG());

    auto mem = MeasureAlloc([&] {
        auto j = nlohmann::json::parse(json, nullptr, false);
        (void)j;
    });

    PrintTestResult("nlohmann", "Mem-Parse", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"nlohmann", "Mem-Parse", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, RapidJson_DOM_Parse_Small)
{
    const std::string json(SMALL_CONFIG());

    auto mem = MeasureAlloc([&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
    });

    PrintTestResult("RapidJSON-DOM", "Mem-Parse", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Mem-Parse", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

// RapidJSON SAX with DiscardHandler performs zero heap allocations — the
// reader object is stack-allocated and the input is read in place.
TEST_F(MemoryBench, RapidJson_SAX_Parse_Small)
{
    const std::string json(SMALL_CONFIG());

    struct Discard {
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

    auto mem = MeasureAlloc([&] {
        rapidjson::StringStream ss(json.c_str());
        Discard handler;
        rapidjson::Reader reader;
        reader.Parse(ss, handler);
    });

    PrintTestResult("RapidJSON-SAX", "Mem-Parse", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"RapidJSON-SAX", "Mem-Parse", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

// ============================================================================
// Parse memory — MEDIUM_RESPONSE
// ============================================================================

TEST_F(MemoryBench, Thunder_Typed_Parse_Medium)
{
    const std::string json(MEDIUM_RESPONSE());

    auto mem = MeasureAlloc([&] {
        JsonRpcDeviceResponse rsp;
        rsp.FromString(json);
    });

    PrintTestResult("Thunder-typed", "Mem-Parse", "Medium", BenchStats{}, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Mem-Parse", "Medium", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, Nlohmann_Parse_Medium)
{
    const std::string json(MEDIUM_RESPONSE());

    auto mem = MeasureAlloc([&] {
        auto j = nlohmann::json::parse(json, nullptr, false);
        (void)j;
    });

    PrintTestResult("nlohmann", "Mem-Parse", "Medium", BenchStats{}, mem);
    ResultCollector::Instance().Record({"nlohmann", "Mem-Parse", "Medium", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, RapidJson_DOM_Parse_Medium)
{
    const std::string json(MEDIUM_RESPONSE());

    auto mem = MeasureAlloc([&] {
        rapidjson::Document doc;
        doc.Parse(json.c_str(), json.size());
    });

    PrintTestResult("RapidJSON-DOM", "Mem-Parse", "Medium", BenchStats{}, mem);
    ResultCollector::Instance().Record({"RapidJSON-DOM", "Mem-Parse", "Medium", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

// ============================================================================
// Serialize memory — SMALL_CONFIG
// ============================================================================

TEST_F(MemoryBench, Thunder_Typed_Serialize_Small)
{
    PluginConfig cfg;
    cfg.FromString(std::string(SMALL_CONFIG()));

    auto mem = MeasureAlloc([&] {
        std::string out;
        cfg.ToString(out);
    });

    PrintTestResult("Thunder-typed", "Mem-Ser", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"Thunder-typed", "Mem-Ser", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, Nlohmann_Serialize_Small)
{
    nlohmann::json j;
    j["callsign"]  = "TestPlugin";
    j["classname"] = "TestPlugin";
    j["autostart"] = true;
    j["configuration"] = {{"delay",5},{"maxretries",3},{"endpoint","http://localhost:80"},
                           {"loglevel",2},{"token","abc123XYZ"}};

    auto mem = MeasureAlloc([&] {
        std::string out = j.dump();
        (void)out;
    });

    PrintTestResult("nlohmann", "Mem-Ser", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"nlohmann", "Mem-Ser", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

TEST_F(MemoryBench, RapidJson_Serialize_Small)
{
    rapidjson::Document d;
    d.SetObject();
    auto& alloc = d.GetAllocator();
    d.AddMember("callsign",  "TestPlugin",          alloc);
    d.AddMember("classname", "TestPlugin",          alloc);
    d.AddMember("autostart", true,                  alloc);

    rapidjson::Value cfg(rapidjson::kObjectType);
    cfg.AddMember("delay",      5,                    alloc);
    cfg.AddMember("maxretries", 3,                    alloc);
    cfg.AddMember("endpoint",   "http://localhost:80", alloc);
    cfg.AddMember("loglevel",   2,                    alloc);
    cfg.AddMember("token",      "abc123XYZ",          alloc);
    d.AddMember("configuration", cfg, alloc);

    auto mem = MeasureAlloc([&] {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        (void)sb.GetSize();
    });

    PrintTestResult("RapidJSON", "Mem-Ser", "Small", BenchStats{}, mem);
    ResultCollector::Instance().Record({"RapidJSON", "Mem-Ser", "Small", {}, mem});
    printf("    AllocCount=%zu  TotalBytes=%zu  PeakBytes=%zu\n",
           mem.allocation_count, mem.total_bytes, mem.peak_bytes);
    fflush(stdout);
}

// ============================================================================
// Summary test — prints the full table at the end of the memory suite
// ============================================================================

TEST_F(MemoryBench, ZZZ_PrintSummary)
{
    // "ZZZ_" prefix ensures this runs last in alphabetical GTest ordering
    ResultCollector::Instance().PrintSummary();
}
