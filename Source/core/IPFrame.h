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

#include <pshpack1.h>

//
// IPv4 Header (without any IP options)
//
typedef struct ip_hdr
{
    unsigned char  ip_verlen;        // 4-bit IPv4 version
                                     // 4-bit header length (in 32-bit words)
    unsigned char  ip_tos;           // IP type of service
    unsigned short ip_totallength;   // Total length
    unsigned short ip_id;            // Unique identifier 
    unsigned short ip_offset;        // Fragment offset field
    unsigned char  ip_ttl;           // Time to live
    unsigned char  ip_protocol;      // Protocol(TCP,UDP etc)
    unsigned short ip_checksum;      // IP checksum
    unsigned int   ip_srcaddr;       // Source address
    unsigned int   ip_destaddr;      // Source address
} IPV4_HDR, *PIPV4_HDR, FAR * LPIPV4_HDR, iphdr;

//
// IPv6 Header
//
typedef struct ipv6_hdr
{
    unsigned long   ipv6_vertcflow;        // 4-bit IPv6 version
                                           // 8-bit traffic class
                                           // 20-bit flow label
    unsigned short  ipv6_payloadlen;       // payload length
    unsigned char   ipv6_nexthdr;          // next header protocol value
    unsigned char   ipv6_hoplimit;         // TTL 
    struct in6_addr ipv6_srcaddr;          // Source address
    struct in6_addr ipv6_destaddr;         // Destination address
} IPV6_HDR, *PIPV6_HDR, FAR * LPIPV6_HDR;

//
// IPv6 Fragmentation Header
//
typedef struct ipv6_fragment_hdr
{
    unsigned char   ipv6_frag_nexthdr;      // Next protocol header
    unsigned char   ipv6_frag_reserved;     // Reserved: zero
    unsigned short  ipv6_frag_offset;       // Offset of fragment
    unsigned long   ipv6_frag_id;           // Unique fragment ID
} IPV6_FRAGMENT_HDR, *PIPV6_FRAGMENT_HDR, FAR * LPIPV6_FRAGMENT_HDR;

//
// Define the UDP header 
//
typedef struct udp_hdr
{
    unsigned short src_portno;       // Source port no.
    unsigned short dest_portno;      // Dest. port no.
    unsigned short udp_length;       // Udp packet length
    unsigned short udp_checksum;     // Udp checksum
} UDP_HDR, *PUDP_HDR, udphdr;

//
// Define the TCP header
//
typedef struct tcp_hdr
{
    unsigned short src_portno;       // Source port no.
    unsigned short dest_portno;      // Dest. port no.
    unsigned long  seq_num;          // Sequence number
    unsigned long  ack_num;          // Acknowledgement number;
    unsigned short lenflags;         // Header length and flags
    unsigned short window_size;      // Window size
    unsigned short tcp_checksum;     // Checksum
    unsigned short tcp_urgentptr;    // Urgent data?
} TCP_HDR, *PTCP_HDR, tcphdr;

//
// Stucture to extract port numbers that overlays the UDP and TCP header
//
typedef struct port_hdr
{
    unsigned short src_portno;
    unsigned short dest_portno;
} PORT_HDR, *PPORT_HDR;

//
// IGMP header
//
typedef struct igmp_hdr
{
    unsigned char   version_type;
    unsigned char   max_resp_time;
    unsigned short  checksum;
    unsigned long   group_addr;
} IGMP_HDR, *PIGMP_HDR;

typedef struct igmp_hdr_query_v3
{
    unsigned char   type;
    unsigned char   max_resp_time;
    unsigned short  checksum;
    unsigned long   group_addr;
    unsigned char   resv_suppr_robust;
    unsigned char   qqi;
    unsigned short  num_sources;
    unsigned long   sources[1];
} IGMP_HDR_QUERY_V3, *PIGMP_HDR_QUERY_V3;

typedef struct igmp_group_record_v3
{
    unsigned char   type;
    unsigned char   aux_data_len;
    unsigned short  num_sources;
    unsigned long   group_addr;
    unsigned long   source_addr[1];
} IGMP_GROUP_RECORD_V3,  *PIGMP_GROUP_RECORD_V3;

typedef struct igmp_hdr_report_v3
{
    unsigned char   type;
    unsigned char   reserved1;
    unsigned short  checksum;
    unsigned short  reserved2;
    unsigned short  num_records;
} IGMP_HDR_REPORT_V3, *PIGMP_HDR_REPORT_V3;

#include <poppack.h>

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
