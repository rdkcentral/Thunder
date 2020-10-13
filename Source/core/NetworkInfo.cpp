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
 
#include "NetworkInfo.h"
#include "IIterator.h"
#include "Netlink.h"
#include "Number.h"
#include "Proxy.h"
#include "Serialization.h"
#include "Sync.h"
#include "Trace.h"

#if defined(__WINDOWS__)
#include <WS2tcpip.h>
#include <Wmistr.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#pragma comment(lib, "iphlpapi.lib")
#elif defined(__POSIX__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if_arp.h>
#include <linux/rtnetlink.h>
#include <list>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#ifdef __APPLE__
#include <net/if_dl.h>
#endif

namespace WPEFramework {

namespace Core {

#ifdef __WINDOWS__

/* Note: could also use malloc() and free() */
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

    static uint16_t AdapterCount = 0;
    static PIP_ADAPTER_ADDRESSES _interfaceInfo = nullptr;

    static void ConvertMACToString(const uint8_t address[], const uint8_t length, const char delimiter, string& output)
    {
        for (uint8_t i = 0; i < length; i++) {
            // Reason for the low-level approch is performance.
            // In stead of using string operations, we know that each byte exists of 2 nibbles,
            // lets just translate these nibbles to Hexadecimal numbers and add them to the output.
            // This saves a setup of several string manipulation operations.
            uint8_t highNibble = ((address[i] & 0xF0) >> 4);
            uint8_t lowNibble = (address[i] & 0x0F);
            if ((i != 0) && (delimiter != '\0')) {
                output += delimiter;
            }
            output += static_cast<char>(highNibble + (highNibble >= 10 ? ('A' - 10) : '0'));
            output += static_cast<char>(lowNibble + (lowNibble >= 10 ? ('A' - 10) : '0'));
        }
    }

    static PIP_ADAPTER_ADDRESSES LoadAdapterInfo(const uint16_t adapterIndex)
    {
        PIP_ADAPTER_ADDRESSES result = nullptr;

        if (_interfaceInfo == nullptr) {
            // Set the flags to pass to GetAdaptersAddresses
            ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

            // See how much we need to allcoate..
            ULONG outBufLen = 0;

            DWORD dwRetVal = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &outBufLen);

            // ALlocate the requested buffer
            _interfaceInfo = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(MALLOC(outBufLen));

            // Now get teh actual payload..
            dwRetVal = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, _interfaceInfo, &outBufLen);

            if (dwRetVal == NO_ERROR) {
                PIP_ADAPTER_ADDRESSES pRunner = _interfaceInfo;

                while (pRunner != nullptr) {
                    AdapterCount++;
                    pRunner = pRunner->Next;
                }

                if (AdapterCount == 0) {
                    FREE(_interfaceInfo);
                    _interfaceInfo = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(~0);
                }
            } else {
                // Oops failed..
                FREE(_interfaceInfo);

                _interfaceInfo = nullptr;
            }
        }

        if (adapterIndex < AdapterCount) {
            uint16_t index = adapterIndex;
            result = _interfaceInfo;

            while (index-- > 0) {
                ASSERT(result != nullptr);

                result = result->Next;
            }

            ASSERT(result != nullptr);
        }

