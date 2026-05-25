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

#pragma once

// ============================================================================
// BenchmarkUtils.h
//
// Lightweight timing, statistics, memory-tracking, and result-reporting
// utilities for the JSON library evaluation suite.
//
// Usage:
//   auto stats = BenchmarkRunner::Run(100000, 1000, [&]{ parseFunc(); },
//                                     payloadBytes);
//   ResultCollector::Instance().Record({"Thunder", "Parse", "Small", stats, {}});
//   ResultCollector::Instance().PrintSummary();
// ============================================================================

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace JsonBenchmark {

// ============================================================================
// Timing statistics
// ============================================================================

struct BenchStats {
    uint64_t iterations    = 0;
    double   mean_ns       = 0.0;
    double   min_ns        = 0.0;
    double   max_ns        = 0.0;
    double   p50_ns        = 0.0;
    double   p95_ns        = 0.0;
    double   p99_ns        = 0.0;
    double   stddev_ns     = 0.0;
    double   throughput_mbs = 0.0; // MB/s; 0 if no payload_bytes given
};

class BenchmarkRunner {
public:
    // Run fn() `warmup` times (un-timed), then `iterations` times (timed).
    // payload_bytes: used to compute throughput; pass 0 to skip.
    template <typename Fn>
    static BenchStats Run(uint64_t iterations, uint64_t warmup,
                          Fn&& fn, size_t payload_bytes = 0)
    {
        for (uint64_t i = 0; i < warmup; ++i) {
            fn();
        }

        std::vector<double> times;
        times.reserve(static_cast<size_t>(iterations));

        for (uint64_t i = 0; i < iterations; ++i) {
            auto t0 = std::chrono::high_resolution_clock::now();
            fn();
            auto t1 = std::chrono::high_resolution_clock::now();
            times.push_back(static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        }

        return ComputeStats(times, iterations, payload_bytes);
    }

private:
    static BenchStats ComputeStats(std::vector<double>& times,
                                   uint64_t iterations, size_t payload_bytes)
    {
        BenchStats s;
        s.iterations = iterations;
        if (times.empty()) return s;

        std::sort(times.begin(), times.end());
        s.min_ns = times.front();
        s.max_ns = times.back();
        s.p50_ns = Percentile(times, 50.0);
        s.p95_ns = Percentile(times, 95.0);
        s.p99_ns = Percentile(times, 99.0);

        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        s.mean_ns = sum / static_cast<double>(times.size());

        double sq_sum = 0.0;
        for (double t : times) {
            double d = t - s.mean_ns;
            sq_sum += d * d;
        }
        s.stddev_ns = std::sqrt(sq_sum / static_cast<double>(times.size()));

        if (payload_bytes > 0 && s.mean_ns > 0.0) {
            // (bytes / ns) = GB/s  ->  * 1000 = MB/s
            s.throughput_mbs =
                (static_cast<double>(payload_bytes) / s.mean_ns) * 1000.0;
        }
        return s;
    }

    static double Percentile(const std::vector<double>& sorted, double pct)
    {
        if (sorted.empty()) return 0.0;
        double idx = pct / 100.0 * static_cast<double>(sorted.size() - 1);
        auto   lo  = static_cast<size_t>(idx);
        auto   hi  = lo + 1;
        if (hi >= sorted.size()) return sorted.back();
        double frac = idx - static_cast<double>(lo);
        return sorted[lo] + frac * (sorted[hi] - sorted[lo]);
    }
};

// ============================================================================
// Memory tracking  (uses global operator new/delete overrides in MemoryHooks.cpp)
// ============================================================================

struct MemStats {
    size_t allocation_count    = 0; // number of heap allocations
    size_t total_bytes         = 0; // cumulative bytes requested
    size_t peak_bytes          = 0; // max concurrent live bytes
};

// Externally defined in MemoryHooks.cpp
extern std::atomic<bool>   g_mem_tracking;
extern std::atomic<size_t> g_mem_alloc_count;
extern std::atomic<size_t> g_mem_total_bytes;
extern std::atomic<size_t> g_mem_current_bytes;
extern std::atomic<size_t> g_mem_peak_bytes;

// RAII guard: enables tracking on construction, disables and captures stats
// on destruction.  Not thread-safe — use from a single thread only.
class MemoryScope {
public:
    MemoryScope()
    {
        g_mem_alloc_count.store(0, std::memory_order_relaxed);
        g_mem_total_bytes.store(0, std::memory_order_relaxed);
        g_mem_current_bytes.store(0, std::memory_order_relaxed);
        g_mem_peak_bytes.store(0, std::memory_order_relaxed);
        g_mem_tracking.store(true, std::memory_order_seq_cst);
    }

    ~MemoryScope()
    {
        g_mem_tracking.store(false, std::memory_order_seq_cst);
    }

    MemStats Stats() const
    {
        MemStats s;
        s.allocation_count = g_mem_alloc_count.load(std::memory_order_relaxed);
        s.total_bytes      = g_mem_total_bytes.load(std::memory_order_relaxed);
        s.peak_bytes       = g_mem_peak_bytes.load(std::memory_order_relaxed);
        return s;
    }
};

// ============================================================================
// Result collection and summary reporting
// ============================================================================

struct BenchRecord {
    std::string library;   // "Thunder", "nlohmann", "RapidJSON-DOM", "RapidJSON-SAX"
    std::string category;  // "Parse", "Serialize"
    std::string fixture;   // "Small", "Medium", "Large", "DeepNested"
    BenchStats  timing;
    MemStats    memory;
};

class ResultCollector {
public:
    static ResultCollector& Instance()
    {
        static ResultCollector inst;
        return inst;
    }

    void Record(BenchRecord r)
    {
        _records.push_back(std::move(r));
    }

    // Print a comparison table grouped by (category, fixture).
    void PrintSummary() const
    {
        // Group by category + fixture
        std::map<std::string, std::vector<const BenchRecord*>> groups;
        for (const auto& r : _records) {
            groups[r.category + "/" + r.fixture].push_back(&r);
        }

        printf("\n");
        printf("==========================================================\n");
        printf("  JSON Library Evaluation — Performance Summary\n");
        printf("==========================================================\n\n");

        for (const auto& kv : groups) {
            printf("--- %s ---\n", kv.first.c_str());
            printf("  %-18s  %9s  %9s  %9s  %9s  %10s  %10s  %10s\n",
                   "Library", "Mean ns", "Min ns", "P50 ns",
                   "P95 ns", "Throughput", "AllocCnt", "TotalBytes");
            printf("  %-18s  %9s  %9s  %9s  %9s  %10s  %10s  %10s\n",
                   "------------------", "---------", "---------",
                   "---------", "---------", "----------", "----------",
                   "----------");

            for (const BenchRecord* r : kv.second) {
                const BenchStats& t = r->timing;
                const MemStats&   m = r->memory;

                char tp[32];
                if (t.throughput_mbs > 0.0)
                    snprintf(tp, sizeof(tp), "%.1f MB/s", t.throughput_mbs);
                else
                    snprintf(tp, sizeof(tp), "        n/a");

                printf("  %-18s  %9.1f  %9.1f  %9.1f  %9.1f  %10s  %10zu  %10zu\n",
                       r->library.c_str(),
                       t.mean_ns, t.min_ns, t.p50_ns, t.p95_ns,
                       tp,
                       m.allocation_count, m.total_bytes);
            }
            printf("\n");
        }

        printf("==========================================================\n\n");
    }

private:
    std::vector<BenchRecord> _records;
};

// ============================================================================
// Formatting helpers
// ============================================================================

inline std::string FormatNs(double ns)
{
    char buf[64];
    if (ns < 1000.0)
        snprintf(buf, sizeof(buf), "%.1f ns", ns);
    else if (ns < 1000000.0)
        snprintf(buf, sizeof(buf), "%.2f us", ns / 1000.0);
    else
        snprintf(buf, sizeof(buf), "%.2f ms", ns / 1000000.0);
    return buf;
}

// Print a per-test result to stdout so it appears in the GTest output.
inline void PrintTestResult(const char* lib, const char* category,
                             const char* fixture, const BenchStats& s,
                             const MemStats& m)
{
    printf("  [BENCH] %-14s %-12s %-12s  mean=%s  p95=%s  allocs=%zu  bytes=%zu\n",
           lib, category, fixture,
           FormatNs(s.mean_ns).c_str(), FormatNs(s.p95_ns).c_str(),
           m.allocation_count, m.total_bytes);
    fflush(stdout);
}

} // namespace JsonBenchmark
