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
 
#ifndef __IPPACKET_H
#define __IPPACKET_H

#include "Module.h"
#include "Portability.h"
#include "Sync.h"
#include "NodeId.h"

#ifdef __UNIX__
#include "net/if_arp.h"
#include<net/ethernet.h>
#include<netinet/udp.h>
#include<netinet/ip.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<net/if.h>
#endif

namespace WPEFramework {
namespace Core {

#ifdef __UNIX__
    class IPPacket {
    public:
        enum Protocol : uint8_t {
            PROTOCOL_UDP = 17
        };
    public:
        IPPacket() = delete;
        IPPacket(const IPPacket&) = delete;
        IPPacket& operator=(const IPPacket&) = delete;

        // Constructor for creating a packet
        IPPacket(uint8_t* buffer, const uint16_t bufferSize, const Core::NodeId& source, const Core::NodeId& target, const Protocol protocol);

        // Constructor for reading a packet
        IPPacket(uint8_t* buffer, const uint16_t bufferSize);

        inline iphdr* IPHeader() const
        {
            return reinterpret_cast<iphdr*>(_buffer);
        }

        inline uint16_t IPHeaderSize() const
        {
            return sizeof(iphdr);
        }

        template<typename T> 
        inline T* Payload() 
        {
            return reinterpret_cast<T*>(Payload());
        }

        uint16_t TotalHeaderLength() const
        {
            return (Payload() - _buffer);
        }

        uint16_t PayloadLength() const 
        {
            return PacketLength() - TotalHeaderLength();
        }

        inline uint16_t PacketLength() const 
        {
            return ntohs(IPHeader()->tot_len);
        }

        virtual void PayloadSet(const uint16_t payloadSize);
        virtual uint8_t* Payload() const;
    private:
        void RecalcIpHeaderChecksum();

    protected:
        uint8_t* _buffer;
        const uint16_t _bufferSize;
    };

    class IPUDPPacket : public IPPacket {
    public:
        IPUDPPacket() = delete;
        IPUDPPacket(const IPUDPPacket&) = delete;
        IPUDPPacket& operator=(const IPUDPPacket&) = delete;

        // Constructor for creating a packet
        IPUDPPacket(uint8_t* buffer, const uint16_t bufferSize, const Core::NodeId& source, const Core::NodeId& target);

        // Constructor for reading a packet
        IPUDPPacket(uint8_t* buffer, const uint16_t bufferSize);

        inline udphdr* UDPHeader() const
        {
            return reinterpret_cast<udphdr*>(IPPacket::Payload());
        }

        inline uint16_t UDPHeaderSize() const
        {
            return sizeof(udphdr);
        }

        virtual void PayloadSet(const uint16_t payloadSize) override;
        virtual uint8_t* Payload() const override;
    };

}
} // namespace Core
#endif

#endif // __IPPACKET_H
