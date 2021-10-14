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

#pragma once

#include <inttypes.h> 

#include "Module.h"
#include "IWarningReportingControl.h"
#include "TextFragment.h"
#include "Trace.h"
#include "SystemInfo.h"
#include "Time.h"
#include "TypeTraits.h"
#include "Optional.h"
#include <vector>

#ifndef __CORE_WARNING_REPORTING__

#define WARNING_REPORTING_THREAD_SETCALLSIGN(CALLSIGN)

#define REPORT_WARNING(CATEGORY, ...)                                                  

#define REPORT_WARNING_GLOBAL(CATEGORY, ...)                             

#define REPORT_OUTOFBOUNDS_WARNING(CATEGORY, ACTUALVALUE, ...)

#define REPORT_DURATION_WARNING(CODE, CATEGORY, ...)                                 \
    CODE

#else

// Note for creating new categories:
//
// CATEGORY requiremements: 
// Name of catagory must be unique
// Methods:
// - <optional> bool Analyze(const char modulename[], const char identifier[], EXTRA_PARAMETERS);  // needs to return true if needs to be Reported
// - <optional> static void Configure(const string& settings); 
// - constructor ()
// - uint16_t Serialize(uint8_t[], const uint16_t) const; (note use the return parameter to indicate how much of the buffer is written)
// - uint16_t Deserialize(const uint8_t[], const uint16_t); (note use the return parameter to indicate how much of the buffer is read)
// - void ToString(string& visitor) const; 
// - <optional> bool IsWarning() const; (if not available uit will always be a warning)
// Constants:
// - if a Duration or OutOfBounds warning category:
//   DefaultWarningBound and DefaultReportBound must be available (and DefaultWarningBound >= DefaultReportBound). if actual value > bound it will
//   be reported/considered a warning
// Types:
// OutOfBounds warning category:
//  - BoundsType to indicate type for boubnds values

#define WARNING_REPORTING_THREAD_SETCALLSIGN(CALLSIGN)    \
    WPEFramework::WarningReporting::CallsignTLS::CallSignTLSGuard callsignguard(CALLSIGN); 

#define REPORT_WARNING(CATEGORY, ...)                                                    \
    if (WPEFramework::WarningReporting::WarningReportingType<CATEGORY>::IsEnabled() == true) { \
        CATEGORY __data__ ;  \
        if( WPEFramework::WarningReporting::WarningReportingType<CATEGORY>::CallAnalyzeIfAvailable(__data__, WPEFramework::Core::System::MODULE_NAME,  \
                             WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                            ##__VA_ARGS__) == true ) {                           \
            WPEFramework::WarningReporting::WarningReportingType<CATEGORY> __message__(__data__);  \
            WPEFramework::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                            \
                WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                __FILE__,                                                                  \
                __LINE__,                                                                  \
                typeid(*this).name(),                                                      \
                __message__);                                                             \
        }                                                                              \
    }

#define REPORT_WARNING_GLOBAL(CATEGORY, ...)                                             \
    if (WPEFramework::WarningReporting::WarningReportingType<CATEGORY>::IsEnabled() == true) { \
        CATEGORY __data__ ;  \
        if( WPEFramework::WarningReporting::WarningReportingType<CATEGORY>::CallAnalyzeIfAvailable(__data__, WPEFramework::Core::System::MODULE_NAME,  \
                             WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                            ##__VA_ARGS__) == true ) {                           \
            WPEFramework::WarningReporting::WarningReportingType<CATEGORY> __message__(__data__);  \
            WPEFramework::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                            \
                WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                __FILE__,                                                                  \
                __LINE__,                                                                  \
            __FUNCTION__,                                                              \
                __message__);                                                             \
        }                                                                              \
    }

