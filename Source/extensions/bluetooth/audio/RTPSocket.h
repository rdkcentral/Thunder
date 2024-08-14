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
#include "IAudioCodec.h"

namespace Thunder {

namespace Bluetooth {

namespace RTP {

    class EXTERNAL MediaPacket {
    public:
        MediaPacket(const uint8_t payloadType, const uint32_t synchronisationSource,
                    const uint16_t sequence, const uint32_t timestamp)
            : _packet(nullptr)
            , _packetLength(0)
            , _sequence(sequence)
            , _timestamp(timestamp)
            , _synchronisationSource(synchronisationSource)
            , _payload(nullptr)
            , _payloadLength(0)
            , _payloadType(payloadType)
        {
        }
        MediaPacket(const uint8_t packet[], const uint16_t packetLength)
            : _packet(packet)
            , _packetLength(packetLength)
            , _sequence(0)
            , _timestamp(0)
            , _synchronisationSource(0)
            , _payload(nullptr)
            , _payloadLength(0)
            , _payloadType(0)
        {
            ASSERT(packet != nullptr);

            if (packetLength >= 12) {
                DataRecordBE pkt(packet, packetLength);

                uint8_t octet;
                pkt.Pop(octet);

                uint8_t csrcCount = (octet & 0x1F);
                const bool extension = (octet & 16);
                const bool padding = (octet & 32);
                const uint8_t version = (octet >> 6);

                if (version == 2) {
                    pkt.Pop(octet);
                    _payloadType = (octet & 0x7F);
                    _marker = !!(octet & 0x80);

                    pkt.Pop(_sequence);
                    pkt.Pop(_timestamp);
                    pkt.Pop(_synchronisationSource); // ssrc

                    // Ignore contributing source IDs...
                    while (csrcCount--) {
                        pkt.Skip(sizeof(uint32_t)); // csrc
                    }

                    // Ignore the application-specific extension.
                    if (extension == true) {
                        TRACE_L1("Have extension in RTP packet!");
                        pkt.Skip(sizeof(uint32_t)); // extension id

                        uint32_t extensionLength;
                        pkt.Pop(extensionLength);
                        pkt.Skip(extensionLength);
                    }

                    // Any padding to remove?
                    const uint8_t paddingLength = (padding? _packet[_packetLength - 1] : 0);

                    // Isolate the payload data
                    _payload = (_packet + pkt.Position());
                    _payloadLength = (_packetLength - pkt.Position() - paddingLength);
                }
                else {
                    TRACE_L1("Unsupported RTP packet version!");
                }
            }
            else {
                TRACE_L1("Invalid RTP packet!");
            }
        }

    public:
        uint16_t Pack(const A2DP::IAudioCodec& codec, const uint8_t payload[], const uint16_t payloadLength,
                      uint8_t buffer[], const uint16_t bufferLength)
        {
            ASSERT(bufferLength >= 12);
            ASSERT(buffer != nullptr);

            DataRecordBE pkt(buffer, bufferLength, 0);

            pkt.Push(static_cast<uint8_t>(2 << 6));
            pkt.Push(_payloadType);
            pkt.Push(_sequence);
            pkt.Push(_timestamp);
            pkt.Push(_synchronisationSource);

            const uint16_t headerLength = pkt.Length();
            ASSERT(headerLength == 12);

            uint16_t length = (bufferLength - headerLength);
            const uint16_t consumed = codec.Encode(payloadLength, payload, length, (buffer + headerLength));

            _packet = buffer;
            _payload = (_packet + headerLength);
            _packetLength = (headerLength + length);
            _payloadLength = length;

            return (consumed);
        }
        uint16_t Unpack(const A2DP::IAudioCodec& codec, uint8_t buffer[], uint16_t& length)
        {
            ASSERT(buffer != nullptr);
            ASSERT(Payload() != nullptr);
            ASSERT(length != 0);

            return (codec.Decode(PayloadLength(), Payload(), length, buffer));
        }

