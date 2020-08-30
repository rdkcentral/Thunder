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

// @stubgen:include "IRPCIterator.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IGraphicsProperties : virtual public Core::IUnknown {
        enum { ID = ID_GRAPHICS_PROPERTIES };

        virtual uint64_t TotalGpuRam() const = 0;
        virtual uint64_t FreeGpuRam() const = 0;
    };

    /* @json */
    struct EXTERNAL IConnectionProperties : virtual public Core::IUnknown {
        enum { ID = ID_CONNECTION_PROPERTIES };

        enum HDCPProtectionType : uint8_t {
            HDCP_Unencrypted,
            HDCP_1X,
            HDCP_2X
        };

        /* @event */
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_CONNECTION_PROPERTIES_NOTIFICATION };

            enum Source : uint8_t {
                PRE_RESOLUTION_CHANGE,
                POST_RESOLUTION_CHANGE,
                HDCP_CHANGE,
            };

            virtual void Updated(const Source) = 0;
        };

        virtual uint32_t Register(INotification*) = 0;
        virtual uint32_t Unregister(INotification*) = 0;

        // @property
        // @brief Is audioPassthrough on HDMI
        // @param passthru: enabled/disabled
        virtual uint32_t IsAudioPassthrough (bool& passthru /* @out */) const = 0;

        // @property
        // @brief Is HDMI connected
        // @param isconnected: connected/disconnected
        virtual uint32_t Connected(bool& isconnected /* @out */) const = 0;

        // @property
        // @brief Horizontal resolution of TV
        // @param width:  width of TV in pixels
        virtual uint32_t Width(uint32_t& width /* @out */) const = 0;

        // @property
        // @brief Vertical resolution of TV
        // @param height:  height of TV in pixels
        virtual uint32_t Height(uint32_t& height /* @out */) const = 0;

        // @property
        // @brief Vertical Frequency
        // @param vf: vertical freq
        virtual uint32_t VerticalFreq(uint32_t& vf /* @out */) const = 0;

        // @brief TV's Extended Display Identification Data
        // @param edid: edid byte string
        // TODO: @sebas, I need to make the length parameter const. Why ?
        virtual uint32_t EDID (const uint16_t& length /* @inout */, uint8_t data[] /* @out @length:length */) const = 0;

        // @property
        // @brief HDCP protocol used for transmission
        // @param value: protocol
        virtual uint32_t HDCPProtection (HDCPProtectionType& value /* @out */) const = 0;
        virtual uint32_t HDCPProtection (const HDCPProtectionType& value) = 0;

        // @property
        // @brief Video output port on the STB used for connection to TV
        // @param name: video output port name
        virtual uint32_t PortName (string& name /* @out */) const = 0;
    };

    /* @json */
    struct EXTERNAL IHDRProperties : virtual public Core::IUnknown {
        enum { ID = ID_HDR_PROPERTIES };

        enum HDRType : uint8_t {
            HDR_OFF,
            HDR_10,
            HDR_10PLUS,
            HDR_HLG,
            HDR_DOLBYVISION,
            HDR_TECHNICOLOR
        };

        typedef RPC::IIteratorType<HDRType, ID_HDR_ITERATOR> IHDRIterator;

        // @property
        // @brief HDR formats supported by TV
        // @return HDRType: array of HDR formats
        virtual uint32_t TVCapabilities(IHDRIterator* type /* out */) const = 0;

        // @property
        // @brief HDR formats supported by STB
        // @return HDRType: array of HDR formats
        virtual uint32_t STBCapabilities(IHDRIterator* type /* out */) const = 0;

        // @property
        // @brief HDR format in use
        // @param type: HDR format
        virtual uint32_t HDRSetting(HDRType& type /* @out */) const = 0;
    };
}
}
