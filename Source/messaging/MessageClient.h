#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    class EXTERNAL MessageClient {
        using Factories = std::unordered_map<Core::MessageInformation::MessageType, Core::IMessageEventFactory*>;
        using MessageDispatcher = Core::MessageDispatcherType<Core::MetaDataSize, Core::DataSize>;

    public:
        ~MessageClient() = default;
        MessageClient(const MessageClient&) = delete;
        MessageClient& operator=(const MessageClient&) = delete;

    public:
        MessageClient(const string& identifer, const string& basePath);

        void AddInstance(uint32_t id);
        void RemoveInstance(uint32_t id);
        void ClearInstances();
        const std::list<uint32_t>& InstanceIds() const;

        void WaitForUpdates(const uint32_t waitTime);
        void SkipWaiting();

        void Enable(const bool enable, Core::MessageInformation::MessageType type, const string& category, const string& module = "MODULE_UNKNOWN");
        bool IsEnabled(Core::MessageInformation::MessageType type, const string& category, const string& module = "MODULE_UNKNOWN") const;

        std::pair<Core::MessageInformation, Core::ProxyType<Core::IMessageEvent>> Pop(uint32_t id);

        void AddFactory(Core::MessageInformation::MessageType type, Core::IMessageEventFactory* factory);
        void RemoveFactory(Core::MessageInformation::MessageType type);

    private:
        mutable Core::CriticalSection _adminLock;
        string _identifier;
        string _basePath;

        std::unordered_map<uint32_t, MessageDispatcher> _clients;
        std::list<uint32_t> _listId;
        Factories _factories;
    };
}
}