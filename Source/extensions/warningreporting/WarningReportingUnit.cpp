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

#include "WarningReportingUnit.h"

#define WARNINGREPORTING_CYCLIC_BUFFER_FILENAME _T("WARNINGREPORTING_FILENAME")
#define WARNINGREPORTING_CYCLIC_BUFFER_DOORBELL _T("WARNINGREPORTING_DOORBELL")

namespace WPEFramework {
namespace WarningReporting {

    /* static */ const TCHAR* CyclicBufferName = _T("warningreportingbuffer");

    WarningReportingUnit::WarningReportingUnit()
        : _categories()
        , _adminLock()
        , _outputChannel(nullptr)
        , _directOutput(false)
    {
        WarningReportingUnitProxy::Instance().Handle(this);
    }

    WarningReportingUnit::ReportingBuffer::ReportingBuffer(const string& doorBell, const string& name)
        : Core::CyclicBuffer(name,
              Core::File::USER_READ | Core::File::USER_WRITE | Core::File::USER_EXECUTE | Core::File::GROUP_READ | Core::File::GROUP_WRITE | Core::File::SHAREABLE,
              CyclicBufferSize, true)
        , _doorBell(doorBell.c_str())
    {
        // Make sure the trace file opened proeprly.
        TRACE_L1("Opened a file to stash my reported warning at: %s [%d] and doorbell: %s", name.c_str(), CyclicBufferSize, doorBell.c_str());
        ASSERT(IsValid() == true);
    }

    WarningReportingUnit::ReportingBuffer::~ReportingBuffer()
    {
    }

    uint32_t WarningReportingUnit::ReportingBuffer::GetOverwriteSize(Cursor& cursor)
    {
        while (cursor.Offset() < cursor.Size()) {
            uint16_t chunkSize = 0;
            cursor.Peek(chunkSize);

            TRACE_L1("Flushing warning reporting data !!! %d", __LINE__);

            cursor.Forward(chunkSize);
        }

        return cursor.Offset();
    }

    void WarningReportingUnit::ReportingBuffer::DataAvailable()
    {
        _doorBell.Ring();
    }

    WarningReportingUnit& WarningReportingUnit::Instance()
    {
        return Core::SingletonType<WarningReportingUnit>::Instance();
    }

    WarningReportingUnit::~WarningReportingUnit()
    {

        if (_outputChannel != nullptr) {
            Close();
        }
        
        while (_categories.size() != 0) {
            _categories.begin()->second->Destroy();
        }

        WarningReportingUnitProxy::Instance().Handle(nullptr);
    }

    uint32_t WarningReportingUnit::Open(const uint32_t identifier)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string fileName;
        string doorBell;
        Core::SystemInfo::GetEnvironment(WARNINGREPORTING_CYCLIC_BUFFER_FILENAME, fileName);
        Core::SystemInfo::GetEnvironment(WARNINGREPORTING_CYCLIC_BUFFER_DOORBELL, doorBell);

        ASSERT(fileName.empty() == false);
        ASSERT(doorBell.empty() == false);

        if (fileName.empty() == false) {

            fileName += '.' + Core::NumberType<uint32_t>(identifier).Text();
            result = Open(doorBell, fileName);
        }

        return result;
    }

    uint32_t WarningReportingUnit::Open(const string& pathName)
    {
        string fileName(Core::Directory::Normalize(pathName) + CyclicBufferName);
#ifdef __WINDOWS__
        string doorBell("127.0.0.1:62002");
#else
        string doorBell(Core::Directory::Normalize(pathName) + CyclicBufferName + ".doorbell");
#endif

        Core::SystemInfo::SetEnvironment(WARNINGREPORTING_CYCLIC_BUFFER_FILENAME, fileName);
        Core::SystemInfo::SetEnvironment(WARNINGREPORTING_CYCLIC_BUFFER_DOORBELL, doorBell);

        return Open(doorBell, fileName);
    }

    uint32_t WarningReportingUnit::Close()
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        
        _outputChannel.reset(nullptr);

        return Core::ERROR_NONE;
    }

    std::list<string> WarningReportingUnit::GetCategories()
    {
        _adminLock.Lock();
        std::list<string> result;
        for (const auto& pair : _categories) {
            result.push_back(pair.first);
        }
        _adminLock.Unlock();
        return result;
    }

    string WarningReportingUnit::Defaults() const
    {
        string result;
        Core::JSON::ArrayType<Setting::JSON> serialized;

        for (const auto& enabledCategory : _enabledCategories) {
            serialized.Add(Setting::JSON(enabledCategory.second));
        }

        serialized.ToString(result);
        return result;
    }

    void WarningReportingUnit::Defaults(const string& jsonCategories)
    {
        Core::JSON::ArrayType<Setting::JSON> serialized;
        Core::OptionalType<Core::JSON::Error> error;
        serialized.FromString(jsonCategories, error);
        if (error.IsSet() == true) {
            SYSLOG(Logging::ParsingError, (_T("Parsing WarningReporting failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
        }

        // Deal with existing categories that might need to be enable/disabled.
        UpdateEnabledCategories(serialized);
    }

    void WarningReportingUnit::UpdateEnabledCategories(const Core::JSON::ArrayType<Setting::JSON>& info)
    {
        Core::JSON::ArrayType<Setting::JSON>::ConstIterator index = info.Elements();
        _adminLock.Lock();

        _enabledCategories.clear();
        while (index.Next()) {
            _enabledCategories.emplace(index.Current().Category.Value(), Setting(index.Current()));
        }

        for (auto& setting : _enabledCategories) {
            auto category = _categories.find(setting.first);

            if (category != _categories.end()) {
                
                if (category->second->Enable() != setting.second.Enabled()) {
                    category->second->Enable(setting.second.Enabled());
                }

                category->second->Configure(setting.second.Configuration());
                category->second->Exclude(setting.second.Excluded());
            }
        }
        _adminLock.Unlock();
    }

    void WarningReportingUnit::FetchCategoryInformation(const string& category, bool& outIsDefaultCategory, bool& outIsEnabled, string& outExcluded, string& outConfiguration) const
    {
        _adminLock.Lock();

        outIsDefaultCategory = true;
        auto setting = _enabledCategories.find(category);

        if (setting != _enabledCategories.end()) {
            outIsDefaultCategory = true;
            outIsEnabled = setting->second.Enabled();
            outExcluded = setting->second.Excluded();
            outConfiguration = setting->second.Configuration();
        }

        _adminLock.Unlock();
    }

    void WarningReportingUnit::ReportWarningEvent(const char identifier[], const IWarningEvent& information)
    {        
        WPEFramework::Core::Messaging::Metadata metadata(WPEFramework::Core::Messaging::Metadata::type::REPORTING, information.Category(), WPEFramework::Core::Messaging::MODULE_REPORTING);
        WPEFramework::Core::Messaging::MessageInfo messageInfo(metadata, WPEFramework::Core::Time::Now().Ticks());
        WPEFramework::Core::Messaging::IStore::WarningReporting report(messageInfo, identifier);

        string text;
        information.ToString(text);
        WPEFramework::Messaging::TextMessage data(text);

        WPEFramework::Messaging::MessageUnit::Instance().Push(report, &data);
    }
}
}
