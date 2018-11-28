#include "ServiceAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    /* virtual */ void ServiceAdministrator::Parser::Handle(const MPEG::Section& section) {
    }

    /* static */ ServiceAdministrator& ServiceAdministrator::Instance() {
        static ServiceAdministrator _instance;
        return (_instance);
    }

    void ServiceAdministrator::Activated(ITuner* tuner) {
        _adminLock.Lock();

        Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

        ASSERT(index == _scanners.end());
        
        if (index == _scanners.end()) {
            _scanners.emplace_back (*this, tuner);
        }

        _adminLock.Unlock();
    }

    void ServiceAdministrator::Deactivated(ITuner* tuner) {
        _adminLock.Lock();

        Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

        ASSERT(index != _scanners.end());
        
        if (index != _scanners.end()) {
            index->Close();
            _scanners.erase(index);
        }

        _adminLock.Unlock();
 
    }

    void ServiceAdministrator::StateChange(ITuner* tuner) {
    }

} } // namespace WPEFramework::Broadcast

