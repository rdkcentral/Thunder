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

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {

namespace Exchange {

    // This interface gives direct access to a Bluetooth server instance, running as a plugin in the framework.
    struct IBluetooth : virtual public Core::IUnknown {

        enum { ID = ID_BLUETOOTH };

        virtual ~IBluetooth() {}

        struct IDevice : virtual public Core::IUnknown {

            enum { ID = ID_BLUETOOTH_DEVICE };

            struct IIterator : virtual public Core::IUnknown {

                enum { ID = ID_BLUETOOTH_DEVICE_ITERATOR };

                virtual ~IIterator() {}

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;
                virtual IDevice* Current() = 0;
            };

            struct ICallback : virtual public Core::IUnknown {

                enum { ID = ID_BLUETOOTH_CALLBACK };

                virtual ~ICallback () {}

                virtual void Updated() = 0;
                virtual bool PasskeyRequest() { return false; }
                virtual bool PasskeyConfirmRequest(const uint32_t passkey) { return false; }
            };

            enum type : uint8_t {
                ADDRESS_BREDR,
                ADDRESS_LE_PUBLIC,
                ADDRESS_LE_RANDOM
            };

            enum capabilities : uint8_t {
                DISPLAY_ONLY = 0x00,
                DISPLAY_YES_NO = 0x01,
                KEYBOARD_ONLY = 0x02,
                NO_INPUT_NO_OUTPUT = 0x03,
                KEYBOARD_DISPLAY = 0x04,
            };

            virtual ~IDevice() {}

            virtual type Type() const = 0;
            virtual bool IsValid() const = 0;
            virtual bool IsBonded() const = 0;
            virtual bool IsConnected() const = 0;
            virtual string LocalId() const = 0;
            virtual string RemoteId() const = 0;
            virtual string Name() const = 0;
            virtual uint32_t Class() const = 0;

            virtual uint32_t Pair(const capabilities) = 0;
            virtual uint32_t AbortPairing() = 0;
            virtual uint32_t Unpair() = 0;
            virtual uint32_t Connect() = 0;
            virtual uint32_t Disconnect(const uint16_t reason) = 0;

            virtual void Passkey(const uint32_t passkey) = 0;
            virtual void ConfirmPasskey(const bool confirm) = 0;

            virtual uint32_t Callback(ICallback* callback) = 0;
        };

        struct INotification : virtual public Core::IUnknown {

            enum { ID = ID_BLUETOOTH_NOTIFICATION };

            virtual ~INotification() {}

            virtual void Update(IDevice* device);
        };

        virtual bool IsScanning() const = 0;
        virtual uint32_t Register(INotification* notification) = 0;
        virtual uint32_t Unregister(INotification* notification) = 0;

        virtual bool Scan(const bool enable) = 0;
        virtual IDevice* Device(const string&) = 0;
        virtual IDevice::IIterator* Devices() = 0;
    };
}
}
