#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IDeviceProperties : virtual public Core::IUnknown {
        enum { ID = ID_DEVICE_PROPERTIES };

        virtual ~IDeviceProperties() { }

        virtual const string Chipset() const = 0;
        virtual const string FirmwareVersion() const = 0;
    };

}
}
