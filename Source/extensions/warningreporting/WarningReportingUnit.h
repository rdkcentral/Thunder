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

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "IWarningReportingMedia.h"

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {

namespace WarningReporting {

    constexpr uint32_t CyclicBufferSize = ((8 * 1024) - (sizeof(struct Core::CyclicBuffer::control))); /* 8Kb */
    extern EXTERNAL const TCHAR* CyclicBufferName;

    // ---- Class Definition ----
    class EXTERNAL WarningReportingUnit : public IWarningReportingUnit {
    public:
        class Setting {
        public:
            class JSON : public Core::JSON::Container {
            public:
                JSON& operator=(const JSON&) = delete;
                JSON()
                    : Core::JSON::Container()
                    , Category()
                    , Enabled(false)
                    , Excluded()
                    , CategoryConfig(false)
                {
                    Add(_T("category"), &Category);
                    Add(_T("enabled"), &Enabled);
                    Add(_T("excluded"), &Excluded);
                    Add(_T("config"), &CategoryConfig);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Category(copy.Category)
                    , Enabled(copy.Enabled)
                    , Excluded(copy.Excluded)
                    , CategoryConfig(copy.CategoryConfig)
                {
                    Add(_T("category"), &Category);
                    Add(_T("enabled"), &Enabled);
                    Add(_T("excluded"), &Excluded);
                    Add(_T("config"), &CategoryConfig);
                }
                JSON(const Setting& rhs)
                    : Core::JSON::Container()
                    , Category()
                    , Enabled()
                    , Excluded()
                    , CategoryConfig(false)
                {
                    Add(_T("category"), &Category);
                    Add(_T("enabled"), &Enabled);
                    Add(_T("excluded"), &Excluded);
                    Add(_T("config"), &CategoryConfig);

                    Category = rhs.Category();
                    Enabled = rhs.Enabled();
                    CategoryConfig = rhs.Configuration();
                }
                ~JSON() override = default;

            public:
                Core::JSON::String Category;
                Core::JSON::Boolean Enabled;
                Core::JSON::String Excluded;
                Core::JSON::String CategoryConfig;
            };

        public:
            Setting(const JSON& source)
                : _category(source.Category.Value())
                , _enabled(source.Enabled.Value())
                , _excluded(source.Excluded.IsSet() ? source.Excluded.Value() : _T(""))
                , _categoryconfig(source.CategoryConfig.IsSet() ? source.CategoryConfig.Value() : _T(""))
            {
            }
            Setting(const Setting& copy)
                : _category(copy._category)
                , _enabled(copy._enabled)
                , _excluded(copy._excluded)
                , _categoryconfig(copy._categoryconfig)
            {
            }
            ~Setting()
            {
            }

        public:
            const string& Category() const
            {
                return _category;
            }
            bool Enabled() const
            {
                return _enabled;
            }
            const string& Excluded() const
            {
                return _excluded;
            }
            const string& Configuration() const
            {
                return _categoryconfig;
            }

        private:
            string _category;
            bool _enabled;
            string _excluded;
            string _categoryconfig;
        };

    public:
        typedef std::unordered_map<string, Setting> Settings;
        typedef std::unordered_map<string, IWarningReportingUnit::IWarningReportingControl*> ControlList;

        IWarningEvent* Clone(const string& categoryName)
        {
            auto index = _categories.find(categoryName);
            if (index != _categories.end()) {
                return index->second->Clone();
            }
            return nullptr;
        }

    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statements.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        WarningReportingUnit(const WarningReportingUnit&) = delete;
        WarningReportingUnit& operator=(const WarningReportingUnit&) = delete;

        class EXTERNAL ReportingBuffer : public Core::CyclicBuffer {

        public:
            ReportingBuffer(const ReportingBuffer&) = delete;
            ReportingBuffer& operator=(const ReportingBuffer&) = delete;

            ReportingBuffer(const string& doorBell, const string& name);
            ~ReportingBuffer();

        public:
            virtual uint32_t GetOverwriteSize(Cursor& cursor) override;
            inline void Ring()
            {
                _doorBell.Ring();
            }
            inline void Acknowledge()
            {
                _doorBell.Acknowledge();
            }
            inline uint32_t Wait(const uint32_t waitTime)
            {
                return _doorBell.Wait(waitTime);
            }
            inline void Relinquish()
            {
                return _doorBell.Relinquish();
            }

        private:
            virtual void DataAvailable() override;

        private:
            Core::DoorBell _doorBell;
        };

    protected:
        WarningReportingUnit();

    public:
        ~WarningReportingUnit() override;

    public:
        static WarningReportingUnit& Instance();

        uint32_t Open(const uint32_t identifier);
        uint32_t Open(const string& pathName);
        uint32_t Close();

        void Announce(IWarningReportingUnit::IWarningReportingControl& Category) override;
        void Revoke(IWarningReportingUnit::IWarningReportingControl& Category) override;
        std::list<string> GetCategories();

        // Default enabled/disabled categories: set via config.json.
        void FetchCategoryInformation(const string& category, bool& outIsDefaultCategory, bool& outIsEnabled, string& outExcluded, string& outConfiguration) const override;
        string Defaults() const;
        void Defaults(const string& jsonCategories);

        void ReportWarningEvent(const char identifier[], const char fileName[], const uint32_t lineNumber, const char className[], const IWarningEvent& information) override;

        inline bool HasDirectOutput() const
        {
            return _directOutput;
        }
        inline void DirectOutput(const bool enabled)
        {
            _directOutput = enabled;
        }
        inline void Announce()
        {
            if (_outputChannel != nullptr) {
                _outputChannel->Ring();
            }
        }
        inline void Acknowledge()
        {
            if (_outputChannel != nullptr) {
                _outputChannel->Acknowledge();
            }
        }
        inline uint32_t Wait(const uint32_t waitTime)
        {
            uint32_t status = Core::ERROR_UNAVAILABLE;
            if (_outputChannel != nullptr) {
                status = _outputChannel->Wait(waitTime);
            }
            return status;
        }
        inline void Relinquish()
        {
            if (_outputChannel != nullptr) {
                _outputChannel->Relinquish();
            }
            return;
        }

    private:
        inline uint32_t Open(const string& doorBell, const string& fileName)
        {

            ASSERT(_outputChannel == nullptr);

            _outputChannel.reset(new ReportingBuffer(doorBell, fileName));

            ASSERT(_outputChannel->IsValid() == true);

            return _outputChannel->IsValid() ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE;

        }
        void UpdateEnabledCategories(const Core::JSON::ArrayType<Setting::JSON>& info);

        ControlList _categories;
        mutable Core::CriticalSection _adminLock;
        std::unique_ptr<ReportingBuffer> _outputChannel;
        Settings _enabledCategories;
        bool _directOutput;
    };
}
}
