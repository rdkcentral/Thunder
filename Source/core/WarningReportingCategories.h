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

namespace WPEFramework {
namespace WarningReporting {

    class EXTERNAL TooLongWaitingForLock {
    public:
        TooLongWaitingForLock(const TooLongWaitingForLock& a_Copy) = delete;
        TooLongWaitingForLock& operator=(const TooLongWaitingForLock& a_RHS) = delete;

        TooLongWaitingForLock()
        {
        }

        ~TooLongWaitingForLock() = default;

        uint16_t Serialize(uint8_t[], const uint16_t) const {
            // HPL Todo: implement
            return(0); 
        }

        uint16_t Deserialize(const uint8_t[], const uint16_t) {
            // HPL Todo: implement
            return(0);
        }

        void ToString(string& visitor) const {
            visitor = (_T("it took suspiciously long to aquire a critical section"));
        };

        static constexpr uint32_t DefaultWarningBound = {1000};
        static constexpr uint32_t DefaultReportBound = {1000};
    };

    class EXTERNAL SinkStillHasReference {
    public:
        SinkStillHasReference(const SinkStillHasReference& a_Copy) = delete;
        SinkStillHasReference& operator=(const SinkStillHasReference& a_RHS) = delete;

        SinkStillHasReference(const uint32_t currentRefCount):
            _currentRefCount(currentRefCount)
        {
        }

        ~SinkStillHasReference() = default;

        uint16_t Serialize(uint8_t buffer[], const uint16_t length) const {
            ASSERT(length >= 4);
            memcpy(buffer, &_currentRefCount, sizeof(uint32_t));
            
            /*
            ASSERT(length >= 4);
            uint32_t val = htonl(_currentRefCount);
            memcpy(buffer, &val, sizeof(uint32_t));
            */
            /*
            buffer[0] = _currentRefCount & 0xFF;
            buffer[1] = (_currentRefCount >> 8 ) & 0xFF;
            buffer[2] = (_currentRefCount >> 16 ) & 0xFF;
            buffer[3] = (_currentRefCount >> 24 ) & 0xFF;
            */

            return sizeof(uint32_t); 
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length) {
            ASSERT(length >= 4);
            memcpy(&_currentRefCount, buffer, sizeof(uint32_t));
            
            /*
            ASSERT(length >= 4);
            uint32_t val = 0;
            memcpy(&val, buffer, sizeof(uint32_t));
            _currentRefCount = ntohl(val);
            */
           
            //_currentRefCount = buffer[0] | (buffer[1]  << 8 ) | (buffer[2]  << 16 ) | (buffer[3]  << 24 );  
            return sizeof(uint32_t);
        }

        void ToString(string& visitor) const {
            visitor = (_T("A sink still holds a reference when it is being destructed"));
        };

        uint32_t _currentRefCount;
        static constexpr uint32_t DefaultWarningBound = {0};
        static constexpr uint32_t DefaultReportBound = {0};
    };
    
}
} 


