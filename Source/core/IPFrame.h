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
        static constexpr uint8_t IPV4_VERSION = 4;
        static constexpr uint8_t IPV4_HEADER_PROTOCOL_OFFSET = offsetof(iphdr, protocol);
        static constexpr uint8_t IPV4_HEADER_LENGTH_OFFSET = offsetof(iphdr, tot_len);

    public:
        IPFrameType(const IPFrameType<PROTOCOL,SIZE>&) = delete;
        IPFrameType<PROTOCOL,SIZE>& operator=(const IPFrameType<PROTOCOL,SIZE>&) = delete;

        IPFrameType() 
            : _length(0)
        {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);

            ::memset(ipHeader, 0, sizeof(iphdr));
            ipHeader->version = IPV4_VERSION;
            ipHeader->id = 37540;
            ipHeader->ihl = 5; // Standard IP header length (for IPV4 16 bits elements)
            ipHeader->ttl = 64; // Standard TTL
            ipHeader->protocol = PROTOCOL;
            ipHeader->check = Checksum();
        }
        IPFrameType(const NodeId& source, const NodeId& destination) : IPFrameType() {
            Source(source);
            Destination(destination);
        }
        IPFrameType(const uint8_t buffer[], const uint16_t size)
            : _length(size - sizeof(iphdr)) {
             ::memcpy(_buffer, buffer, sizeof(iphdr) + SIZE);
        }
        ~IPFrameType() = default;

    public:
        bool IsValid() const {
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            return ((ipHeader->protocol == PROTOCOL) && (Checksum() == ipHeader->check));
        }
        inline NodeId Source() const {
            NodeId result;
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            if (ipHeader->version == IPV4_VERSION) {
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
            if (ipHeader->version == IPV4_VERSION) {
                ASSERT (node.Type() == NodeId::TYPE_IPV4);
                const sockaddr_in& result = static_cast<const NodeId::SocketInfo&>(node).IPV4Socket;
                ipHeader->saddr = result.sin_addr.s_addr;
                ipHeader->check = Checksum();
            }
        }
        inline NodeId Destination() const {
            NodeId result;
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            if (ipHeader->version == IPV4_VERSION) {
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
            if (ipHeader->version == IPV4_VERSION) {
                ASSERT (node.Type() == NodeId::TYPE_IPV4);
                const sockaddr_in& result = static_cast<const NodeId::SocketInfo&>(node).IPV4Socket;
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
        inline void Length(const uint16_t length) {
            _length = length;

            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            ipHeader->tot_len = htons(sizeof(iphdr) + SIZE + length);
            ipHeader->check = Checksum();
        }
        inline uint16_t Length() {
            return _length;
        }
        inline uint16_t Size() const {
            return (sizeof(iphdr) + SIZE);
        }
        inline uint8_t Version() const {
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            return ipHeader->version;
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(_buffer[sizeof(iphdr)]) : nullptr);
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(_buffer[sizeof(iphdr)]) : nullptr);
        }
        uint8_t* Header() {
            return _buffer;
        }
        const uint8_t* Header() const {
            return _buffer;
        }
        inline uint16_t Offset() const {
            return offsetof(iphdr, protocol);
        }

    protected:
        void FillChecksumDummyHeader(uint8_t* header, uint16_t size) const {
            const iphdr* ipHeader = reinterpret_cast<const iphdr*>(_buffer);
            iphdr* dummy = reinterpret_cast<iphdr*>(header);

            dummy->check = 0;
            dummy->tot_len = size;
            dummy->saddr = ipHeader->saddr;
            dummy->daddr = ipHeader->daddr;
            dummy->protocol = ipHeader->protocol;

            return;
        }
        uint16_t Checksum(const uint16_t* data, const uint16_t size) const {
            // src: https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
            uint32_t  sum = 0;

            uint16_t count = size;
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

            return (static_cast<uint16_t>(sum));
        }

    private:
        inline uint16_t Checksum() const {

            iphdr*    ipHeader = const_cast<iphdr*>(reinterpret_cast<const iphdr*>(_buffer));
            uint16_t  org = ipHeader->check;
            ipHeader->check = 0;
            uint16_t checksum = Checksum(reinterpret_cast<uint16_t*>(ipHeader), (ipHeader->ihl) << 2);
            ipHeader->check = org;
            return checksum;
        }

    private:
        uint8_t _buffer[SIZE + sizeof(iphdr)];
        uint16_t _length;
    };

#ifndef __WINDOWS__

    template <uint16_t SIZE = 0>
    class TCPFrameType : public IPFrameType<IPPROTO_TCP, SIZE + sizeof(tcphdr)> {
    private:
        using Base = IPFrameType<IPPROTO_TCP, SIZE + sizeof(tcphdr)>;

    public:
        TCPFrameType(const TCPFrameType<SIZE>&) = delete;
        TCPFrameType<SIZE>& operator=(const TCPFrameType<SIZE>&) = delete;

        TCPFrameType() : Base() {
            tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());

            ::memset(tcpHeader, 0, sizeof(tcphdr));
        }
        TCPFrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            tcphdr* tcpHeader = reinterpret_cast<tcphdr*>(Base::Frame());

            tcpHeader->source = htons(source.PortNumber());
            tcpHeader->dest = htons(destination.PortNumber());
        }
        TCPFrameType(const uint8_t buffer[], const uint16_t size) : Base(buffer, size) {
        }
        ~TCPFrameType() = default;

    public:
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
        inline uint16_t Size() const {
            return (Base::Size());
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(Frame()[sizeof(tcphdr)]) : nullptr);   
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(Frame()[sizeof(tcphdr)]) : nullptr);   
        }
        uint8_t* Header() {
            return Base::Header();
        }
        const uint8_t* Header() const {
            return Base::Header();
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

            ::memset(udpHeader, 0, sizeof(udphdr));
            Length(0);
        }
        UDPFrameType(const NodeId& source, const NodeId& destination) : Base(source, destination) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            ::memset(udpHeader, 0, sizeof(udphdr));

            udpHeader->source = htons(source.PortNumber());
            udpHeader->dest = htons(destination.PortNumber());
            Length(0);
        }
        UDPFrameType(const uint8_t buffer[], const uint16_t size) : Base(buffer, size - sizeof(udphdr)) {
        }

        ~UDPFrameType() = default;

    public:
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
        void Length(const uint16_t length) {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            udpHeader->len = htons(sizeof(udphdr) + length);
            Base::Length(length);
        }
        inline uint16_t CheckPayloadChecksum(const uint8_t payload[], uint32_t size) {

            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            uint16_t org = udpHeader->check;
            udpHeader->check = 0; // Reset checksum
            udpHeader->check = Checksum(payload, size);
            return (org == udpHeader->check);
        }
        inline uint16_t UpdatePayloadChecksum(const uint8_t payload[], uint32_t size) {
            Length(size);
            udphdr* udpHeader = reinterpret_cast<udphdr*>(Base::Frame());
            udpHeader->check = 0; // Reset checksum

            udpHeader->check = Checksum(payload, size);
            return Size() + Base::Length();
        }
        inline uint16_t Size() const {
            return (Base::Size());
        }
        uint8_t* Frame() {
            return (SIZE > 0 ? &(Frame()[sizeof(udphdr)]) : nullptr);
        }
        const uint8_t* Frame() const {
            return (SIZE > 0 ? &(Frame()[sizeof(udphdr)]) : nullptr);
        }
        uint8_t* Header() {
            return Base::Header();
        }
        const uint8_t* Header() const {
            return Base::Header();
        }

    private:
        void FillChecksumDummyHeader(uint8_t packet[], uint16_t size) {
            const udphdr* udpHeader = reinterpret_cast<const udphdr*>(Base::Frame());
            Base::FillChecksumDummyHeader(packet, udpHeader->len);

            udphdr* dummy = reinterpret_cast<udphdr*>(packet + (Base::Size() - sizeof(udphdr)));
            dummy->check = 0;
            dummy->dest = udpHeader->dest;
            dummy->source = udpHeader->source;
            dummy->len = udpHeader->len;
        }
        inline uint16_t Checksum(const uint8_t payload[], uint32_t size) {
            uint8_t packet[Base::Size() + size];
            ::memset(packet, 0, Base::Size() + size);

            if (Base::Version() == IPFrameType<IPPROTO_UDP, 0>::IPV4_VERSION) {
                FillChecksumDummyHeader(packet, Base::Size() + size);
            }
            ::memcpy(packet + Base::Size(), payload, size);

            return (Base::Checksum(reinterpret_cast<const uint16_t*>(packet), Base::Size() + size));
        }
    };
#endif
} } // namespace WPEFramework::Core
