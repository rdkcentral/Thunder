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
 
// NodeId.h: interface for the NodeId class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __NODEID_H
#define __NODEID_H

#include "Module.h"
#include "Portability.h"

#ifdef __WINDOWS__
#include <Ws2ipdef.h>
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib")
#endif

#ifdef __LINUX__
#include <linux/netlink.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#endif

#ifdef __CORE_BLUETOOTH_SUPPORT__
#include <../include/bluetooth/bluetooth.h>
#include <../include/bluetooth/hci.h>
#include <../include/bluetooth/l2cap.h>
#else
#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH 60000
#endif
#endif

#include "TextFragment.h"

namespace WPEFramework {
namespace Core {
    class EXTERNAL NodeId {
    private:
#ifndef __WINDOWS__
        struct netlink_extended : public sockaddr_nl {
            uint32_t nl_destination;
        };
        struct domain_extended : public sockaddr_un {
            uint16_t un_access;
        };
#endif
        struct ipv4_extended : public sockaddr_in {
            uint16_t in_protocol;
        };
        struct ipv6_extended : public sockaddr_in6 {
            uint16_t in_protocol;
        };
#ifdef __CORE_BLUETOOTH_SUPPORT__
        struct bluetooth_extended : public sockaddr_l2 {
            uint16_t l2_type;
        };
#endif

    public:
        enum enumType {
            TYPE_UNSPECIFIED = AF_UNSPEC,
            TYPE_IPV4 = AF_INET,
            TYPE_IPV6 = AF_INET6,
            TYPE_DOMAIN = AF_UNIX,
            TYPE_NETLINK = AF_NETLINK,
            TYPE_BLUETOOTH = AF_BLUETOOTH,
            TYPE_PACKET = AF_PACKET,
            TYPE_EMPTY = 0xFF
        };

        union SocketInfo {
#ifdef __WINDOWS__
            ADDRESS_FAMILY FamilyType;
#else
            sa_family_t FamilyType;
#endif
            struct ipv4_extended IPV4Socket;
            struct ipv6_extended IPV6Socket;
#ifndef __WINDOWS__
            struct domain_extended DomainSocket;
            struct netlink_extended NetlinkSocket;
            struct sockaddr_ll RawSocket;
#endif
#ifdef __CORE_BLUETOOTH_SUPPORT__
            struct sockaddr_hci BTSocket;
            struct bluetooth_extended L2Socket;
#endif

        public:
            uint32_t Extension() const
            {
                switch (FamilyType) {
                case AF_INET:
                    return (IPV4Socket.in_protocol);
                    break;

                case AF_INET6:
                    return(IPV6Socket.in_protocol);

    #ifdef __CORE_BLUETOOTH_SUPPORT__
                case AF_BLUETOOTH:
                    return (L2Socket.l2_type);
                    break;
    #endif
    #ifndef __WINDOWS__
                case AF_NETLINK:
                    return (NetlinkSocket.nl_destination);
                    break;
    #endif
                default:
                    break;
                }

                return (0);
            }
            void Extension(const uint32_t extension)
            {
                switch (FamilyType) {
                case TYPE_IPV4:
                    IPV4Socket.in_protocol = extension;
                    break;

                case TYPE_IPV6:
                    IPV6Socket.in_protocol = extension;
                    break;

    #ifdef __CORE_BLUETOOTH_SUPPORT__
                case AF_BLUETOOTH:
                    L2Socket.l2_type = extension;
                    break;
    #endif
    #ifndef __WINDOWS__
            case AF_NETLINK:
                    NetlinkSocket.nl_destination = extension;
                    break;
    #endif
                default:
                    break;
                }
            }
        };

        static bool IsIPV6Enabled()
        {
            return (m_isIPV6Enabled);
        }
        static void ClearIPV6Enabled()
        {
            m_isIPV6Enabled = false;
        }

