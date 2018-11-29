#include "ServiceAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    /* virtual */ void ServiceAdministrator::Parser::Handle(const MPEG::Section& section) {

        ASSERT (section.IsValid());

        if (_state == METADATA) {
            if (section.TableId() == DVB::NIT::ACTUAL) {
                _NIT.AddSection(section);
                if (_NIT.IsValid() == true) {
                    _parent.Load(DVB::NIT(_NIT));
                }
            }
            else if (section.TableId() == DVB::SDT::ACTUAL) {
                _SDT.AddSection(section);
                if (_SDT.IsValid() == true) {
                    _parent.Load(DVB::SDT(_SDT));
                }
            }
            else if (section.TableId() == DVB::TDT::ID) {
                _parent.Load(DVB::TDT(section));
            }
        }
        else if (_state == SCHEDULE) {
        }
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
        if (tuner->State() != ITuner::IDLE) {
            Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

            ASSERT(index != _scanners.end());
        
            if ((index != _scanners.end()) && (index->State() == Parser::IDLE)) {
                index->Open();
            }
        }
    }

    void ServiceAdministrator::Load(const DVB::NIT& table) {
    
    }

    void ServiceAdministrator::Load(const DVB::SDT& table) {
        
    }

    void ServiceAdministrator::Load(const DVB::TDT& table) {
    }


} } // namespace WPEFramework::Broadcast

