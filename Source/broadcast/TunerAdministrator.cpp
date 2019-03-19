#include "TunerAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    /* static */ TunerAdministrator& TunerAdministrator::Instance()
    {
        static TunerAdministrator _instance;
        return (_instance);
    }
}
} // namespace WPEFramework::Broadcast