        return (result);
    }

    //  IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = nullptr;
    //  IP_ADAPTER_PREFIX *pPrefix = nullptr;

    //          printf("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n",
    //                 pCurrAddresses->Length);
    //          printf("\tIfIndex (IPv4 interface): %u\n", pCurrAddresses->IfIndex);
    //          printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

    //          pDnServer = pCurrAddresses->FirstDnsServerAddress;
    //          if (pDnServer) {
    //              for (i = 0; pDnServer != nullptr; i++)
    //                  pDnServer = pDnServer->Next;
    //              printf("\tNumber of DNS Server Addresses: %d\n", i);
    //          } else
    //              printf("\tNo DNS Server Addresses\n");

    //          printf("\tDNS Suffix: %wS\n", pCurrAddresses->DnsSuffix);
    //          printf("\tDescription: %wS\n", pCurrAddresses->Description);
    //          printf("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);

    //          printf("\tFlags: %ld\n", pCurrAddresses->Flags);
    //          printf("\tMtu: %lu\n", pCurrAddresses->Mtu);
    //          printf("\tIfType: %ld\n", pCurrAddresses->IfType);
    //          printf("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
    //          printf("\tIpv6IfIndex (IPv6 interface): %u\n",
    //                 pCurrAddresses->Ipv6IfIndex);
    //          printf("\tZoneIndices (hex): ");
    //          for (i = 0; i < 16; i++)
    //              printf("%lx ", pCurrAddresses->ZoneIndices[i]);
    //          printf("\n");

    //          printf("\tTransmit link speed: %I64u\n", pCurrAddresses->TransmitLinkSpeed);
    //          printf("\tReceive link speed: %I64u\n", pCurrAddresses->ReceiveLinkSpeed);

    //          pPrefix = pCurrAddresses->FirstPrefix;
    //          if (pPrefix) {
    //              for (i = 0; pPrefix != nullptr; i++)
    //                  pPrefix = pPrefix->Next;
    //              printf("\tNumber of IP Adapter Prefix entries: %d\n", i);
    //          } else
    //              printf("\tNumber of IP Adapter Prefix entries: 0\n");

    IPV4AddressIterator::IPV4AddressIterator(const uint16_t adapter)
        : _adapter(adapter)
        , _index(static_cast<uint16_t>(~0))
    {
        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_adapter);

        _section1 = 0;
        _section2 = 0;
        _section3 = 0;

        if (info != nullptr) {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = info->FirstUnicastAddress;
            while (pUnicast != nullptr) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    _section1++;
                }

                pUnicast = pUnicast->Next;
            }

            PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = info->FirstMulticastAddress;
            while (pMulticast != nullptr) {
                if (pMulticast->Address.lpSockaddr->sa_family == AF_INET) {
                    _section2++;
                }

                pMulticast = pMulticast->Next;
            }

            _section2 += _section1;

            PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = info->FirstAnycastAddress;
            while (pAnycast != nullptr) {
                if (pAnycast->Address.lpSockaddr->sa_family == AF_INET) {
                    _section3++;
                }

                pAnycast = pAnycast->Next;
            }

            _section3 += _section2;
        }
    }

    IPNode IPV4AddressIterator::Address() const
    {
        IPNode result;

        ASSERT(IsValid());

        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_adapter);

        ASSERT(info != nullptr);

        if (_index < _section1) {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = info->FirstUnicastAddress;
            uint16_t steps = _index + 1;

            while ((steps != 0) && (pUnicast != nullptr)) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    steps--;
                }
                if (steps != 0) {
                    pUnicast = pUnicast->Next;
                }
            }

            ASSERT(pUnicast != nullptr);
            ASSERT(pUnicast->Address.lpSockaddr->sa_family == AF_INET);

            result = IPNode(*reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr), (32 - pUnicast->OnLinkPrefixLength));
        } else if ((_index >= _section1) && (_index < _section2)) {
            PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = info->FirstMulticastAddress;
            uint16_t steps = _index - _section1 + 1;

            while ((steps != 0) && (pMulticast != nullptr)) {
                if (pMulticast->Address.lpSockaddr->sa_family == AF_INET) {
                    steps--;
                }
                if (steps != 0) {
                    pMulticast = pMulticast->Next;
                }
            }

            ASSERT(pMulticast != nullptr);
            ASSERT(pMulticast->Address.lpSockaddr->sa_family == AF_INET);

            result = IPNode(*reinterpret_cast<sockaddr_in*>(pMulticast->Address.lpSockaddr), 32);
        } else {
            PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = info->FirstAnycastAddress;
            uint16_t steps = _index - _section2;

            while ((steps != 0) && (pAnycast != nullptr)) {
                if (pAnycast->Address.lpSockaddr->sa_family == AF_INET) {
                    steps--;
                }
                if (steps != 0) {
                    pAnycast = pAnycast->Next;
                }
            }

            ASSERT(pAnycast != nullptr);
            ASSERT(pAnycast->Address.lpSockaddr->sa_family == AF_INET);

            result = IPNode(*reinterpret_cast<sockaddr_in*>(pAnycast->Address.lpSockaddr), 32);
        }
        return (result);
    }

    IPV6AddressIterator::IPV6AddressIterator(const uint16_t adapter)
        : _adapter(adapter)
        , _index(static_cast<uint16_t>(~0))
    {
        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_adapter);

        _section1 = 0;
        _section2 = 0;
        _section3 = 0;

        if (info != nullptr) {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = info->FirstUnicastAddress;
            while (pUnicast != nullptr) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) {
                    _section1++;
                }

                pUnicast = pUnicast->Next;
            }

            PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = info->FirstMulticastAddress;
            while (pMulticast != nullptr) {
                if (pMulticast->Address.lpSockaddr->sa_family == AF_INET6) {
                    _section2++;
                }

                pMulticast = pMulticast->Next;
            }

            _section2 += _section1;

            PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = info->FirstAnycastAddress;
            while (pAnycast != nullptr) {
                if (pAnycast->Address.lpSockaddr->sa_family == AF_INET6) {
                    _section3++;
                }

                pAnycast = pAnycast->Next;
            }

            _section3 += _section2;
        }
    }

    IPNode IPV6AddressIterator::Address() const
    {
        IPNode result;

        ASSERT(IsValid());

        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_adapter);

        ASSERT(info != nullptr);

        if (_index < _section1) {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = info->FirstUnicastAddress;
            uint16_t steps = _index + 1;

            while ((steps != 0) && (pUnicast != nullptr)) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) {
                    steps--;
                }
                if (steps != 0) {
                    pUnicast = pUnicast->Next;
                }
            }

            ASSERT(pUnicast != nullptr);
            ASSERT(pUnicast->Address.lpSockaddr->sa_family == AF_INET6);

            result = IPNode(*reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr), (128 - pUnicast->OnLinkPrefixLength));
        } else if ((_index >= _section1) && (_index < _section2)) {
            PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = info->FirstMulticastAddress;
            uint16_t steps = _index - _section1 + 1;

            while ((steps != 0) && (pMulticast != nullptr)) {
                if (pMulticast->Address.lpSockaddr->sa_family == AF_INET6) {
                    steps--;
                }
                if (steps != 0) {
                    pMulticast = pMulticast->Next;
                }
            }

            ASSERT(pMulticast != nullptr);
            ASSERT(pMulticast->Address.lpSockaddr->sa_family == AF_INET6);

            result = IPNode(*reinterpret_cast<sockaddr_in*>(pMulticast->Address.lpSockaddr), 128);
        } else {
            PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = info->FirstAnycastAddress;
            uint16_t steps = _index - _section2;

            while ((steps != 0) && (pAnycast != nullptr)) {
                if (pAnycast->Address.lpSockaddr->sa_family == AF_INET6) {
                    steps--;
                }
                if (steps != 0) {
                    pAnycast = pAnycast->Next;
                }
            }

            ASSERT(pAnycast != nullptr);
            ASSERT(pAnycast->Address.lpSockaddr->sa_family == AF_INET6);

            result = IPNode(*reinterpret_cast<sockaddr_in*>(pAnycast->Address.lpSockaddr), 128);
        }
        return (result);
    }

    uint16_t AdapterIterator::Count() const
    {
        // MAke sure it gets calculated if it was not yet calulated...
        // Lazy creation.
        LoadAdapterInfo(0);

        return (AdapterCount);
    }
    string AdapterIterator::Name() const
    {
        ASSERT(IsValid());
        string result(_T("Unknown"));
        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_index);

        if (info != nullptr) {
            // info->AdapterName
            ToString(info->FriendlyName, result);
        }

        return (result);
    }
    string AdapterIterator::MACAddress(const char delimiter) const
    {
        ASSERT(IsValid());
        string result;
        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_index);

        if (info->PhysicalAddressLength != 0) {
            ConvertMACToString(info->PhysicalAddress, static_cast<uint8_t>(info->PhysicalAddressLength), delimiter, result);
        }
        return (result);
    }

    uint32_t AdapterIterator::Up(const bool)
    {
        // TODO: Implement
        ASSERT(IsValid());

        return (Core::ERROR_NONE);
    }
    bool AdapterIterator::IsUp() const
    {
        // TODO: Implement
        ASSERT(false);

        return (false);
    }

    bool AdapterIterator::IsRunning() const
    {
        return (true);
    }

    void AdapterIterator::MACAddress(uint8_t buffer[], const uint8_t length) const
    {
        ASSERT(IsValid());

        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_index);

        if (info->PhysicalAddressLength != 0) {
            ASSERT(length >= info->PhysicalAddressLength);
            ::memcpy(buffer, info->PhysicalAddress, info->PhysicalAddressLength);
            if (length > info->PhysicalAddressLength) {
                ::memset(&buffer[info->PhysicalAddressLength], 0, length - info->PhysicalAddressLength);
            }
        }
    }

    static std::map<uint64_t, ULONG> _contextSaving;

    uint32_t AdapterIterator::Add(const IPNode& address)
    {
        uint32_t result = Core::ERROR_NONE;
        ULONG NTEContext = 0;
        ULONG NTEInstance = 0;

        PIP_ADAPTER_ADDRESSES info = LoadAdapterInfo(_index);

        UINT iaIPAddress = inet_addr(address.HostAddress().c_str());
        UINT iaIPMask = htonl(~(0xFFFFFFFF >> address.Mask()));

        DWORD dwRetVal = AddIPAddress(iaIPAddress, iaIPMask, info->IfIndex, &NTEContext, &NTEInstance);
        if (dwRetVal != NO_ERROR) {
            result = Core::ERROR_BAD_REQUEST;
        } else {

            uint64_t id = iaIPAddress;
            id <<= 32;
            id |= iaIPMask;

            _contextSaving[id] = NTEContext;
        }

        return (result);
    }

    uint32_t AdapterIterator::Delete(const IPNode& address)
    {

        uint32_t result = Core::ERROR_NONE;

        UINT iaIPAddress = inet_addr(address.HostAddress().c_str());
        UINT iaIPMask = htonl(~(0xFFFFFFFF >> address.Mask()));

        uint64_t id = iaIPAddress;
        id <<= 32;
        id |= iaIPMask;

        if (_contextSaving.find(id) != _contextSaving.end()) {
            ULONG NTEContext = _contextSaving[id];

            DWORD dwRetVal = DeleteIPAddress(NTEContext);
            if (dwRetVal != NO_ERROR) {
                result = Core::ERROR_BAD_REQUEST;
            }
        }

        return (result);
    }

    uint32_t AdapterIterator::Gateway(const IPNode& network, const NodeId& gateway)
    {

        //TODO: Needs implementation
        ASSERT(false);

        return (Core::ERROR_BAD_REQUEST);
    }

    uint32_t AdapterIterator::Broadcast(const Core::NodeId& address)
    {

        //TODO: Needs implementation
        ASSERT(false);

        return (Core::ERROR_BAD_REQUEST);
    }

