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

#pragma once

#include <string>
#include <time.h>
#include <atomic>
#include <functional>

#ifndef MODULE_NAME
#include "Module.h"
#endif

#include <core/core.h>

#ifndef __LINUX__
static_assert(false, "Only LINUX is supported");
#endif

class IPTestAdministrator
{
public :

    using Callback =  std::function<void (IPTestAdministrator&)>;

    IPTestAdministrator(Callback /* executed by parent */, Callback /* executed by child */, const uint32_t initHandshakeValue, const uint32_t waitTime /* seconds */);
    ~IPTestAdministrator();

    IPTestAdministrator(const IPTestAdministrator&) = delete;
    const IPTestAdministrator& operator=(const IPTestAdministrator&) = delete;

    uint32_t Wait(uint32_t expectedHandshakeValue) const;
    uint32_t Signal(uint32_t expectedNextHandshakeValue, uint8_t maxRetries = 0);

    uint32_t WaitTimeDivisor() const {
        return _waitTimeDivisor;
    }

private :

    struct SharedData
    {
        // std::atomic integral> has standard layout!
        // A pointer may be converted to a pointer of the first non-static data element with reinterpret_cast
        std::atomic<uint32_t> handshakeValue;
    };

    constexpr static uint32_t _waitTimeDivisor = 10;

    SharedData* _sharedData;
    int _shm_id;
    pid_t _pid;
    uint32_t _waitTime; // Seconds
};
