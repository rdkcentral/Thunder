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

namespace Thunder {
namespace WarningReporting {

    WarningReportingUnit::WarningReportingUnit()
        : _categories()
        , _adminLock()
    {
        WarningReportingUnitProxy::Instance().Handle(this);
    }

    WarningReportingUnit& WarningReportingUnit::Instance()
    {
        return Core::SingletonType<WarningReportingUnit>::Instance();
    }

    WarningReportingUnit::~WarningReportingUnit()
    {
        while (_categories.size() != 0) {
            _categories.begin()->second->Destroy();
        }

        WarningReportingUnitProxy::Instance().Handle(nullptr);
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

    void WarningReportingUnit::AddToCategoryList(IWarningReportingUnit::IWarningReportingControl& category)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
    
        _categories[category.Metadata().Category()] = &category;
    }

    void WarningReportingUnit::RemoveFromCategoryList(IWarningReportingUnit::IWarningReportingControl& category)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        
        _categories.erase(category.Metadata().Category());
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
        Thunder::Core::Messaging::Metadata metadata(Thunder::Core::Messaging::Metadata::type::REPORTING, information.Category(), Thunder::Core::Messaging::MODULE_REPORTING);
        Thunder::Core::Messaging::MessageInfo messageInfo(metadata, Thunder::Core::Time::Now().Ticks());
        Thunder::Core::Messaging::IStore::WarningReporting report(messageInfo, identifier);

        string text;
        information.ToString(text);
        Thunder::Messaging::TextMessage data(text);

        Thunder::Messaging::MessageUnit::Instance().Push(report, &data);
    }
}
}
