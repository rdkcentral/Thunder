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

#include "Module.h"
#include "Portability.h"
#include "SystemInfo.h"

namespace Thunder {
namespace Core {

    class StopWatch {
    public:
        StopWatch(const StopWatch&) = delete;
        StopWatch& operator=(const StopWatch&) = delete;

        StopWatch() : _systemInfo(SystemInfo::Instance()) {
            _lastMeasurement = _systemInfo.Ticks();
        }
        ~StopWatch() {
        }

        public:
        inline uint64_t Elapsed() const {
            return (_systemInfo.Ticks() - _lastMeasurement);
        }
        inline uint64_t Reset() {
            uint64_t now = _systemInfo.Ticks();
            uint64_t result = now - _lastMeasurement;
            _lastMeasurement = now;
            return (result);
        }

    private:
        SystemInfo& _systemInfo;
        uint64_t _lastMeasurement;
    };

} } // namespace Thunder::Core
