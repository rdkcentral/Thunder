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
    struct EXTERNAL IExternal : virtual public Core::IUnknown {
        enum { ID = ID_EXTERNAL };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_EXTERNAL_NOTIFICATION };

            // Push changes. If the Current value changes, the next method is called.
            virtual void Update(const uint32_t id) = 0;
        };

        struct EXTERNAL ICatalog : virtual public Core::IUnknown {
            enum { ID = ID_EXTERNAL_CATALOG };

            struct EXTERNAL INotification : virtual public Core::IUnknown {
                enum { ID = ID_EXTERNAL_CATALOG_NOTIFICATION };

                virtual void Activated(IExternal* source) = 0;
                virtual void Deactivated(IExternal* source) = 0;
            };

            // Pushing notifications to interested sinks
            virtual void Register(ICatalog::INotification* sink) = 0;
            virtual void Unregister(ICatalog::INotification* sink) = 0;
        };

        //  Basic/specific and dimension together define the Type.
        // 32     13    | 3 |  4  |     12     |
        //  +---------------+------------------+
        //  | dimension |FLT|basic|  specific  |
        //  +---------------+------------------+
        //  FLT = Floating point. The number of decimals thats
        //        should be considerd to be the remainder.
        //        3 bits (0..7)
        //
        enum basic : uint8_t { /* 4 bits (16)*/
            regulator = 0x0,
            measurement = 0x1
        };

        enum specific : uint16_t { /* 12 bits (4096) */
            general = 0x000,
            electricity = 0x001,
            water = 0x002,
            gas = 0x003,
            air = 0x004,
            smoke = 0x005,
            carbonMonoxide = 0x006,
            carbonDioxide = 0x007,
            temperature = 0x008,
            accessControl = 0x009,
            burglar = 0x00A,
            powerManagement = 0x00B,
            system = 0x00C,
            emergency = 0x00D,
            clock = 0x00E
        };

        enum dimension : uint16_t { /* 13 bits (8192) */
            logic = 0x0000, /* values 0 or 1  */
            percentage = 0x0001, /* values 0 - 100 */
            kwh = 0x0002, /* kilo Watt hours  */
            kvah = 0x0003, /* kilo Volt Ampere hours */
            pulses = 0x0004, /* counter */
            degrees = 0x0005, /* temperture in degrees celsius */
            units = 0x0006, /* unqualified value, just units */
        };

        enum condition : uint8_t {
            constructing = 0x00,
            activated    = 0x01,
            deactivated  = 0x02
        };

        // Pushing notifications to interested sinks
        virtual void Register(INotification* sink) = 0;
        virtual void Unregister(INotification* sink) = 0;

        // Element require communication, so might fail, report our condition
        virtual condition Condition() const = 0;

        // Identification of this element.
        virtual uint32_t Identifier() const = 0;

        // The module is the top 8 bits of the Identifier. The value of 0 is reserved,
        // it means that the module is not assigned. Any other number indicates that the 
        // IExternal is allocated to a module and should not be overwritten with an other
        // number than 0.
        virtual uint32_t Module(const uint8_t module) = 0;

        // Characteristics of this element
        virtual uint32_t Type() const = 0;

        // Value determination of this element
        virtual int32_t Minimum() const = 0;
        virtual int32_t Maximum() const = 0;

        virtual uint32_t Get(int32_t& value /* @out */) const = 0;
        virtual uint32_t Set(const int32_t value) = 0;

        // Periodically we might like to be evaluated, call this method at a set time.
        virtual void Evaluate() = 0;

        // ------------------------------------------------------------------------
        // Convenience methods to extract interesting information from the Type()
        // ------------------------------------------------------------------------
        static basic Basic(const uint32_t instanceType)
        {
            return (static_cast<basic>((instanceType >> 12) & 0xF));
        }
        static dimension Dimension(const uint32_t instanceType)
        {
            return (static_cast<dimension>((instanceType >> 19) & 0x1FFF));
        }
        static specific Specific(const uint32_t instanceType)
        {
            return (static_cast<specific>(instanceType & 0xFFF));
        }
        static uint8_t Decimals(const uint32_t instanceType)
        {
            return ((instanceType >> 16) & 0x07);
        }
        static uint32_t Type(const basic base, const specific spec, const dimension dim, const uint8_t decimals) {
            return ((dim << 19) | ((decimals & 0x07) << 16) | (base << 12) | spec);
        }
    };

} } // Namespace Exchange
