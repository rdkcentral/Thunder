#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    class EXTERNAL MessageClient {
        using Factories = std::unordered_map<Core::MessageMetaData::MessageType, Core::IMessageEventFactory*>;

    public:
        using Message = Core::OptionalType<std::pair<Core::MessageInformation, Core::ProxyType<Core::IMessageEvent>>>;
        ~MessageClient() = default;
        MessageClient(const MessageClient&) = delete;
        MessageClient& operator=(const MessageClient&) = delete;

    public:
        MessageClient(const string& identifer, const string& basePath);

        void AddInstance(uint32_t id);
        void RemoveInstance(uint32_t id);
        void ClearInstances();

        void WaitForUpdates(const uint32_t waitTime);
        void SkipWaiting();

        void Enable(Core::MessageMetaData::MessageType type, const string& category, const bool enable, const string& module = "MODULE_UNKNOWN");
        bool IsEnabled(Core::MessageMetaData::MessageType type, const string& category, const string& module = "MODULE_UNKNOWN") const;

        Message Pop();

        void AddFactory(Core::MessageMetaData::MessageType type, Core::IMessageEventFactory* factory);
        void RemoveFactory(Core::MessageMetaData::MessageType type);

    private:
        mutable Core::CriticalSection _adminLock;
        string _identifier;
        string _basePath;
        uint8_t _readBuffer[Core::MessageUnit::DataSize];
        uint8_t _writeBuffer[Core::MessageUnit::MetaDataSize];

        std::unordered_map<uint32_t, Core::MessageUnit::MessageDispatcher> _clients;
        Factories _factories;
    };
}
}