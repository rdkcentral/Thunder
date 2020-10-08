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

    struct EXTERNAL IKeyProducer;
    struct EXTERNAL IWheelProducer;
    struct EXTERNAL IPointerProducer;
    struct EXTERNAL ITouchProducer;

    enum ProducerEvents : uint8_t {
        PairingStarted = 1,
        PairingSuccess,
        PairingFailed,
        PairingTimedout,
        UnpairingStarted,
        UnpairingSuccess,
        UnpairingFailed,
        UnpairingTimedout
    };

    struct EXTERNAL IKeyHandler : virtual public Core::IUnknown {
        enum { ID = ID_KEYHANDLER };

        virtual uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& table) = 0;
        virtual void ProducerEvent(const string& producerName, const ProducerEvents event) = 0;

        virtual IKeyProducer* Producer(const string& name) = 0;
    };

    struct EXTERNAL IWheelHandler : virtual public Core::IUnknown {
        enum { ID = ID_WHEELHANDLER };

        virtual uint32_t AxisEvent(const int16_t x, const int16_t y) = 0;

        virtual IWheelProducer* WheelProducer(const string& name) = 0;
    };

    struct EXTERNAL IPointerHandler : virtual public Core::IUnknown {
        enum { ID = ID_POINTERHANDLER };

        virtual uint32_t PointerMotionEvent(const int16_t x, const int16_t y) = 0;
        virtual uint32_t PointerButtonEvent(const bool pressed, const uint8_t button) = 0;

        virtual IPointerProducer* PointerProducer(const string& name) = 0;
    };

    struct EXTERNAL ITouchHandler : virtual public Core::IUnknown {
        enum { ID = ID_TOUCHHANDLER };

        enum class touchstate : uint8_t {
            TOUCH_MOTION,
            TOUCH_RELEASED,
            TOUCH_PRESSED
        };

        virtual uint32_t TouchEvent(const uint8_t index, const touchstate state, const uint16_t x, const uint16_t y) = 0;

        virtual ITouchProducer* TouchProducer(const string& name) = 0;
    };

    struct EXTERNAL IKeyProducer : virtual public Core::IUnknown {
        enum { ID = ID_KEYPRODUCER };

        virtual string Name() const = 0;
        virtual uint32_t Callback(IKeyHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;

        virtual bool Pair() = 0;
        virtual bool Unpair(string bindingId) = 0;
    };

    struct EXTERNAL IWheelProducer : virtual public Core::IUnknown {
        enum { ID = ID_WHEELPRODUCER };

        virtual string Name() const = 0;
        virtual uint32_t Callback(IWheelHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

    struct EXTERNAL IPointerProducer : virtual public Core::IUnknown {
        enum { ID = ID_POINTERPRODUCER };

        virtual string Name() const = 0;
        virtual uint32_t Callback(IPointerHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

    struct EXTERNAL ITouchProducer : virtual public Core::IUnknown {
        enum { ID = ID_TOUCHPRODUCER };

        virtual string Name() const = 0;
        virtual uint32_t Callback(ITouchHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

}
}
