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
namespace WPEFramework {
namespace Messaging {

    /**
     * @brief Construct a new Message Client:: Message Client object
     * 
     * @param identifer identifier of the buffers
     * @param basePath where are those buffers located
     * @param socketPort triggers the use of using a IP socket in stead of a domain socket if the port value is not 0.
     */
    MessageClient::MessageClient(const string& identifer, const string& basePath, const uint16_t socketPort)
        : _identifier(identifer)
        , _basePath(basePath)
        , _socketPort(socketPort)
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
            std::forward_as_tuple(_identifier, id, false, _basePath, _socketPort));

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
     * @return Core::ControlList::InformationIterator iterator for all the controls
     */
    Core::Messaging::ControlList::InformationIterator MessageClient::Enabled()
    {
        _enabledCategories.clear();

        uint16_t bufferSize = sizeof(_writeBuffer);

        for (auto& client : _clients) {
            auto writtenBack = client.second.PushMetadata(0, _writeBuffer, bufferSize);
            if (writtenBack > 0) {
                Core::Messaging::ControlList controlList;
                controlList.Deserialize(_writeBuffer, writtenBack);

                auto it = controlList.Information();
                while (it.Next()) {
                    _enabledCategories.push_back(it.Current());
                }
            }
        }

        return Core::Messaging::ControlList::InformationIterator(_enabledCategories);
    }

    MessageClient::Messages MessageClient::PopMessagesAsList()
    {
        Messages result;
        PopMessagesAndCall([&result](const Core::Messaging::Information& info, const Core::ProxyType<Core::Messaging::IEvent>& message) {
            result.emplace_back(info, message);
        });

        return result;
    }

    /**
     * @brief Pop all messages from all buffers, and for each of them call a passed function, with information about popped message
     *        This method should be called after receiving doorbell ring (after WaitForUpdated function)
     * 
     * @param function function to be called on each of the messages in the buffer
     */
    void MessageClient::PopMessagesAndCall(std::function<void(const Core::Messaging::Information& info, const Core::ProxyType<Core::Messaging::IEvent>& message)> function)
    {
        _adminLock.Lock();
        uint16_t size = sizeof(_readBuffer);

        Core::Messaging::Information information;
        Core::ProxyType<Core::Messaging::IEvent> message;

        for (auto& client : _clients) {
            while (client.second.PopData(size, _readBuffer) != Core::ERROR_READ_ERROR) {
                auto length = information.Deserialize(_readBuffer, size);

                if (length > sizeof(Core::Messaging::MetaData::MessageType) && length < sizeof(_readBuffer)) {
                    auto factory = _factories.find(information.MessageMetaData().Type());
                    if (factory != _factories.end()) {
                        message = factory->second->Create();
                        message->Deserialize(_readBuffer + length, size - length);
                        function(information, message);
                    }
                }
                else {
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
