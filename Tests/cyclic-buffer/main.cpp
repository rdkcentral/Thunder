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

#include "process.h"

int main(int argc, char* argv[])
{
    using namespace WPEFramework::Core;
    using namespace WPEFramework::Tests;

    constexpr uint8_t maxChildren = 3;

    constexpr uint32_t memoryMappedFileRequestedSize = 446;
    constexpr uint32_t internalBufferSize = 446;

    constexpr char fileName[] = "/tmp/SharedCyclicBuffer";

    constexpr uint32_t totalRuntime = 10000; // Milliseconds

    Process<memoryMappedFileRequestedSize, internalBufferSize, maxChildren> process(fileName);

    bool result =    process.SetTotalRuntime(totalRuntime)
                  && process.Execute()
                  ;

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
