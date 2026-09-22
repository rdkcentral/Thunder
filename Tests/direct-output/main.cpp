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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <vector>

#include "Logging.h"
#include "LoggingCategories.h"
#include "MessageUnit.h"

namespace {

constexpr size_t DefaultIterations = 10000;

uint64_t NowNanoseconds()
{
    struct timespec time;

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    return (static_cast<uint64_t>(time.tv_sec) * UINT64_C(1000000000)) + static_cast<uint64_t>(time.tv_nsec);
}

uint64_t Percentile(const std::vector<uint64_t>& samples, const unsigned int percentage)
{
    const size_t index = ((samples.size() - 1) * percentage) / 100;

    return samples[index];
}

size_t ParseIterations(const char* value)
{
    char* end = nullptr;
    const unsigned long long parsed = strtoull(value, &end, 10);

    if ((value[0] == '\0') || (*end != '\0') || (parsed == 0) || (parsed > SIZE_MAX)) {
        fprintf(stderr, "Invalid iteration count: %s\n", value);
        exit(EXIT_FAILURE);
    }

    return static_cast<size_t>(parsed);
}

bool ParseEnabled(const char* value)
{
    if (strcmp(value, "enabled") == 0) {
        return true;
    }
    if (strcmp(value, "disabled") == 0) {
        return false;
    }

    fprintf(stderr, "Invalid mode: %s (expected enabled or disabled)\n", value);
    exit(EXIT_FAILURE);
}

bool ParseHandler(const char* value)
{
    if (strcmp(value, "direct") == 0) {
        return false;
    }
    if (strcmp(value, "handler") == 0) {
        return true;
    }

    fprintf(stderr, "Invalid route: %s (expected direct or handler)\n", value);
    exit(EXIT_FAILURE);
}

}

int main(int argc, char* argv[])
{
    const size_t iterations = (argc > 1) ? ParseIterations(argv[1]) : DefaultIterations;
    const char* mode = (argc > 2) ? argv[2] : "enabled";
    const char* route = (argc > 3) ? argv[3] : "direct";
    const bool enabled = ParseEnabled(mode);
    const bool handler = ParseHandler(route);
    Thunder::Messaging::MessageUnit& messageUnit = Thunder::Messaging::MessageUnit::Instance();
    Thunder::Messaging::MessageUnit::Settings::Config configuration;
    std::vector<uint64_t> samples(iterations);
    uint64_t total = 0;

    configuration.Output = handler ? Thunder::Core::Messaging::OutputMode::HANDLER : Thunder::Core::Messaging::OutputMode::DIRECT;
    configuration.Out = false;
    configuration.Error = false;

    if (handler == true) {
        configuration.DataSize = Thunder::Messaging::MessageUnit::MaxDataBufferSize;
    }

    SYSLOG_ANNOUNCE(Thunder::Logging::Startup);

    if (messageUnit.Open("/tmp/thunder-direct-output-benchmark", configuration, false, Thunder::Messaging::MessageUnit::flush::OFF) != Thunder::Core::ERROR_NONE) {
        fprintf(stderr, "MessageUnit::Open failed\n");
        return EXIT_FAILURE;
    }
    Thunder::Logging::Startup::Enable(enabled);

    for (size_t index = 0; index < 1000; ++index) {
        SYSLOG(Thunder::Logging::Startup, ("benchmark message sequence=%u", static_cast<unsigned>(index)));
    }

    for (size_t index = 0; index < iterations; ++index) {
        const uint64_t start = NowNanoseconds();
        SYSLOG(Thunder::Logging::Startup, ("benchmark message sequence=%u", static_cast<unsigned>(index)));
        samples[index] = NowNanoseconds() - start;
        total += samples[index];
    }

    std::sort(samples.begin(), samples.end());

    fprintf(stderr, "output=thunder route=%s mode=%s iterations=%zu\n", route, mode, iterations);
    fprintf(stderr, "latency_us min=%.3f avg=%.3f median=%.3f p90=%.3f p95=%.3f p99=%.3f\n",
        samples[0] / 1000.0,
        (total / static_cast<double>(iterations)) / 1000.0,
        Percentile(samples, 50) / 1000.0,
        Percentile(samples, 90) / 1000.0,
        Percentile(samples, 95) / 1000.0,
        Percentile(samples, 99) / 1000.0);

    messageUnit.Close();
    return EXIT_SUCCESS;
}