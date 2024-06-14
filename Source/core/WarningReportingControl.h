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

#include "Module.h"
#include "Optional.h"
#include "SystemInfo.h"
#include "TextFragment.h"
#include "Time.h"
#include "Trace.h"
#include "TypeTraits.h"
#include "CallsignTLS.h"
#include "IWarningReportingControl.h"

#include <inttypes.h>
#include <unordered_set>
#include <vector>

#if defined(__GNUC__)
    #pragma GCC system_header
#elif defined(__clang__)
    #pragma clang system_header
#endif

#ifndef __CORE_WARNING_REPORTING__

#define ANNOUNCE_WARNING(CATEGORY)

#define REPORT_WARNING(CATEGORY, ...)

#define REPORT_WARNING_GLOBAL(CATEGORY, ...)

#define REPORT_OUTOFBOUNDS_WARNING(CATEGORY, ACTUALVALUE, ...)

#define REPORT_OUTOFBOUNDS_WARNING_EX(CATEGORY, CALLSIGN, ACTUALVALUE, ...)

#define REPORT_DURATION_WARNING(CODE, CATEGORY, ...) \
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

#define ANNOUNCE_WARNING(CATEGORY)  \
    Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>>::IsEnabled()

#define REPORT_WARNING(CATEGORY, ...)                                                                                  \
    if (Thunder::WarningReporting::WarningReportingType<CATEGORY>::IsEnabled()) {                                 \
        Thunder::WarningReporting::WarningReportingType<CATEGORY> __message__;                                    \
        if (__message__.Analyze(Thunder::Core::System::MODULE_NAME,                                               \
                Thunder::Core::CallsignTLS::CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(), \
                ##__VA_ARGS__)                                                                                         \
            == true) {                                                                                                 \
            Thunder::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                  \
                Thunder::Core::CallsignTLS::CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(), \
                __message__);                                                                                          \
        }                                                                                                              \
    }

#define REPORT_OUTOFBOUNDS_WARNING(CATEGORY, ACTUALVALUE, ...)                                                                                                                  \
    if (Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) {                  \
        Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>> __message__;                             \
        if (__message__.Analyze(Thunder::Core::System::MODULE_NAME, Thunder::Core::CallsignTLS::CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(), \
                ACTUALVALUE,                                                                                                                                                    \
                ##__VA_ARGS__)                                                                                                                                                  \
            == true) {                                                                                                                                                          \
            Thunder::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                                                           \
                Thunder::Core::CallsignTLS::CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(),                                                          \
                __message__);                                                                                                                                                   \
        }                                                                                                                                                                       \
    }

#define REPORT_OUTOFBOUNDS_WARNING_EX(CATEGORY, CALLSIGN, ACTUALVALUE, ...)                                                                                                     \
    if (Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) {                  \
        Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>> __message__;                             \
        if (__message__.Analyze(Thunder::Core::System::MODULE_NAME, CALLSIGN,                                                                                              \
                ACTUALVALUE,                                                                                                                                                    \
                ##__VA_ARGS__)                                                                                                                                                  \
            == true) {                                                                                                                                                          \
            Thunder::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                                                           \
                CALLSIGN,                                                                                                                                                       \
                __message__);                                                                                                                                                   \
        }                                                                                                                                                                       \
    }


