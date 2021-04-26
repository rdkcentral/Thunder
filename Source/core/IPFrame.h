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
#include "Portability.h"
#include "NodeId.h"

#ifdef __WINDOWS__
#include <net/ipheaders.h>
#else
#include<netinet/ip.h>
#include<netinet/udp.h>
#include<netinet/tcp.h>
#endif

namespace WPEFramework {

namespace Core {

    static constexpr uint16_t EthernetFrameSize = 14;

    template <uint16_t SIZE>
    class EthernetFrameType {
    public:
        EthernetFrameType (const EthernetFrameType<SIZE>&) = delete;
        EthernetFrameType<SIZE>& operator=(const EthernetFrameType<SIZE>&) = delete;

        EthernetFrameType() {
            memset(&(_buffer[0]), 0xFF, 12);
	    _buffer[12] = 0x08;
	    _buffer[13] = 0x00;
        }
        ~EthernetFrameType() = default;

    public:
        static constexpr uint16_t MACSize = 6;
        static constexpr uint16_t FrameSize = SIZE;
        static constexpr uint16_t HeaderSize = EthernetFrameSize;

        uint16_t Load(const uint8_t buffer[], const uint16_t size) {
             uint16_t copySize = std::min(size, static_cast<uint16_t>(sizeof(_buffer)));
             ::memcpy(_buffer, buffer, copySize);
             return (copySize);
        }
        const uint8_t* SourceMAC() const {
            return (&(_buffer[MACSize]));
        }
        void SourceMAC(const uint8_t MACAddress[]) {
            memcpy(&(_buffer[MACSize]), MACAddress, MACSize);
        }
        const uint8_t* DestinationMAC() const {
            return (&(_buffer[MACSize]));
        }
        void DestinationMAC(const uint8_t MACAddress[]) {
            memcpy(&(_buffer[0]), &MACAddress, MACSize);
        }

        uint8_t* Frame() {
            return (SIZE > 0 ? &(_buffer[HeaderSize]) : nullptr);
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(_buffer[HeaderSize]) : nullptr);
        }
        const uint8_t* Data() const {
            return _buffer;
        }
     
    private:
        uint8_t _buffer[SIZE + HeaderSize];
    };

    static constexpr uint16_t IPv4FrameSize = sizeof(iphdr);

    template <uint8_t PROTOCOL, uint16_t SIZE = 0>
    class IPv4FrameType : public EthernetFrameType<SIZE + sizeof(iphdr)> {
    private:
        static constexpr uint8_t IPV4_VERSION = 4;

    public:
        using Base = EthernetFrameType<SIZE + sizeof(iphdr)>;
        static constexpr uint16_t FrameSize = SIZE;
        static constexpr uint16_t HeaderSize = IPv4FrameSize;

        IPv4FrameType(const IPv4FrameType<PROTOCOL,SIZE>&) = delete;
        IPv4FrameType<PROTOCOL,SIZE>& operator=(const IPv4FrameType<PROTOCOL,SIZE>&) = delete;

        IPv4FrameType() : Base() {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(Base::Frame());

            ::memset(ipHeader, 0, HeaderSize);
            ipHeader->version = IPV4_VERSION;
            ipHeader->id =  htons(0xBEEF); // 37540;
            ipHeader->ihl = 5; // Standard IP header length (for IPV4 16 bits elements)
            ipHeader->ttl = 64; // Standard TTL
            ipHeader->protocol = PROTOCOL;
            ipHeader->tot_len = htons(HeaderSize);
            ipHeader->check = Checksum();
        }
        IPv4FrameType(const NodeId& source, const NodeId& destination) : IPv4FrameType() {
            Source(source);
            Destination(destination);
        }
        ~IPv4FrameType() = default;

