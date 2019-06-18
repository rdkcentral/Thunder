#ifndef WEBPA_H
#define WEBPA_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IWebPAService : virtual public Core::IUnknown {
        enum { ID = ID_WEBPA };
        virtual uint32_t Configure(PluginHost::IShell*) = 0;
        virtual void Launch() = 0;
    };
    struct IWebPAClient : virtual public Core::IUnknown {
        enum { ID = ID_WEBPA_CLIENT };
        virtual uint32_t Configure(PluginHost::IShell*) = 0;
        virtual void Launch() = 0;
    };
}
}

#endif // WEBPA_H
