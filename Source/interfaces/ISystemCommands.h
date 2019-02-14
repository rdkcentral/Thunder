#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ISystemCommands : virtual public Core::IUnknown {
        enum { ID = 0x00000082 };

        virtual uint32_t USBReset(const string& device) = 0;
    };
}
}