    public:
        bool IsValid() const {
            return ((_payloadType != 0) && (_payloadLength != 0));
        }
        uint16_t Sequence() const {
            return (_sequence);
        }
        uint32_t Timestamp() const {
            return (_timestamp);
        }
        uint8_t Type() const {
            return (_payloadType);
        }
        uint32_t SyncSource() const {
            return (_synchronisationSource);
        }
        const uint8_t* Data() const {
            return (_packet);
        }
        const uint16_t Length() const {
            return (_packetLength);
        }
        const uint8_t* Payload() const {
            return (_payload);
        }
        const uint16_t PayloadLength() const {
            return (_payloadLength);
        }

    private:
        const uint8_t* _packet;
        uint16_t _packetLength;
        uint16_t _sequence;
        uint32_t _timestamp;
        uint32_t _synchronisationSource;
        const uint8_t* _payload;
        uint16_t _payloadLength;
        uint8_t _payloadType;
        bool _marker;
    };

    template<uint8_t TYPE, uint16_t SIZE = 2048>
    class EXTERNAL OutboundMediaPacketType : public MediaPacket, public Core::IOutbound {
    public:
        static_assert(SIZE >= 48, "Too small packet buffer");

        OutboundMediaPacketType(const OutboundMediaPacketType&) = delete;
        OutboundMediaPacketType& operator=(const OutboundMediaPacketType&) = delete;
        ~OutboundMediaPacketType() = default;

        OutboundMediaPacketType(const uint16_t MTU, const uint32_t synchronisationSource, uint16_t sequence, const uint32_t timestamp)
            : MediaPacket(TYPE, synchronisationSource, sequence, timestamp)
            , _buffer()
            , _offset(0)
            , _mtu(MTU)
        {
            ASSERT(MTU <= sizeof(_buffer));
        }

    public:
        uint16_t Ingest(const A2DP::IAudioCodec& codec, const uint8_t data[], const uint16_t dataLength)
        {
            ASSERT(data != nullptr);

            return (Pack(codec, data, dataLength, _buffer, _mtu));
        }

    private:
        void Reload() const override
        {
            _offset = 0;
        }
        uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
        {
            ASSERT(stream != nullptr);

            uint16_t result = std::min<uint16_t>((Length() - _offset), length);
            if (result > 0) {
                ::memcpy(stream, (Data() + _offset), result);
                _offset += result;

                // CMD_DUMP("RTP send", stream, result);
            }

            return (result);
        }

    private:
        uint8_t _buffer[SIZE];
        mutable uint16_t _offset;
        uint16_t _mtu;
    }; // class OutboundMediaPacketType

    class EXTERNAL ClientSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        static constexpr uint32_t CommunicationTimeout = 500;

    public:
        ClientSocket(const ClientSocket&) = delete;
        ClientSocket& operator=(const ClientSocket&) = delete;
        ~ClientSocket() = default;

        ClientSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, 2048, 2048)
            , _outputMTU(0)
        {
        }

        uint16_t OutputMTU() const {
            return (_outputMTU);
        }

    public:
        virtual void Operational(const bool upAndRunning) = 0;

    private:
        void StateChange() override
        {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

            if (IsOpen() == true) {
                struct l2cap_options options{};
                socklen_t len = sizeof(options);

                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_OPTIONS, &options, &len);

                ASSERT(options.omtu <= SendBufferSize());
                ASSERT(options.imtu <= ReceiveBufferSize());

                _outputMTU = options.omtu;

                TRACE(Trace::Information, (_T("Transport channel input MTU: %d, output MTU: %d"), options.imtu, options.omtu));

                Operational(true);
            } else {
                Operational(false);
            }
        }

    private:
        uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t availableData) override
        {
            // Do not expect anything inbound on this channel...

            uint32_t result = 0;

            if (availableData != 0) {
                TRACE_L1("Unexpected RTP data received [%d bytes]", availableData);

                CMD_DUMP("RTP received unexpected", dataFrame, availableData);
            }

            return (result);
        }

    private:
        uint16_t _outputMTU;
    }; // class ClientSocket

} // namespace RTP

} // namespace Bluetooth

}