#define REPORT_OUTOFBOUNDS_WARNING(CATEGORY, ACTUALVALUE, ...)                                 \
    if (WPEFramework::WarningReporting::WarningReportingType<WPEFramework::WarningReporting::WarningReportingBoundsCategory<CATEGORY, uint32_t>>::IsEnabled() == true) { \
        WPEFramework::WarningReporting::WarningReportingBoundsCategory<CATEGORY, uint32_t> __data__ ;  \
        if( __data__.Analyze(WPEFramework::Core::System::MODULE_NAME,  \
                             WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                            ACTUALVALUE,  \
                            ##__VA_ARGS__) == true ) {                           \
            WPEFramework::WarningReporting::WarningReportingType<WPEFramework::WarningReporting::WarningReportingBoundsCategory<CATEGORY, uint32_t>> __message__(__data__);  \
            WPEFramework::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                            \
                WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                __FILE__,                                                                  \
                __LINE__,                                                                  \
                typeid(*this).name(),                                                      \
                __message__);                                                             \
        }                                                                              \
    }

#define REPORT_DURATION_WARNING(CODE, CATEGORY, ...)                                 \
    if (WPEFramework::WarningReporting::WarningReportingType<WPEFramework::WarningReporting::WarningReportingBoundsCategory<CATEGORY, uint32_t>>::IsEnabled() == true) { \
        WPEFramework::Core::Time start = WPEFramework::Core::Time::Now();                     \
        CODE                                                                            \
        uint32_t duration( (Core::Time::Now().Ticks() - start.Ticks()) / Core::Time::TicksPerMillisecond);              \
        WPEFramework::WarningReporting::WarningReportingBoundsCategory<CATEGORY, uint32_t> __data__ ;  \
        if( __data__.Analyze(WPEFramework::Core::System::MODULE_NAME,  \
                             WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                            duration,  \
                            ##__VA_ARGS__) == true ) {                           \
            WPEFramework::WarningReporting::WarningReportingType<WPEFramework::WarningReporting::WarningReportingBoundsCategory<CATEGORY, uint32_t>> __message__(__data__);  \
            WPEFramework::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                            \
                WPEFramework::WarningReporting::CallsignTLS::CallsignAccess<&WPEFramework::Core::System::MODULE_NAME>::Callsign(),        \
                __FILE__,                                                                  \
                __LINE__,                                                                  \
                typeid(*this).name(),                                                      \
                __message__);                                                             \
        }                                                                              \
    } else {                                                                           \
        CODE                                                                           \
    }


namespace WPEFramework {

namespace Core {
    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType;
}

namespace WarningReporting {

    class CallsignTLS {
    public:

        template <const char** MODULENAME>
        struct CallsignAccess {
            static const char* Callsign() {
                const char* callsign = CallsignTLS::Callsign();
                if( callsign == nullptr ) {
                    callsign = *MODULENAME;
                }
                return callsign;
            }
        };

        class CallSignTLSGuard {
        public:
            CallSignTLSGuard(const CallSignTLSGuard&) = delete;
            CallSignTLSGuard& operator=(const CallSignTLSGuard&) = delete;

            explicit CallSignTLSGuard(const char* callsign) {
                CallsignTLS::Callsign(callsign);
            }

            ~CallSignTLSGuard() {
                CallsignTLS::Callsign(nullptr);
            }

        };

        static const char* Callsign();
        static void Callsign(const char* callsign);

    private:
        friend class Core::ThreadLocalStorageType<CallsignTLS>;
    
        CallsignTLS(const CallsignTLS&) = delete;
        CallsignTLS& operator=(const CallsignTLS&) = delete;

        CallsignTLS() : _name() {};
        ~CallsignTLS() = default;

        void Name(const char* name) {  
            if ( name != nullptr ) {
                _name = name; 
            } else {
                _name.clear(); 
            }
        }
        const char* Name() const { 
            return ( _name.empty() == false ? _name.c_str() : nullptr ); 
        }

    private:
        string _name;
    };

    class WarningReportingUnitProxy {
    public:
        WarningReportingUnitProxy(const WarningReportingUnitProxy&) = delete;
        WarningReportingUnitProxy& operator=(const WarningReportingUnitProxy&) = delete;

        ~WarningReportingUnitProxy();

        static WarningReportingUnitProxy& Instance();

        void ReportWarningEvent(const char identifier[], const char fileName[], const uint32_t lineNumber, const char className[], const IWarningEvent& information);
        bool IsDefaultCategory(const string& category, bool& enabled, string& specific) const;
        void Announce(IWarningReportingUnit::IWarningReportingControl& Category);
        void Revoke(IWarningReportingUnit::IWarningReportingControl& Category);

