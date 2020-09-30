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

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IGraphicsProperties : virtual public Core::IUnknown {
        enum { ID = ID_GRAPHICS_PROPERTIES };

        virtual uint64_t TotalGpuRam() const = 0;
        virtual uint64_t FreeGpuRam() const = 0;
    };

    struct EXTERNAL IConnectionProperties : virtual public Core::IUnknown {
        enum { ID = ID_CONNECTION_PROPERTIES };

        enum HDRType : uint8_t {
            HDR_OFF,
            HDR_10,
            HDR_10PLUS,
            HDR_DOLBYVISION,
            HDR_TECHNICOLOR
        };

        enum HDCPProtectionType : uint8_t {
            HDCP_Unencrypted,
            HDCP_1X,
            HDCP_2X
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_CONNECTION_PROPERTIES_NOTIFICATION };

            virtual void Updated() = 0;
        };

        virtual uint32_t Register(INotification*) = 0;
        virtual uint32_t Unregister(INotification*) = 0;

        virtual bool IsAudioPassthrough () const = 0;
        virtual bool Connected() const = 0;
        virtual uint32_t Width() const = 0;
        virtual uint32_t Height() const = 0;
        virtual uint32_t VerticalFreq() const = 0;
        virtual HDRType Type() const = 0;
        virtual HDCPProtectionType HDCPProtection() const = 0;
    };
}
}
