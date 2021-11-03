/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

namespace Bluetooth {

    class EXTERNAL RTPSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        static constexpr uint32_t CommunicationTimeout = 500;
        static constexpr uint32_t MinMTU = 48;

    public:
        template<uint16_t SIZE, uint8_t TYPE, typename ENCODER, uint8_t VERSION = 2>
        class EXTERNAL MediaPacketType : public Core::IOutbound {
        private:
            struct RTPHeader {
                uint8_t octet0;
                uint8_t octet1;
                uint16_t sequence;
                uint32_t timestamp;
                uint32_t ssrc;
                uint32_t csrc[0];
            } __attribute__((packed));

            static_assert(SIZE >= MinMTU, "Too small packet buffer");

        public:
            MediaPacketType(const MediaPacketType&) = delete;
            MediaPacketType& operator=(const MediaPacketType&) = delete;
            MediaPacketType(const ENCODER& encoder, const uint32_t synchronisationSource, uint16_t sequence, const uint32_t timestamp)
                : _dataSize(sizeof(RTPHeader))
                , _offset(0)
                , _encoder(encoder)
            {
                RTPHeader* header = reinterpret_cast<RTPHeader*>(_buffer);
                header->octet0 = (VERSION << 6);
                header->octet1 = (TYPE & 0x7F);
                header->sequence = htons(sequence);
                header->timestamp = htonl(timestamp);
                header->ssrc = htonl(synchronisationSource);
            }
            ~MediaPacketType() = default;

        public:
            uint16_t Ingest(const uint16_t length, const uint8_t inputBuffer[])
            {
                ASSERT(inputBuffer != nullptr);
                uint16_t produced = (SIZE - _dataSize);
                const uint16_t consumed = _encoder.Encode(length, inputBuffer, produced, (_buffer + _dataSize));
                _dataSize += produced;
                return (consumed);
            }
            uint16_t Length() const
            {
                return (_dataSize);
            }
            uint16_t Capacity() const
            {
                return (SIZE);
            }
            uint8_t Type() const
            {
                return (TYPE);
            }

        private:
            void Reload() const override
            {
                _offset = 0;
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                ASSERT(stream != nullptr);

                uint16_t result = std::min<uint16_t>((_dataSize - _offset), length);
                if (result > 0) {
                    ::memcpy(stream, (_buffer + _offset), result);
                    _offset += result;
                    //CMD_DUMP("RTP send", stream, length);
                }

                return (result);
            }

        private:
            uint8_t _buffer[SIZE];
            uint16_t _dataSize;
            mutable uint16_t _offset;
            const ENCODER& _encoder;
        }; // class MediaPacketType

    public:
        RTPSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, 1024, 1024)
            , _adminLock()
        {
        }
        ~RTPSocket() = default;
        RTPSocket(const RTPSocket&) = delete;
        RTPSocket& operator=(const RTPSocket&) = delete;

    private:
        virtual void Operational(const bool upAndRunning) = 0;

        void StateChange() override
        {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

            if (IsOpen() == true) {
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);

                Operational(true);
            } else {
                Operational(false);
            }
        }
        uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t availableData) override
        {
            uint32_t result = 0;

            if (availableData != 0) {
                TRACE_L1("Unexpected data for deserialization [%d bytes]", availableData);
                CMD_DUMP("RTP received unexpected", dataFrame, availableData);
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        struct l2cap_conninfo _connectionInfo;
    }; // class RTPSocket

} // namespace Bluetooth

}