#elif defined(__POSIX__)

    static IPNode ToIPNode(const bool IPV6, const struct rtattr* rtatp, const uint8_t prefixlen) {

        IPNode result;

        /* Here we hit the fist chunk of the message. Time to validate the    *
        * the type. For more info on the different types see man(7) rtnetlink*
        * The table below is taken from man pages.                           *
        * Attributes                                                         *
        * rta_type        value type             description                 *
        * -------------------------------------------------------------      *
        * IFA_UNSPEC      -                      unspecified.                *
        * IFA_ADDRESS     raw protocol address   interface address           *
        * IFA_LOCAL       raw protocol address   local address               *
        * IFA_LABEL       asciiz string          name of the interface       *
        * IFA_BROADCAST   raw protocol address   broadcast address.          *
        * IFA_ANYCAST     raw protocol address   anycast address             *
        * IFA_CACHEINFO   struct ifa_cacheinfo   Address information.        */
        if ( ((RTA_PAYLOAD(rtatp) == 4) && (IPV6 == false)) || ((RTA_PAYLOAD(rtatp) == 6) && (IPV6 == true)) ) {
            switch (rtatp->rta_type) {
            case IFA_CACHEINFO: {
                // const struct ifa_cacheinfo* cache_info = reinterpret_cast<const struct ifa_cacheinfo *>(RTA_DATA(rtatp));
                // cache_info->ifa_valid == InfiniteLifeTime)

                // cache_info->ifa_prefered == InfiniteLifeTime)
                break;
            }
            case IFA_ADDRESS: /* POINT-TO-POINT destination address */
                if (IPV6 == false)
                    result = IPNode(*reinterpret_cast<const struct in_addr *>(RTA_DATA(rtatp)), prefixlen);
                else 
                    result = IPNode(*reinterpret_cast<const struct in6_addr*>(RTA_DATA(rtatp)), prefixlen);
                break;
            case IFA_LOCAL:
                if (IPV6 == false)
                    result = IPNode(*reinterpret_cast<const struct in_addr *>(RTA_DATA(rtatp)), prefixlen);
                else 
                    result = IPNode(*reinterpret_cast<const struct in6_addr*>(RTA_DATA(rtatp)), prefixlen);
                break;
            case IFA_BROADCAST:
                if (IPV6 == false)
                    result = IPNode(*reinterpret_cast<const struct in_addr *>(RTA_DATA(rtatp)), prefixlen);
                else 
                    result = IPNode(*reinterpret_cast<const struct in6_addr*>(RTA_DATA(rtatp)), prefixlen);
                break;
            case IFA_ANYCAST:
                if (IPV6 == false)
                    result = IPNode(*reinterpret_cast<const struct in_addr *>(RTA_DATA(rtatp)), prefixlen);
                else 
                    result = IPNode(*reinterpret_cast<const struct in6_addr*>(RTA_DATA(rtatp)), prefixlen);
                break;
            case IFA_MULTICAST:
                if (IPV6 == false)
                    result = IPNode(*reinterpret_cast<const struct in_addr *>(RTA_DATA(rtatp)), prefixlen);
                else 
                    result = IPNode(*reinterpret_cast<const struct in6_addr*>(RTA_DATA(rtatp)), prefixlen);
                break;
            case IFA_LABEL:
                //   _name = string(reinterpret_cast<const char*>(RTA_DATA(rtatp)), (RTA_PAYLOAD(rtatp) - 1));
                break;
                /* Only RPI newer kernels work with this flag. Skip it for now, it is not used. Yet!!!! 
                case IFA_FLAGS: 
                printf("Flags: %X\n", *reinterpret_cast<const uint32_t*>(RTA_DATA(rtatp)));
                break;
                */
            default:
                TRACE_L1("ToIPNode: Not handling: %d\n", rtatp->rta_type);
                break;
            }
        }

        return (result);
    }

    template <const bool ADD>
    class IPAddressModifyType : public Netlink {
    public:
        IPAddressModifyType() = delete;
        IPAddressModifyType(const IPAddressModifyType<ADD>&) = delete;
        IPAddressModifyType<ADD>& operator=(const IPAddressModifyType<ADD>&) = delete;

        IPAddressModifyType(Network& network, const IPNode& address)
            : _network(network)
            , _node(address)
        {
        }
        ~IPAddressModifyType() override = default;

    private:
        uint16_t Write(uint8_t stream[], const uint16_t length) const override
        {
            uint16_t result = sizeof(struct ifaddrmsg) + 2 * (RTA_LENGTH(_node.Type() == NodeId::TYPE_IPV6 ? 16 : 4));

            ASSERT(length >= result);
            ::memset(stream, 0, result);

            Flags(ADD == true ? NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK : NLM_F_REQUEST | NLM_F_ACK);
            Type(ADD == true ? RTM_NEWADDR : RTM_DELADDR);

            struct ifaddrmsg* message(reinterpret_cast<struct ifaddrmsg*>(stream));
            message->ifa_family = (_node.Type() == NodeId::TYPE_IPV6 ? AF_INET6 : AF_INET);
            message->ifa_prefixlen = _node.Mask();
            message->ifa_index = _network.Id();
            message->ifa_flags = IFA_F_PERMANENT;
            message->ifa_scope = RT_SCOPE_UNIVERSE;

            const uint8_t* data = reinterpret_cast<const uint8_t*>(&static_cast<const struct sockaddr*>(_node)->sa_data[2]);

            struct rtattr* attribs(reinterpret_cast<struct rtattr*>(stream + sizeof(struct ifaddrmsg)));
            attribs->rta_type = IFA_LOCAL;
            attribs->rta_len = (_node.Type() == NodeId::TYPE_IPV6 ? RTA_LENGTH(16) : RTA_LENGTH(4));
            memcpy(RTA_DATA(attribs), data, _node.Type() == NodeId::TYPE_IPV6 ? 16 : 4);

            attribs = reinterpret_cast<struct rtattr*>(stream + sizeof(struct ifaddrmsg) + (RTA_LENGTH(_node.Type() == NodeId::TYPE_IPV6 ? 16 : 4)));
            attribs->rta_type = IFA_ADDRESS;
            attribs->rta_len = (_node.Type() == NodeId::TYPE_IPV6 ? RTA_LENGTH(16) : RTA_LENGTH(4));
            memcpy(RTA_DATA(attribs), data, _node.Type() == NodeId::TYPE_IPV6 ? 16 : 4);

            return (result);
        }
        uint16_t Read(const uint8_t stream[], const uint16_t length) override
        {
            if (Type() == NLMSG_ERROR) {
                const nlmsgerr* error = reinterpret_cast<const nlmsgerr*>(stream);

                if (error->error != 0) {
                    TRACE_L1("IPAddressModify: Request failed with code %d", error->error);
                } 
                else if (ADD == true) {
                    _network.Added(_node);
                }
                else if (ADD == false) {
                    _network.Removed(_node);
                }
            }
            else if (Type() != NLMSG_DONE) {
                TRACE_L1("IPAddressModifyType: Read unexpected type: %d", Type());
            }

            return (length);
        }

    private:
        Network& _network;
        IPNode _node;
    };

    template <const bool ADD>
    class IPRouteModifyType : public Netlink {
    public:
        IPRouteModifyType() = delete;
        IPRouteModifyType(const IPRouteModifyType<ADD>&) = delete;
        IPRouteModifyType<ADD>& operator=(const IPRouteModifyType<ADD>&) = delete;

        IPRouteModifyType(Network& targetInterface, const IPNode& network, const NodeId& gateway)
            : _interface(targetInterface)
            , _network(network)
            , _gateway(gateway)
        {
        }
        ~IPRouteModifyType() override = default;

    private:
        uint16_t Write(uint8_t stream[], const uint16_t length) const override
        {
            Flags(ADD == true ? NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK | NLM_F_REPLACE : NLM_F_REQUEST | NLM_F_ACK);
            Type(ADD == true ? RTM_NEWROUTE : RTM_DELROUTE);

            struct rtmsg message;

            ::memset(&message, 0, sizeof(message));

            message.rtm_family = (_network.Type() == NodeId::TYPE_IPV6 ? AF_INET6 : AF_INET);
            message.rtm_dst_len = _network.Mask();
            message.rtm_src_len = 0;
            message.rtm_tos = 0;
            message.rtm_table = RT_TABLE_MAIN;
            message.rtm_protocol = RTPROT_BOOT;
            message.rtm_scope = RT_SCOPE_UNIVERSE;
            message.rtm_type = RTN_UNICAST;
            message.rtm_flags = 0;

            // Some send a gateway address that is outside
            // the local subnet. Kernel needs to be
            // explicitly told to use this route on the
            // interface specified by RTA_OID
            // if ((gateway.s_addr & netmask) != (address.s_addr & netmask.s_addr))
            // message->rtm_flags |= RTNH_F_ONLINK;

            Netlink::Parameters<struct rtmsg> parameters(message, stream, length);

            if (_network.Type() == NodeId::TYPE_IPV4) {
                ASSERT(_gateway.Type() == NodeId::TYPE_IPV4);
                const struct sockaddr_in* gateway = reinterpret_cast<const struct sockaddr_in*>(static_cast<const struct sockaddr*>(_gateway));
                const struct sockaddr_in* network = reinterpret_cast<const struct sockaddr_in*>(static_cast<const struct sockaddr*>(_network));
                parameters.Add(RTA_DST, network->sin_addr.s_addr);
                parameters.Add(RTA_GATEWAY, gateway->sin_addr.s_addr);
            } else if (_network.Type() == Core::NodeId::TYPE_IPV6) {
                ASSERT(_gateway.Type() == NodeId::TYPE_IPV6);
                const struct sockaddr_in6* gateway = reinterpret_cast<const struct sockaddr_in6*>(static_cast<const struct sockaddr*>(_gateway));
                const struct sockaddr_in6* network = reinterpret_cast<const struct sockaddr_in6*>(static_cast<const struct sockaddr*>(_network));
                parameters.Add(RTA_DST, network->sin6_addr.s6_addr);
                parameters.Add(RTA_GATEWAY, gateway->sin6_addr.s6_addr);
            } else {
                // What kind of network is this ???
                ASSERT(false);
            }

            parameters.Add(RTA_OIF, _interface.Id());

            TRACE_L1("Gateway: %s, Network: %s, Interface %d, result %d", _gateway.HostAddress().c_str(), _network.HostAddress().c_str(), _interface.Id(), parameters.Size());

            /*
                for (uint8_t teller = 0; teller < parameters.Size(); teller++) {
                    fprintf (stderr, "%02X:", stream[teller]);

                    if ((teller % 8) == 7) {
                        fprintf(stderr, "\n");
                    }
                }

                fprintf(stderr, "\n <<< ---- Tabel before this line\n"); fflush (stderr);
            */

            return (parameters.Size());
        }
        uint16_t Read(const uint8_t stream[], const uint16_t length) override
        {
            if ( (ADD == true) && (Type() == RTM_NEWROUTE) ) {

                ASSERT(_interface.Id() == reinterpret_cast<const struct ifaddrmsg*>(stream)->ifa_index);

            } else if ( (ADD == false) && (Type() == RTM_DELROUTE) ) {

                ASSERT(_interface.Id() == reinterpret_cast<const struct ifaddrmsg*>(stream)->ifa_index);

            } else if (Type() == NLMSG_ERROR) {
                const nlmsgerr* error = reinterpret_cast<const nlmsgerr*>(stream);

                if (error->error != 0) {
                    TRACE_L1("IPRouteModifyType: Request failed with code %d", error->error);
                } 
            }
            else if (Type() != NLMSG_DONE) {
                TRACE_L1("IPRouteModifyType: Read unexpected type: %d", Type());
            }

            return (length);
        }

    private:
        Network& _interface;
        IPNode _network;
        NodeId _gateway;
    };

    class IPNetworks {
    private:        
        using Map = std::map<uint32_t, Core::ProxyType<Network> >;
        using Element = std::pair<uint32_t, Core::ProxyType<Network> >;
        using Iterator = IteratorMapType<Map, const Core::ProxyType<const Network>&, uint32_t>;

        class Observer : public SocketDatagram {
        private:
            class Message : public Netlink {
            public:
                Message() = delete;
                Message(const Message&) = delete;
                Message& operator=(const Message&) = delete;

                Message(IPNetworks& parent)
                    : _parent(parent) {
                }
                ~Message() override = default;

            public:
                uint16_t Write(uint8_t stream[], const uint16_t length) const override
                {
                    return (0);
                }
                uint16_t Read(const uint8_t stream[], const uint16_t length) override
                {
                    const struct ifinfomsg* ifi = reinterpret_cast<const struct ifinfomsg*>(stream);

                    if (Type() == RTM_NEWLINK) {
                        _parent.Add(ifi->ifi_index, reinterpret_cast<const struct rtattr*>(IFLA_RTA(ifi)), length - sizeof(struct ifinfomsg));
                    } else if (Type() == RTM_DELLINK) {
                        _parent.Remove(ifi->ifi_index);
                    }

                    return (length);
                }

            private:
                IPNetworks& _parent;
            };

        public:
            Observer() = delete;
            Observer(const Observer&) = delete;
            Observer& operator=(const Observer&) = delete;

            Observer(IPNetworks& parent)
                : SocketDatagram(
                    true,
                    NodeId(NETLINK_ROUTE, 0, RTMGRP_LINK),
                    NodeId(),
                    64,
                    4000)
                , _parser(parent) {
                SocketDatagram::Open(Core::infinite);
            }
            ~Observer() override {
                Close(Core::infinite);
            }

        public:
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                return (0);
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                return (_parser.Deserialize(dataFrame, receivedSize));
            }
            void StateChange() override
            {
            }

        private:
            Message _parser;
        };

        class Channel {
        private:
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;

        public:
            Channel()
                : _adminLock()
            {
                _fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

                if (_fd != -1) {
                    sockaddr_nl netlinkSocket;

                    // setup local address & bind using this address
                    ::memset(&netlinkSocket, 0, sizeof(netlinkSocket));
                    netlinkSocket.nl_family = AF_NETLINK;
                    if (::bind(_fd, reinterpret_cast<struct sockaddr*>(&netlinkSocket), sizeof(netlinkSocket)) == -1) {
                        close(_fd);
                        _fd = -1;
                    }
                }
            }
            ~Channel()
            {
                if (_fd != -1) {
                    close(_fd);
                    _fd = -1;
                }
            }

        public:
            inline bool IsValid() const
            {
                return (_fd != -1);
            }
            uint32_t Exchange(const Netlink& outbound, Netlink& inbound)
            {
                uint8_t buffer[4 * 1024];
                uint32_t result = ERROR_BAD_REQUEST;

                _adminLock.Lock();

                uint16_t length = outbound.Serialize(buffer, static_cast<uint16_t>(sizeof(buffer)));

                if (send(_fd, buffer, length, 0) != -1) {

                    result = ERROR_GENERAL;
                    size_t amount;

                    while ((result == ERROR_GENERAL) && ((amount = recv(_fd, buffer, sizeof(buffer), 0)) > 0)) {
                        uint16_t handled = inbound.Deserialize(buffer, static_cast<uint16_t>(amount));

                        if (handled == static_cast<uint16_t>(~0)) {
                            result = ERROR_RPC_CALL_FAILED;
                        } else if (handled == static_cast<uint16_t>(amount)) {
                            result = ERROR_NONE;
                        }
                    }
                }

                _adminLock.Unlock();

                return (result);
            }

        private:
            CriticalSection _adminLock;
            int _fd;
        };
 
        class InterfacesFetch : public Netlink {
        public:
            InterfacesFetch() = delete;
            InterfacesFetch(const InterfacesFetch&) = delete;
            InterfacesFetch& operator=(const InterfacesFetch&) = delete;

            InterfacesFetch(IPNetworks& parent)
                : _parent(parent)
            {
            }
            ~InterfacesFetch() override = default;

        private:
            uint16_t Write(uint8_t stream[], const uint16_t length) const override
            {
                uint16_t result = sizeof(struct rtgenmsg);

                ASSERT(length >= result);
                ::memset(stream, 0, result);

                Flags(NLM_F_REQUEST | NLM_F_DUMP | NLM_F_ACK);
                Type(RTM_GETLINK);

                struct rtgenmsg* message(reinterpret_cast<struct rtgenmsg*>(stream));
                message->rtgen_family = AF_PACKET; /*  no preferred AF, we will get *all* interfaces */

                return (result);
            }
            uint16_t Read(const uint8_t stream[], const uint16_t length) override
            {
                if (Type() == RTM_NEWLINK) {
                    const struct ifinfomsg* iface = reinterpret_cast<const struct ifinfomsg*>(stream);
                    _parent.Add(iface->ifi_index, reinterpret_cast<const struct rtattr*>(IFLA_RTA(iface)), length - sizeof(struct ifinfomsg));
                } else if (Type() == NLMSG_ERROR) {
                    const nlmsgerr* error = reinterpret_cast<const nlmsgerr*>(stream);

                    if (error->error != 0) {
                        TRACE_L1("InterfacesFetch: Request failed with code %d", error->error);
                    } 
                } 
                else if (Type() != NLMSG_DONE) {
                    TRACE_L1("InterfacesFetch: Read unexpected type: %d", Type());
                }
                return (length);
            }

        private:
            IPNetworks& _parent;
        };

        template <const bool IPV6>
        class IPAddressFetchType : public Netlink {
        public:
            IPAddressFetchType() = delete;
            IPAddressFetchType(const IPAddressFetchType<IPV6>&) = delete;
            IPAddressFetchType<IPV6>& operator=(const IPAddressFetchType<IPV6>&) = delete;

            IPAddressFetchType(IPNetworks& parent)
                : _parent(parent)
            {
            }
            ~IPAddressFetchType() override = default;

        private:
            uint16_t Write(uint8_t stream[], const uint16_t length) const override
            {
                uint16_t result = sizeof(struct ifaddrmsg) + RTA_LENGTH(IPV6 ? 16 : 4);

                ASSERT(length >= result);
                ::memset(stream, 0, result);

                Flags(NLM_F_REQUEST | NLM_F_ROOT | NLM_F_ACK);
                Type(RTM_GETADDR);

                struct ifaddrmsg* message(reinterpret_cast<struct ifaddrmsg*>(stream));
                message->ifa_family = (IPV6 ? AF_INET6 : AF_INET);
                message->ifa_index = 0;

                struct rtattr* attribs(reinterpret_cast<struct rtattr*>(stream + sizeof(struct ifaddrmsg)));
                attribs->rta_len = (IPV6 ? RTA_LENGTH(16) : RTA_LENGTH(4));
                attribs->rta_type = 0;

                return (result);
            }
            uint16_t Read(const uint8_t stream[], const uint16_t length) override
            {
                if ( (Type() == RTM_GETADDR) || (Type() == RTM_NEWADDR) || (Type() == RTM_DELADDR) ) {
                    const struct ifaddrmsg* rtmp = reinterpret_cast<const struct ifaddrmsg*>(stream);
                    
                    const struct rtattr* rtatp = reinterpret_cast<const struct rtattr*>(IFA_RTA(rtmp));
                    const uint8_t prefixlen = static_cast<uint8_t>(rtmp->ifa_prefixlen);
                    uint16_t rtattrlen = length - sizeof(struct ifaddrmsg);

                    for (; RTA_OK(rtatp, rtattrlen); rtatp = RTA_NEXT(rtatp, rtattrlen)) {

                        if ((rtatp->rta_type == IFA_ADDRESS) || (rtatp->rta_type == IFA_LOCAL)) {
                            IPNode result (ToIPNode(IPV6, rtatp, prefixlen));

                            if (result.IsValid() == true) {
                                if (Type() != RTM_DELADDR) {
                                    _parent.Added(rtmp->ifa_index, result);
                                }
                                else {
                                    _parent.Removed(rtmp->ifa_index, result);
                                }
                            }
                        }
                    }
                } else if (Type() == NLMSG_ERROR) {
                    const nlmsgerr* error = reinterpret_cast<const nlmsgerr*>(stream);

                    if (error->error != 0) {
                        TRACE_L1("IPAddressFetchType: Request failed with code %d", error->error);
                    } 
                }
                else if (Type() != NLMSG_DONE) {
                    TRACE_L1("IPAddressFetchType: Read unexpected type: %d", Type());
                }

                return (length);
            }

        private:
            IPNetworks& _parent;
        };

    protected:
        IPNetworks()
            : _adminLock()
            , _channel(ProxyType<Channel>::Create())
            , _networks()
            , _observer(*this)
            , _observers()
        {
            ASSERT(IsValid());

            InterfacesFetch ifInfo(*this);

            uint32_t result = _channel->Exchange(ifInfo, ifInfo);

            if (result != ERROR_NONE) {
                TRACE_L1("Could not load the base set of interfaces.");
            }
            else {
                IPAddressFetchType<false> ipv4(*this);
            
                if (_channel->Exchange(ipv4, ipv4) != ERROR_NONE) {
                    TRACE_L1("IPNetworks(): Could not read ipv4 Nodes");
                }
                else {
                    IPAddressFetchType<false> ipv6(*this);

                    if (_channel->Exchange(ipv6, ipv6) != ERROR_NONE) {
                        TRACE_L1("IPNetworks(): Could not read ipv6 Nodes");
                    }
                }
            }
        }

    public:
        IPNetworks(const IPNetworks&) = delete;
        IPNetworks& operator=(const IPNetworks&) = delete;

        ~IPNetworks() = default;

        static IPNetworks& Instance()
        {
            static IPNetworks& _instance = SingletonType<IPNetworks>::Instance();
            return (_instance);
        }

    public:
        inline bool IsValid() const
        {
            return ((_channel.IsValid()) && (_channel->IsValid() == true));
        }
        void Load (std::list<Core::ProxyType<Network> >& list) {
            _adminLock.Lock();
            for (const Element& element : _networks) {
                list.push_back(element.second);
            }
            _adminLock.Unlock();
        }
        void Register(AdapterObserver::INotification* client) {
            _adminLock.Lock();
            std::list<AdapterObserver::INotification*>::iterator index (std::find(_observers.begin(), _observers.end(), client));
            if (index == _observers.end()) {
                _observers.push_back(client);
            }
            _adminLock.Unlock();
        }
        void Unregister(AdapterObserver::INotification* client) {
            _adminLock.Lock();
            std::list<AdapterObserver::INotification*>::iterator index (std::find(_observers.begin(), _observers.end(), client));
            if (index != _observers.end()) {
                _observers.erase(index);
            }
            _adminLock.Unlock();            
        }
        inline uint32_t Exchange(const Netlink& outbound, Netlink& inbound) {
            return(_channel->Exchange(outbound, inbound));
        }

    private:
        void Add(const uint32_t id, const struct rtattr* data, const uint16_t length) {
            string interfaceName;

            _adminLock.Lock();

            Map::iterator index (_networks.find(id));
            if (index == _networks.end()) {
                Core::ProxyType<Network> newNetwork (Core::ProxyType<Network>::Create(id, data, length));
                _networks.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(newNetwork));
                interfaceName = newNetwork->Name();
            }
            else {
                index->second->Update(data, length);
                interfaceName = index->second->Name();
            }

            Notify(interfaceName);

            _adminLock.Unlock();            
        }
        void Remove(const uint32_t id) {
            _adminLock.Lock();
            Map::iterator index (_networks.find(id));
            if (index != _networks.end()) {
                string interfaceName(index->second->Name());
                _networks.erase(index);
                Notify(interfaceName);
            }
            _adminLock.Unlock();            
        }
        void Notify(const string& name) {
            for (AdapterObserver::INotification* callback : _observers) {
                callback->Event(name);
            }
        }
        void Added(const uint32_t id, const Core::IPNode& node) {
            Map::iterator index(_networks.find(id));
            if (index != _networks.end()) {
                index->second->Added(node);
            }
        }
        void Removed(const uint32_t id, const Core::IPNode& node) {
            Map::iterator index(_networks.find(id));
            if (index != _networks.end()) {
                index->second->Removed(node);
            }
        }

    private:
        CriticalSection _adminLock;
        ProxyType<Channel> _channel;
        Map _networks;
        Observer _observer;
        std::list<AdapterObserver::INotification*> _observers;
    };

    Network::Network(const uint32_t index, const struct rtattr* iface, const uint32_t length)
        : _adminLock()
        , _index(index)
        , _name()
        , _ipv4Nodes()
        , _ipv6Nodes()
    {
        Update(iface, length);
    }

    bool Network::IsUp() const {

        bool result = false;
        int sockfd;

        sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (sockfd >= 0) {

            struct ifreq ifr;

            result = Core::ERROR_NONE;

            ::memset(&ifr, 0, sizeof ifr);

            _adminLock.Lock();

            ::strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ);

            _adminLock.Unlock();

            ::ioctl(sockfd, SIOCGIFFLAGS, &ifr);

            result = ((ifr.ifr_flags & IFF_UP) == IFF_UP);
            ::close(sockfd);
        }

        return (result);
    }

    bool Network::IsRunning() const {

        bool result = false;
        int sockfd;

        sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (sockfd >= 0) {

            struct ifreq ifr;

            result = Core::ERROR_NONE;

            ::memset(&ifr, 0, sizeof ifr);

            _adminLock.Lock();

            ::strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ);

            _adminLock.Unlock();

            ::ioctl(sockfd, SIOCGIFFLAGS, &ifr);

            result = ((ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING));
            ::close(sockfd);
        }

        return (result);
    }

    uint32_t Network::Up(const bool enabled)
    {
        uint32_t result = Core::ERROR_GENERAL;
        int sockfd;

        sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (sockfd >= 0) {

            struct ifreq ifr;

            result = Core::ERROR_NONE;

            ::memset(&ifr, 0, sizeof ifr);

            _adminLock.Lock();

            ::strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ);

            _adminLock.Unlock();

            ::ioctl(sockfd, SIOCGIFFLAGS, &ifr);

            if ((enabled == true) && ((ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING))) {
                ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
                ::ioctl(sockfd, SIOCSIFFLAGS, &ifr);
            } else if ((enabled == false) && ((ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) != 0)) {
                ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
                ::ioctl(sockfd, SIOCSIFFLAGS, &ifr);
            }

            ::close(sockfd);
        }

        return (result);
    }

    uint32_t Network::Broadcast(const Core::NodeId& address)
    {
        uint32_t result = Core::ERROR_GENERAL;
        int sockfd;
        struct ifreq ifr;

        sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (sockfd >= 0) {

            result = Core::ERROR_NONE;

            ::memset(&ifr, 0, sizeof ifr);

            _adminLock.Lock();

            ::strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ);

            _adminLock.Unlock();

            ifr.ifr_flags = IFF_BROADCAST;
            ::memcpy(&ifr.ifr_broadaddr, static_cast<const struct sockaddr*>(address), sizeof(struct sockaddr));

            if (ioctl(sockfd, SIOCSIFBRDADDR, &ifr) >= 0) {
                result = Core::ERROR_NONE;
            }

            ::close(sockfd);
        }

        return (result);
    }

    uint32_t Network::Add(const IPNode& address)
    {
        IPAddressModifyType<true> modifier(*this, address);

        return (IPNetworks::Instance().Exchange(modifier, modifier));
    }

    uint32_t Network::Delete(const IPNode& address)
    {
        IPAddressModifyType<false> modifier(*this, address);

        return (IPNetworks::Instance().Exchange(modifier, modifier));
    }

    uint32_t Network::Gateway(const IPNode& network, const NodeId& gateway)
    {
        IPRouteModifyType<true> modifier(*this, network, gateway);

        return (IPNetworks::Instance().Exchange(modifier, modifier));
    }

    NodeId Network::Broadcast() const
    {
        Core::NodeId result;

        struct sockaddr_ll target;

        memset(&target, 0, sizeof(target));
        target.sll_family   = PF_PACKET;
        target.sll_protocol = htons(ETH_P_IP);
        target.sll_ifindex  = _index;
        target.sll_hatype   = 0;
        target.sll_pkttype  = PACKET_BROADCAST;
        target.sll_halen    = ETH_ALEN;

        // Fill with broadcast addr
        memset(target.sll_addr, 0xff, ETH_ALEN);

        return NodeId(target);
    }
            
    void Network::Update(const struct rtattr* rtatp, const uint16_t length)
    {
        uint16_t rtattrlen = length;

        for (; (rtattrlen <= length) && RTA_OK(rtatp, rtattrlen); rtatp = RTA_NEXT(rtatp, rtattrlen)) {

            /* Here we hit the fist chunk of the message. Time to validate the    *
                * the type. For more info on the different types see man(7) rtnetlink*
                * The table below is taken from man pages.                           *
                * Attributes                                                         *
                * rta_type        value type             description                 *
                * -------------------------------------------------------------      *
                * IFLA_UNSPEC     -                      unspecified.                *
                * IFLA_ADDRESS    hardware address       interface L2 address        *
                * IFLA_BROADCAST  hardware address       L2 broadcast address.       *
                * IFLA_IFNAME     asciiz string          Device name.                *
                * IFLA_MTU        unsigned int           MTU of the device.          *
                * IFLA_LINK       int                    Link type.                  *
                * IFLA_QDISC      asciiz string          Queueing discipline.        *
                * IFLA_STATS      see below              Interface Statistics.       */

            switch (rtatp->rta_type) {
            case IFLA_ADDRESS: {
                _adminLock.Lock();
                ::memcpy(_MAC, RTA_DATA(rtatp), sizeof(_MAC));
                _adminLock.Unlock();
                break;
            }
            case IFLA_IFNAME: {
                _adminLock.Lock();
                string newName(reinterpret_cast<const char*>(RTA_DATA(rtatp)), (RTA_PAYLOAD(rtatp) - 1));
                if (newName != _name) {
                    _name = newName;
                }
                _adminLock.Unlock();
                break;
            }
            case IFLA_BROADCAST:
            case IFLA_MTU:
            case IFLA_LINK:
            case IFLA_QDISC:
            case IFLA_STATS:
            case IFLA_COST:
            case IFLA_PRIORITY:
            case IFLA_MASTER:
            case IFLA_WIRELESS:
            case IFLA_PROTINFO:
            case IFLA_TXQLEN:
            case IFLA_MAP:
            case IFLA_WEIGHT:
            case IFLA_OPERSTATE:
            case IFLA_LINKMODE:
            case IFLA_LINKINFO:
            case IFLA_NET_NS_PID:
            case IFLA_IFALIAS:
            case IFLA_NUM_VF: /* Number of VFs if device is SR-IOV PF */
            case IFLA_VFINFO_LIST:
            case IFLA_STATS64:
            case IFLA_VF_PORTS:
            case IFLA_PORT_SELF:
                /* Next options are available on newer kernels but as these options are no used at
                    the moment, will keep them out to support all kernels from 3.3 and higher !!!

                    case IFLA_AF_SPEC:
                    case IFLA_GROUP:             / * Group the device belongs to * /
                    case IFLA_NET_NS_FD:
                    case IFLA_EXT_MASK:          / * Extended info mask, VFs, etc * /

                    case IFLA_PROMISCUITY:       Promiscuity count: > 0 means acts PROMISC 
                    case IFLA_NUM_TX_QUEUES:
                    case IFLA_NUM_RX_QUEUES:
                    case IFLA_CARRIER:
                    case IFLA_PHYS_PORT_ID:
                    case IFLA_CARRIER_CHANGES:
                    case IFLA_PHYS_SWITCH_ID:
                    case IFLA_LINK_NETNSID:
                    case IFLA_PHYS_PORT_NAME:
                    case IFLA_PROTO_DOWN:
                */
                {
                    break;
                }
            default:
                // TRACE_L1("Unknown option encountered: %d", rtatp->rta_type);
		break;
            }
        }
    }

    AdapterIterator::AdapterIterator()
        : _reset(true)
        , _list()
        , _index() {
        // Time to get the current set of networks..
        IPNetworks::Instance().Load(_list);
        _index = _list.begin();
    }

    AdapterIterator::AdapterIterator(const string& name) 
        : AdapterIterator() {
        while ( (Next() == true) && (Name() != name) ) { /* Intentionally left empty */ }
    }

    AdapterIterator::AdapterIterator(const AdapterIterator& copy)
        : AdapterIterator() {
        if (copy.IsValid() == true) {
            const string name (copy.Name());
            while ( (Next() == true) && (Name() != name) ) { /* Intentionally left empty */ }
        }
    }

    AdapterIterator& AdapterIterator::operator=(const AdapterIterator& RHS)
    {
        _reset = true;
        _list = RHS._list;
        _index = _list.begin();

        if (RHS.IsValid()) {
            string name (RHS.Name());
            while ( (Next() == true) && (Name() != name) ) { /* Intentionally left empty */ }
        }

        return (*this);
    }