        //------------------------------------------------------------------------
        // Constructors/Destructors
        //------------------------------------------------------------------------
    public:
        NodeId();
        NodeId(const struct sockaddr_in& rInfo, const uint32_t protocol = 0);
        NodeId(const struct in_addr& rInfo, const uint32_t protocol = 0);
        NodeId(const struct sockaddr_in6& rInfo, const uint32_t protocol = 0);
        NodeId(const struct in6_addr& rInfo, const uint32_t protocol = 0);
#ifndef __WINDOWS__
        NodeId(const struct sockaddr_un& rInfo, const uint16_t access = ~0);
        NodeId(const uint32_t destination, const pid_t pid, const uint32_t groups);
        NodeId(const struct sockaddr_ll& rInfo);
        NodeId(const uint16_t interfaceIndex, const uint16_t protocol, const uint8_t pkgtype, const uint8_t haType, const uint8_t length, const uint8_t* address);
        NodeId(const TCHAR interfaceName[], const uint16_t protocol, const uint8_t pkgType, const uint8_t haType, const uint8_t length, const uint8_t* address);
#endif

#ifdef __CORE_BLUETOOTH_SUPPORT__
        NodeId(const uint16_t device, const uint16_t channel);
        NodeId(const bdaddr_t& address, const uint8_t addressType, const uint16_t cid, const uint16_t psm);
#endif
        NodeId(const TCHAR strHostName[], const enumType defaultType = TYPE_UNSPECIFIED, const uint32_t protocol = 0);
        NodeId(const TCHAR strHostName[], const uint16_t nPortNumber, const enumType defaultType = TYPE_UNSPECIFIED, const uint32_t protocol = 0);
        NodeId(const NodeId& rInfo);
        NodeId(const NodeId& rInfo, const uint16_t portNumber);

        //------------------------------------------------------------------------
        // Public Methods
        //------------------------------------------------------------------------
    public:
        inline uint32_t Extension() const
        {
            return (m_structInfo.Extension());
        }

        inline uint16_t Rights() const
        {
#ifndef __WINDOWS__
            return (Type() == TYPE_DOMAIN ? m_structInfo.DomainSocket.un_access : 0);
#else
            return (0);
#endif
        }

        NodeId::enumType Type() const
        {
            return (static_cast<NodeId::enumType>(m_structInfo.FamilyType));
        }
        inline uint16_t PortNumber() const
        {
#ifdef __CORE_BLUETOOTH_SUPPORT__
            return (Type() == TYPE_BLUETOOTH ? m_structInfo.BTSocket.hci_dev : ntohs(m_structInfo.IPV4Socket.sin_port));
#else
            return (ntohs(m_structInfo.IPV4Socket.sin_port));
#endif
        }

#ifndef __WINDOWS__
        inline uint16_t HardwareType() const {
            return (m_structInfo.RawSocket.sll_hatype);
        }
        inline void HardwareType(const uint16_t type) {
            m_structInfo.RawSocket.sll_hatype = type;
        }
#endif

        inline void PortNumber(const uint16_t portNumber)
        {
            m_structInfo.IPV4Socket.sin_port = ntohs(portNumber);
        }
        inline bool IsValid() const
        {
            return ((Type() != TYPE_UNSPECIFIED) && (Type() != TYPE_EMPTY));
        }
        inline bool IsEmpty() const
        {
            return (Type() == TYPE_EMPTY);
        }
        inline unsigned short Size() const
        {
#ifndef __WINDOWS__
            return (m_structInfo.FamilyType == AF_INET ? sizeof(struct sockaddr_in) : 
                   (m_structInfo.FamilyType == AF_INET6 ? sizeof(struct sockaddr_in6) : 
                   (m_structInfo.FamilyType == AF_NETLINK ? sizeof(struct sockaddr_nl) :
                   (m_structInfo.FamilyType == AF_PACKET ? sizeof(struct sockaddr_ll) :
#ifdef __CORE_BLUETOOTH_SUPPORT__
                   (m_structInfo.BTSocket.hci_family == AF_BLUETOOTH ? (m_structInfo.L2Socket.l2_type == BTPROTO_HCI ? sizeof(struct sockaddr_hci) : sizeof(struct sockaddr_l2)) : sizeof(struct sockaddr_un))))));
#else
                    sizeof(struct sockaddr_un)))));
#endif

