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
#include "MessageUnit.h"

namespace WPEFramework {

namespace Messaging {

    /**
     * @brief Class responsible for:
     *        - retreiving data from buffers
     *        - sending metadata to buffers (enabling categories)
     * 
     */
    class EXTERNAL MessageClient {
    public:
        MessageClient() = delete;
        ~MessageClient() = default;
        MessageClient(const MessageClient&) = delete;
        MessageClient& operator=(const MessageClient&) = delete;
        MessageClient(const string& identifer, const string& basePath, const uint16_t socketPort = 0);

    public:
        void AddInstance(const uint32_t id);
        void RemoveInstance(const uint32_t id);
        void ClearInstances();

        void WaitForUpdates(const uint32_t waitTime);
        void SkipWaiting();

        void Enable(const Core::Messaging::Metadata& metadata, const bool enable);
        void Controls(Messaging::MessageUnit::Iterator& controls) const;

        using MessageHandler = std::function<void(const Core::Messaging::Metadata&, const Core::ProxyType<Core::Messaging::IEvent>&)>;
        void PopMessagesAndCall(const MessageHandler& handler);

        void AddFactory(Core::Messaging::Metadata::type type, IEventFactory* factory);
        void RemoveFactory(Core::Messaging::Metadata::type type);

    private:
        template<typename METADATA>
        uint16_t DeserializeAndSendMessage(const uint16_t length, const MessageHandler& handler)
        {
            ASSERT(handler != nullptr);

            METADATA metadata;
            const uint16_t offset = metadata.Deserialize(_readBuffer, length);

            if (offset != 0) {
                auto factory = _factories.find(metadata.Type());

                if (factory != _factories.end()) {
                    Core::ProxyType<Core::Messaging::IEvent> message;

                    message = factory->second->Create();
                    message->Deserialize((_readBuffer + offset), (length - offset));

                    handler(metadata, message);
                }
            }

            return (offset);
        }

    private:
        using Factories = std::unordered_map<Core::Messaging::Metadata::type, IEventFactory*>;
        using Clients = std::map<uint32_t, MessageUnit::Client>;

        mutable Core::CriticalSection _adminLock;
        const string _identifier;
        const string _basePath;
        const uint16_t _socketPort;

        mutable uint8_t _readBuffer[Messaging::MessageUnit::DataSize];
        mutable uint8_t _writeBuffer[Messaging::MessageUnit::MetadataSize];

        Clients _clients;
        Factories _factories;
    };

} // namespace Messaging
}
