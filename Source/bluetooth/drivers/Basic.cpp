#include "BlueDriver.h"

namespace WPEFramework {

namespace Bluetooth {

    /* static */ Driver* Driver::Instance(const string& /* configuration */)
    {
        return (new Driver());
    }
}
} // namespace WPEFramework::Bluetooth
