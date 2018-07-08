#ifndef __IKEYHANDLER_H__
#define __IKEYHANDLER_H__

#include "Module.h"

namespace WPEFramework {
namespace Exchange {
    struct IKeyHandler : virtual public Core::IUnknown {
        enum { ID = 0x00000050 };

        virtual ~IKeyHandler(){};

        virtual uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& table) = 0;
    };

    struct IKeyProducer : virtual public Core::IUnknown {
        enum { ID = 0x00000051 };

        virtual ~IKeyProducer(){};

        virtual const TCHAR* Name() const = 0;
        virtual bool Pair() = 0;
        virtual bool Unpair(string bindingId) = 0;
        virtual uint32_t Callback(IKeyHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };
}
}

#endif // __IKEYHANDLER_H__
