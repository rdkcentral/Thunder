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

#include "IPPacket.h"

namespace WPEFramework {
namespace Core {

    IPPacket::IPPacket(uint8_t* buffer, const uint16_t bufferSize, const Core::NodeId& source, const Core::NodeId& target, const Protocol protocol) 
        : _buffer(buffer)
        , _bufferSize(bufferSize)
    {
        memset(IPHeader(), 0, sizeof(iphdr));
        IPHeader()->version = 4; // Use IPv4
        IPHeader()->ihl = 5; // Standard IP header length
        IPHeader()->ttl = 64; // Standard TTL
        IPHeader()->protocol = protocol;

        memcpy(&(IPHeader()->saddr), &(reinterpret_cast<const sockaddr_in*>(static_cast<const struct sockaddr*>(source))->sin_addr), sizeof(IPHeader()->saddr));
        memcpy(&(IPHeader()->daddr), &(reinterpret_cast<const sockaddr_in*>(static_cast<const struct sockaddr*>(target))->sin_addr), sizeof(IPHeader()->saddr));
    }

    IPPacket::IPPacket(uint8_t* buffer, const uint16_t bufferSize) 
        : _buffer(buffer)
        , _bufferSize(bufferSize)
    {
    }

    void IPPacket::PayloadSet(const uint16_t payloadSize)
    {
        IPHeader()->tot_len = htons(sizeof(iphdr) + payloadSize);
        RecalcIpHeaderChecksum();
    }

    uint8_t* IPPacket::Payload() const
    {
        return _buffer + sizeof(iphdr);
    }

    bool IPPacket::IsIncomingMessage() const 
    {
        return (TotalBufferSize() > TotalHeaderLength());
    }

    void IPPacket::RecalcIpHeaderChecksum() 
    {
        // src: https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
        iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
        uint16_t* data = reinterpret_cast<uint16_t*>(ipHeader);
        uint32_t count = (ipHeader->ihl) << 2;

        // Checksum should be calculated on itself set to 0
        ipHeader->check = 0;

        uint32_t sum = 0;
        while (count > 1) {
            sum += * data++;
            count -= 2;
        }

        if(count > 0) {
            sum += ((*data)&htons(0xFF00));
        }
        
        while (sum>>16) {
            sum = (sum & 0xffff) + (sum >> 16);
        }

        sum = ~sum;
        ipHeader->check = static_cast<uint16_t>(sum);
    }

    IPUDPPacket::IPUDPPacket(uint8_t* buffer, const uint16_t bufferSize, const Core::NodeId& source, const Core::NodeId& target)
        : IPPacket(buffer, bufferSize, source, target, IPPacket::PROTOCOL_UDP)
    {
        UDPHeader()->source = reinterpret_cast<const sockaddr_in*>(static_cast<const struct sockaddr*>(source))->sin_port;
        UDPHeader()->dest = reinterpret_cast<const sockaddr_in*>(static_cast<const struct sockaddr*>(target))->sin_port;
    }

    // Constructor for reading a packet
    IPUDPPacket::IPUDPPacket(uint8_t* buffer, const uint16_t bufferSize) 
        : IPPacket(buffer, bufferSize)
    {
    }

    void IPUDPPacket::PayloadSet(const uint16_t payloadSize)
    {
        UDPHeader()->len = htons(sizeof(udphdr) + payloadSize);
        IPPacket::PayloadSet(payloadSize + sizeof(udphdr));
    }

    uint8_t* IPUDPPacket::Payload() const
    {
        return IPPacket::Payload() + sizeof(udphdr);
    }

}
} // namespace Solution::Core
