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

    struct EXTERNAL IComposition : virtual public Core::IUnknown {
        enum { ID = ID_COMPOSITION };

        static constexpr uint32_t maxOpacity = 255;
        static constexpr uint32_t minOpacity = 0;

        static constexpr uint32_t maxZOrder = 255;
        static constexpr uint32_t minZOrder = 0;

        enum ScreenResolution : uint8_t {
            ScreenResolution_Unknown = 0,
            ScreenResolution_480i = 1,
            ScreenResolution_480p = 2,
            ScreenResolution_720p = 3,
            ScreenResolution_720p50Hz = 4,
            ScreenResolution_1080p24Hz = 5,
            ScreenResolution_1080i50Hz = 6,
            ScreenResolution_1080p50Hz = 7,
            ScreenResolution_1080p60Hz = 8,
            ScreenResolution_2160p50Hz = 9,
            ScreenResolution_2160p60Hz = 10
        };

        static uint32_t WidthFromResolution(const ScreenResolution resolution);
        static uint32_t HeightFromResolution(const ScreenResolution resolution);

        struct Rectangle {
            uint32_t x;
            uint32_t y;
            uint32_t width;
            uint32_t height;
        };

        struct EXTERNAL IClient : virtual public Core::IUnknown {
            enum { ID = ID_COMPOSITION_CLIENT };

            virtual string Name() const = 0;
            virtual void Opacity(const uint32_t value) = 0;
            virtual uint32_t Geometry(const Rectangle& rectangle) = 0;
            virtual Rectangle Geometry() const = 0; 
            virtual uint32_t ZOrder(const uint16_t index) = 0;
            virtual uint32_t ZOrder() const = 0;
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_COMPOSITION_NOTIFICATION };

            virtual void Attached(const string& name, IClient* client) = 0;
            virtual void Detached(const string& name) = 0;
        };

        virtual void Register(IComposition::INotification* notification) = 0;
        virtual void Unregister(IComposition::INotification* notification) = 0;

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;

        // Set and get output resolution
        virtual uint32_t Resolution(const ScreenResolution) = 0;
        virtual ScreenResolution Resolution() const = 0;
    };
}
}

