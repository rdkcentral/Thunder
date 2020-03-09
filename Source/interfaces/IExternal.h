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

// ---- Include system wide include files ----
#include "Module.h"

// ---- Include local include files ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Referenced classes and types ----

// ---- Class Definition ----

namespace WPEFramework {
namespace Exchange {
    struct IExternal : virtual public Core::IUnknown {
        virtual ~IExternal() {}

        enum { ID = ID_EXTERNAL };

        struct INotification : virtual public Core::IUnknown {

            virtual ~INotification() {}

            enum { ID = ID_EXTERNAL_NOTIFICATION };

            // Push changes. If the Current value changes, the next method is called.
            virtual void Update() = 0;
        };

        struct IFactory : virtual public Core::IUnknown {
            virtual ~IFactory() {}

            enum { ID = ID_EXTERNAL_FACTORY };

            struct IProduced : virtual public Core::IUnknown {

                virtual ~IProduced () {}

                enum { ID = ID_EXTERNAL_FACTORY_NOTIFICATION };

                virtual void Activated(IExternal* source) = 0;
                virtual void Deactivated(IExternal* source) = 0;
            };

            // Pushing notifications to interested sinks
            virtual void Register(IFactory::IProduced* sink) = 0;
            virtual void Unregister(IFactory::IProduced* sink) = 0;
            virtual IExternal* Resource(const uint32_t id) = 0;
        };

        enum identification {
            ZWAVE = 0x10000000,
            GPIO = 0x20000000,
            I2C = 0x30000000,
            ZIGBEE = 0x40000000
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
        enum basic { /* 4 bits (16)*/
            regulator = 0x0,
            measurement = 0x1
        };

        enum specific { /* 12 bits (4096) */
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

        enum dimension { /* 13 bits (8192) */
            logic = 0x0000, /* values 0 or 1  */
            percentage = 0x0001, /* values 0 - 100 */
            kwh = 0x0002, /* kilo Watt hours  */
            kvah = 0x0003, /* kilo Volt Ampere hours */
            pulses = 0x0004, /* counter */
            degrees = 0x0005, /* temperture in degrees celsius */
            units = 0x0006, /* unqualified value, just units */
        };

        enum condition {
            constructing = 0x0000,
            activated = 0x0001,
            deactivated = 0x0002
        };

        // Pushing notifications to interested sinks
        virtual void Register(INotification* sink) = 0;
        virtual void Unregister(INotification* sink) = 0;

        // Element require communication, so might fail, report our condition
        virtual condition Condition() const = 0;

        // Identification of this element.
        virtual uint32_t Identifier() const = 0;

        // Characteristics of this element
        virtual uint32_t Type() const = 0;

        // Value determination of this element
        virtual int32_t Minimum() const = 0;
        virtual int32_t Maximum() const = 0;

        virtual uint32_t Get(int32_t& value /* @out */) const = 0;
        virtual uint32_t Set(const int32_t value) = 0;

        // Periodically we might like to be triggered, call this method at a set time.
        virtual void Trigger() = 0;
    };

} } // Namespace Exchange
