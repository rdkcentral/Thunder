#include "MessageUnit.h"

namespace WPEFramework {
namespace Core {
    namespace Messaging {
        using namespace std::placeholders;

        MetaData::MetaData()
            : _type(INVALID)
        {
        }
        /**
     * @brief Construct a new MetaData object
     * 
     * NOTE: Category and module can be set as empty
     * @param type type of the message
     * @param category category name of the message
     * @param module module name of the message
     */
        MetaData::MetaData(const MessageType type, const string& category, const string& module)
            : _type(type)
            , _category(category)
            , _module(module)
        {
        }
        uint16_t MetaData::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = sizeof(_type) + _category.size() + 1 + _module.size() + 1;

            if (bufferSize >= length) {
                ASSERT(bufferSize >= length);

                Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
                Core::FrameType<0>::Writer frameWriter(frame, 0);
                frameWriter.Number(_type);
                frameWriter.NullTerminatedText(_category);
                frameWriter.NullTerminatedText(_module);
            } else {
                length = 0;
            }

            return length;
        }
        uint16_t MetaData::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Reader frameReader(frame, 0);
            _type = frameReader.Number<MetaData::MessageType>();
            _category = frameReader.NullTerminatedText();
            _module = frameReader.NullTerminatedText();

            return sizeof(_type) + _category.size() + 1 + _module.size() + 1;
        }

        Information::Information(const MetaData::MessageType type, const string& category, const string& module, const string& filename, uint16_t lineNumber, const uint64_t timeStamp)
            : _metaData(type, category, module)
            , _filename(filename)
            , _lineNumber(lineNumber)
            , _timeStamp(timeStamp)
        {
        }

        uint16_t Information::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            auto length = _metaData.Serialize(buffer, bufferSize);

            if (length != 0) {
                if (bufferSize >= length + _filename.size() + 1 + sizeof(_lineNumber) + sizeof(_timeStamp)) {

                    Core::FrameType<0> frame(buffer + length, bufferSize - length, bufferSize - length);
                    Core::FrameType<0>::Writer frameWriter(frame, 0);
                    frameWriter.NullTerminatedText(_filename);
                    frameWriter.Number(_lineNumber);
                    frameWriter.Number(_timeStamp);
                    length += _filename.size() + 1 + sizeof(_lineNumber) + sizeof(_timeStamp);

                } else {
                    length = 0;
                }
            }

