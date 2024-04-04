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

#include <gtest/gtest.h>

#include <future>

#include "../../cyclic-buffer/process.h"


namespace WPEFramework {
namespace Core {
namespace Tests {

#define ASYNC_TIMEOUT_BEGIN                                               \
    std::promise<bool> promise;                                           \
    std::future<bool> future = promise.get_future();                      \
    std::thread([&](std::promise<bool> completed)                         \
        {   /* Before code that should complete before timeout expires */

#define ASYNC_TIMEOUT_END(MILLISECONDS /* timeout in milliseconds */, EXPECTATION /* boolean */)                                                \
            /* After code that should complete timely */                                                                                        \
            /* completed.set_value(true); */                                                                                                    \
            completed.set_value_at_thread_exit(true);                                                                                           \
        }                                                                                                                                       \
        , std::move(promise)).detach()                                                                                                          \
    ;                                                                                                                                           \
    bool result = future.wait_for(std::chrono::milliseconds(MILLISECONDS)) != std::future_status::timeout;  /* Task completed before timeout */ \
    EXPECT_TRUE(result == EXPECTATION); /* Task completed before timeout */                                                                     \
    if (!result) {                                                                                                                              \
        TRACE_L1(_T("Error : Stopping unresposive process."));                                                                                  \
        killpg(getpgrp(), SIGUSR1); /* Possible 'unresponsive' system, 'unlock' all related 'child' processes, default action is terminate */   \
    }

TEST(Core_CyclicBuffer, DataExchangeTimeout)
{
    using namespace WPEFramework::Tests;

    constexpr uint8_t maxChildren = 1;

    constexpr uint32_t memoryMappedFileRequestedSize = 30;//446;
    constexpr uint32_t internalBufferSize = 40;//446;

    constexpr char fileName[] = "/tmp/SharedCyclicBuffer";

    constexpr uint32_t totalRuntime = Core::infinite; // Milliseconds
    constexpr uint32_t timeout = 10000; // Milliseconds

    Process<memoryMappedFileRequestedSize, internalBufferSize, maxChildren> process(fileName);

    ASYNC_TIMEOUT_BEGIN; // Avoid leaking resources, eg, children

    EXPECT_TRUE(process.SetTotalRuntime(totalRuntime)
                  && process.SetNumReservedBlocks(2)
                  && process.SetParentUsers(0, 0) /* 0 extra writer(s), 0 reader(s) */
                  && process.SetChildUsers(0, 1) /* 1 writer(s), 1 reader(s) */
                  && process.Execute()
               );

    ASYNC_TIMEOUT_END(timeout, false /* expect no timeout */);
}

TEST(Core_CyclicBuffer, DataExchange)
{
    using namespace WPEFramework::Tests;

    constexpr uint8_t maxChildren = 1;

    constexpr uint32_t memoryMappedFileRequestedSize = 30;//446;
    constexpr uint32_t internalBufferSize = 40;//446;

    constexpr char fileName[] = "/tmp/SharedCyclicBuffer";

    constexpr uint32_t totalRuntime = 1000; // Milliseconds
    constexpr uint32_t timeout = totalRuntime + 10000; // Milliseconds

    Process<memoryMappedFileRequestedSize, internalBufferSize, maxChildren> process(fileName);

    ASYNC_TIMEOUT_BEGIN; // Avoid leaking resources, eg, children

    EXPECT_TRUE(process.SetTotalRuntime(totalRuntime)
                  && process.SetNumReservedBlocks(2)
                  && process.SetParentUsers(0, 0) /* 0 extra writer(s), 0 reader(s) */
                  && process.SetChildUsers(0, 1) /* 1 writer(s), 1 reader(s) */
                  && process.Execute()
               );

    ASYNC_TIMEOUT_END(timeout, true /* expect no timeout */);
}

} // Tests
} // Core
} // WPEFramework
