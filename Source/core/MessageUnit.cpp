#include "MessageUnit.h"

//const string& vs string for tmp objects

namespace WPEFramework {
namespace Core {

    MessageMetaData::MessageMetaData()
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
    MessageMetaData::MessageMetaData(const MessageType type, const string& category, const string& module)
        : _type(type)
        , _category(category)
        , _module(module)
    {
    }
    uint16_t MessageMetaData::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
    {
        uint16_t length = sizeof(_type) + _category.size() + 1 + _module.size() + 1;
        ASSERT(bufferSize >= length);

        Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
        Core::FrameType<0>::Writer frameWriter(frame, 0);
        frameWriter.Number(_type);
        frameWriter.NullTerminatedText(_category);
        frameWriter.NullTerminatedText(_module);

        return length;
    }
    uint16_t MessageMetaData::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
    {
        Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
        Core::FrameType<0>::Reader frameReader(frame, 0);
        _type = frameReader.Number<Core::MessageMetaData::MessageType>();
        _category = frameReader.NullTerminatedText();
        _module = frameReader.NullTerminatedText();

        return sizeof(_type) + _category.size() + 1 + _module.size() + 1;
    }

    MessageInformation::MessageInformation(const MessageMetaData::MessageType type, const string& category, const string& module, const string& filename, uint16_t lineNumber)
        : _metaData(type, category, module)
        , _filename(filename)
        , _lineNumber(lineNumber)
    {
    }

    uint16_t MessageInformation::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
    {
        auto length = _metaData.Serialize(buffer, bufferSize);
        ASSERT(bufferSize >= length);

        Core::FrameType<0> frame(buffer + length, bufferSize - length, bufferSize - length);
        Core::FrameType<0>::Writer frameWriter(frame, 0);
        frameWriter.NullTerminatedText(_filename);
        frameWriter.Number(_lineNumber);
        length += _filename.size() + 1 + sizeof(_lineNumber);

        return length;
    }
    uint16_t MessageInformation::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
    {
        auto length = _metaData.Deserialize(buffer, bufferSize);

        Core::FrameType<0> frame(buffer + length, bufferSize - length, bufferSize - length);
        Core::FrameType<0>::Reader frameReader(frame, 0);
        _filename = frameReader.NullTerminatedText();
        _lineNumber = frameReader.Number<uint16_t>();
        length += _filename.size() + 1 + sizeof(_lineNumber);

        return length;
    }

    //----------MessageUNIT----------
    MessageUnit& MessageUnit::Instance()
    {
        return (Core::SingletonType<MessageUnit>::Instance());
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
                _dispatcher->RegisterDataAvailable(std::bind(&MessageUnit::ReceiveMetaData, this, std::placeholders::_1, std::placeholders::_2));
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
        _dispatcher.reset(nullptr);
    }

    /**
     * @brief Set default settings (eg. what categories are enabled) for all the components. 
     * 
     * @param setting json able to be parsed by @ref MessageUnit::Settings
     */
    void MessageUnit::Defaults(const string& setting)
    {
        _adminLock.Lock();

        _defaultSettings = setting;

        Settings serialized;
        serialized.FromString(setting);

        Core::JSON::ArrayType<Core::TraceSetting> traceSettings;
        traceSettings.FromString(serialized.Tracing.Value());

        auto traceSettingsIterator = traceSettings.Elements();

        while (traceSettingsIterator.Next()) {
            auto setting = traceSettingsIterator.Current();
            _defaultTraceSettings.emplace(setting.Category.Value(), setting);

            auto control = std::find_if(_controls.begin(), _controls.end(), [&](const IControl* control) {
                return control->Type() == MessageMetaData::MessageType::TRACING && control->Category() == setting.Category.Value();
            });

            if (control != _controls.end()) {
                if (!setting.Module.IsSet() || setting.Module.Value() == (*control)->Module()) {
                    (*control)->Enable(setting.Enabled.Value());
                }
            }
        }

        _adminLock.Unlock();
    }

    /**
     * @brief Get defaults settings
     * 
     * @return string json containing information about default values
     */
    string MessageUnit::Defaults() const
    {
        return _defaultSettings;
    }

    /**
     * @brief Retreive information about category controlled by IControl implementation. 
     *        When category is not enabled, it must not be pushed.
     * 
     * @param control Implementaion controlling enablind/disabling specific category
     * @param outIsEnabled is the category enabled
     * @param outIsDefault is the category default
     */
    void MessageUnit::FetchDefaultSettingsForCategory(const IControl* control, bool& outIsEnabled, bool& outIsDefault)
    {
        _adminLock.Lock();

        if (control->Type() == Core::MessageMetaData::MessageType::TRACING) {
            auto it = _defaultTraceSettings.find(control->Category());
            if (it != _defaultTraceSettings.end()) {
                if (!it->second.Module.IsSet() || it->second.Module.Value() == control->Module()) {
                    outIsDefault = true;
                    outIsEnabled = it->second.Enabled.Value();
                } else {
                    outIsDefault = false;
                    outIsEnabled = false;
                }
            }
        }

        _adminLock.Unlock();
    }

    /**
     * @brief Push a message and its information to a buffer
     * 
     * @param info contains information about the event (where it happened)
     * @param message message
     */
    void MessageUnit::Push(const MessageInformation& info, const IMessageEvent* message)
    {
        uint16_t length = 0;

        length = info.Serialize(_serializationBuffer, sizeof(_serializationBuffer));
        length += message->Serialize(_serializationBuffer + length, sizeof(_serializationBuffer) - length);

        _dispatcher->PushData(length, _serializationBuffer);
        _dispatcher->Ring();
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
     * @brief Notification, that there is metadata available (somebody requested change in the enabled categories)
     * 
     * @param size size of the buffer
     * @param data buffer containing data
     */
    void MessageUnit::ReceiveMetaData(uint16_t size, const uint8_t* data)
    {
        MessageMetaData metaData;
        auto length = metaData.Deserialize(const_cast<uint8_t*>(data), size); //for now, FrameType is not handling const buffers :/

        if (length <= size - 1) {
            bool enabled = data[length];

            for (auto& control : _controls) {

                if (metaData.Type() == control->Type()) {

                    if (!metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Module() == control->Module() && metaData.Category() == control->Category()) {
                            control->Enable(enabled);
                        }
                    } else if (!metaData.Module().empty() && metaData.Category().empty()) {
                        if (metaData.Module() == control->Module()) {
                            control->Enable(enabled);
                        }
                    } else {
                        //enable/disable all categories for all modules
                        control->Enable();
                    }
                }
            }
        }
    }

}
}