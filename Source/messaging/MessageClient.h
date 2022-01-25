#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    /**
     * @brief Class responsible for:
     *        - retreiving data from buffers
     *        - sending metadata to buffers (enabling categories)
     * 
     */
    class EXTERNAL MessageClient {
        using Factories = std::unordered_map<Core::Messaging::MetaData::MessageType, Core::Messaging::IEventFactory*>;

    public:
        using Messages = std::list<std::pair<Core::Messaging::Information, Core::ProxyType<Core::Messaging::IEvent>>>;
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

        void Enable(const Core::Messaging::MetaData& metaData, const bool enable);
        Core::Messaging::ControlList::InformationIterator Enabled();

        Messages Pop();

        void AddFactory(Core::Messaging::MetaData::MessageType type, Core::Messaging::IEventFactory* factory);
        void RemoveFactory(Core::Messaging::MetaData::MessageType type);

    private:
        using Clients = std::unordered_map<uint32_t, Core::Messaging::MessageUnit::MessageDispatcher>;
        mutable Core::CriticalSection _adminLock;
        string _identifier;
        string _basePath;
        uint8_t _readBuffer[Core::Messaging::MessageUnit::DataSize];
        uint8_t _writeBuffer[Core::Messaging::MessageUnit::MetaDataSize];

        Clients _clients;
        Factories _factories;
        Core::Messaging::ControlList::InformationStorage _enabledCategories;
    };
}
}