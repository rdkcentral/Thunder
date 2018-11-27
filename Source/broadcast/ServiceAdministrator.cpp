#include "ServiceAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    /* static */ ServiceAdministrator& ServiceAdministrator::Instance() {
        static ServiceAdministrator _instance;
        return (_instance);
    }

    void ServiceAdministrator::Activated(ITuner* tuner) {

    }

    void ServiceAdministrator::Deactivated(ITuner* tuner) {
    }

    void ServiceAdministrator::StateChange(ITuner* tuner) {
    }

} } // namespace WPEFramework::Broadcast

