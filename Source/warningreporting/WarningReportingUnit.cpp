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
        : m_Categories()
        , m_Admin()
        , m_OutputChannel(nullptr)
        , m_DirectOut(false)
    {
        WarningReportingUnitProxy::Instance().Handler(this);
    }

    WarningReportingUnit::ReportingBuffer::ReportingBuffer(const string& doorBell, const string& name)
        : Core::CyclicBuffer(name, 
                                Core::File::USER_READ    | 
                                Core::File::USER_WRITE   | 
                                Core::File::USER_EXECUTE | 
                                Core::File::GROUP_READ   |
                                Core::File::GROUP_WRITE  |
                                Core::File::OTHERS_READ  |
                                Core::File::OTHERS_WRITE | 
                                Core::File::SHAREABLE,
                             CyclicBufferSize, true)
        , _doorBell(doorBell.c_str())
    {
        // Make sure the trace file opened proeprly.
        TRACE_L1("Opened a file to stash my reported warning at: %s [%d] and doorbell: %s", name.c_str(), CyclicBufferSize, doorBell.c_str());
        ASSERT (IsValid() == true);
    }

    WarningReportingUnit::ReportingBuffer::~ReportingBuffer()
    {
    }

    /* virtual */ uint32_t WarningReportingUnit::ReportingBuffer::GetOverwriteSize(Cursor& cursor)
    {
        while (cursor.Offset() < cursor.Size()) {
            uint16_t chunkSize = 0;
            cursor.Peek(chunkSize);

            TRACE_L1("Flushing warning reporting data !!! %d", __LINE__);

            cursor.Forward(chunkSize);
        }

        return cursor.Offset();
    }

    /* virtual */ void WarningReportingUnit::ReportingBuffer::DataAvailable()
    {
        _doorBell.Ring();
    }

    /* static */ WarningReportingUnit& WarningReportingUnit::Instance()
    {
        return (Core::SingletonType<WarningReportingUnit>::Instance());
    }

    WarningReportingUnit::~WarningReportingUnit()
    {

        WarningReportingUnitProxy::Instance().Handler(nullptr);

        m_Admin.Lock();

        if (m_OutputChannel != nullptr) {
            Close();
        }

        for (auto const& category : m_Categories)
        {
              category.second->Destroy();
        }
        

        m_Admin.Unlock();
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
       
            fileName +=  '.' + Core::NumberType<uint32_t>(identifier).Text();
            result = Open(doorBell, fileName);
        }

        return (result);
    }

    uint32_t WarningReportingUnit::Open(const string& pathName)
    {
        string fileName(Core::Directory::Normalize(pathName) + CyclicBufferName);
        #ifdef __WINDOWS__
        string doorBell("127.0.0.1:62002"); 
        #else
        string doorBell(Core::Directory::Normalize(pathName) + CyclicBufferName + ".doorbell" );
        #endif

        Core::SystemInfo::SetEnvironment(WARNINGREPORTING_CYCLIC_BUFFER_FILENAME, fileName);
        Core::SystemInfo::SetEnvironment(WARNINGREPORTING_CYCLIC_BUFFER_DOORBELL, doorBell);

        return (Open(doorBell, fileName));
    }

    uint32_t WarningReportingUnit::Close()
    {
        m_Admin.Lock();

        //ASSERT(m_OutputChannel != nullptr);

        if (m_OutputChannel != nullptr) {
            delete m_OutputChannel;
        }

        m_OutputChannel = nullptr;

        m_Admin.Unlock();

        return (Core::ERROR_NONE);
    }

    void WarningReportingUnit::Announce(IWarningReportingUnit::IWarningReportingControl& Category)
    {
        m_Admin.Lock();

        std::string categoryName = Category.Category();
        m_Categories[categoryName] = &Category;

        m_Admin.Unlock();
    }

    void WarningReportingUnit::Revoke(IWarningReportingUnit::IWarningReportingControl& Category)
    {
        m_Admin.Lock();
        std::string categoryName = Category.Category();
        m_Categories.erase(categoryName);

        m_Admin.Unlock();
    }

    std::list<string> WarningReportingUnit::GetCategories()
    {
        std::list<string> result;
        for (const auto& pair : m_Categories) {
            result.push_back(pair.first);
        }
        return result;
    }

    uint32_t WarningReportingUnit::SetCategories(const bool enable, const char* category)
    {
        uint32_t modifications = 0;

        if (category != nullptr) {
            auto index = m_Categories.find(category);
            if (index != m_Categories.end()) {
                index->second->Enabled(enable);
                ++modifications;
            }
        } else {
            for (auto& pair : m_Categories) {
                pair.second->Enabled(enable);
                ++modifications;
            }
        }

        return modifications;
    }

    string WarningReportingUnit::Defaults() const
    {
        string result;
        Core::JSON::ArrayType<Setting::JSON> serialized;
        Settings::const_iterator index = m_EnabledCategories.begin();
        
        while (index != m_EnabledCategories.end()) {
            serialized.Add(Setting::JSON(*index));
            index++;
        }

        serialized.ToString(result);
        return (result);
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

    void WarningReportingUnit::Defaults(Core::File& file) {
        Core::JSON::ArrayType<Setting::JSON> serialized;
        Core::OptionalType<Core::JSON::Error> error;
        serialized.IElement::FromFile(file, error);
        if (error.IsSet() == true) {
            SYSLOG(WPEFramework::Logging::ParsingError, (_T("Parsing WarningReporting failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
        }

        // Deal with existing categories that might need to be enable/disabled.
        UpdateEnabledCategories(serialized);
    }

    void WarningReportingUnit::UpdateEnabledCategories(const Core::JSON::ArrayType<Setting::JSON>& info)
    {
        // HPL todo: there might be a synchronization issue here??? (at least the enabled should be atomic (altough if aligned on most platforms it will not be an issue)
        Core::JSON::ArrayType<Setting::JSON>::ConstIterator index = info.Elements();

        m_EnabledCategories.clear();

        while (index.Next()) {
            m_EnabledCategories.emplace_back(Setting(index.Current()));
        }

        for (auto& entry : m_Categories) {
            Settings::const_iterator index = m_EnabledCategories.begin(); 
            while (index != m_EnabledCategories.end()) {
                const Setting& setting = *index;

                if ( setting.Category() == entry.second->Category() ) {
                    if( setting.Enabled() != entry.second->Enabled() ) {
                        entry.second->Enabled(setting.Enabled()); 
                    }
                    entry.second->Configure(setting.Configuration());
                    break; // HPL todo: you might to add this also on TraceControl
                }

                index++;
            }
        }
    }

    bool WarningReportingUnit::IsDefaultCategory(const string& category, bool& enabled, string& config) const
    {

        bool isDefaultCategory = false;

        Settings::const_iterator index = m_EnabledCategories.begin(); 
        while (index != m_EnabledCategories.end()) {
            const Setting& setting = *index;

            if ( setting.Category() == category) {
                isDefaultCategory = true;
                enabled = setting.Enabled();
                config = setting.Configuration();
                break; // HPL todo: also in tracecontrol you probably want to stop the loop once found
            }
            index++; 
        }

        return isDefaultCategory;
    }

    void WarningReportingUnit::ReportWarningEvent(const char identifier[], const char file[], const uint32_t lineNumber, const char className[], const IWarningEvent& information)
    { 

        const char* fileName(Core::FileNameOnly(file));

        m_Admin.Lock();

        if (m_OutputChannel != nullptr) {

            const char* category(information.Category());
            const uint64_t current = Core::Time::Now().Ticks();

            const uint16_t fileNameLength = static_cast<uint16_t>(strlen(fileName) + 1); // File name.
            const uint16_t categoryLength = static_cast<uint16_t>(strlen(category) + 1); // Cateogory.
            const uint16_t identifierLength = static_cast<uint16_t>(strlen(identifier) + 1); // Identifier name.

            // length(2 bytes) - clock ticks (8 bytes) - lineNumber (4 bytes) - fileNameLength - categoryLength - identifierLength - data
            const uint16_t headerLength = 2 + 8 + 4 + fileNameLength + categoryLength + identifierLength;
                 
            const uint16_t bufferSize = 1024;
            uint8_t buffer[bufferSize];
            uint16_t result = information.Serialize(buffer, bufferSize);
            
            const uint16_t fullLength = headerLength + result;

            // Tell the buffer how much we are going to write.
            // stack buffer 1kB, serialize 
            const uint32_t actualLength = m_OutputChannel->Reserve(fullLength);
            

            if (actualLength >= fullLength) {
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(&fullLength), 2); //fullLength
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(&current), 8);    //timestamp
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(&lineNumber), 4); //lineNumber
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(fileName), fileNameLength); //filename
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(category), categoryLength); //category name
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(identifier), identifierLength); //identifier aka. callsign
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(buffer), result);
            }
        }

        if ( ( m_DirectOut == true ) && ( information.IsWarning() == true ) ) {

            string text;
            string time(Core::Time::Now().ToRFC1123(true));
            Core::TextFragment cleanClassName(Core::ClassNameOnly(className));

            information.ToString(text);

            fprintf(stdout, "\033[1;32mSUSPICIOUS [%s]: [%s:%s]: %s\n\033[0m", time.c_str(), identifier, information.Category(), text.c_str());
            fflush(stdout);
        }

        m_Admin.Unlock();
    }
}
} 
