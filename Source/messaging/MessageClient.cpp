/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MessageClient.h"

namespace Thunder {

namespace Messaging {

    /**
     * @brief Construct a new Message Client:: Message Client object
     *
     * @param identifer identifier of the buffers
     * @param basePath where are those buffers located
     * @param socketPort triggers the use of using a IP socket in stead of a domain socket if the port value is not 0.
     */
    MessageClient::MessageClient(const string& identifer, const string& basePath, const uint16_t socketPort)
        : _adminLock()
        , _identifier(identifer)
        , _basePath(basePath)
        , _socketPort(socketPort)
        , _clients()
        , _factories()
    {
        ::memset(_readBuffer, 0, sizeof(_readBuffer));
    }

    /**
     * @brief Add buffer for instance of given id
     *
     * @param id
     */
    void MessageClient::AddInstance(const uint32_t id)
    {
        _adminLock.Lock();
        _clients.emplace(std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(_identifier, id, _basePath, _socketPort));
        _adminLock.Unlock();
    }

    /**
     * @brief Remove buffer for instance of given id
     *
     * @param id
     */
    void MessageClient::RemoveInstance(const uint32_t id)
    {
        _adminLock.Lock();
        _clients.erase(id);
        _adminLock.Unlock();
    }

    /**
     * @brief Remove all buffers
     *
     */
    void MessageClient::ClearInstances()
    {
        _adminLock.Lock();
        _clients.clear();
        _adminLock.Unlock();
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
        }
        else {
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
        }
        else {
            _adminLock.Unlock();
        }
    }

    /**
     * @brief Enable or disable message (specified by the metaData)
     *
     * @param metaData information about the message
     * @param enable should it be enabled or not
     */
    void MessageClient::Enable(const Core::Messaging::Metadata& metadata, const bool enable)
    {
        _adminLock.Lock();

        for (auto& client : _clients) {
            client.second.Update(1000, metadata, enable);
        }

        _adminLock.Unlock();
    }

    /**
     * @brief Get list of currently announced message modules
     */
    void MessageClient::Modules(std::vector<string>& modules) const
    {
        _adminLock.Lock();

        for (auto& client : _clients) {
            client.second.Modules(modules);
        }

        _adminLock.Unlock();
    }

    /**
     * @brief Get list of currently announced message controls for a given module
     */
    void MessageClient::Controls(Messaging::MessageUnit::Iterator& controls, const string& module) const
    {
        Messaging::MessageUnit::ControlList list;

        _adminLock.Lock();

        for (auto& client : _clients) {
            client.second.Load(list, module);
        }

        _adminLock.Unlock();

        controls = std::move(list);
    }

    /**
     * @brief Pop all messages from all buffers, and for each of them call a passed function, with information about popped message
     *        This method should be called after receiving doorbell ring (after WaitForUpdated function)
     *
     * @param function function to be called on each of the messages in the buffer
     */
    void MessageClient::PopMessagesAndCall(const MessageHandler& handler)
    {
        _adminLock.Lock();

        for (auto& client : _clients) {
            client.second.Validate();
            uint16_t size = sizeof(_readBuffer);

            while (client.second.PopData(size, _readBuffer) != Core::ERROR_READ_ERROR) {
                ASSERT(size != 0);

                if (size > sizeof(_readBuffer)) {
                    size = sizeof(_readBuffer);
                }

                const Core::Messaging::Metadata::type type = static_cast<Core::Messaging::Metadata::type>(_readBuffer[0]);
                ASSERT(type != Core::Messaging::Metadata::type::INVALID);

                uint16_t length = 0;

                ASSERT(handler != nullptr);

                auto factory = _factories.find(type);

                if (factory != _factories.end()) {
                    Core::ProxyType<Core::Messaging::MessageInfo> metadata;
                    Core::ProxyType<Core::Messaging::IEvent> message;

                    metadata = factory->second->GetMetadata();
                    message = factory->second->GetMessage();

                    length = metadata->Deserialize(_readBuffer, size);
                    length += message->Deserialize((&_readBuffer[length]), (size - length));

                    handler(metadata, message);
                }

                if (length == 0) {
                    client.second.FlushDataBuffer();
                }

                size = sizeof(_readBuffer);
            }
        }
 
        _adminLock.Unlock();
    }

    /**
     * @brief Register factory for a given message type. The factory will spawn a message suitable for deserializing received bytes
     *
     * @param type for which message type the factory should be used
     * @param factory
     */
    void MessageClient::AddFactory(Core::Messaging::Metadata::type type, IEventFactory* factory)
    {
        _adminLock.Lock();
        _factories.emplace(type, factory);
        _adminLock.Unlock();
    }

    /**
     * @brief Unregister factory for given type
     *
     * @param type
     */
    void MessageClient::RemoveFactory(Core::Messaging::Metadata::type type)
    {
        _adminLock.Lock();
        _factories.erase(type);
        _adminLock.Unlock();
    }

} // namespace Messaging
}