        void Handler(IWarningReportingUnit* handler);

    protected:
        WarningReportingUnitProxy() : _handler(nullptr), _waitingannounces() {};

    private:
        // HPL todo: find better solution? we can't be dependednd on JSON.h in this file (cirlcular dependencies) so must be in CPP and therfore not template class and don't want it in the accesible namespace
        //           and perhaps remove the whole doubs temnplate param, beter make it uint32_t always...
        struct BoundsConfigValues {
            Core::OptionalType<uint64_t> reportbound;
            Core::OptionalType<uint64_t> warningbound;
            Core::OptionalType<string> config;

            BoundsConfigValues(const string& config);
        };

        template <typename CONTROLCATEGORY, typename BOUNDSTYPE>
        friend class WarningReportingBoundsCategory;

    private:
        using WaitingAnnounceContainer = std::vector<IWarningReportingUnit::IWarningReportingControl*>;

       IWarningReportingUnit* _handler; 
       WaitingAnnounceContainer _waitingannounces;
    };

    template <typename CONTROLCATEGORY, typename BOUNDSTYPE = typename CONTROLCATEGORY::BoundsType>
    class WarningReportingBoundsCategory {
    public:
        WarningReportingBoundsCategory(const WarningReportingBoundsCategory&) = delete;
        WarningReportingBoundsCategory& operator=(const WarningReportingBoundsCategory&) = delete;

        WarningReportingBoundsCategory() : _category(), _actualvalue(0) {}
        ~WarningReportingBoundsCategory() = default;

        static void Configure(const string& settings) {
            WarningReportingUnitProxy::BoundsConfigValues boundsconfig(settings);

            if( boundsconfig.reportbound.IsSet() ) {
                _reportingbound.store(static_cast<BOUNDSTYPE>(boundsconfig.reportbound.Value()), std::memory_order_relaxed);
            }

            if( boundsconfig.warningbound.IsSet() ) {
                _warningbound.store(static_cast<BOUNDSTYPE>(boundsconfig.warningbound.Value()), std::memory_order_relaxed);
            }

            if( boundsconfig.config.IsSet() ) {
                CallConfigure(boundsconfig.config.Value());
            }
        }

        static string CategoryName() {
            return Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text();
        }
        
        template <typename... Args>
        bool Analyze(const char modulename[], const char identifier[], const BOUNDSTYPE actualvalue, Args&&... args) {                
            bool report = false;
            _actualvalue = actualvalue;
            if( actualvalue > _reportingbound.load(std::memory_order_relaxed) ) {
                report = CallAnalyze(modulename, identifier, std::forward<Args>(args)...);
            }
            return report;
        }
        
        uint16_t Serialize(uint8_t buffer[], const uint16_t length) const {
            // HPL Todo: not implemented yet: we should also serialize our part, then only pass on to the actual class
            return (_category.Serialize(buffer, length));
        }

        uint16_t Deserialize(const uint8_t data[], const uint16_t size) {
            // HPL Todo: not implemented yet: we should also extract our part then pass on the rest to tyhe actual category
            return (_category.Deserialize(data, size));
        }

        void ToString(string& visitor) const {
            // HPL todo: perhaps give the Catagoryb to influence this, e.g. add unit (ms)
            _category.ToString(visitor);
            visitor += Core::Format(_T(", value %u, max allowed %u"), _actualvalue, _warningbound.load(std::memory_order_relaxed));
        }

        bool IsWarning() const {
            bool iswarning = false;
            if( _actualvalue > _warningbound.load(std::memory_order_relaxed) ) {
                iswarning = CallIsWarning();
            }
            return iswarning;
        }

