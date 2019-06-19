#pragma once

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IDropbearServer : virtual public Core::IUnknown {
        enum { ID = ID_DROPBEARSERVER };

        virtual uint32_t StartService(const string& port) = 0;
        virtual uint32_t StopService() = 0;
    };
}
}
