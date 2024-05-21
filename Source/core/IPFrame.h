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
using ip = iphdr;
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
        static constexpr uint8_t  MACSize = 6;
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
            return (&(_buffer[0]));
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
        bool IsBroadcastMAC(const uint8_t MAC[MACSize]) const {
            return ( (MAC[0] == 0xFF) && (MAC[1] == 0xFF) && (MAC[2] == 0xFF) && (MAC[3] == 0xFF) && (MAC[4] == 0xFF) && (MAC[5] == 0xFF) );
        }
     
    private:
        uint8_t _buffer[SIZE + HeaderSize];
    };

    static constexpr uint16_t IPv4FrameSize = sizeof(ip);

    template <uint8_t PROTOCOL, uint16_t SIZE = 0>
    class IPv4FrameType : public EthernetFrameType<SIZE + sizeof(ip)> {
    private:
        static constexpr uint8_t IPV4_VERSION = 4;

    public:
        using Base = EthernetFrameType<SIZE + sizeof(ip)>;
        static constexpr uint16_t FrameSize = SIZE;
        static constexpr uint16_t HeaderSize = IPv4FrameSize;

        IPv4FrameType(const IPv4FrameType<PROTOCOL,SIZE>&) = delete;
        IPv4FrameType<PROTOCOL,SIZE>& operator=(const IPv4FrameType<PROTOCOL,SIZE>&) = delete;

        IPv4FrameType() : Base() {
            ip* ipHeader = reinterpret_cast<ip*>(Base::Frame());

            ::memset(ipHeader, 0, HeaderSize);
            #ifdef __WINDOWS__
            ipHeader->version = IPV4_VERSION;
            ipHeader->id =  htons(0xBEEF); // 37540;
            ipHeader->ihl = 5; // Standard IP header length (for IPV4 16 bits elements)
            ipHeader->ttl = 64; // Standard TTL
            ipHeader->protocol = PROTOCOL;
            ipHeader->tot_len = htons(HeaderSize);
            ipHeader->check = Checksum();
            #else
            ipHeader->ip_v = IPV4_VERSION;
            ipHeader->ip_id =  htons(0xBEEF); // 37540;
            ipHeader->ip_hl = 5; // Standard IP header length (for IPV4 16 bits elements)
            ipHeader->ip_ttl = 64; // Standard TTL
            ipHeader->ip_p = PROTOCOL;
            ipHeader->ip_len = htons(HeaderSize);
            ipHeader->ip_sum = Checksum();
            #endif
        }
        IPv4FrameType(const NodeId& source, const NodeId& destination) : IPv4FrameType() {
            Source(source);
            Destination(destination);
        }
        ~IPv4FrameType() = default;

    public:
       bool IsValid() const {
            const ip* ipHeader = reinterpret_cast<const ip*>(Base::Frame());
            #ifdef __WINDOWS__
            return ((ipHeader->protocol == PROTOCOL) && (Checksum() == ipHeader->check));
            #else
            return ((ipHeader->ip_p == PROTOCOL) && (Checksum() == ipHeader->ip_sum));
            #endif
        }
        uint16_t Load(const uint8_t buffer[], const uint16_t size) {
             uint16_t copySize = std::min(size, static_cast<uint16_t>(SIZE + HeaderSize));
             ::memcpy(Base::Frame(), buffer, copySize);
             return (copySize);
        }
        uint8_t Protocol() const {
            #ifdef __WINDOWS__
            return (reinterpret_cast<const ip*>(Base::Frame())->protocol);
            #else
            return (reinterpret_cast<const ip*>(Base::Frame())->ip_p);
            #endif
        }
        inline NodeId Source() const {
            NodeId result;
            const ip* ipHeader = reinterpret_cast<const ip*>(Base::Frame());
            #ifdef __WINDOWS__
            ASSERT(ipHeader->version == IPV4_VERSION);
            #else
            ASSERT(ipHeader->ip_v == IPV4_VERSION);
            #endif

            sockaddr_in node;
            ::memset (&node, 0, sizeof(node));
            node.sin_family = AF_INET;
            node.sin_port = 0;
            #ifdef __WINDOWS__
            node.sin_addr.S_un.S_addr = ipHeader->saddr;
            #else
            node.sin_addr = ipHeader->ip_src;
            #endif
            result = node;

            return (result);
        }
        inline void Source(const NodeId& node) {
            ip* ipHeader = reinterpret_cast<ip*>(Base::Frame());
            #ifdef __WINDOWS__
            ASSERT(ipHeader->version == IPV4_VERSION);
            #else
            ASSERT(ipHeader->ip_v == IPV4_VERSION);
            #endif
            ASSERT (node.Type() == NodeId::TYPE_IPV4);

            const sockaddr_in& result = static_cast<const NodeId::SocketInfo&>(node).IPV4Socket;
            #ifdef __WINDOWS__
            ipHeader->saddr = result.sin_addr.S_un.S_addr;
            ipHeader->check = Checksum();
            #else
            ipHeader->ip_src = result.sin_addr;
            ipHeader->ip_sum = Checksum();
            #endif
        }
        inline NodeId Destination() const {
            NodeId result;
            const ip* ipHeader = reinterpret_cast<const ip*>(Base::Frame());
            #ifdef __WINDOWS__
            ASSERT(ipHeader->version == IPV4_VERSION);
            #else
            ASSERT(ipHeader->ip_v == IPV4_VERSION);
            #endif

            sockaddr_in node;
            ::memset (&node, 0, sizeof(node));
            node.sin_family = AF_INET;
            node.sin_port = 0;
            #ifdef __WINDOWS__
            node.sin_addr.S_un.S_addr = ipHeader->daddr;
            #else
            node.sin_addr = ipHeader->ip_dst;
            #endif
            result = node;

            return (result);
        }
        inline void Destination(const NodeId& node) {
            ip* ipHeader = reinterpret_cast<ip*>(Base::Frame());
    
            #ifdef __WINDOWS__
            ASSERT (ipHeader->version == IPV4_VERSION);
            #else
            ASSERT(ipHeader->ip_v == IPV4_VERSION);
            #endif
            ASSERT (node.Type() == NodeId::TYPE_IPV4);

            const sockaddr_in& result = static_cast<const NodeId::SocketInfo&>(node).IPV4Socket;
            #ifdef __WINDOWS__
            ipHeader->daddr = result.sin_addr.S_un.S_addr;
            ipHeader->check = Checksum();
            #else
            ipHeader->ip_dst = result.sin_addr;
            ipHeader->ip_sum = Checksum();
            #endif
        }
        inline uint8_t TTL() const {
            #ifdef __WINDOWS__
            return (reinterpret_cast<const ip*>(Base::Frame())->ttl);
            #else
            return (reinterpret_cast<const ip*>(Base::Frame())->ip_ttl);
            #endif
        }
        inline void TTL(const uint8_t ttl) {
            ip* ipHeader = reinterpret_cast<ip*>(Base::Frame());
            #ifdef __WINDOWS__
            ipHeader->ttl   = ttl;
            ipHeader->check = Checksum();
            #else
            ipHeader->ip_ttl = ttl;
            ipHeader->ip_sum = Checksum();
            #endif
        }
        inline uint16_t Length() const {
            #ifdef __WINDOWS__
            return ntohs(reinterpret_cast<const ip*>(Base::Frame())->tot_len) - HeaderSize;
            #else
            return ntohs(reinterpret_cast<const ip*>(Base::Frame())->ip_len) - HeaderSize;
            #endif
        }
        inline void Length(const uint16_t length) {
            ip* ipHeader = reinterpret_cast<ip*>(Base::Frame());
            #ifdef __WINDOWS__
            ipHeader->tot_len = ntohs(length + HeaderSize);
            ipHeader->check = Checksum();
            #else
            ipHeader->ip_len = ntohs(length + HeaderSize);
            ipHeader->ip_sum = Checksum();
            #endif
        }
        inline uint8_t Version() const {
            #ifdef __WINDOWS__
            return (reinterpret_cast<const ip*>(Base::Frame())->version);
            #else
            return (reinterpret_cast<const ip*>(Base::Frame())->ip_v);
            #endif
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(Base::Frame()[HeaderSize]) : nullptr);
        }
        inline uint16_t Size() const {
            #ifdef __WINDOWS__
            return ntohs(reinterpret_cast<const ip*>(Base::Frame())->tot_len) + Base::HeaderSize;
            #else
            return ntohs(reinterpret_cast<const ip*>(Base::Frame())->ip_len) + Base::HeaderSize;
            #endif
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
            ip*       ipHeader = const_cast<ip*>(reinterpret_cast<const ip*>(Base::Frame()));

            #ifdef __WINDOWS__
            uint16_t  original = ipHeader->check;
            ipHeader->check = 0;
            uint32_t result = Checksum(0, reinterpret_cast<const uint16_t*>(ipHeader), HeaderSize);
            ipHeader->check = original;
            #else
            uint16_t  original = ipHeader->ip_sum;
            ipHeader->ip_sum = 0;
            uint32_t result = Checksum(0, reinterpret_cast<const uint16_t*>(ipHeader), HeaderSize);
            ipHeader->ip_sum = original;
            #endif
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

            tcpHeader->th_sport = htons(source.PortNumber());
            tcpHeader->th_dport = htons(destination.PortNumber());

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
            return (result.IsValid() ? NodeId(result, ntohs(tcpHeader->th_sport)) : result);
        }
        inline void Source(const NodeId& node) {
            if (node.IsValid()) {
                Base::Source(node);
                tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());
                tcpHeader->th_sport = htons(node.PortNumber());
            }
        }
        inline NodeId Destination() const {
            const tcphdr* tcpHeader = reinterpret_cast<const tcphdr*>(Base::Frame());
            NodeId result(Base::Destination());
            return (result.IsValid() ? NodeId(result, ntohs(tcpHeader->th_dport)) : result);
        }
        inline void Destination(const NodeId& node) {
            if (node.IsValid()) {
                Base::Destination(node);
                tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());
                tcpHeader->th_dport = htons(node.PortNumber());
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

            udpHeader->uh_sum = Checksum();
        }
        UDPv4FrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            ::memset(udpHeader, 0, HeaderSize);

            #ifdef __WINDOWS__
            udpHeader->source = htons(source.PortNumber());
            udpHeader->dest = htons(destination.PortNumber());
            #else
            udpHeader->uh_sport = htons(source.PortNumber());
            udpHeader->uh_dport = htons(destination.PortNumber());
            #endif

            Base::Length(HeaderSize);

            #ifdef __WINDOWS__
            udpHeader->check = Checksum();
            #else
            udpHeader->uh_sum = Checksum();
            #endif
        }
        ~UDPv4FrameType() = default;

    public:
        bool IsValid() const {
                bool result = false;
                if (Base::IsValid()) {
                   result = true;
                   uint16_t csum = reinterpret_cast<const udphdr*>(Base::Frame())->uh_sum;
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
            return (result.IsValid() ? NodeId(result, ntohs(udpHeader->uh_sport)) : result);
        }
        inline void Source(const NodeId& node) {
            if (node.IsValid()) {
                Base::Source(node);
                udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
                udpHeader->uh_sport = htons(node.PortNumber());
            }
        }
        inline NodeId Destination() const {
            const udphdr* udpHeader = reinterpret_cast<const udphdr*>(Base::Frame());
            NodeId result(Base::Destination());
            return (result.IsValid() ? NodeId(result, ntohs(udpHeader->uh_dport)) : result);
        }
        inline void Destination(const NodeId& node) {
            if (node.IsValid()) {
                Base::Destination(node);
                udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
                udpHeader->uh_dport = htons(node.PortNumber());
            }
        }
        inline uint16_t SourcePort() const {
            return (ntohs(reinterpret_cast<const udphdr*>(Base::Frame())->uh_sport));
        }
        inline void SourcePort(const uint16_t port) {
            reinterpret_cast<udphdr*>(Base::Frame())->uh_sport = ntohs(port);
        }
        inline uint16_t DestinationPort() const {
            return (ntohs(reinterpret_cast<const udphdr*>(Base::Frame())->uh_dport));
        }
        inline void DestinationPort(const uint16_t port) {
            reinterpret_cast<udphdr*>(Base::Frame())->uh_dport = ntohs(port);
        }
        void Length(const uint16_t length) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            #ifdef __WINDOWS__
            udpHeader->len = htons(HeaderSize + length);    
            #else
            udpHeader->uh_len = htons(HeaderSize + length);
            #endif
            Base::Length(HeaderSize + length);
            #ifdef __WINDOWS__
            udpHeader->check = Checksum();
            #else
            udpHeader->uh_sum = Checksum();
            #endif
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
            #ifdef __WINDOWS__
            uint16_t  original = udpHeader->check;
            udpHeader->check = 0;
            #else
            uint16_t  original = udpHeader->uh_sum;
            udpHeader->uh_sum = 0;
            #endif

            uint32_t result = Base::Checksum(0, reinterpret_cast<const uint16_t*>(pseudoHeader), sizeof(pseudoHeader));
            result = Base::Checksum (result, reinterpret_cast<const uint16_t*>(udpHeader), Base::Length());
            #ifdef __WINDOWS__
            udpHeader->check = original;
            #else
            udpHeader->uh_sum = original;
            #endif
            return (Base::Shrink(result));
        }
    };

} } // namespace WPEFramework::Core
