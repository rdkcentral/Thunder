#pragma once

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IKeyProducer;
    struct IWheelProducer;
    struct IPointerProducer;
    struct ITouchProducer;

    enum ProducerEvents {
        ePairingStarted = 1,
        ePairingSuccess,
        ePairingFailed,
        ePairingTimedout
    };

    struct IKeyHandler : virtual public Core::IUnknown {
        enum { ID = ID_KEYHANDLER };

        virtual ~IKeyHandler(){};

        virtual uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& table) = 0;
        virtual void ProducerEvent(const string& producerName, const ProducerEvents event) = 0;

        virtual IKeyProducer* Producer(const string& name) = 0;
    };

    struct IWheelHandler : virtual public Core::IUnknown {
        enum { ID = ID_WHEELHANDLER };

        virtual ~IWheelHandler(){};

        virtual uint32_t AxisEvent(const int16_t x, const int16_t y) = 0;

        virtual IWheelProducer* WheelProducer(const string& name) = 0;
    };

    struct IPointerHandler : virtual public Core::IUnknown {
        enum { ID = ID_POINTERHANDLER };

        virtual ~IPointerHandler(){};

        virtual uint32_t PointerMotionEvent(const int16_t x, const int16_t y) = 0;
        virtual uint32_t PointerButtonEvent(const bool pressed, const uint8_t button) = 0;

        virtual IPointerProducer* PointerProducer(const string& name) = 0;
    };

    struct ITouchHandler : virtual public Core::IUnknown {
        enum { ID = ID_TOUCHHANDLER };

        virtual ~ITouchHandler(){};

        enum class touchstate {
            TOUCH_MOTION,
            TOUCH_RELEASED,
            TOUCH_PRESSED
        };

        virtual uint32_t TouchEvent(const uint8_t index, const touchstate state, const uint16_t x, const uint16_t y) = 0;

        virtual ITouchProducer* TouchProducer(const string& name) = 0;
    };

    struct IKeyProducer : virtual public Core::IUnknown {
        enum { ID = ID_KEYPRODUCER };

        virtual ~IKeyProducer(){};

        virtual const TCHAR* Name() const = 0;
        virtual bool Pair() = 0;
        virtual bool Unpair(string bindingId) = 0;
        virtual uint32_t Callback(IKeyHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

    struct IWheelProducer : virtual public Core::IUnknown {
        enum { ID = ID_WHEELPRODUCER };

        virtual ~IWheelProducer(){};

        virtual const TCHAR* Name() const = 0;
        virtual uint32_t Callback(IWheelHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

    struct IPointerProducer : virtual public Core::IUnknown {
        enum { ID = ID_POINTERPRODUCER };

        virtual ~IPointerProducer(){};

        virtual const TCHAR* Name() const = 0;
        virtual uint32_t Callback(IPointerHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

    struct ITouchProducer : virtual public Core::IUnknown {
        enum { ID = ID_TOUCHPRODUCER };

        virtual ~ITouchProducer(){};

        virtual const TCHAR* Name() const = 0;
        virtual uint32_t Callback(ITouchHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

}
}
