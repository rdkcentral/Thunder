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
        //Private classes
        class Packet {
        public:
            Packet(const uint8_t type, const uint16_t length, const uint8_t* value)
                : _type(type)
            {
                _buffer.resize(length);
                std::copy(value, value + length, _buffer.begin());
            }

            Packet(const uint16_t fullLength, const uint8_t* buffer)
            {
                Deserialize(fullLength, buffer);
            }

            //should be fast enough due to NRVO
            std::vector<uint8_t> Serialize()
            {
                std::vector<uint8_t> result;
                uint32_t bufferLength = _buffer.size();
                uint32_t fullLength = sizeof(bufferLength) + sizeof(_type) + _buffer.size();
                result.resize(fullLength);

                uint32_t offset = 0;
                memcpy(result.data(), &fullLength, sizeof(fullLength));
                offset += sizeof(fullLength);

                memcpy(result.data() + offset, &_type, sizeof(_type));
                offset += sizeof(_type);

                memcpy(result.data() + offset, _buffer.data(), bufferLength);
                offset += bufferLength;

                return result;
            }

            void Deserialize(const uint16_t fullLength, const uint8_t* buffer)
            {
                uint32_t offset = 0;
                uint32_t bufferLength = 0;

                ::memcpy(&bufferLength, &(buffer[offset]), sizeof(bufferLength));
                offset += sizeof(bufferLength);

                ASSERT(fullLength == bufferLength);

                ::memcpy(&_type, &(buffer[offset]), sizeof(_type));
                offset += sizeof(_type);

                bufferLength -= offset; //fullLength - ( length of type + length of message)
                _buffer.resize(bufferLength);

                ::memcpy(_buffer.data(), &(buffer[offset]), bufferLength);
            }


            uint8_t Type() const
            {
                return _type;
            }

            uint32_t Length() const
            {
                return _buffer.size();
            }

            const uint8_t* Value() const
            {
                return _buffer.data();
            }

        private:
            uint8_t _type;
            std::vector<uint8_t> _buffer;
        };

        class EXTERNAL DataBuffer : public Core::CyclicBuffer {
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

        class EXTERNAL MetaDataBuffer : public Core::IPCChannelClientType<Core::Void, true, true> {
        public:
            static constexpr uint16_t MetaDataBufferSize = 1 * 1024;
            using MetaDataFrame = Core::IPCMessageType<1, Core::IPC::BufferType<MetaDataBufferSize>, Core::IPC::ScalarType<uint32_t>>;

        private:
            using BaseClass = Core::IPCChannelClientType<Core::Void, true, true>;

            class MetaDataFrameHandler : public Core::IIPCServer {
            public:
                MetaDataFrameHandler(const MetaDataFrameHandler&) = delete;
                MetaDataFrameHandler& operator=(const MetaDataFrameHandler&) = delete;

                MetaDataFrameHandler(MetaDataBuffer* parent)
                    : _parent(*parent)
                {
                }
                ~MetaDataFrameHandler() override = default;

            public:
                void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data) override
                {
                    auto message = Core::ProxyType<MetaDataFrame>(data);

                    auto length = message->Parameters().Length();
                    auto value = message->Parameters().Value();

                    Packet packet(length, value);
                    if (_parent._notification != nullptr) {
                        _parent._notification(packet.Type(), packet.Length(), packet.Value());
                        message->Response() = Core::ERROR_NONE;

                    } else {
                        message->Response() = Core::ERROR_UNAVAILABLE;
                    }

                    source.ReportResponse(data);
                }

            private:
                MetaDataBuffer& _parent;
            };

        public:
            MetaDataBuffer(const std::string& binding)
                : BaseClass(Core::NodeId(binding.c_str()), MetaDataBufferSize)
            {
                CreateFactory<MetaDataFrame>(1);
                Register(MetaDataFrame::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<MetaDataFrameHandler>::Create(this)));
                Open(Core::infinite);
            }

            ~MetaDataBuffer() override
            {
                Close(Core::infinite);
                Unregister(MetaDataFrame::Id());
                DestroyFactory<MetaDataFrame>();
            }
            MetaDataBuffer();
            MetaDataBuffer(const MetaDataBuffer&) = delete;
            MetaDataBuffer& operator=(const MetaDataBuffer&) = delete;

            void RegisterDataAvailable(std::function<void(const uint8_t, const uint16_t, const uint8_t*)> notification)
            {
                _notification = notification;
            }
            void UnregisterDataAvailable()
            {
                _notification = nullptr;
            }

        private:
            std::function<void(const uint8_t, const uint16_t, const uint8_t*)> _notification;
        };

        class Reader {
        public:
            Reader(MessageDispatcher& parent, uint32_t dataBufferSize);
            ~Reader() = default;
            Reader(Reader&& other);

            Reader(const Reader&) = delete;
            Reader& operator=(const Reader&) = delete;

            //Non-blocking call
            uint32_t Data(uint8_t& outType, uint16_t& outLength, uint8_t* outValue);

            uint32_t Wait(const uint32_t waitTime);

            bool IsEmpty() const;

        private:
            MessageDispatcher& _parent;
            std::vector<uint8_t> _dataBuffer;
        };
        class Writer {
        public:
            Writer(MessageDispatcher& parent);
            ~Writer() = default;
            Writer(Writer&& other);

            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;

            //Blocking call
            uint32_t Metadata(const uint8_t type, const uint16_t length, const uint8_t* value);

            //Non-blocking call
            uint32_t Data(const uint8_t type, const uint16_t length, const uint8_t* value);

            void Ring();

        private:
            MessageDispatcher& _parent;
        };

    public:
        //public methods
        static MessageDispatcher Create(const string& identifier, const uint32_t instanceId, const uint32_t dataSize);
        static MessageDispatcher Open(const string& identifier, const uint32_t instanceId);

        ~MessageDispatcher();
        MessageDispatcher(MessageDispatcher&& other)
            : _dataLock()
            , _metaDataLock()
            , _mappedFile(std::move(other._mappedFile))
            , _dataBuffer(std::move(other._dataBuffer))
            , _metaDataBuffer(std::move(other._metaDataBuffer))
            , _reader(std::move(other._reader))
            , _writer(std::move(other._writer))
        {
        }

        MessageDispatcher(const MessageDispatcher&) = delete;
        MessageDispatcher& operator=(const MessageDispatcher&) = delete;

        Reader& GetReader();
        Writer& GetWriter();

        void RegisterDataAvailable(std::function<void(const uint8_t, const uint16_t, const uint8_t*)> notification)
        {
            _metaDataBuffer->RegisterDataAvailable(notification);
        }
        void UnregisterDataAvailable()
        {
            _metaDataBuffer->UnregisterDataAvailable();
        }

    private:
        //private constructors
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

        Reader _reader;
        Writer _writer;
    };
}
}
