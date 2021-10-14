 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "WarningReportingControl.h"
#include "Sync.h"
#include "Singleton.h"
#include "Thread.h"
#include "JSON.h" 

namespace {
    WPEFramework::Core::CriticalSection adminlock; // we cannot have this as a member as Sync.h might also need WarningReporting. but as WarningReportingUnitProxy that is not a problem

    class WarningReportingBoundsCategoryConfig : public WPEFramework::Core::JSON::Container {
    public:
        WarningReportingBoundsCategoryConfig(const WarningReportingBoundsCategoryConfig&) = delete;
        WarningReportingBoundsCategoryConfig& operator=(const WarningReportingBoundsCategoryConfig&) = delete;

        WarningReportingBoundsCategoryConfig()
            : WPEFramework::Core::JSON::Container()
            , ReportBound()
            , WarningBound()
            , CategoryConfig(false)
        {
            Add(_T("reportbound"), &ReportBound);
            Add(_T("warningbound"), &WarningBound);
            Add(_T("config"), &CategoryConfig);
        }

        ~WarningReportingBoundsCategoryConfig() override = default;

    public:
        WPEFramework::Core::JSON::DecUInt64 ReportBound; // HPL: this used to be a template but we must put it in a cpp file...
        WPEFramework::Core::JSON::DecUInt64 WarningBound;
        WPEFramework::Core::JSON::String CategoryConfig;
    };
}

namespace WPEFramework {
namespace WarningReporting {

    const char* CallsignTLS::Callsign() {

        Core::ThreadLocalStorageType<CallsignTLS>& instance = Core::ThreadLocalStorageType<CallsignTLS>::Instance();
        const char* name = nullptr;
        if( ( instance.IsSet() == true ) && ( instance.Context().Name() != nullptr ) ) {
            name = instance.Context().Name(); // should be safe, nobody should for this thread be able to change this while we are using it 
        }
        return name;
    }

    void CallsignTLS::Callsign(const char* callsign) {
        Core::ThreadLocalStorageType<CallsignTLS>::Instance().Context().Name(callsign);
    }

    WarningReportingUnitProxy& WarningReportingUnitProxy::Instance()
    {
        return (Core::SingletonType<WarningReportingUnitProxy>::Instance());
    }

    WarningReportingUnitProxy::~WarningReportingUnitProxy()
    {
        while (!_waitingannounces.empty()) {
            _waitingannounces.back()->Destroy();
        }
    }

    void WarningReportingUnitProxy::ReportWarningEvent(const char module[], const char fileName[], const uint32_t lineNumber, const char className[], const IWarningEvent& information) {
        Core::SafeSyncType<Core::CriticalSection> guard(adminlock);
        if( _handler != nullptr ) {
            _handler->ReportWarningEvent(module, fileName, lineNumber, className, information);
        }
    }

    bool WarningReportingUnitProxy::IsDefaultCategory(const string& category, bool& enabled, string& specific) const {
        bool retval = false; 
        adminlock.Lock();
        if( _handler != nullptr ) {
            retval = _handler->IsDefaultCategory(category, enabled, specific);
        }
        adminlock.Unlock();
        return retval;
    }

    void WarningReportingUnitProxy::Announce(IWarningReportingUnit::IWarningReportingControl& Category) {
        Core::SafeSyncType<Core::CriticalSection> guard(adminlock);

        if( _handler != nullptr ) {
            _handler->Announce(Category);
        }
        else {
            _waitingannounces.emplace_back(&Category);
        }
    }

    void WarningReportingUnitProxy::Revoke(IWarningReportingUnit::IWarningReportingControl& Category) {
        Core::SafeSyncType<Core::CriticalSection> guard(adminlock);
        if( _handler != nullptr ) {
             ASSERT(_waitingannounces.size() == 0);
            _handler->Revoke(Category);
        } else {
            WaitingAnnounceContainer::iterator it = std::find(std::begin(_waitingannounces), std::end(_waitingannounces), &Category);
            if( it != std::end(_waitingannounces) ) {
                _waitingannounces.erase(it);
            }
        }
    }

    void WarningReportingUnitProxy::Handler(IWarningReportingUnit* handler) {
        ASSERT( ( _handler == nullptr && handler != nullptr ) || ( _handler != nullptr && handler == nullptr ) );
        Core::SafeSyncType<Core::CriticalSection> guard(adminlock);
        _handler = handler;
        if( _handler != nullptr) {

            for (IWarningReportingUnit::IWarningReportingControl* category : _waitingannounces) {
                ASSERT(category != nullptr);
                _handler->Announce(*category);
            }
            _waitingannounces.clear();
        }
    }

    WarningReportingUnitProxy::BoundsConfigValues::BoundsConfigValues(const string& settings)
        : reportbound()
        , warningbound()
        , config() {
            WarningReportingBoundsCategoryConfig boundsconfig;

            boundsconfig.FromString(settings);

            // HPL todo: add check for correct settings? (reportbound <= warningbound)

            if( boundsconfig.ReportBound.IsSet() ) {
                reportbound  = boundsconfig.ReportBound.Value();
            }

            if( boundsconfig.WarningBound.IsSet() ) {
                warningbound  = boundsconfig.WarningBound.Value();
            }

            if( boundsconfig.CategoryConfig.IsSet() ) {
                config = boundsconfig.CategoryConfig.Value();
            }
    }

}
} 