#else
#ifdef __CORE_BLUETOOTH_SUPPORT__
        return (m_structInfo.IPV4Socket.sin_family   == AF_INET      ? sizeof(struct sockaddr_in)  : 
               (m_structInfo.IPV6Socket.sin6_family  == AF_INET6     ? sizeof(struct sockaddr_in6) : 
               (m_structInfo.BTSocket.hci_family     == AF_BLUETOOTH ? (m_structInfo.L2Socket.l2_type == BTPROTO_HCI ? sizeof(struct sockaddr_hci) : sizeof(struct sockaddr_l2)))));
#else
            return (m_structInfo.IPV4Socket.sin_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
#endif
#endif
        }
        inline operator const struct sockaddr*() const
        {
            return (reinterpret_cast<const struct sockaddr*>(&(m_structInfo)));
        }
        inline operator const union SocketInfo&() const
        {
            return (m_structInfo);
        }
        inline bool operator!=(const NodeId& rInfo) const
        {
            return (!NodeId::operator==(rInfo));
        }
        inline bool IsUnicast() const
        {
            return ((IsMulticast() == false) && (IsLocalInterface() == false) && (IsAnyInterface() == false));
        }
        inline string Group() const
        {
            return (m_group);
        }

        string HostName() const;

        NodeId AnyInterface() const;
        string HostAddress() const;
        string QualifiedName() const;
        NodeId Origin() const;
        bool IsLocalInterface() const;
        bool IsAnyInterface() const;
        bool IsMulticast() const;
        uint8_t DefaultMask() const;

        bool operator==(const NodeId& rInfo) const;

        NodeId& operator=(const NodeId& rInfo);
        NodeId& operator=(const struct sockaddr_in& rInfo);
        NodeId& operator=(const struct sockaddr_in6& rInfo);
        NodeId& operator=(const union SocketInfo& rInfo); 

#ifndef __WINDOWS__
        NodeId& operator=(const struct sockaddr_un& rInfo);
        NodeId& operator=(const struct sockaddr_nl& rInfo);
        NodeId& operator=(const struct sockaddr_ll& rInfo);
#endif
#ifdef __CORE_BLUETOOTH_SUPPORT__
        NodeId& operator=(const struct sockaddr_hci& rInfo);
        NodeId& operator=(const struct sockaddr_l2& rInfo);
#endif

        //------------------------------------------------------------------------
        // Protected Methods
        //------------------------------------------------------------------------
    protected:
        void Resolve(const TextFragment& strName, const enumType defaultType);

    private:
        friend class IPNode;
        inline operator struct sockaddr*()
        {
            return (reinterpret_cast<struct sockaddr*>(&(m_structInfo)));
        }

        string m_group;
        mutable string m_hostName;
        SocketInfo m_structInfo;
        static bool m_isIPV6Enabled;
    };

    class EXTERNAL IPNode : public Core::NodeId {
    public:
        IPNode()
            : Core::NodeId()
            , _mask(0)
        {
        }
        IPNode(const NodeId& node, const uint8_t mask)
            : Core::NodeId(node)
            , _mask(mask)
        {
        }
        IPNode(const IPNode& copy)
            : Core::NodeId(copy)
            , _mask(copy._mask)
        {
        }
        ~IPNode()
        {
        }

    public:
        uint8_t Mask() const
        {
            return (_mask == static_cast<uint8_t>(~0) ? (Type() == Core::NodeId::TYPE_IPV4 ? 32 : 128) : _mask);
        }
        inline bool operator==(const IPNode& rInfo) const
        {
            return ((rInfo._mask == _mask) && (NodeId::operator==(rInfo)));
        }
        inline bool operator!=(const IPNode& rInfo) const
        {
            return (!IPNode::operator==(rInfo));
        }
        inline bool operator==(const NodeId& rInfo) const
        {
            return (NodeId::operator==(rInfo));
        }
        inline bool operator!=(const NodeId& rInfo) const
        {
            return (!IPNode::operator==(rInfo));
        }

        NodeId Broadcast() const;

    private:
        uint8_t _mask;
    };
}
} // namespace Core

#endif // __NODEID_H
