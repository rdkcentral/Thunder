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
        MessageClient(const string& identifer, const string& basePath, const uint16_t socketPort = 0);

        void AddInstance(uint32_t id);
        void RemoveInstance(uint32_t id);
        void ClearInstances();

        void WaitForUpdates(const uint32_t waitTime);
        void SkipWaiting();

        void Enable(const Core::Messaging::MetaData& metaData, const bool enable);
        Core::Messaging::ControlList::InformationIterator Enabled();

        Messages PopMessagesAsList();
        void PopMessagesAndCall(std::function<void(const Core::Messaging::Information& info, const Core::ProxyType<Core::Messaging::IEvent>& message)> function);

        void AddFactory(Core::Messaging::MetaData::MessageType type, Core::Messaging::IEventFactory* factory);
        void RemoveFactory(Core::Messaging::MetaData::MessageType type);

    private:
        using Clients = std::unordered_map<uint32_t, Core::Messaging::MessageUnit::MessageDispatcher>;
        mutable Core::CriticalSection _adminLock;
        const string _identifier;
        const string _basePath;
        const uint16_t _socketPort;

        uint8_t _readBuffer[Core::Messaging::MessageUnit::DataSize];
        uint8_t _writeBuffer[Core::Messaging::MessageUnit::MetaDataSize];

        Clients _clients;
        Factories _factories;
        Core::Messaging::ControlList::InformationStorage _enabledCategories;
    };
}
}
