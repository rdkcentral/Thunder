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
#include <functional>

namespace WPEFramework {

namespace WarningReporting { 

    struct IWarningEvent { 

        using SerializeVisitor = std::function<void(const uint8_t[], const uint16_t)>;
        using ToStringVisitor = std::function<void(const string&)>;

        virtual ~IWarningEvent() = default;
        virtual const char* Category() const = 0;
        virtual void Serialize(const SerializeVisitor& visitor) const = 0;
        virtual void Deserialize(const uint8_t[], const uint16_t) = 0;
        virtual void ToString(const ToStringVisitor& visitor) const = 0;
        virtual bool IsWarning() const = 0; 
    };

    struct IWarningReportingUnit {

        struct IWarningReportingControl {
            virtual ~IWarningReportingControl() = default;
            virtual void Destroy() = 0;
            virtual const char* Category() const = 0;
            virtual bool Enabled() const = 0;
            virtual void Enabled(const bool enabled) = 0;
            virtual void Configure(const string& setting) = 0;
        };

        virtual ~IWarningReportingUnit() = default;
        virtual void ReportWarningEvent(const char identifier[], const char fileName[], const uint32_t lineNumber, const char className[], const IWarningEvent& information) = 0;
        virtual bool IsDefaultCategory(const string& category, bool& enabled, string& configuration) const = 0;
        virtual void Announce(IWarningReportingControl& Category) = 0;
        virtual void Revoke(IWarningReportingControl& Category) = 0;
    };

}
}


