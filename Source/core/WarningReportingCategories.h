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

namespace Thunder {
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
            visitor = (_T("It took suspiciously long to acquire a critical section"));
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
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
            visitor += Core::Format(_T(", value %" PRId64 ", max allowed %" PRId64), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 0 };
        static constexpr uint32_t DefaultReportBound = { 0 };
    };

    class EXTERNAL TooLongInvokeRPC {
    public:
        TooLongInvokeRPC(const TooLongInvokeRPC&) = delete;
        TooLongInvokeRPC& operator=(const TooLongInvokeRPC&) = delete;
        TooLongInvokeRPC() = default;
        ~TooLongInvokeRPC() = default;

        uint16_t Serialize(uint8_t buffer[], const uint16_t length) const
        {
            uint16_t serialized = 0;

            if (sizeof(_interfaceId) + sizeof(_methodId) <= length) {
                memcpy(buffer, &_interfaceId, sizeof(_interfaceId));
                serialized += sizeof(_interfaceId);

                memcpy(buffer + serialized, &_methodId, sizeof(_methodId));
                serialized += sizeof(_methodId);
            }

            return serialized;
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
        {
            uint16_t deserialized = 0;

            if (sizeof(_interfaceId) + sizeof(_methodId) <= length) {
                memcpy(&_interfaceId, buffer, sizeof(_interfaceId));
                deserialized += sizeof(_interfaceId);

                memcpy(&_methodId, buffer + deserialized, sizeof(_methodId));
                deserialized += sizeof(_methodId);
            }

            return deserialized;
        }

        bool Analyze(const char[], const char[], const uint32_t interfaceId, const uint32_t methodId)
        {
            _interfaceId = interfaceId;
            _methodId = methodId;

            return true;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = _T("RPC call of method ");
            visitor += Core::Format(_T("[%d] on interface [%d] took to long"), _methodId, _interfaceId);
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 750 };
        static constexpr uint32_t DefaultReportBound = { 250 };

    private:
        uint32_t _interfaceId;
        uint32_t _methodId;
    };

    class EXTERNAL JobTooLongToFinish {
    public:
        JobTooLongToFinish(const JobTooLongToFinish&) = delete;
        JobTooLongToFinish& operator=(const JobTooLongToFinish&) = delete;
        JobTooLongToFinish() = default;
        ~JobTooLongToFinish() = default;

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
            visitor = (_T("Job took too long to finish"));
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 7500 };
        static constexpr uint32_t DefaultReportBound = { 5000 };
    };

    class EXTERNAL JobTooLongWaitingInQueue {
    public:
        JobTooLongWaitingInQueue(const JobTooLongWaitingInQueue&) = delete;
        JobTooLongWaitingInQueue& operator=(const JobTooLongWaitingInQueue&) = delete;
        JobTooLongWaitingInQueue() = default;
        ~JobTooLongWaitingInQueue() = default;

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
            visitor = (_T("Job waiting in queue to be executed took too long"));
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 400 };
        static constexpr uint32_t DefaultReportBound = { 200 };
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
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);

        };

        static constexpr uint32_t DefaultWarningBound = { 20 };
        static constexpr uint32_t DefaultReportBound = { 15 };
    };

    class EXTERNAL JobActiveForTooLong {
    public:
        JobActiveForTooLong(const JobActiveForTooLong&) = delete;
        JobActiveForTooLong& operator=(const JobActiveForTooLong&) = delete;
        JobActiveForTooLong() = default;
        ~JobActiveForTooLong() = default;

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
            visitor = (_T("Job is taking too long to complete the execution, a potentail deadlock issue.."));
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 20000 };
        static constexpr uint32_t DefaultReportBound = { 15000 };
    };

}
}
