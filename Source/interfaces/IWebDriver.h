#ifndef __IWEBDRIVER_H
#define __IWEBDRIVER_H

#include "Module.h"

namespace WPEFramework {

namespace PluginHost {
struct IShell;
}

namespace Exchange {

    // This interface gives direct access to a WebDriver instance
    struct IWebDriver : virtual public Core::IUnknown {

        enum { ID = 0x00000065 };

        virtual ~IWebDriver() {}
        virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
    };
}
}

#endif // __WEBDRIVER_
