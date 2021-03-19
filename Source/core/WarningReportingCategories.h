/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "IWarningReportingControl.h" 

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

        void Serialize(const IWarningEvent::SerializeVisitor& visitor) const {
            // HPL Todo: implement
            visitor(nullptr, 0); 
        }

        void Deserialize(const uint8_t[], const uint16_t) {
            // HPL Todo: implement
        }

        void ToString(const IWarningEvent::ToStringVisitor& visitor) const {
            string text(_T("it took suspiciously long to aquire a critical section"));
            visitor(text);
        };

        static constexpr uint32_t DefaultWarningBound = {1000};
        static constexpr uint32_t DefaultReportBound = {1000};
    };

    class EXTERNAL SinkStillHasReference {
    public:
        SinkStillHasReference(const SinkStillHasReference& a_Copy) = delete;
        SinkStillHasReference& operator=(const SinkStillHasReference& a_RHS) = delete;

        SinkStillHasReference() 
        {
        }

        ~SinkStillHasReference() = default;

        void Serialize(const IWarningEvent::SerializeVisitor& visitor) const {
            // HPL Todo: implement
            visitor(nullptr, 0); 
        }

        void Deserialize(const uint8_t[], const uint16_t) {
            // HPL Todo: implement
        }

        void ToString(const IWarningEvent::ToStringVisitor& visitor) const {
            string text(_T("A sink still holds a reference when it is being destructed"));
            visitor(text);
        };

        static constexpr uint32_t DefaultWarningBound = {0};
        static constexpr uint32_t DefaultReportBound = {0};
    };
    
}
} 