    private:
        HAS_MEMBER_NAME(Analyze, hasAnalyze);
        template<typename T = CONTROLCATEGORY, typename... Args> 
        inline typename Core::TypeTraits::enable_if<!hasAnalyze<T, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(const char[], const char[], Args&&...) {
            return true;
        }
        template<typename T = CONTROLCATEGORY, typename... Args> 
        inline typename Core::TypeTraits::enable_if<hasAnalyze<T, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(const char modulename[] , const char identifier[], Args&&... args) {
            return _category.Analyze(modulename, identifier, std::forward<Args>(args)...);
        }
        HAS_STATIC_MEMBER(Configure, hasConfigure);
        template<typename T = CONTROLCATEGORY>
         inline static typename Core::TypeTraits::enable_if<!hasConfigure<T>::value, void>::type
        CallConfigure(const string&) {
        }
        template<typename T = CONTROLCATEGORY>
         inline static typename Core::TypeTraits::enable_if<hasConfigure<T>::value, void>::type
        CallConfigure(const string& settings) {
            return CONTROLCATEGORY::Configure(settings);
        }
        HAS_MEMBER(IsWarning, hasIsWarning);
        template<typename T = CONTROLCATEGORY>
        inline typename Core::TypeTraits::enable_if<!hasIsWarning<T, bool (T::*)() const>::value, bool>::type
        CallIsWarning() const {
            return true;
        }
        template<typename T = CONTROLCATEGORY>
        inline typename Core::TypeTraits::enable_if<hasIsWarning<T, bool (T::*)() const>::value, bool>::type
        CallIsWarning() const {
            return _category.IsWarning();
        }

    private: 
        CONTROLCATEGORY _category;
        BOUNDSTYPE _actualvalue;

        static std::atomic<BOUNDSTYPE> _reportingbound; 
        static std::atomic<BOUNDSTYPE> _warningbound;

        // HPL todo; test later if this works, not important now
        //static_assert(WarningReportingBoundsCategory<BOUNDSTYPE, CONTROLCATEGORY>::_reportingbound <= WarningReportingBoundsCategory<BOUNDSTYPE, CONTROLCATEGORY>::_warningbound);
    };

    template <typename CATEGORY>
    class WarningReportingType : public IWarningEvent {
    public:

    template<typename CONTROLCATEGORY, typename... Args>
    static inline bool CallAnalyzeIfAvailable(CONTROLCATEGORY& category, const char modulename[], const char identifier[], Args&&... args) {
        return CallAnalyze<CONTROLCATEGORY>(category, modulename, identifier, std::forward<Args>(args)...);
    }

    private:
        template <typename CONTROLCATEGORY>
        class WarningReportingControl : public IWarningReportingUnit::IWarningReportingControl {
        private:
            // HPL todo: this is now duplicated code from the class above, certainly when nested again that could be prevented
            HAS_STATIC_MEMBER(Configure, hasConfigure);
            template<typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<!hasConfigure<T>::value, void>::type
            CallConfigure(const string&) {
            }
            template<typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<hasConfigure<T>::value, void>::type
            CallConfigure(const string& settings) {
                return CONTROLCATEGORY::Configure(settings);
            }
            HAS_STATIC_MEMBER(CategoryName, hasCategoryName);
            template<typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<!hasCategoryName<T>::value, string>::type
            CallCategoryName() {
                return Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text();
            }
            template<typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<hasCategoryName<T>::value, string>::type
            CallCategoryName() {
                return CONTROLCATEGORY::CategoryName();
            }

        public:
            WarningReportingControl(const WarningReportingControl&) = delete;
            WarningReportingControl& operator=(const WarningReportingControl&) = delete;

            WarningReportingControl() 
                : m_CategoryName(CallCategoryName())
                , m_Enabled(0x03) 
            {
                // Register Our control unit, so it can be influenced from the outside
                // if nessecary..
                WarningReportingUnitProxy::Instance().Announce(*this); 

                bool enabled = false;
                string settings;
                if (WarningReportingUnitProxy::Instance().IsDefaultCategory(m_CategoryName, enabled, settings)) {
                    if (enabled) {
                        // Better not to use virtual Enabled(...), because derived classes aren't finished yet.
                        m_Enabled = m_Enabled | 0x01;
                    }
                    CallConfigure(settings);

                }
            }
            ~WarningReportingControl() override
            {
                Destroy();
            }

