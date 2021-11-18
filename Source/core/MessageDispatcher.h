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

    template <uint16_t METADATA_SIZE, uint16_t DATA_SIZE>
    class EXTERNAL MessageDispatcher {
    private:
        using MetaDataCallback = std::function<void(const uint8_t, const uint16_t, const uint8_t*)>;

        //Private classes
        class Packet {
        public:
            /**
            * @brief Construct a new Message Dispatcher:: Packet:: Packet object
            * 
            * @param type type of message
            * @param length length of value buffer
            * @param value buffer
            */
            Packet(const uint8_t type, const uint16_t length, const uint8_t* value)
                : _type(type)
            {
                _buffer.resize(length);
                std::copy(value, value + length, _buffer.begin());
            }

            /**
            * @brief Construct a new Message Dispatcher:: Packet:: Packet object.
            *        Passed buffer  will be deserialized and written to member variables.
            *        Serialized buffer should be following pattern as specified in @ref Serialize.
            * 
            * @param fullLength buffer length
            * @param buffer buffer (serialized)
            */
            Packet(const uint16_t fullLength, const uint8_t* buffer)
            {
                //create a buffer to store deserialized value (reusing _buffer here)
                _buffer.resize(fullLength);
                uint16_t actualLength = 0;

                //fill in type, length and value
                Deserialize(buffer, _type, actualLength, _buffer.data());

                //after _buffer is filled with value, resize it to the size of actualLength which is the size of deserialized value
                _buffer.resize(actualLength);
                _buffer.shrink_to_fit();
            }
            /**
            * @brief Write member variables into buffer. Pattern is:
            *       - uint16_t - full length of message
            *       - uint8_t - type of message
            *       - uint8_t* - buffer
            * 
            * @return std::vector<uint8_t> serialized buffer
            */
            std::vector<uint8_t> Serialize()
            {
                std::vector<uint8_t> result;
                uint16_t bufferLength = _buffer.size();
                uint16_t fullLength = sizeof(bufferLength) + sizeof(_type) + _buffer.size();
                result.resize(fullLength);

                uint32_t offset = 0;
                memcpy(result.data(), &fullLength, sizeof(fullLength));
                offset += sizeof(fullLength);

                memcpy(result.data() + offset, &_type, sizeof(_type));
                offset += sizeof(_type);

                memcpy(result.data() + offset, _buffer.data(), bufferLength);
                offset += bufferLength;

                //should be fast enough due to NRVO
                return result;
            }

            /**
            * @brief Deserialize buffer and get its contents. Serialized buffer should be following pattern as specified in @ref Serialize.
            * 
            * @param buffer serialized buffer
            * @param outType type of message
            * @param outLength length of message
            * @param outValue message buffer
            */
            static void Deserialize(const uint8_t* buffer, uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
            {
                uint32_t offset = 0;

                ::memcpy(&outLength, &(buffer[offset]), sizeof(outLength));
                offset += sizeof(outLength);

                ::memcpy(&outType, &(buffer[offset]), sizeof(_type));
                offset += sizeof(_type);

                outLength -= offset; //fullLength - ( length of type + length of message)
                ::memcpy(outValue, &(buffer[offset]), outLength);
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

        class DataBuffer : public Core::CyclicBuffer {
        public:
            DataBuffer(const string& doorBell, const string& fileName, const uint32_t mode, const uint32_t bufferSize, const bool overwrite)
                : CyclicBuffer(fileName, mode, bufferSize, overwrite)
                , _doorBell(doorBell.c_str())
            {
            }
            ~DataBuffer() override = default;
            DataBuffer(DataBuffer&&) = default;

            DataBuffer() = delete;
            DataBuffer(const DataBuffer&) = delete;
            DataBuffer& operator=(const DataBuffer&) = delete;

            /**
            * @brief Signal that data is available
            * 
            */
            void Ring()
            {
                _doorBell.Ring();
            }

            /**
            * @brief Wait for the doorbell and acknowledge if rang in given time
            * 
            * @param waitTime how much should we wait for the doorbell
            * @return uint32_t ERROR_UNAVAILABLE: doorbell is not connected to its counterpart
            *                  ERROR_TIMEDOUT: ring not rang in given time
            *                  ERROR_NONE: OK
            */
            uint32_t Wait(const uint32_t waitTime)
            {
                auto result = _doorBell.Wait(waitTime);
                if (result != Core::ERROR_TIMEDOUT) {
                    _doorBell.Acknowledge();
                }
                return result;
            }
            void Relinquish()
            {
                return (_doorBell.Relinquish());
            }
            uint32_t GetOverwriteSize(Cursor& cursor) override
            {
                while (cursor.Offset() < cursor.Size()) {
                    uint16_t chunkSize = 0;
                    cursor.Peek(chunkSize);

                    TRACE_L1("Flushing buffer data !!! %d", __LINE__);

                    cursor.Forward(chunkSize);
                }

                return cursor.Offset();
            }
            uint32_t GetReadSize(Cursor& cursor) override
            {
                // Just read one entry.
                uint16_t entrySize = 0;
                cursor.Peek(entrySize);
                return entrySize;
            }

        private:
            Core::DoorBell _doorBell;
        };

        template <uint16_t SIZE>
        class MetaDataBuffer : public Core::IPCChannelClientType<Core::Void, true, true> {
        private:
            using BaseClass = Core::IPCChannelClientType<Core::Void, true, true>;

            class MetaDataFrameHandler : public Core::IIPCServer {
            public:
                MetaDataFrameHandler(MetaDataBuffer* parent)
                    : _parent(*parent)
                {
                }
                ~MetaDataFrameHandler() override = default;

                MetaDataFrameHandler(const MetaDataFrameHandler&) = delete;
                MetaDataFrameHandler& operator=(const MetaDataFrameHandler&) = delete;

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
            using MetaDataFrame = Core::IPCMessageType<1, Core::IPC::BufferType<SIZE>, Core::IPC::ScalarType<uint32_t>>;

            MetaDataBuffer() = default;
            MetaDataBuffer(const std::string& binding)
                : BaseClass(Core::NodeId(binding.c_str()), SIZE)
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

            MetaDataBuffer(const MetaDataBuffer&) = delete;
            MetaDataBuffer& operator=(const MetaDataBuffer&) = delete;

            void RegisterMetaDataCallback(MetaDataCallback notification)
            {
                _notification = notification;
            }
            void UnregisterMetaDataCallback()
            {
                _notification = nullptr;
            }

        private:
            MetaDataCallback _notification;
        };

    public:
        //public methods
        /**
         * @brief Construct a new Message Dispatcher object
         * 
         * @param identifier name of the instance
         * @param instanceId number of the instance
         * @param initialize should dispatcher be initialzied. Should be done only once, on the server side
         * @param baseDirectory where to place all the related files. If initialize=true everything will be deleted in this directory and directory will be recreated
         */
        MessageDispatcher(const string& identifier, const uint32_t instanceId, bool initialize, string baseDirectory = _T("/tmp/MessageDispatcher"))
            : _filenames(PrepareFilenames(initialize, baseDirectory, identifier, instanceId))
            // clang-format off
            , _dataBuffer(_filenames.doorBell, _filenames.data,  Core::File::USER_READ    | 
                                                                             Core::File::USER_WRITE   | 
                                                                             Core::File::USER_EXECUTE | 
                                                                             Core::File::GROUP_READ   |
                                                                             Core::File::GROUP_WRITE  |
                                                                             Core::File::OTHERS_READ  |
                                                                             Core::File::OTHERS_WRITE | 
                                                                             Core::File::SHAREABLE, DATA_SIZE + sizeof(Core::CyclicBuffer::control), true)
            // clang-format on
            , _metaDataBuffer(initialize ? new MetaDataBuffer<METADATA_SIZE>(_filenames.metaData) : nullptr)
        {
            ASSERT(DATA_SIZE > sizeof(Core::CyclicBuffer::control));

            if (!IsValid()) {
                TRACE_L1("MessageDispatcher is not valid!");
            }
        }

        ~MessageDispatcher()
        {
            _dataBuffer.Relinquish();
        }

        MessageDispatcher(const MessageDispatcher&) = delete;
        MessageDispatcher& operator=(const MessageDispatcher&) = delete;

        /**
        * @brief Writes data into cyclic buffer. If it does not fit the data already in the cyclic buffer will be flushed.
        *        After writing everything, this side should call Ring() to notify other side.
        *        To receive this data other side needs to wait for the doorbel ring and then use Reader::Data
        *
        * @param type type of message
        * @param length length of message
        * @param value buffer 
        * @return uint32_t ERROR_WRITE_ERROR: failed to reserve enough space - eg, value size is exceeding max cyclic buffer size
        *                  ERROR_NONE: OK
        */
        uint32_t PushData(const uint8_t type, const uint16_t length, const uint8_t* value)
        {
            _dataLock.Lock();

            ASSERT(length > 0);
            uint32_t result = Core::ERROR_WRITE_ERROR;
            const uint16_t fullLength = sizeof(type) + sizeof(length) + length; // headerLength + informationLength

            const uint16_t reservedLength = _dataBuffer.Reserve(fullLength);

            if (reservedLength >= fullLength) {
                //no need to serialize because we can write to CyclicBuffer step by step
                _dataBuffer.Write(reinterpret_cast<const uint8_t*>(&fullLength), sizeof(fullLength)); //fullLength
                _dataBuffer.Write(reinterpret_cast<const uint8_t*>(&type), sizeof(type)); //type
                _dataBuffer.Write(value, length); //value
                result = Core::ERROR_NONE;

            } else {
                TRACE_L1("Buffer to small to fit message!\n");
            }

            _dataLock.Unlock();

            return result;
        }

        /**
         * @brief Read data after doorbell ringed.
         * 
         * @param type type of message
         * @param length length of message
         * @param value buffer
         * @return uint32_t ERROR_READ_ERROR - unable to read or data is corrupted
         *                  ERROR_NONE - OK
         */
        uint32_t PopData(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
        {
            _dataLock.Lock();

            uint32_t result = Core::ERROR_READ_ERROR;

            bool available = _dataBuffer.IsValid();

            if (available == false) {
                available = _dataBuffer.Validate();
            }

            if (available) {
                uint32_t length = _dataBuffer.Read(_dataReadBuffer, DATA_SIZE);
                if (length < 3) {
                    //did not receive type and length, this is not valid message
                    TRACE_L1("Inconsistent message\n");
                    _dataBuffer.Flush();

                } else {
                    Packet::Deserialize(_dataReadBuffer, outType, outLength, outValue);

                    result = Core::ERROR_NONE;
                }
            }

            _dataLock.Unlock();

            return result;
        }
        void Ring()
        {
            _dataBuffer.Ring();
        }
        uint32_t Wait(const uint32_t waitTime)
        {
            return _dataBuffer.Wait(waitTime);
        }

        /**
         * @brief Writes metadata. Reader needs to register for notifications to recevie this message
         * 
         * @param type type of message
         * @param length length of message
         * @param value vbuffer
         * @return uint32_t ERROR_GENERAL: unable to open communication channel
         *                  ERROR_WRITE_ERROR: unable to write
         *                  ERROR_UNAVAILABLE: message was sent but not reported (missing Register call on the other side)
         *                                     caller should send this message again
         *                  ERROR_NONE: OK
         */
        uint32_t PushMetadata(const uint8_t type, const uint16_t length, const uint8_t* value)
        {
            _metaDataLock.Lock();

            ASSERT(length > 0);

            uint32_t result = Core::ERROR_GENERAL;

            Core::IPCChannelClientType<Core::Void, false, true> channel(Core::NodeId(_filenames.metaData.c_str()), METADATA_SIZE);

            auto metaDataFrame = Core::ProxyType<typename MetaDataBuffer<METADATA_SIZE>::MetaDataFrame>::Create();

            if (channel.Open(Core::infinite) == Core::ERROR_NONE) {
                Packet packet(type, length, value);
                auto serialized = packet.Serialize();
                metaDataFrame->Parameters().Set(serialized.size(), serialized.data());

                if (channel.Invoke(metaDataFrame, Core::infinite) == Core::ERROR_NONE) {
                    result = metaDataFrame->Response();
                } else {
                    result = Core::ERROR_WRITE_ERROR;
                }

                channel.Close(Core::infinite);
            }

            _metaDataLock.Unlock();

            return result;
        }

        void RegisterDataAvailable(MetaDataCallback notification)
        {
            if (_metaDataBuffer->IsOpen()) {
                _metaDataBuffer->RegisterMetaDataCallback(notification);
            }
        }
        void UnregisterDataAvailable()
        {
            _metaDataBuffer->UnregisterMetaDataCallback();
        }

        bool IsValid() const
        {

            bool result = true;

            if (!_dataBuffer.IsValid()) {
                result = false;
            }
            if (_metaDataBuffer != nullptr) {
                if (!_metaDataBuffer->IsOpen()) {
                    result = false;
                }
            }
            return result;
        }

    private:
        struct Filenames {
            string doorBell;
            string metaData;
            string data;
        } _filenames;

        /**
        * @brief Prepare filenames for MessageDispatcher
        * 
        * @param initialize should the base directory be created (if it exists everythiing inside will be deleted)
        * @param baseDirectory where are those filed stored
        * @param identifier identifer of the instance
        * @param instanceId number of instance
        * @return std::tuple<string, string, string> 
        *         0 - doorBellFilename
        *         1 - dataFileName
        *         2 - metaDataFilename
        */
        static Filenames PrepareFilenames(const bool initialize, const string& baseDirectory, const string& identifier, const uint32_t instanceId)
        {
            if (initialize) {
                if (Core::File(baseDirectory).IsDirectory()) {
                    //if directory exists remove it to clear data (eg. sockets) that can remain after previous creation
                    Core::Directory(baseDirectory.c_str()).Destroy(false);
                }

                if (!Core::Directory(baseDirectory.c_str()).CreatePath()) {
                    TRACE_L1(_T("Unable to create MessageDispatcher directory"));
                }
            }

            string doorBellFilename = Core::Format("%s/%s.doorbell", baseDirectory.c_str(), identifier.c_str());
            string dataFilename = Core::Format("%s/%s.%d.data", baseDirectory.c_str(), identifier.c_str(), instanceId);
            string metaDataFilename = Core::Format("%s/%s.%d.metadata", baseDirectory.c_str(), identifier.c_str(), instanceId);

            return { doorBellFilename, dataFilename, metaDataFilename };
        }

        //private variables
        Core::CriticalSection _dataLock;
        Core::CriticalSection _metaDataLock;

        DataBuffer _dataBuffer;
        std::unique_ptr<MetaDataBuffer<METADATA_SIZE>> _metaDataBuffer;

        uint8_t _dataReadBuffer[DATA_SIZE];
    };
}
}
