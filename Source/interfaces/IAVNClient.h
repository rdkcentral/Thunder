#ifndef _AVNCLIENT_H
#define _AVNCLIENT_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IAVNClient : virtual public Core::IUnknown {
        enum { ID = 0x0000006C };
        virtual uint32_t Configure(PluginHost::IShell*) = 0;
        virtual void Launch(const string&) = 0;
    };
}
}

#endif // _AVNCLIENT_H