        public:
            inline bool IsEnabled() const
            {
                return ((m_Enabled & 0x01) != 0);
            }
            const char* Category() const override
            {
                return (m_CategoryName.c_str());
            }
            bool Enabled() const override
            {
                return (IsEnabled());
            }
            void Enabled(const bool enabled) override
            {
                m_Enabled = (m_Enabled & 0xFE) | (enabled ? 0x01 : 0x00);
            }
            void Configure(const string& settings) override {
                    CallConfigure(settings);
            }
            void Destroy() override
            {
                if ((m_Enabled & 0x02) != 0) {
                    WarningReportingUnitProxy::Instance().Revoke(*this);
                    m_Enabled = 0;
                }
            }

        protected:
            const string m_CategoryName;
            uint8_t m_Enabled;
        };


    public:
        WarningReportingType(const WarningReportingType&) = delete;
        WarningReportingType& operator=(const WarningReportingType&) = delete;

        WarningReportingType(CATEGORY& category)
            : _info(category)
        {
        }
        ~WarningReportingType() override = default;

    private:
        // HPL todo: this is now duplicated code from the class above, certainly when nested again that could be prevented
        HAS_MEMBER(IsWarning, hasIsWarning);
        template<typename T = CATEGORY>
        inline typename Core::TypeTraits::enable_if<!hasIsWarning<T, bool (T::*)() const>::value, bool>::type
        CallIsWarning() const {
            return true;
        }
        template<typename T = CATEGORY>
        inline typename Core::TypeTraits::enable_if<hasIsWarning<T, bool (T::*)() const>::value, bool>::type
        CallIsWarning() const {
            return _info.IsWarning();
        }
        HAS_MEMBER_NAME(Analyze, hasAnalyze);
        template<typename T = CATEGORY, typename... Args> 
        inline static typename Core::TypeTraits::enable_if<!hasAnalyze<T, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(T& category, const char[], const char[], Args&&...) {
            return true;
        }
        template<typename T = CATEGORY, typename... Args> 
        inline static typename Core::TypeTraits::enable_if<hasAnalyze<T, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(T& category, const char modulename[] , const char identifier[], Args&&... args) {
            return category.Analyze(modulename, identifier, std::forward<Args>(args)...);
        }

    public:
        // No dereference etc.. 1 straight line to enabled or not... Quick method..
        inline static bool IsEnabled()
        {
            return (s_control.IsEnabled());
        }

        inline static void Enable(const bool status)
        {
            s_control.Enabled(status);
        }

        const char* Category() const override
        {
            return (s_control.Category());
        }

        uint16_t Serialize(uint8_t data[], const uint16_t size) const override {
            return(_info.Serialize(data, size));
        }

        uint16_t Deserialize(const uint8_t data[], const uint16_t size) override {
            return(_info.Deserialize(data, size));
        }

        void ToString(string& visitor) const override {
            _info.ToString(visitor);
        }

        bool IsWarning() const override {
            return CallIsWarning();
        }

        bool Enabled() const
        {
            return (s_control.Enabled());
        }

        void Enabled(const bool enabled)
        {
            s_control.Enabled(enabled);
        }

        void Destroy()
        {
            s_control.Destroy();
        }

    private:
        CATEGORY& _info;
        static WarningReportingControl<CATEGORY> s_control;
    };

    template <typename CATEGORY>
    EXTERNAL_HIDDEN typename WarningReportingType<CATEGORY>::template WarningReportingControl<CATEGORY> WarningReportingType<CATEGORY>::s_control;
    template <typename CONTROLCATEGORY, typename BOUNDSTYPE>
    EXTERNAL_HIDDEN std::atomic<BOUNDSTYPE> WarningReportingBoundsCategory<CONTROLCATEGORY, BOUNDSTYPE>::_reportingbound(CONTROLCATEGORY::DefaultReportBound);
    template <typename CONTROLCATEGORY, typename BOUNDSTYPE>
    EXTERNAL_HIDDEN std::atomic<BOUNDSTYPE> WarningReportingBoundsCategory<CONTROLCATEGORY, BOUNDSTYPE>::_warningbound(CONTROLCATEGORY::DefaultWarningBound);
}

} 

#endif