#define REPORT_DURATION_WARNING(CODE, CATEGORY, ...)                                                                                                           \
    if (Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>>::IsEnabled() == true) { \
        uint64_t start = Thunder::Core::SystemInfo::Instance().Ticks();                                                                                   \
        CODE                                                                                                                                                   \
        uint64_t duration = (Thunder::Core::SystemInfo::Instance().Ticks() - start) / Thunder::Core::Time::MicroSecondsPerMilliSecond;               \
        Thunder::WarningReporting::WarningReportingType<Thunder::WarningReporting::WarningReportingBoundsCategory<CATEGORY>> __message__;            \
        if (__message__.Analyze(Thunder::Core::System::MODULE_NAME,                                                                                       \
                Thunder::Core::CallsignTLS::CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(),                                         \
                duration,                                                                                                                                      \
                ##__VA_ARGS__)                                                                                                                                 \
            == true) {                                                                                                                                         \
            Thunder::WarningReporting::WarningReportingUnitProxy::Instance().ReportWarningEvent(                                                          \
                Thunder::Core::CallsignTLS::CallsignAccess<&Thunder::Core::System::MODULE_NAME>::Callsign(),                                         \
                __message__);                                                                                                                                  \
        }                                                                                                                                                      \
    } else {                                                                                                                                                   \
        CODE                                                                                                                                                   \
    }

namespace Thunder {

namespace Core {
    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType;
    class CriticalSection;
}

namespace WarningReporting {

    class ExcludedWarnings {
    public:
        ExcludedWarnings()
            : _callsigns()
            , _modules()
        {
        }
        ~ExcludedWarnings() = default;
        ExcludedWarnings(const ExcludedWarnings&) = delete;
        ExcludedWarnings& operator=(const ExcludedWarnings&) = delete;

        bool IsCallsignExcluded(const string& callsign) const
        {
            return _callsigns.find(callsign) != _callsigns.end();
        }
        bool IsModuleExcluded(const string& module) const
        {
            return _modules.find(module) != _callsigns.end();
        }
        void InsertCallsign(const string& callsign)
        {
            _callsigns.insert(callsign);
        }
        void InsertModule(const string& module)
        {
            _modules.insert(module);
        }

    private:
        std::unordered_set<string> _callsigns;
        std::unordered_set<string> _modules;
    };

    class EXTERNAL WarningReportingUnitProxy {
    public:
        WarningReportingUnitProxy(const WarningReportingUnitProxy&) = delete;
        WarningReportingUnitProxy& operator=(const WarningReportingUnitProxy&) = delete;

        ~WarningReportingUnitProxy();

        static WarningReportingUnitProxy& Instance();

        void ReportWarningEvent(const char identifier[], const IWarningEvent& information);
        void FetchCategoryInformation(const string& category, bool& outIsDefaultCategory, bool& outIsEnabled, string& outExcluded, string& outConfiguration) const;
        void AddToCategoryList(IWarningReportingUnit::IWarningReportingControl& Category);
        void RemoveFromCategoryList(IWarningReportingUnit::IWarningReportingControl& Category);

        void Handle(IWarningReportingUnit* handler);
        void FillExcludedWarnings(const string& excludedJsonList, ExcludedWarnings& outExcludedWarnings) const;
        void FillBoundsConfig(const string& boundsConfig, uint32_t& outReportingBound, uint32_t& outWarningBound, string& outSpecificConfig) const;

    protected:
        WarningReportingUnitProxy();

    private:
        using WaitingAnnounceContainer = std::vector<IWarningReportingUnit::IWarningReportingControl*>;

        IWarningReportingUnit* _handler;
        WaitingAnnounceContainer _waitingAnnounces;
        Core::CriticalSection* _adminLock;
    };

    template <typename CONTROLCATEGORY>
    class WarningReportingBoundsCategory {
    public:
        WarningReportingBoundsCategory(const WarningReportingBoundsCategory&) = delete;
        WarningReportingBoundsCategory& operator=(const WarningReportingBoundsCategory&) = delete;

        WarningReportingBoundsCategory()
            : _category()
            , _actualValue(0)
        {
        }
        ~WarningReportingBoundsCategory() = default;

        static void Configure(const string& settings)
        {
            if (settings.length() != 0) {
                uint32_t reportBound = 0;
                uint32_t warningBound = 0;
                string specificConfig;

                WarningReportingUnitProxy::Instance().FillBoundsConfig(settings, reportBound, warningBound, specificConfig);

                if (reportBound != 0) {
                    _reportingBound.store(reportBound, std::memory_order_relaxed);
                }
                if (warningBound != 0) {
                    _warningBound.store(warningBound, std::memory_order_relaxed);
                }

                if (!specificConfig.empty()) {
                    CallConfigure(specificConfig);
                }
            }
        }

        static string CategoryName()
        {
            return Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text();
        }

        template <typename... Args>
        bool Analyze(const char moduleName[], const char identifier[], const uint32_t actualValue, Args&&... args)
        {
            bool report = false;
            _actualValue = actualValue;
            if (actualValue > _reportingBound.load(std::memory_order_relaxed)) {
                report = CallAnalyze(moduleName, identifier, std::forward<Args>(args)...);
            }
            return report;
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t length) const
        {
            const std::size_t boundsTypeSize = sizeof(_actualValue);
            ASSERT(length >= boundsTypeSize);

            uint16_t serialized = _category.Serialize(buffer, length);
            memcpy(buffer + serialized, &_actualValue, boundsTypeSize);

            return serialized + boundsTypeSize;
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
        {
            const std::size_t boundsTypeSize = sizeof(_actualValue);
            ASSERT(length >= boundsTypeSize);

            uint16_t deserialized = _category.Deserialize(buffer, length);
            memcpy(&_actualValue, buffer + deserialized, boundsTypeSize);

            return deserialized + boundsTypeSize;
        }

        void ToString(string& visitor) const
        {
            _category.ToString(visitor, _actualValue, IsWarning() ? _warningBound.load(std::memory_order_relaxed) : _reportingBound.load(std::memory_order_relaxed));
        }

        bool IsWarning() const
        {
            bool isWarning = false;
            if (_actualValue > _warningBound.load(std::memory_order_relaxed)) {
                isWarning = CallIsWarning();
            }
            return isWarning;
        }

    private:
        IS_MEMBER_AVAILABLE(Analyze, hasAnalyze);
        template <typename T = CONTROLCATEGORY, typename... Args>
        inline typename Core::TypeTraits::enable_if<!hasAnalyze<T, bool, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(const char[], const char[], Args&&...)
        {
            return true;
        }
        template <typename T = CONTROLCATEGORY, typename... Args>
        inline typename Core::TypeTraits::enable_if<hasAnalyze<T, bool, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(const char modulename[], const char identifier[], Args&&... args)
        {
            return _category.Analyze(modulename, identifier, std::forward<Args>(args)...);
        }
        IS_STATIC_MEMBER_AVAILABLE(Configure, hasConfigure);
        template <typename T = CONTROLCATEGORY>
        inline static typename Core::TypeTraits::enable_if<!hasConfigure<T, void, const string&>::value, void>::type
        CallConfigure(const string&)
        {
        }
        template <typename T = CONTROLCATEGORY>
        inline static typename Core::TypeTraits::enable_if<hasConfigure<T, void, const string&>::value, void>::type
        CallConfigure(const string& settings)
        {
            return CONTROLCATEGORY::Configure(settings);
        }
        IS_MEMBER_AVAILABLE(IsWarning, hasIsWarning);
        template <typename T = CONTROLCATEGORY>
        inline typename Core::TypeTraits::enable_if<!hasIsWarning<const T, bool>::value, bool>::type
        CallIsWarning() const
        {
            return true;
        }
        template <typename T = CONTROLCATEGORY>
        inline typename Core::TypeTraits::enable_if<hasIsWarning<const T, bool>::value, bool>::type
        CallIsWarning() const
        {
            return _category.IsWarning();
        }

    private:
        CONTROLCATEGORY _category;
        uint32_t _actualValue;

        static std::atomic<uint32_t> _reportingBound;
        static std::atomic<uint32_t> _warningBound;
    };

    template <typename CATEGORY>
    class WarningReportingType : public IWarningEvent {
    public:
        template <typename... Args>
        inline bool Analyze(const char modulename[], const char identifier[], Args&&... args)
        {
            bool result = false;
            if (!_sWarningControl.IsCallsignExcluded(identifier) && !_sWarningControl.IsModuleExcluded(modulename)) {
                result = CallAnalyze(_info, modulename, identifier, std::forward<Args>(args)...);
            }
            return result;
        }

    private:
        template <typename CONTROLCATEGORY>
        class WarningReportingControl : public IWarningReportingUnit::IWarningReportingControl {
        private:
            // HPL todo: The code is duplicated, perhaps possible to nest it?
            IS_STATIC_MEMBER_AVAILABLE(Configure, hasConfigure);
            template <typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<!hasConfigure<T, void, const string&>::value, void>::type
            CallConfigure(const string&)
            {
            }
            template <typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<hasConfigure<T, void, const string&>::value, void>::type
            CallConfigure(const string& settings)
            {
                return CONTROLCATEGORY::Configure(settings);
            }
            IS_STATIC_MEMBER_AVAILABLE(CategoryName, hasCategoryName);
            template <typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<!hasCategoryName<T, string>::value, string>::type
            CallCategoryName()
            {
                return Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text();
            }
            template <typename T = CONTROLCATEGORY>
            static inline typename Core::TypeTraits::enable_if<hasCategoryName<T, string>::value, string>::type
            CallCategoryName()
            {
                return CONTROLCATEGORY::CategoryName();
            }

        public:
            WarningReportingControl(const WarningReportingControl&) = delete;
            WarningReportingControl& operator=(const WarningReportingControl&) = delete;

            WarningReportingControl()
                : _categoryName(CallCategoryName())
                , _enabled(0x03)
                , _metadata(Thunder::Core::Messaging::Metadata::type::REPORTING, _categoryName, Thunder::Core::Messaging::MODULE_REPORTING)
            {
                // Register Our control unit, so it can be influenced from the outside
                // if nessecary..
                Core::Messaging::IControl::Announce(this);
                WarningReportingUnitProxy::Instance().AddToCategoryList(*this);

                bool isDefaultCategory = false;
                bool isEnabled = false;
                string settings;
                string excluded;
                WarningReportingUnitProxy::Instance().FetchCategoryInformation(_categoryName, isDefaultCategory, isEnabled, excluded, settings);

                if (isDefaultCategory) {
    
                    if (isEnabled) {
                        _enabled = _enabled | 0x01;
                    }
                    WarningReportingUnitProxy::Instance().FillExcludedWarnings(excluded, _excludedWarnings);

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
                return (_enabled & 0x01) != 0;
            }
            inline bool IsCallsignExcluded(const string& callsign)
            {
                return _excludedWarnings.IsCallsignExcluded(callsign);
            }
            inline bool IsModuleExcluded(const string& module)
            {
                return _excludedWarnings.IsModuleExcluded(module);
            }
            IWarningEvent* Clone() const override
            {
                return new WarningReportingType<CONTROLCATEGORY>();
            }
            bool Enable() const override
            {
                return IsEnabled();
            }
            void Enable(const bool enabled) override
            {
                _enabled = (_enabled & 0xFE) | (enabled ? 0x01 : 0x00);
            }
            void Exclude(const string& toExclude) override
            {
                WarningReportingUnitProxy::Instance().FillExcludedWarnings(toExclude, _excludedWarnings);
            }
            void Configure(const string& settings) override
            {
                CallConfigure(settings);
            }
            void Destroy() override
            {
                if ((_enabled & 0x02) != 0) {
                    Core::Messaging::IControl::Revoke(this);

                    WarningReportingUnitProxy::Instance().RemoveFromCategoryList(*this);
                    _enabled = 0;
                }
            }
            const Core::Messaging::Metadata& Metadata() const override
            {
                return (_metadata);
            }

        protected:
            const string _categoryName;
            uint8_t _enabled;
            ExcludedWarnings _excludedWarnings;
            Core::Messaging::Metadata _metadata;
        };

    public:
        WarningReportingType(const WarningReportingType&) = delete;
        WarningReportingType& operator=(const WarningReportingType&) = delete;

        WarningReportingType()
        {
        }
        ~WarningReportingType() override = default;

    private:
        // HPL todo: The code is duplicated, perhaps possible to nest it?        
        IS_MEMBER_AVAILABLE(IsWarning, hasIsWarning);
        template <typename T = CATEGORY>
        inline typename Core::TypeTraits::enable_if<!hasIsWarning<const T, bool>::value, bool>::type
        CallIsWarning() const
        {
            return true;
        }
        template <typename T = CATEGORY>
        inline typename Core::TypeTraits::enable_if<hasIsWarning<const T, bool>::value, bool>::type
        CallIsWarning() const
        {
            return _info.IsWarning();
        }
        IS_MEMBER_AVAILABLE(Analyze, hasAnalyze);
        template <typename T = CATEGORY, typename... Args>
        inline static typename Core::TypeTraits::enable_if<!hasAnalyze<T, bool, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(T& category, const char[], const char[], Args&&...)
        {
            return true;
        }
        template <typename T = CATEGORY, typename... Args>
        inline static typename Core::TypeTraits::enable_if<hasAnalyze<T, bool, const char[], const char[], Args&&...>::value, bool>::type
        CallAnalyze(T& category, const char modulename[], const char identifier[], Args&&... args)
        {
            return category.Analyze(modulename, identifier, std::forward<Args>(args)...);
        }

    public:
        // No dereference etc.. 1 straight line to enabled or not... Quick method..
        inline static bool IsEnabled()
        {
            return _sWarningControl.IsEnabled();
        }

        inline static void Enable(const bool status)
        {
            _sWarningControl.Enabled(status);
        }

        const char* Category() const override
        {
            return _sWarningControl.Metadata().Category().c_str();
        }

        uint16_t Serialize(uint8_t data[], const uint16_t size) const override
        {
            return _info.Serialize(data, size);
        }

        uint16_t Deserialize(const uint8_t data[], const uint16_t size) override
        {
            return _info.Deserialize(data, size);
        }

        void ToString(string& visitor) const override
        {
            _info.ToString(visitor);
        }

        bool IsWarning() const override
        {
            return CallIsWarning();
        }

        bool Enabled() const
        {
            return _sWarningControl.Enabled();
        }

        void Enabled(const bool enabled)
        {
            _sWarningControl.Enabled(enabled);
        }

        void Destroy()
        {
            _sWarningControl.Destroy();
        }

    private:
        CATEGORY _info;
        static WarningReportingControl<CATEGORY> _sWarningControl;
    };

    #ifdef __WINDOWS__
    template <typename CATEGORY>
    typename WarningReportingType<CATEGORY>::template WarningReportingControl<CATEGORY> WarningReportingType<CATEGORY>::_sWarningControl;
    template <typename CONTROLCATEGORY>
    std::atomic<uint32_t> WarningReportingBoundsCategory<CONTROLCATEGORY>::_reportingBound(CONTROLCATEGORY::DefaultReportBound);
    template <typename CONTROLCATEGORY>
    std::atomic<uint32_t> WarningReportingBoundsCategory<CONTROLCATEGORY>::_warningBound(CONTROLCATEGORY::DefaultWarningBound);
    #else
    template <typename CATEGORY>
    EXTERNAL typename WarningReportingType<CATEGORY>::template WarningReportingControl<CATEGORY> WarningReportingType<CATEGORY>::_sWarningControl;
    template <typename CONTROLCATEGORY>
    EXTERNAL std::atomic<uint32_t> WarningReportingBoundsCategory<CONTROLCATEGORY>::_reportingBound(CONTROLCATEGORY::DefaultReportBound);
    template <typename CONTROLCATEGORY>
    EXTERNAL std::atomic<uint32_t> WarningReportingBoundsCategory<CONTROLCATEGORY>::_warningBound(CONTROLCATEGORY::DefaultWarningBound);
    #endif
}
}

#endif
