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

#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    template <const uint16_t DATA_BUFFER_SIZE, const uint16_t METADATA_SIZE>
    class MessageDataBufferType {
    private:
        /**
        * @brief Metdata Callback. First two arguments are for data in. Two later for data out (responded to the other side).
        *        Third parameter is initially set to maximum length that can be written to the out buffer
        *
        */
        class DataBuffer : public Core::CyclicBuffer {
        public:
            DataBuffer(const string& doorBell, const string& fileName, const uint32_t mode, const uint32_t bufferSize, const bool overwrite)
                : CyclicBuffer(fileName, mode, bufferSize, overwrite)
                , _doorBell(doorBell.c_str())
            {
            }
            ~DataBuffer() override = default;

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
                _doorBell.Relinquish();
            }
            uint32_t GetOverwriteSize(Cursor& cursor) override
            {
                while (cursor.Offset() < cursor.Size()) {
                    uint16_t chunkSize = 0;
                    cursor.Peek(chunkSize);

                    TRACE_L1("Flushing buffer data!");

                    cursor.Forward(chunkSize);
                }

                return cursor.Offset();
            }
            uint32_t GetReadSize(Cursor& cursor) override
            {
                // Just read one entry.
                uint16_t entrySize = 0;
                cursor.Peek(entrySize);
                cursor.Forward(sizeof(entrySize));
                return entrySize > sizeof(entrySize) ? entrySize - sizeof(entrySize) : 0;
            }
            void Destroy() {
                Core::CyclicBuffer::Destroy();
            }

        private:
            Core::DoorBell _doorBell;
        };

    public:
        using MetadataFrame = Core::IPCMessageType<1, Core::IPC::BufferType<METADATA_SIZE>, Core::IPC::BufferType<METADATA_SIZE>>;

        MessageDataBufferType(const MessageDataBufferType&) = delete;
        MessageDataBufferType& operator=(const MessageDataBufferType&) = delete;

        /**
         * @brief Construct a new Message Dispatcher object
         *
         * @param identifier name of the instance
         * @param instanceId number of the instance
         * @param baseDirectory where to place all the necessary files. This directory should exist before creating this class.
         * @param socketPort triggers the use of using a IP socket in stead of a domain socket if the port value is not 0.
         */
        MessageDataBufferType(const string& identifier, const uint32_t instanceId, const string& baseDirectory, const uint16_t socketPort = 0, const bool initialize = false)
            : _filenames(PrepareFilenames(baseDirectory, identifier, instanceId, socketPort))
            , _dataLock()
            // clang-format off
            , _dataBuffer(_filenames.doorBell, _filenames.data,  Core::File::USER_READ    |
                                                                 Core::File::USER_WRITE   |
                                                                 Core::File::USER_EXECUTE |
                                                                 Core::File::GROUP_READ   |
                                                                 Core::File::GROUP_WRITE  |
                                                                 Core::File::OTHERS_READ  |
                                                                 Core::File::OTHERS_WRITE |
                                                                 Core::File::SHAREABLE,
                                                                 (initialize == true ? DATA_BUFFER_SIZE : 0), true)
            // clang-format on
        {
            if (_dataBuffer.IsValid() == false) {
                _dataBuffer.Validate();
            }

            if (_dataBuffer.IsValid() == true) {
                if ( (initialize == false) && (_dataBuffer.Used() > 0) ) {
                    TRACE_L1("%d bytes already in the buffer instance %d", _dataBuffer.Used(), instanceId);
                    _dataBuffer.Ring();
                }
            } else {
                TRACE_L1("MessageDispatcher instance %d is not valid!", instanceId);
            }
        }
        ~MessageDataBufferType() {
            _dataBuffer.Relinquish();
            _dataLock.Lock();
            _dataBuffer.Destroy();
            _dataLock.Unlock();
        }

    public:
        inline const string& Name () const {
            return (_filenames.data);
        }

        /**
        * @brief Writes data into cyclic buffer. After writing everything, this side should call Ring() to notify other side.
        *        To receive this data other side needs to wait for the doorbell ring and then use PopData
        *
        * @param length length of message
        * @param value buffer
        * @return uint32_t ERROR_WRITE_ERROR: failed to reserve enough space - eg, value size is exceeding max cyclic buffer size
        *                  ERROR_NONE: OK
        */
        uint32_t PushData(const uint16_t length, const uint8_t* value)
        {
            uint32_t result = Core::ERROR_WRITE_ERROR;
            const uint16_t fullLength = sizeof(length) + length; // headerLength + informationLength

            ASSERT(length > 0);
            ASSERT(value != nullptr);

            _dataLock.Lock();

            if (_dataBuffer.IsValid() == true) {
                const uint16_t reservedLength = _dataBuffer.Reserve(fullLength);

                if (reservedLength >= fullLength) {
                    //no need to serialize because we can write to CyclicBuffer step by step
                    _dataBuffer.Write(reinterpret_cast<const uint8_t*>(&fullLength), sizeof(fullLength)); //fullLength
                    _dataBuffer.Write(value, length); //value
                    _dataBuffer.Ring();
                    result = Core::ERROR_NONE;
                } else {
                    TRACE_L1("Buffer to small to fit message!");
                }
            }

            _dataLock.Unlock();

            return result;
        }

        /**
         * @brief Read data after doorbell ringed. If buffer is too small to fit whole message it will be partially filled.
         *
         * @param outLength ERROR_NONE - read bytes.
         *                  ERROR_GENERAL - mimimal required bytes to fit whole message.
         *                  ERROR_READ_ERROR - the same value as passed in
         * @param outValue buffer
         * @return uint32_t ERROR_READ_ERROR - no data or data is corrupted
         *                  ERROR_NONE - OK
         *                  ERROR_GENERAL - buffer too small to fit whole message at once
         */
        uint32_t PopData(uint16_t& outLength, uint8_t* outValue)
        {
            uint32_t result = Core::ERROR_READ_ERROR;

            ASSERT(outLength != 0);
            ASSERT(outValue != nullptr);

            _dataLock.Lock();

            if (_dataBuffer.IsValid() == true) {
                const uint32_t length = _dataBuffer.Read(outValue, outLength, true);
                if (length > 0) {
                    if (length > outLength) {
                        TRACE_L1("Lost part of the message");
                        result = Core::ERROR_GENERAL;
                    } else {
                        result = Core::ERROR_NONE;
                    }
                }

                outLength = length;
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
        void FlushDataBuffer()
        {
            _dataLock.Lock();
            if (_dataBuffer.IsValid() == true) {
                _dataBuffer.Flush();
            }
            _dataLock.Lock();
        }
        bool IsValid() const
        {
            return (_dataBuffer.IsValid());
        }
        const string& MetadataName() const {
            return (_filenames.metaData);
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
        * @param baseDirectory where are those filed stored. This directory should already exist.
        * @param identifier identifer of the instance
        * @param instanceId number of instance
        * @param socketPort triggers the use of using a IP socket in stead of a domain socket if the port value is not 0.
        * @return std::tuple<string, string, string>
        *         0 - doorBellFilename
        *         1 - dataFileName
        *         2 - metaDataFilename
        */
        static Filenames PrepareFilenames(const string& baseDirectory, const string& identifier, const uint32_t instanceId, const uint16_t socketPort)
        {
            ASSERT(Core::File(baseDirectory).IsDirectory() && "Directory for message files does not exist");

            string doorBellFilename;
            string metaDataFilename;
            string basePath = Core::Directory::Normalize(baseDirectory) + identifier;

            if (socketPort != 0) {
                doorBellFilename = _T("127.0.0.1:") + Core::NumberType<uint16_t>(socketPort).Text();
                metaDataFilename = _T("127.0.0.1:") + Core::NumberType<uint16_t>(socketPort + instanceId + 1).Text();
            }
            else {
                doorBellFilename = basePath + _T(".doorbell");
                metaDataFilename = basePath + '.' + Core::NumberType<uint32_t>(instanceId).Text() + _T(".metadata");
            }

            string dataFilename = basePath + '.' + Core::NumberType<uint32_t>(instanceId).Text() + _T(".data");

            return { doorBellFilename, metaDataFilename, dataFilename };
        }

    private:
        mutable Core::CriticalSection _dataLock;
        DataBuffer _dataBuffer;
    };
}
}
