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
#include <iostream>

namespace WPEFramework {
namespace WarningReporting {

    class EXTERNAL TooLongWaitingForLock {
    public:
        TooLongWaitingForLock(const TooLongWaitingForLock&) = delete;
        TooLongWaitingForLock& operator=(const TooLongWaitingForLock&) = delete;
        TooLongWaitingForLock() = default;
        ~TooLongWaitingForLock() = default;

        //nothing to serialize/deserialize here
        uint16_t Serialize(uint8_t[], const uint16_t) const
        {
            return 0;
        }

        uint16_t Deserialize(const uint8_t[], const uint16_t)
        {
            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = (_T("It took suspiciously long to aquire a critical section"));
            visitor += Core::Format(_T(", value %lld [ms], max allowed %lld [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 1000 };
        static constexpr uint32_t DefaultReportBound = { 1000 };
    };

    class EXTERNAL SinkStillHasReference {
    public:
        SinkStillHasReference(const SinkStillHasReference&) = delete;
        SinkStillHasReference& operator=(const SinkStillHasReference&) = delete;
        SinkStillHasReference() = default;
        ~SinkStillHasReference() = default;

        //nothing to serialize/deserialize here
        uint16_t Serialize(uint8_t[], const uint16_t) const
        {
            return 0;
        }

        uint16_t Deserialize(const uint8_t[], const uint16_t)
        {
            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = (_T("A sink still holds a reference when it is being destructed"));
            visitor += Core::Format(_T(", value %lld, max allowed %lld"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 0 };
        static constexpr uint32_t DefaultReportBound = { 0 };
    };


    class EXTERNAL TooLongDecrypt {
    public:
        TooLongDecrypt(const TooLongDecrypt&) = delete;
        TooLongDecrypt& operator=(const TooLongDecrypt&) = delete;
        TooLongDecrypt() = default;
        ~TooLongDecrypt() = default;

        //nothing to serialize/deserialize here
        uint16_t Serialize(uint8_t[], const uint16_t) const
        {
            return 0;
        }

        uint16_t Deserialize(const uint8_t[], const uint16_t)
        {
            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = (_T("Decrypt call took too long"));
            visitor += Core::Format(_T(", value %lld [ms], max allowed %lld [ms]"), actualValue, maxValue);

        };

        static constexpr uint32_t DefaultWarningBound = { 20 };
        static constexpr uint32_t DefaultReportBound = { 15 };
    };
}
}
