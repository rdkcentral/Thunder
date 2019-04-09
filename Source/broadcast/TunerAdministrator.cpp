#include "TunerAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    /* static */ TunerAdministrator& TunerAdministrator::Instance()
    {
        static TunerAdministrator _instance;
        return (_instance);
    }

    // Accessor to metadata on the tuners.
    /* static */ void ITuner::Register(ITuner::INotification* notify)
    {
        TunerAdministrator::Instance().Register(notify);
    }

    /* static */ void ITuner::Unregister(ITuner::INotification* notify)
    {
        TunerAdministrator::Instance().Unregister(notify);
    }

}
} // namespace WPEFramework::Broadcast
