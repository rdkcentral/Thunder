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
// MemoryHooks.cpp
//
// Overrides the global operator new/delete to track heap allocations when
// JsonBenchmark::g_mem_tracking is true.
//
// Important: tracking is guarded by an atomic flag so GTest's own allocations
// are not counted — only allocations that occur between MemoryScope construction
// and destruction are recorded.
//
// This file MUST be compiled as part of the benchmark executable (not as a
// shared library) so that the linker resolves the global new/delete from here.
// ============================================================================

#include "BenchmarkUtils.h"

#include <atomic>
#include <cstdlib>
#include <new>

namespace JsonBenchmark {

std::atomic<bool>   g_mem_tracking{false};
std::atomic<size_t> g_mem_alloc_count{0};
std::atomic<size_t> g_mem_total_bytes{0};
std::atomic<size_t> g_mem_current_bytes{0};
std::atomic<size_t> g_mem_peak_bytes{0};

} // namespace JsonBenchmark

// ---------------------------------------------------------------------------
// Global operator new / delete replacements
// ---------------------------------------------------------------------------

void* operator new(std::size_t size)
{
    void* ptr = std::malloc(size);
    if (!ptr) {
        // Cannot throw (callers may build with -fno-exceptions).  Terminate
        // immediately; OOM is unrecoverable in embedded contexts anyway.
        std::abort();
    }

    if (JsonBenchmark::g_mem_tracking.load(std::memory_order_relaxed)) {
        JsonBenchmark::g_mem_alloc_count.fetch_add(1, std::memory_order_relaxed);
        JsonBenchmark::g_mem_total_bytes.fetch_add(size, std::memory_order_relaxed);

        size_t cur  = JsonBenchmark::g_mem_current_bytes.fetch_add(
                          size, std::memory_order_relaxed) + size;
        size_t peak = JsonBenchmark::g_mem_peak_bytes.load(std::memory_order_relaxed);
        while (cur > peak) {
            if (JsonBenchmark::g_mem_peak_bytes.compare_exchange_weak(
                    peak, cur, std::memory_order_relaxed)) {
                break;
            }
        }
    }
    return ptr;
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}

// C++14 sized deallocation — allows tracking live bytes accurately.
void operator delete(void* ptr, std::size_t size) noexcept
{
    if (JsonBenchmark::g_mem_tracking.load(std::memory_order_relaxed)) {
        JsonBenchmark::g_mem_current_bytes.fetch_sub(size, std::memory_order_relaxed);
    }
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept
{
    if (JsonBenchmark::g_mem_tracking.load(std::memory_order_relaxed)) {
        JsonBenchmark::g_mem_current_bytes.fetch_sub(size, std::memory_order_relaxed);
    }
    std::free(ptr);
}