    public:
       bool IsValid() const {
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(Base::Frame());
            return ((ipHeader->protocol == PROTOCOL) && (Checksum() == ipHeader->check));
        }
        uint16_t Load(const uint8_t buffer[], const uint16_t size) {
             uint16_t copySize = std::min(size, static_cast<uint16_t>(SIZE + HeaderSize));
             ::memcpy(Base::Frame(), buffer, copySize);
             return (copySize);
        }
        uint8_t Protocol() const {
            return (reinterpret_cast<const iphdr*>(Base::Frame())->protocol);
        }
        inline NodeId Source() const {
            NodeId result;
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(Base::Frame());
            ASSERT (ipHeader->version == IPV4_VERSION);

            sockaddr_in node;
            ::memset (&node, 0, sizeof(node));
            node.sin_family = AF_INET;
            node.sin_port = 0;
            node.sin_addr.s_addr = ipHeader->saddr;
            result = node;

            return (result);
        }
        inline void Source(const NodeId& node) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(Base::Frame());
            ASSERT (ipHeader->version == IPV4_VERSION);
            ASSERT (node.Type() == NodeId::TYPE_IPV4);

            const sockaddr_in& result = static_cast<const NodeId::SocketInfo&>(node).IPV4Socket;
            ipHeader->saddr = result.sin_addr.s_addr;
            ipHeader->check = Checksum();
        }
        inline NodeId Destination() const {
            NodeId result;
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(Base::Frame());
            ASSERT (ipHeader->version == IPV4_VERSION);

            sockaddr_in node;
            ::memset (&node, 0, sizeof(node));
            node.sin_family = AF_INET;
            node.sin_port = 0;
            node.sin_addr.s_addr = ipHeader->daddr;
            result = node;

            return (result);
        }
        inline void Destination(const NodeId& node) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(Base::Frame());
            ASSERT (ipHeader->version == IPV4_VERSION);
            ASSERT (node.Type() == NodeId::TYPE_IPV4);

