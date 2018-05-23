#ifndef __IWEBSERVER_H
#define __IWEBSERVER_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a WebServer to change
    // Server specific properties like .....
    struct IWebServer : virtual public Core::IUnknown {

        enum { ID = 0x00000066 };

        virtual ~IWebServer() {}

        virtual void AddProxy(const string& path, const string& subst, const string& address) = 0;
        virtual void RemoveProxy(const string& path) = 0;

        virtual string Accessor() const = 0;
    };
}
}

#endif // __IWEBSERVER_H
