#include "MessageClient.h"
namespace WPEFramework {
namespace Messaging {

    MessageClient::MessageClient(const string& identifer, const string& basePath)
        : _identifier(identifer)
        , _basePath(basePath)
    {
    }

    void MessageClient::AddInstance(uint32_t id)
    {
        _adminLock.Lock();

        _clients.emplace(std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(_identifier, id, false, _basePath));

        _listId.emplace_back(id);

        _adminLock.Unlock();
    }

    void MessageClient::RemoveInstance(uint32_t id)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _clients.erase(id);
    }

    const std::list<uint32_t>& MessageClient::InstanceIds() const
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        return _listId;
    }

    void MessageClient::ClearInstances()
    {
        _clients.clear();
    }

    void MessageClient::WaitForUpdates(const uint32_t waitTime)
    {
        _adminLock.Lock();

        if (!_clients.empty()) {
            //ring is same for all dispatchers
            auto firstEntry = _clients.begin();
            _adminLock.Unlock();

            firstEntry->second.Wait(waitTime);
        } else {
            _adminLock.Unlock();
        }
    }

    void MessageClient::SkipWaiting()
    {
        _adminLock.Lock();

        if (!_clients.empty()) {
            //ring is same for all dispatchers
            auto firstEntry = _clients.begin();
            _adminLock.Unlock();

            firstEntry->second.Ring();
        } else {
            _adminLock.Unlock();
        }
    }

    void MessageClient::Enable(const bool, Core::MessageInformation::MessageType, const string&, const string&)
    {
        //todo: send metadata
    }
    bool MessageClient::IsEnabled(Core::MessageInformation::MessageType, const string&, const string&) const
    {
        //todo: //check metadataa
        return false;
    }

    std::pair<Core::MessageInformation, Core::ProxyType<Core::IMessageEvent>> MessageClient::Pop(uint32_t id)
    {
        _adminLock.Lock();

        uint8_t buffer[Core::DataSize];
        uint16_t size = sizeof(buffer);

        Core::MessageInformation information;
        Core::ProxyType<Core::IMessageEvent> message;

        auto client = _clients.find(id);

        if (client != _clients.end()) {

            if (client->second.PopData(size, buffer) != Core::ERROR_NONE) {
                //warning trace here
            } else {
                auto length = information.Deserialize(buffer, size);

                auto factory = _factories.find(information.Type());
                if (factory != _factories.end()) {
                    message = factory->second->Create();
                    message->Deserialize(buffer + length, size - length);
                }
            }
        }
        _adminLock.Unlock();

        return { information, message };
    }

    void MessageClient::AddFactory(Core::MessageInformation::MessageType type, Core::IMessageEventFactory* factory)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _factories.emplace(type, factory);
    }
    void MessageClient::RemoveFactory(Core::MessageInformation::MessageType type)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _factories.erase(type);
    }
}
}