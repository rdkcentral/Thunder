#include "MessageUnit.h"

//const string& vs string for tmp objects

namespace WPEFramework {
namespace Core {

    MessageMetaData::MessageMetaData()
        : _type(INVALID)
    {
    }
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

    uint32_t MessageUnit::Open(const uint32_t instanceId)
    {
        string basePath;
        string identifier;

        Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
        Core::SystemInfo::GetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);

        _dispatcher.reset(new MessageDispatcher(identifier, instanceId, true, basePath));

        return Core::ERROR_NONE;
    }
    uint32_t MessageUnit::Open(const string& pathName)
    {
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
        return Core::ERROR_NONE;
    }

    void MessageUnit::Ring()
    {
        //ring will affect all dispatchers with given identifier
        _dispatcher->Ring();
    }

    uint32_t MessageUnit::Close()
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _dispatcher.reset(nullptr);
        return Core::ERROR_NONE;
    }

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

    string MessageUnit::Defaults() const
    {
        return _defaultSettings;
    }

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

    void MessageUnit::Push(const MessageInformation& info, const IMessageEvent* message)
    {
        uint16_t length = 0;

        length = info.Serialize(_serializationBuffer, sizeof(_serializationBuffer));
        length += message->Serialize(_serializationBuffer + length, sizeof(_serializationBuffer) - length);

        _dispatcher->PushData(length, _serializationBuffer);
        _dispatcher->Ring();
    }

    void MessageUnit::Announce(IControl* control)
    {
        ASSERT(control != nullptr);
        ASSERT(std::find(_controls.begin(), _controls.end(), control) == _controls.end());

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _controls.emplace_back(control);
    }

    void MessageUnit::Revoke(IControl* control)
    {
        ASSERT(control != nullptr);
        ASSERT(std::find(_controls.begin(), _controls.end(), control) != _controls.end());

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);

        auto entry = std::find(_controls.begin(), _controls.end(), control);
        _controls.erase(entry);
    }

}
}