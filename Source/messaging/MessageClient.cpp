#include "MessageClient.h"
namespace WPEFramework {
namespace Messaging {

    /**
     * @brief Construct a new Message Client:: Message Client object
     * 
     * @param identifer identifier of the buffers
     * @param basePath where are those buffers located
     */
    MessageClient::MessageClient(const string& identifer, const string& basePath)
        : _identifier(identifer)
        , _basePath(basePath)
    {
    }

    /**
     * @brief Add buffer for instance of given id
     * 
     * @param id 
     */
    void MessageClient::AddInstance(uint32_t id)
    {
        _adminLock.Lock();

        _clients.emplace(std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(_identifier, id, false, _basePath));

        _adminLock.Unlock();
    }

    /**
     * @brief Remove buffer for instance of given id
     * 
     * @param id 
     */
    void MessageClient::RemoveInstance(uint32_t id)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _clients.erase(id);
    }

    /**
     * @brief Remove all buffers
     * 
     */
    void MessageClient::ClearInstances()
    {
        _clients.clear();
    }

    /**
     * @brief Wait for updates in any of the buffers
     * 
     * @param waitTime for how much should this function block
     */
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

    /**
     * @brief When @ref WaitForUpdates is waiting in a blocking state, this function can be used to force it to stop.
     *        It can be also used to "flush" the buffers (for example, data was already waiting, but the buffers were not registered on this side yet)
     *        
     */
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

    /**
     * @brief Enable or disable message (specified by the metaData)
     * 
     * @param metaData information about the message
     * @param enable should it be enabled or not
     */
    void MessageClient::Enable(const Core::Messaging::MetaData& metaData, const bool enable)
    {
        uint16_t bufferSize = sizeof(_writeBuffer);

        for (auto& client : _clients) {
            auto length = metaData.Serialize(_writeBuffer, bufferSize);

            if (length < bufferSize - 1) {
                _writeBuffer[length++] = static_cast<uint8_t>(enable);

                client.second.PushMetadata(length, _writeBuffer, bufferSize);
            }
        }
    }

    /**
     * @brief Get list of currently active message controls
     * 
     * @return Core::ControlList::Iterator iterator for all the controls
     */
    Core::Messaging::ControlList::Iterator MessageClient::Enabled()
    {
        _enabledCategories.clear();

        uint16_t bufferSize = sizeof(_writeBuffer);

        for (auto& client : _clients) {
            auto writtenBack = client.second.PushMetadata(0, _writeBuffer, bufferSize);
            if (writtenBack > 0) {
                Core::Messaging::ControlList controlList;
                controlList.Deserialize(_writeBuffer, writtenBack);

                auto it = controlList.Controls();
                while (it.Next()) {
                    _enabledCategories.push_back(it.Current());
                }
            }
        }

        return Core::Messaging::ControlList::Iterator(_enabledCategories);
    }

    /**
     * @brief Pop will return a message if available in any of the added message dispatchers. The Pop is non blocking and threadsafe
     *        Popped messages are not guaranteed to be in the same order as pushed. Caller should call this function until Pop returns invalid Message,
     *        then WaitForUpdates should be called that will block until data is available
     * 
     * @return Message = Core::OptionalType<std::pair<Core::MessageInformation, Core::ProxyType<Core::IMessageEvent>>>
     *         Optional pair that contains information about the message and the deserialized message itself.
     */
    MessageClient::Message MessageClient::Pop()
    {
        _adminLock.Lock();
        uint16_t size = sizeof(_readBuffer);

        Message result;
        Core::Messaging::Information information;
        Core::ProxyType<Core::Messaging::IEvent> message;

        auto currentClientIt = _clients.begin();
        while (currentClientIt != _clients.end()) {

            if (currentClientIt->second.PopData(size, _readBuffer) == Core::ERROR_NONE) {
                auto length = information.Deserialize(_readBuffer, size);

                if (length != 0 && length <= sizeof(_readBuffer)) {
                    auto factory = _factories.find(information.MessageMetaData().Type());
                    if (factory != _factories.end()) {
                        message = factory->second->Create();
                        message->Deserialize(_readBuffer + length, size - length);
                        result.Value() = std::make_pair(information, message);
                        break;
                    }
                }
            }

            ++currentClientIt;
        }

        _adminLock.Unlock();
        return result;
    }

    /**
     * @brief Register factory for a given message type. The factory will spawn a message suitable for deserializing received bytes
     * 
     * @param type for which message type the factory should be used
     * @param factory 
     */
    void MessageClient::AddFactory(Core::Messaging::MetaData::MessageType type, Core::Messaging::IEventFactory* factory)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _factories.emplace(type, factory);
    }

    /**
     * @brief Unregister factory for given type
     * 
     * @param type 
     */
    void MessageClient::RemoveFactory(Core::Messaging::MetaData::MessageType type)
    {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _factories.erase(type);
    }
}
}