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
#include "Module.h"

namespace WPEFramework {
namespace Core {

    constexpr uint32_t CyclicBufferSize = ((8 * 1024) - (sizeof(struct Core::CyclicBuffer::control))); /* 8Kb */

    class EXTERNAL MessageDispatcher {
    private:
        //Private classes
        class EXTERNAL MessageBuffer : public Core::CyclicBuffer {
        private:
            MessageBuffer() = delete;
            MessageBuffer(const MessageBuffer&) = delete;
            MessageBuffer& operator=(const MessageBuffer&) = delete;

        public:
            MessageBuffer(const string& doorBell, const string& name);
            ~MessageBuffer() override = default;

            void Ring();
            void Acknowledge();
            uint32_t Wait(const uint32_t waitTime);
            void Relinquish();
            uint32_t GetOverwriteSize(Cursor& cursor) override;

        private:
            void DataAvailable() override;

        private:
            Core::DoorBell _doorBell;
        };

        class Reader {
        public:
            Reader(MessageDispatcher& parent);
            ~Reader() = default;

            Reader(const Reader&) = delete;
            Reader& operator=(const Reader&) = delete;

            //Blocking call
            uint32_t Metadata(uint8_t& outType, uint16_t& outLength, uint8_t* outValue);

            //Non-blocking call
            uint32_t Data(uint8_t& outType, uint16_t& outLength, uint8_t* outValue);

            uint32_t Wait(const uint32_t waitTime);

            bool IsEmpty() const;

        private:
            MessageDispatcher& _parent;
            uint8_t _buffer[CyclicBufferSize];
        };
        class Writer {
        public:
            Writer(MessageDispatcher& parent);
            ~Writer() = default;
            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;

            //Blocking call
            uint32_t Metadata(const uint8_t type, const uint16_t length, const uint8_t* value);

            //Non-blocking call
            uint32_t Data(const uint8_t type, const uint16_t length, const uint8_t* value);

        private:
            MessageDispatcher& _parent;
        };

    public:
        //public methods
        MessageDispatcher();
        ~MessageDispatcher() = default;

        MessageDispatcher(const MessageDispatcher&) = delete;
        MessageDispatcher& operator=(const MessageDispatcher&) = delete;

        uint32_t Open(const string& doorBell, const string& fileName);
        uint32_t Close();
        Reader& GetReader();
        Writer& GetWriter();

    private:
        //private variables
        Core::CriticalSection _lock;
        std::unique_ptr<MessageBuffer> _outputChannel;
        bool _directOutput;

        Reader _reader;
        Writer _writer;
    };
}
}
