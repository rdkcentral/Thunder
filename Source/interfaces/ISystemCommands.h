#pragma once

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ISystemCommands : virtual public Core::IUnknown {
        enum { ID = ID_SYSTEMCOMMAND };

        virtual uint32_t USBReset(const string& device) = 0;
    };
}
}
