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
#include "MessageStore.h"

namespace Thunder {

namespace WarningReporting { 

    struct EXTERNAL IWarningEvent { 

        virtual ~IWarningEvent() = default;
        virtual const char* Category() const = 0;
        virtual uint16_t Serialize(uint8_t[], const uint16_t) const = 0;
        virtual uint16_t Deserialize(const uint8_t[], const uint16_t) = 0;
        virtual void ToString(string& text) const = 0;
        virtual bool IsWarning() const = 0; 
    };

    struct EXTERNAL IWarningReportingUnit {

        struct EXTERNAL IWarningReportingControl : public Core::Messaging::IControl {
            virtual void Exclude(const string& toExclude) = 0;
            virtual void Configure(const string& setting) = 0;
            virtual IWarningEvent* Clone() const = 0;
        };

        virtual ~IWarningReportingUnit() = default;
        virtual void ReportWarningEvent(const char identifier[], const IWarningEvent& information) = 0;
        virtual void FetchCategoryInformation(const string& category, bool& outIsDefaultCategory, bool& outIsEnabled, string& outExcluded, string& outConfiguration) const = 0;
        virtual void AddToCategoryList(IWarningReportingControl& Category) = 0;
        virtual void RemoveFromCategoryList(IWarningReportingControl& Category) = 0;
    };

}
}