            return length;
        }
        uint16_t Information::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
        {
            auto length = _metaData.Deserialize(buffer, bufferSize);

            if (length <= bufferSize) {
                Core::FrameType<0> frame(buffer + length, bufferSize - length, bufferSize - length);
                Core::FrameType<0>::Reader frameReader(frame, 0);
                _filename = frameReader.NullTerminatedText();
                _lineNumber = frameReader.Number<uint16_t>();
                _timeStamp = frameReader.Number<uint64_t>();

                length += _filename.size() + 1 + sizeof(_lineNumber) + sizeof(_timeStamp);
            } else {
                length = 0;
            }

            return length;
        }

        //----------TraceSettings----------
        TraceSetting::TraceSetting(const string& module, const string& category, const bool enabled)
            : Core::JSON::Container()
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);

            //if done in initializer list, set values are seen as "Defaults", not as "Values"
            Module = module;
            Category = category;
            Enabled = enabled;
        }
        TraceSetting::TraceSetting()
            : Core::JSON::Container()
            , Module()
            , Category()
            , Enabled(false)
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);
        }

        TraceSetting::TraceSetting(const TraceSetting& other)
            : Core::JSON::Container()
            , Module(other.Module)
            , Category(other.Category)
            , Enabled(other.Enabled)
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);
        }

        TraceSetting& TraceSetting::operator=(const TraceSetting& other)
        {
            if (&other == this) {
                return *this;
            }

            Module = other.Module;
            Category = other.Category;
            Enabled = other.Enabled;

            return *this;
        }

        //----------Settings----------
        Settings::Settings()
            : Core::JSON::Container()
            , Tracing()
            , Logging()
            , WarningReporting(false)
        {
            Add(_T("tracing"), &Tracing);
            Add(_T("logging"), &Logging);
            Add(_T("warning_reporting"), &WarningReporting);
        }
        Settings::Settings(const Settings& other)
            : Core::JSON::Container()
            , Tracing(other.Tracing)
            , Logging(other.Logging)
            , WarningReporting(other.WarningReporting)
        {
            Add(_T("tracing"), &Tracing);
            Add(_T("logging"), &Logging);
            Add(_T("warning_reporting"), &WarningReporting);
        }

        Settings& Settings::operator=(const Settings& other)
        {
            if (this == &other) {
                return *this;
            }

            Tracing = other.Tracing;
            Logging = other.Logging;
            WarningReporting = other.WarningReporting;
            return *this;
        }

        //----------MessageList----------
        /**
     * @brief Based on metadata, update specific message. If there is no match, add entry to the list
     * 
     * @param metaData information about the message
     * @param isEnabled should the message be enabled 
     */
        void MessageList::Update(const MetaData& metaData, const bool isEnabled)
        {
            if (metaData.Type() == MetaData::MessageType::TRACING) {
                bool found = false;
                auto it = _settings.Tracing.Elements();
                while (it.Next()) {

                    //toggle for module and category
                    if (!metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Module() == it.Current().Module.Value() && metaData.Category() == it.Current().Category.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                        //toggle all categories for module
                    } else if (!metaData.Module().empty() && metaData.Category().empty()) {
                        if (metaData.Module() == it.Current().Module.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                    }
                    //toggle category for all modules
                    else if (metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Category() == it.Current().Category.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                    }
                }
                if (!found) {
                    _settings.Tracing.Add({ metaData.Module(), metaData.Category(), isEnabled });
                }
            }
        }
        Settings MessageList::JsonSettings() const
        {
            return _settings;
        }
        void MessageList::JsonSettings(const Settings& settings)
        {
            _settings = settings;
        }
        /**
     * @brief Check if speific message (control) should be enabled
     * 
     * @param metaData information about the message
     * @return true should be enabled
     * @return false should not be enabled
     */
        bool MessageList::IsEnabled(const MetaData& metaData) const
        {
            bool result = false;
            if (metaData.Type() == MetaData::MessageType::TRACING) {
                auto it = _settings.Tracing.Elements();

                while (it.Next()) {
                    if ((!it.Current().Module.IsSet() && it.Current().Category.Value() == metaData.Category()) || (it.Current().Module.Value() == metaData.Module() && it.Current().Category.Value() == metaData.Category())) {
                        result = it.Current().Enabled.Value();
                    } else {
                        result = false;
                    }
                }
            }
            return result;
        }

        //----------ControlList----------

        /**
     * @brief Write information about the announced controls to the buffer
     * 
     * @param buffer buffer to be written to
     * @param length max length of the buffer
     * @param controls controls to be serialized
     * @return uint16_t how much bytes serialized
     */
        uint16_t ControlList::Serialize(uint8_t buffer[], const uint16_t length, const std::list<IControl*>& controls) const
        {
            ASSERT(length > 0);

            uint16_t serialized = 0;
            buffer[serialized++] = static_cast<uint8_t>(controls.size()); //num of entries
            for (const auto& control : controls) {
                serialized += control->MessageMetaData().Serialize(buffer + serialized, length - serialized);
                buffer[serialized++] = control->Enable();
            }
            return serialized;
        }

        /**
     * @brief Restore information about announced controls from the buffer
     * 
     * @param buffer serialized buffer 
     * @param length max length of the buffer
     * @return uint16_t how much bytes deserialized
     */
        uint16_t ControlList::Deserialize(uint8_t buffer[], const uint16_t length)
        {
            uint16_t deserialized = 0;
            uint8_t entries = buffer[deserialized++];

            for (int i = 0; i < entries; i++) {
                MetaData metadata;
                bool isEnabled = false;
                deserialized += metadata.Deserialize(buffer + deserialized, length - deserialized);
                isEnabled = buffer[deserialized++];

                _info.push_back({ metadata, isEnabled });
            }

            return deserialized;
        }
        //----------MessageUNIT----------
        MessageUnit& MessageUnit::Instance()
        {
            return (Core::SingletonType<MessageUnit>::Instance());
        }

        MessageUnit::~MessageUnit()
        {
            Close();
        }

        /**
     * @brief Open MessageUnit. This method is used on the WPEFramework side. 
     *        This method: 
     *        - sets env variables, so the OOP components will get information (eg. where to place its files)
     *        - create buffer where all InProcess components will write
     * 
     * @param pathName volatile path (/tmp/ by default)
     * @return uint32_t ERROR_NONE: opened sucessfully
     *                  ERROR_OPENING_FAILED failed to open
     */
        uint32_t MessageUnit::Open(const string& pathName)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            string basePath = Core::Format("%s%s", pathName.c_str(), _T("MessageDispatcher"));
            string identifier = _T("md");

            if (Core::File(basePath).IsDirectory()) {
                //if directory exists remove it to clear data (eg. sockets) that can remain after previous run
                Core::Directory(basePath.c_str()).Destroy(false);
            }

            if (!Core::Directory(basePath.c_str()).CreatePath()) {
                TRACE_L1(_T("Unable to create MessageDispatcher directory"));
            }

            Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
            Core::SystemInfo::SetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);

            _dispatcher.reset(new MessageDispatcher(identifier, 0, true, basePath));
            if (_dispatcher != nullptr) {
                if (_dispatcher->IsValid()) {
                    _dispatcher->RegisterDataAvailable(std::bind(&MessageUnit::ReceiveMetaData, this, _1, _2, _3, _4));
                    result = Core::ERROR_NONE;
                }
            }
            return result;
        }

        /**
     * @brief Open MessageUnit. Method used in OOP components
     * 
     * @param instanceId number of the instance
     * @return uint32_t ERROR_NONE: Opened sucesfully
     *                  ERROR_OPENING_FAILED: failed to open
     * 
     */
        uint32_t MessageUnit::Open(const uint32_t instanceId)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            string basePath;
            string identifier;

            Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
            Core::SystemInfo::GetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);

            _dispatcher.reset(new MessageDispatcher(identifier, instanceId, true, basePath));
            if (_dispatcher != nullptr) {
                if (_dispatcher->IsValid()) {
                    result = Core::ERROR_NONE;
                }
            }

            return result;
        }

        void MessageUnit::Close()
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);

            while (_controls.size() != 0) {
                (*_controls.begin())->Destroy();
            }

            _dispatcher.reset(nullptr);
        }

        /**
     * @brief Read defaults settings form string
     * @param setting json able to be parsed by @ref MessageUnit::Settings
     */
        void MessageUnit::Defaults(const string& setting)
        {
            _adminLock.Lock();

            Settings serialized;

            Core::OptionalType<Core::JSON::Error> error;
            serialized.IElement::FromString(setting, error);
            if (error.IsSet() == true) {

                TRACE_L1(_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str());
            }

            SetDefaultSettings(serialized);

            _adminLock.Unlock();
        }

        /**
     * @brief Read default settings from file
     * 
     * @param file file containing configuraton
     */
        void MessageUnit::Defaults(Core::File& file)
        {
            _adminLock.Lock();

            Settings serialized;

            Core::OptionalType<Core::JSON::Error> error;
            serialized.IElement::FromFile(file, error);
            if (error.IsSet() == true) {

                TRACE_L1(_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str());
            }

            SetDefaultSettings(serialized);

            _adminLock.Unlock();
        }

        /**
     * @brief Set defaults acording to settings
     * 
     * @param serialized settings
     */
        void MessageUnit::SetDefaultSettings(const Settings& serialized)
        {
            _messages.JsonSettings(serialized);

            for (auto& control : _controls) {
                auto enabled = _messages.IsEnabled(control->MessageMetaData());
                control->Enable(enabled);
            }
        }

        /**
     * @brief Get defaults settings
     * 
     * @return string json containing information about default values
     */
        string MessageUnit::Defaults() const
        {
            string result;
            auto settings = _messages.JsonSettings();
            settings.ToString(result);

            return result;
        }

        /**
     * @brief Check if given control is enabled (by default setings, or by user input)
     * 
     * @param control specified control
     * @return true enabled
     * @return false not enabled (messages from this control should not be pushed)
     */
        bool MessageUnit::IsControlEnabled(const IControl* control)
        {
            Core::SafeSyncType<CriticalSection> guard(_adminLock);
            return _messages.IsEnabled(control->MessageMetaData());
        }

        /**
     * @brief Push a message and its information to a buffer
     * 
     * @param info contains information about the event (where it happened)
     * @param message message
     */
        void MessageUnit::Push(const Information& info, const IEvent* message)
        {
            uint16_t length = 0;

            length = info.Serialize(_serializationBuffer, sizeof(_serializationBuffer));

            //only serialize message if the information could fit
            if (length != 0) {
                length += message->Serialize(_serializationBuffer + length, sizeof(_serializationBuffer) - length);

                if (_dispatcher->PushData(length, _serializationBuffer) != Core::ERROR_WRITE_ERROR) {
                    _dispatcher->Ring();
                }

            } else {
                TRACE_L1(_T("Unable to push data, buffer is too small!"));
            }
        }

        /**
     * @brief When IControl spawns it should announce itself to the unit, so it can be influenced from here
     *        (For example for enabling the category it controls)
     * 
     * @param control IControl implementation
     */
        void MessageUnit::Announce(IControl* control)
        {
            ASSERT(control != nullptr);
            ASSERT(std::find(_controls.begin(), _controls.end(), control) == _controls.end());

            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            _controls.emplace_back(control);
        }

        /**
     * @brief When IControl dies it should be unregistered
     * 
     * @param control IControl implementation
     */
        void MessageUnit::Revoke(IControl* control)
        {
            ASSERT(control != nullptr);
            ASSERT(std::find(_controls.begin(), _controls.end(), control) != _controls.end());

            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);

            auto entry = std::find(_controls.begin(), _controls.end(), control);
            _controls.erase(entry);
        }

        /**
     * @brief Notification, that there is metadata available
     * 
     * @param size size of the buffer
     * @param data buffer containing in data
     * @param outSize size of the out buffer (initially set to the maximum one can write)
     * @param outData out buffer (passed to the other side)
     */
        void MessageUnit::ReceiveMetaData(const uint16_t size, const uint8_t* data, uint16_t& outSize, uint8_t* outData)
        {
            if (size != 0) {
                MetaData metaData;
                //last byte is enabled flag
                auto length = metaData.Deserialize(const_cast<uint8_t*>(data), size - 1); //for now, FrameType is not handling const buffers :/

                if (length <= size - 1) {
                    bool enabled = data[length];
                    UpdateControls(metaData, enabled);
                    _messages.Update(metaData, enabled);
                }
            } else {
                auto length = _controlList.Serialize(outData, outSize, _controls);
                outSize = length;
            }
        }

        /**
     * @brief Update announced controls
     * 
     * @param metaData information about the message
     * @param enabled should the control be enabled
     */
        void MessageUnit::UpdateControls(const MetaData& metaData, const bool enabled)
        {
            for (auto& control : _controls) {

                if (metaData.Type() == control->MessageMetaData().Type()) {

                    //toggle for module and category
                    if (!metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Module() == control->MessageMetaData().Module() && metaData.Category() == control->MessageMetaData().Category()) {
                            control->Enable(enabled);
                        }
                        //toggle all categories for module
                    } else if (!metaData.Module().empty() && metaData.Category().empty()) {
                        if (metaData.Module() == control->MessageMetaData().Module()) {
                            control->Enable(enabled);
                        }
                    }
                    //toggle category for all modules
                    else if (metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Category() == control->MessageMetaData().Category()) {
                            control->Enable(enabled);
                        }
                        //toggle all categories for all modules
                    } else {
                        control->Enable(enabled);
                    }
                }
            }
        }
    }
}
}