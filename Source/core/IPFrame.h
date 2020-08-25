/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

    template <uint8_t PROTOCOL, uint16_t SIZE = 0>
    class IPFrameType {
    public:
        IPFrameType(const IPFrameType<PROTOCOL,SIZE>&) = delete;
        IPFrameType<PROTOCOL,SIZE>& operator=(const IPFrameType<PROTOCOL,SIZE>&) = delete;

        IPFrameType() 
            : _length(0)
        {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);

            ::memset(ipHeader, 0, sizeof(iphdr));
            ipHeader->version = 4; // Use IPv4
            ipHeader->ihl = 5; // Standard IP header length (for IPV4 16 bits elements)
            ipHeader->ttl = 64; // Standard TTL
            ipHeader->protocol = PROTOCOL;
            ipHeader->check = Checksum();
        }
        IPFrameType(const NodeId& source, const NodeId& destination) : IPFrameType() {
            Source(source);
            Destination(destination);
        }
        ~IPFrameType() = default;

    public:
        bool IsValid() const {
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            return ((ipHeader->protocol == PROTOCOL) && (Checksum() == ipHeader->check));
        }
        inline NodeId& Source() const {
            NodeId result;
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            if (ipHeader->version == 4) {
                sockaddr_in node;
                ::memset (&node, 0, sizeof(node));
                node.sin_family = AF_INET;
                node.sin_port = 0;
                node.sin_addr.s_addr = ipHeader->saddr;
                result = node;
            }
            return (result);
        }
        inline void Source(const NodeId& node) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            if (ipHeader->version == 4) {
                ASSERT (node.Type() == NodeId::TYPE_IPV4);
                const sockaddr_in& result = static_cast<const union SocketInfo&>(node).IPV4Socket;
                ipHeader->saddr = result.sin_addr.s_addr;
                ipHeader->check = Checksum();
            }
        }
        inline NodeId& Destination() const {
            NodeId result;
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            if (ipHeader->version == 4) {
                sockaddr_in node;
                ::memset (&node, 0, sizeof(node));
                node.sin_family = AF_INET;
                node.sin_port = 0;
                node.sin_addr.s_addr = ipHeader->daddr;
                result = node;
            }
            return (result);
        }
        inline void Destination(const NodeId& node) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            if (ipHeader->version == 4) {
                ASSERT (node.Type() == NodeId::TYPE_IPV4);
                const sockaddr_in& result = static_cast<const union SocketInfo&>(node).IPV4Socket;
                ipHeader->daddr = result.sin_addr.s_addr;
                ipHeader->check = Checksum();
            }
        }
        inline uint8_t TTL() const {
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            return (ipHeader->ttl);
        }
        inline void TTL(const uint8_t ttl) {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            ipHeader->ttl   = ttl;
            ipHeader->check = Checksum() ;
        }
        inline uint16_t Size() const {
            return (sizeof(iphdr) + _length);
        }
        uint8_t* Frame() 
        {
            return (SIZE > 0 ? &(_buffer[sizeof(iphdr)]) : nullptr);   
        }
        const uint8_t* Frame() const
        {
            return (SIZE > 0 ? &(_buffer[sizeof(iphdr)]) : nullptr);   
        }
        void Length(const uint16_t length) {
            ASSERT (length <= SIZE);
            _length = length;
        }

    private:
        uint16_t Checksum() const
        {
            // src: https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
            iphdr*    ipHeader = const_cast<iphdr*>(reinterpret_cast<const iphdr*>(_buffer));
            uint16_t* data     = reinterpret_cast<uint16_t*>(ipHeader);
            uint32_t  sum      = 0;
            uint8_t   count    = (ipHeader->ihl) << 2;
            uint16_t  org      = ipHeader->check;

            // Checksum should be calculated on itself set to 0
            ipHeader->check = 0;

            while (count > 1) {
                sum += *data++;
                count -= 2;
            }

            if(count > 0) {
                sum += ((*data)&htons(0xFF00));
            }
            
            if (sum > 0xFFFF) {
                sum = (sum & 0xFFFF) + (sum >> 16);
            }

            sum = ~sum;
            ipHeader->check = org;

            return (static_cast<uint16_t>(sum));
        }

    private:
        uint8_t _buffer[SIZE + sizeof(iphdr)];
        uint16_t _length;
    };

    template <uint16_t SIZE = 0>
    class TCPFrameType : public IPFrameType<IPPROTO_TCP, SIZE + sizeof(tcphdr)> {
    private:
        using Base = IPFrameType<IPPROTO_TCP, SIZE + sizeof(tcphdr)>;

    public:
        TCPFrameType(const TCPFrameType<SIZE>&) = delete;
        TCPFrameType<SIZE>& operator=(const TCPFrameType<SIZE>&) = delete;

        TCPFrameType() : Base() {
            tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());

            memset(tcpHeader, 0, sizeof(tcphdr));
        }
        TCPFrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());

            tcpHeader->source = htons(source.PortNumber());
            tcpHeader->dest = htons(destination.PortNumber());
        }
        ~TCPFrameType() = default;

    public:
        inline NodeId& Source() const {
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
        inline NodeId& Destination() const {
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
        inline uint16_t Size() const {
            return (sizeof(tcphdr) + Base::Size());
        }
        uint8_t* Frame() 
        {
            return (SIZE > 0 ? &(Frame()[sizeof(tcphdr)]) : nullptr);   
        }
        const uint8_t* Frame() const
        {
            return (SIZE > 0 ? &(Frame()[sizeof(tcphdr)]) : nullptr);   
        }
    };

    template <uint16_t SIZE = 0>
    class UDPFrameType : public IPFrameType<IPPROTO_UDP, SIZE + sizeof(udphdr)> {
    private:
        using Base = IPFrameType<IPPROTO_UDP, SIZE + sizeof(udphdr)>;

    public:
        UDPFrameType(const UDPFrameType<SIZE>&) = delete;
        UDPFrameType<SIZE>& operator=(const UDPFrameType<SIZE>&) = delete;

        UDPFrameType() : Base() {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());

            memset(udpHeader, 0, sizeof(udphdr));
        }
        UDPFrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());

            udpHeader->source = htons(source.PortNumber());
            udpHeader->dest = htons(destination.PortNumber());
        }
        ~UDPFrameType() = default;

    public:
        inline NodeId& Source() const {
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
        inline NodeId& Destination() const {
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
        inline uint16_t Size() const {
            return (sizeof(udphdr) + Base::Size());
        }
        uint8_t* Frame() 
        {
            return (SIZE > 0 ? &(Frame()[sizeof(udphdr)]) : nullptr);   
        }
        const uint8_t* Frame() const
        {
            return (SIZE > 0 ? &(Frame()[sizeof(udphdr)]) : nullptr);   
        }
    };

} } // namespace WPEFramework::Core
