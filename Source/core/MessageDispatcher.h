/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
#include "CyclicBuffer.h"
#include "DoorBell.h"
#include "IPCChannel.h"
#include "IPCMessage.h"
#include "Module.h"

namespace WPEFramework {
namespace Core {

    class EXTERNAL MessageDispatcher {
    private:
        using MetaDataCallback = std::function<void(const uint8_t, const uint16_t, const uint8_t*)>;

        //Private classes
        class Packet {
        public:
            Packet(const uint8_t type, const uint16_t length, const uint8_t* value);
            Packet(const uint16_t fullLength, const uint8_t* buffer);

            std::vector<uint8_t> Serialize();
            static void Deserialize(const uint8_t* buffer, uint8_t& outType, uint16_t& outLength, uint8_t* outValue);

            uint8_t Type() const;
            uint32_t Length() const;
            const uint8_t* Value() const;

        private:
            uint8_t _type;
            std::vector<uint8_t> _buffer;
        };

        class DataBuffer : public Core::CyclicBuffer {
        public:
            DataBuffer(const string& doorBell, Core::DataElementFile& buffer, const bool initiator, const uint32_t offset, const uint32_t bufferSize, const bool overwrite);
            ~DataBuffer() override = default;
            DataBuffer(DataBuffer&&) = default;

            DataBuffer() = delete;
            DataBuffer(const DataBuffer&) = delete;
            DataBuffer& operator=(const DataBuffer&) = delete;

            void Ring();
            uint32_t Wait(const uint32_t waitTime);
            void Relinquish();
            uint32_t GetOverwriteSize(Cursor& cursor) override;
            uint32_t GetReadSize(Cursor& cursor) override;

        private:
            Core::DoorBell _doorBell;
        };

        class MetaDataBuffer : public Core::IPCChannelClientType<Core::Void, true, true> {
        private:
            using BaseClass = Core::IPCChannelClientType<Core::Void, true, true>;

            class MetaDataFrameHandler : public Core::IIPCServer {
            public:
                MetaDataFrameHandler(MetaDataBuffer* parent);
                ~MetaDataFrameHandler() override = default;

                MetaDataFrameHandler(const MetaDataFrameHandler&) = delete;
                MetaDataFrameHandler& operator=(const MetaDataFrameHandler&) = delete;

            public:
                void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data) override;

            private:
                MetaDataBuffer& _parent;
            };

        public:
            static constexpr uint16_t MetaDataBufferSize = 1 * 1024;
            using MetaDataFrame = Core::IPCMessageType<1, Core::IPC::BufferType<MetaDataBufferSize>, Core::IPC::ScalarType<uint32_t>>;

            MetaDataBuffer() = default;
            MetaDataBuffer(const std::string& binding);
            ~MetaDataBuffer() override;

            MetaDataBuffer(const MetaDataBuffer&) = delete;
            MetaDataBuffer& operator=(const MetaDataBuffer&) = delete;

            void RegisterMetaDataCallback(MetaDataCallback notification);
            void UnregisterMetaDataCallback();

        private:
            MetaDataCallback _notification;
        };

    public:
        //public methods
        static MessageDispatcher Create(const string& identifier, const uint32_t instanceId, const uint32_t dataSize);
        static MessageDispatcher Open(const string& identifier, const uint32_t instanceId);
        ~MessageDispatcher();
        MessageDispatcher(MessageDispatcher&& other);

        MessageDispatcher(const MessageDispatcher&) = delete;
        MessageDispatcher& operator=(const MessageDispatcher&) = delete;

        //data
        uint32_t PushData(const uint8_t type, const uint16_t length, const uint8_t* value);
        uint32_t PopData(uint8_t& outType, uint16_t& outLength, uint8_t* outValue);
        void Ring();
        uint32_t Wait(const uint32_t waitTime);

        //metadata
        uint32_t PushMetadata(const uint8_t type, const uint16_t length, const uint8_t* value);

        void RegisterDataAvailable(MetaDataCallback notification);
        void UnregisterDataAvailable();

        bool IsValid() const;
        uint32_t DataSize() const;
        uint32_t MetaDataSize() const;

    private:
        static std::tuple<string, string, string> PrepareFilenames(const string& baseDirectory, const string& identifier, const uint32_t instanceId);

        MessageDispatcher(const string& doorBellFilename, const string& dataBufferFilename, const string& metaDataFilename, uint32_t dataSize);
        MessageDispatcher(const string& doorBellFilename, Core::DataElementFile&& mappedFile, const string& metaDataFilename, uint32_t dataSize);

    private:
        //private variables
        Core::CriticalSection _dataLock;
        Core::CriticalSection _metaDataLock;

        Core::DataElementFile _mappedFile;

        std::unique_ptr<DataBuffer> _dataBuffer;
        std::unique_ptr<MetaDataBuffer> _metaDataBuffer;

        string _metaDataFilename;

        std::vector<uint8_t> _dataReadBuffer;
    };
}
}
