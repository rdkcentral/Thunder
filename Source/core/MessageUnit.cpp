#include "MessageUnit.h"

namespace WPEFramework {
namespace Core {
    MessageInformation::MessageInformation()
        : _type(INVALID)
    {
    }

    MessageInformation::MessageInformation(MessageType type, string category, string filename, uint16_t lineNumber)
        : _type(type)
        , _category(category)
        , _filename(filename)
        , _lineNumber(lineNumber)
    {
    }

    MessageInformation::MessageType MessageInformation::Type() const
    {
        return _type;
    }

    string MessageInformation::Category() const
    {
        return _category;
    }

    string MessageInformation::FileName() const
    {
        return _filename;
    }

    uint16_t MessageInformation::LineNumber() const
    {
        return _lineNumber;
    }

    void MessageInformation::Type(MessageType type)
    {
        _type = type;
    }

    void MessageInformation::Category(string category)
    {
        _category = category;
    }

    void MessageInformation::FileName(string filename)
    {
        _filename = filename;
    }

    void MessageInformation::LineNumber(uint16_t lineNumber)
    {
        _lineNumber = lineNumber;
    }

    uint16_t MessageInformation::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
    {
        ASSERT(bufferSize >= sizeof(Core::MessageInformation::MessageType) + _category.size() + 1 + _filename.size() + 1 + sizeof(_lineNumber));
        uint16_t length = sizeof(Core::MessageInformation::MessageType) + _category.size() + 1 + _filename.size() + 1 + sizeof(_lineNumber);

        Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
        Core::FrameType<0>::Writer frameWriter(frame, 0);
        frameWriter.Number(_type);
        frameWriter.NullTerminatedText(_category);
        frameWriter.NullTerminatedText(_filename);
        frameWriter.Number(_lineNumber);

        return length;
    }
    uint16_t MessageInformation::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
    {
        Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
        Core::FrameType<0>::Reader frameReader(frame, 0);
        _type = frameReader.Number<Core::MessageInformation::MessageType>();
        _category = frameReader.NullTerminatedText();
        _filename = frameReader.NullTerminatedText();
        _lineNumber = frameReader.Number<uint16_t>();

        return sizeof(_type) + _category.size() + 1 + _filename.size() + 1 + sizeof(_lineNumber);
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

            auto control = _controls.find({ MessageInformation::MessageType::TRACING, setting.Category.Value() });

            if (control != _controls.end()) {
                if (control->second->Category() == setting.Category.Value()) {
                    if (!setting.Module.IsSet() || setting.Module.Value() == control->second->Module()) {
                        control->second->Enable(setting.Enabled.Value());
                    }
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

        if (control->Type() == Core::MessageInformation::TRACING) {
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
        uint8_t buffer[DataSize];
        uint16_t length = 0;

        length = info.Serialize(buffer, sizeof(buffer));
        length += message->Serialize(buffer + length, sizeof(buffer) - length);

        _dispatcher->PushData(length, buffer);
        _dispatcher->Ring();
    }

    void MessageUnit::Announce(Core::MessageInformation::MessageType type, const string& category, IControl* control)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _controls.emplace(std::make_pair(type, category), control);
    }
    void MessageUnit::Revoke(Core::MessageInformation::MessageType type, const string& category)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        auto entry = _controls.find({ type, category });
        _controls.erase(entry);
    }

}
}