#endif

    AdapterObserver::AdapterObserver(INotification* callback)
        : _callback(callback)
    {
#ifdef __WINDOWS__
        //IoWMIOpenBlock(&GUID_NDIS_STATUS_LINK_STATE, WMIGUID_NOTIFICATION, . . .);
        //IoWMISetNotificationCallback(. . ., Callback, . . .);

        //void Callback(PWNODE_HEADER wnode, . . .)
        //{
        //	auto instance = (PWNODE_SINGLE_INSTANCE)wnode;
        //	auto header = (PNDIS_WMI_EVENT_HEADER)((PUCHAR)instance +
        //		instance->DataBlockOffset + sizeof(ULONG));
        //	auto linkState = (PNDIS_LINK_STATE)(header + 1);

        //	switch (linkState->MediaConnectState)
        //	{
        //	case MediaConnectStateConnected:
        //		. . .
        //	}
        //}

#endif
    }

    AdapterObserver::~AdapterObserver()
    {
        Close();
    }

    uint32_t AdapterObserver::Open() {
#ifndef __WINDOWS__
        IPNetworks::Instance().Register(_callback);
#endif
        return (Core::ERROR_NONE);
    }

    uint32_t AdapterObserver::Close() {
#ifndef __WINDOWS__
        IPNetworks::Instance().Unregister(_callback);
#endif
        return (Core::ERROR_NONE);
    }

    bool AdapterIterator::HasMAC() const
    {
        uint8_t index = 0;
        uint8_t mac[6];
        MACAddress(mac, sizeof(mac));

        while ((index < sizeof(mac)) && (mac[index] == 0)) {
            index++;
        }

        return (index < sizeof(mac));
    }
}
}
