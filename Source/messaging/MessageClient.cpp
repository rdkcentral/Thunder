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

    /**
     * @brief Pop will return a message if available in any of the added message dispatchers. The Pop is non blocking and threadsafe
     *        No data order guaranteed. Caller should call this function until Pop returns invalid Message, then WaitForUpdates should be called that will block
     *        until data is available
     * 
     * @return Message = Core::OptionalType<std::pair<Core::MessageInformation, Core::ProxyType<Core::IMessageEvent>>>
     *         Optional pair that contains information about the message and the deserialized message itself.
     */
    MessageClient::Message MessageClient::Pop()
    {
        _adminLock.Lock();
        uint16_t size = sizeof(_readBuffer);

        Message result;
        Core::MessageInformation information;
        Core::ProxyType<Core::IMessageEvent> message;

        auto currentClientIt = _clients.begin();
        while (currentClientIt != _clients.end()) {

            if (currentClientIt->second.PopData(size, _readBuffer) != Core::ERROR_NONE) {
                //warning trace here
            } else {
                auto length = information.Deserialize(_readBuffer, size);

                auto factory = _factories.find(information.Type());
                if (factory != _factories.end()) {
                    message = factory->second->Create();
                    message->Deserialize(_readBuffer + length, size - length);
                    result.Value() = std::make_pair(information, message);
                    break;
                }
            }

            ++currentClientIt;
        }

        _adminLock.Unlock();
        return result;
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