            const sockaddr_in& result = static_cast<const NodeId::SocketInfo&>(node).IPV4Socket;
            ipHeader->daddr = result.sin_addr.s_addr;
            ipHeader->check = Checksum();
        }
        inline uint8_t TTL() const {
            return (reinterpret_cast<const iphdr*>(Base::Frame())->ttl);
        }
        inline void TTL(const uint8_t ttl) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(Base::Frame());
            ipHeader->ttl   = ttl;
            ipHeader->check = Checksum();
        }
        inline uint16_t Length() const {
            return ntohs(reinterpret_cast<const iphdr*>(Base::Frame())->tot_len) - HeaderSize;
        }
        inline void Length(const uint16_t length) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(Base::Frame());
            ipHeader->tot_len = ntohs(length + HeaderSize);
            ipHeader->check = Checksum();
        }
        inline uint8_t Version() const {
            return (reinterpret_cast<const iphdr*>(Base::Frame())->version);
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);
        }
        inline uint16_t Size() const {
            return ntohs(reinterpret_cast<const iphdr*>(Base::Frame())->tot_len) + Base::HeaderSize;
        }

    protected:
        uint32_t Checksum(const uint32_t startValue, const uint16_t data[], const uint16_t lengthInBytes) const {

            // src: https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
            uint32_t  sum = startValue;

            uint16_t count = lengthInBytes;
            while (count > 1) {
                sum += *data++;
                count -= 2;
            }

            if(count > 0) {
                sum += ((*data)&htons(0xFF00));
            }

            return (sum);
        }
        uint16_t Shrink(const uint32_t value) const {

            uint32_t sum = value;

            if (sum > 0xFFFF) {
                sum = (sum & 0xFFFF) + (sum >> 16);
            }

            sum = ~sum;

            return (static_cast<uint16_t>(sum));
        }

    private:
        uint16_t Checksum() const {

            iphdr*    ipHeader = const_cast<iphdr*>(reinterpret_cast<const iphdr*>(Base::Frame()));
            uint16_t  original = ipHeader->check;
            ipHeader->check = 0;
            uint32_t result = Checksum(0, reinterpret_cast<const uint16_t*>(ipHeader), HeaderSize);
            ipHeader->check = original;
            return (Shrink(result));
        }

    };

    static constexpr uint16_t TCPv4FrameSize = sizeof(tcphdr);

    template <uint16_t SIZE = 0, uint8_t PROTOCOL = IPPROTO_TCP>
    class TCPv4FrameType : public IPv4FrameType<PROTOCOL, SIZE + sizeof(tcphdr)> {
    public:
        using Base = IPv4FrameType<PROTOCOL, SIZE + sizeof(tcphdr)>;
        static constexpr uint16_t FrameSize = SIZE;
        static constexpr uint16_t HeaderSize = TCPv4FrameSize;

        TCPv4FrameType(const TCPv4FrameType<SIZE>&) = delete;
        TCPv4FrameType<SIZE>& operator=(const TCPv4FrameType<SIZE>&) = delete;

        TCPv4FrameType() : Base() {
            tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());

            ::memset(tcpHeader, 0, HeaderSize);

            Base::Length(HeaderSize);
        }
        TCPv4FrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());

            tcpHeader->source = htons(source.PortNumber());
            tcpHeader->dest = htons(destination.PortNumber());

            Base::Length(HeaderSize);
        }
        ~TCPv4FrameType() = default;

    public:
        uint16_t Load(const uint8_t buffer[], const uint16_t size) {
             uint16_t copySize = std::min(size, static_cast<uint16_t>(SIZE + HeaderSize));
             ::memcpy(Base::Frame(), buffer, copySize);
             return (copySize);
        }
        inline NodeId Source() const {
            const tcphdr* tcpHeader = reinterpret_cast<const tcphdr*>(Base::Frame());
            NodeId result (Base::Source());
            return (result.IsValid() ? NodeId(result, ntohs(tcpHeader->source)) : result);
        }
        inline void Source(const NodeId& node) {
            if (node.IsValid()) {
                Base::Source(node);
                tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());
                tcpHeader->source = htons(node.PortNumber());
            }
        }
        inline NodeId Destination() const {
            const tcphdr* tcpHeader = reinterpret_cast<const tcphdr*>(Base::Frame());
            NodeId result(Base::Destination());
            return (result.IsValid() ? NodeId(result, ntohs(tcpHeader->dest)) : result);
        }
        inline void Destination(const NodeId& node) {
            if (node.IsValid()) {
                Base::Destination(node);
                tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());
                tcpHeader->dest = htons(node.PortNumber());
            }
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);   
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);   
        }
    };
	
    static constexpr uint16_t UDPv4FrameSize = sizeof(udphdr);

    template <uint16_t SIZE = 0, uint8_t PROTOCOL = IPPROTO_UDP>
    class UDPv4FrameType : public IPv4FrameType<PROTOCOL, SIZE + sizeof(udphdr)> {
    public:
        using Base = IPv4FrameType<PROTOCOL, SIZE + sizeof(udphdr)>;
        static constexpr uint16_t FrameSize = SIZE;
        static constexpr uint16_t HeaderSize = UDPv4FrameSize;

        UDPv4FrameType(const UDPv4FrameType<SIZE>&) = delete;
        UDPv4FrameType<SIZE>& operator=(const UDPv4FrameType<SIZE>&) = delete;

        UDPv4FrameType() : Base() {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());

            ::memset(udpHeader, 0, HeaderSize);

            Base::Length(HeaderSize);

            udpHeader->check = Checksum();
        }
        UDPv4FrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            ::memset(udpHeader, 0, HeaderSize);

            udpHeader->source = htons(source.PortNumber());
            udpHeader->dest = htons(destination.PortNumber());

            Base::Length(HeaderSize);

            udpHeader->check = Checksum();
        }
        ~UDPv4FrameType() = default;

    public:
        bool IsValid() const {
                bool result = false;
                if (Base::IsValid()) {
                   result = true;
                   uint16_t csum = reinterpret_cast<const udphdr*>(Base::Frame())->check;
                   //As per RFC768, If Checksum is transmitted as 0, then the transmitter generated no checksum and need not be verified.
                   if(csum != 0) {
                       result = (Checksum() == csum);
                   }
                }
                return result;
        }
        uint16_t Load(const uint8_t buffer[], const uint16_t size) {
             uint16_t copySize = std::min(size, static_cast<uint16_t>(SIZE + HeaderSize));
             ::memcpy(Base::Frame(), buffer, copySize);
             return (copySize);
        }
        inline NodeId Source() const {
            const udphdr* udpHeader = reinterpret_cast<const udphdr*>(Base::Frame());
            NodeId result(Base::Source());
            return (result.IsValid() ? NodeId(result, ntohs(udpHeader->source)) : result);
        }
        inline void Source(const NodeId& node) {
            if (node.IsValid()) {
                Base::Source(node);
                udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
                udpHeader->source = htons(node.PortNumber());
            }
        }
        inline NodeId Destination() const {
            const udphdr* udpHeader = reinterpret_cast<const udphdr*>(Base::Frame());
            NodeId result(Base::Destination());
            return (result.IsValid() ? NodeId(result, ntohs(udpHeader->dest)) : result);
        }
        inline void Destination(const NodeId& node) {
            if (node.IsValid()) {
                Base::Destination(node);
                udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
                udpHeader->dest = htons(node.PortNumber());
            }
        }
        inline uint16_t SourcePort() const {
            return (ntohs(reinterpret_cast<const udphdr*>(Base::Frame())->source));
        }
        inline void SourcePort(const uint16_t port) {
            reinterpret_cast<udphdr*>(Base::Frame())->source = ntohs(port);
        }
        inline uint16_t DestinationPort() const {
            return (ntohs(reinterpret_cast<const udphdr*>(Base::Frame())->dest));
        }
        inline void DestinationPort(const uint16_t port) {
            reinterpret_cast<udphdr*>(Base::Frame())->dest = ntohs(port);
        }
        void Length(const uint16_t length) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            udpHeader->len = htons(HeaderSize + length);
            Base::Length(HeaderSize + length);
            udpHeader->check = Checksum();
        }
        uint16_t Length() const {
            return (Base::Length() - HeaderSize);
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);
        }

    private:
        uint16_t Checksum() const {
            uint16_t length      = Base::Length();
            uint32_t source      = ntohl(static_cast<const NodeId::SocketInfo&>(Base::Source()).IPV4Socket.sin_addr.s_addr);
            uint32_t destination = ntohl(static_cast<const NodeId::SocketInfo&>(Base::Destination()).IPV4Socket.sin_addr.s_addr);

            uint8_t pseudoHeader[12];
            pseudoHeader[0] = (source >> 24) & 0xFF;
            pseudoHeader[1] = (source >> 16) & 0xFF;
            pseudoHeader[2] = (source >> 8)  & 0xFF;
            pseudoHeader[3] =  source        & 0xFF;
            pseudoHeader[4] = (destination >> 24) & 0xFF;
            pseudoHeader[5] = (destination >> 16) & 0xFF;
            pseudoHeader[6] = (destination >> 8)  & 0xFF;
            pseudoHeader[7] =  destination        & 0xFF;
            pseudoHeader[8] = 0x00;
            pseudoHeader[9] = Base::Protocol();
            pseudoHeader[10] = (length >> 8);
            pseudoHeader[11] = (length & 0xFF);

            udphdr* udpHeader = const_cast<udphdr*>(reinterpret_cast<const udphdr*>(Base::Frame()));
            uint16_t  original = udpHeader->check;
            udpHeader->check = 0;
            uint32_t result = Base::Checksum(0, reinterpret_cast<const uint16_t*>(pseudoHeader), sizeof(pseudoHeader));
            result = Base::Checksum (result, reinterpret_cast<const uint16_t*>(udpHeader), Base::Length());
            udpHeader->check = original;

            return (Base::Shrink(result));
        }
    };

} } // namespace WPEFramework::Core
