// NodeId.h: interface for the NodeId class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __NODEID_H
#define __NODEID_H

#include "Module.h"

#ifdef __WIN32__
#include <winsock2.h>
#include <Ws2ipdef.h>
#pragma comment(lib, "wsock32.lib")
#endif

#ifdef __UNIX__
#include <netinet/in.h>
#include <sys/un.h>
#include <linux/netlink.h>
#endif

#include "TextFragment.h"

namespace WPEFramework {
namespace Core {
class EXTERNAL NodeId {
private:
    #ifndef __WIN32__
    struct netlink_extended : public sockaddr_nl {
        uint32_t nl_destination;
    };
    #endif
public:
    enum enumType {
        TYPE_UNSPECIFIED = AF_UNSPEC,
        TYPE_IPV4        = AF_INET,
        TYPE_IPV6        = AF_INET6,
        TYPE_DOMAIN      = AF_UNIX,
        TYPE_NETLINK     = AF_NETLINK,
        TYPE_EMPTY       = 0xFF
    };

    union SocketInfo {
        struct sockaddr_in      IPV4Socket;
        struct sockaddr_in6     IPV6Socket;
        #ifndef __WIN32__
        struct sockaddr_un      DomainSocket;
        struct netlink_extended NetlinkSocket;
        #endif
    };

    static bool IsIPV6Enabled() {
        return (m_isIPV6Enabled);
    }
    static void ClearIPV6Enabled() {
        m_isIPV6Enabled = false;
    }

    //------------------------------------------------------------------------
    // Constructors/Destructors
    //------------------------------------------------------------------------
public:
    NodeId();
    NodeId(const struct sockaddr_in& rInfo);
    NodeId(const struct in_addr& rInfo);
    NodeId(const struct sockaddr_in6& rInfo);
    NodeId(const struct in6_addr& rInfo);
    #ifndef __WIN32__
    NodeId(const struct sockaddr_un& rInfo);
    NodeId(const uint32_t destination, const pid_t pid, const uint32_t groups);
    #endif
    NodeId(const TCHAR strHostName[], const enumType defaultType = TYPE_UNSPECIFIED);
    NodeId(const TCHAR strHostName[], const uint16_t nPortNumber, const enumType defaultType = TYPE_UNSPECIFIED);
    NodeId(const NodeId& rInfo);
    NodeId(const NodeId& rInfo, const uint16_t portNumber);

    //------------------------------------------------------------------------
    // Public Methods
    //------------------------------------------------------------------------
public:
    inline uint32_t Extension() const {
#ifndef __WIN32__
		return ( Type() == TYPE_NETLINK ? m_structInfo.NetlinkSocket.nl_destination : 0 );
#else
		return (0);
#endif
    }
    NodeId::enumType Type() const {
        return (static_cast<NodeId::enumType>(m_structInfo.IPV4Socket.sin_family));
    }
    inline uint16_t PortNumber() const {
        return (ntohs(m_structInfo.IPV4Socket.sin_port));
    }
    inline void PortNumber(const uint16_t portNumber) {
        m_structInfo.IPV4Socket.sin_port = ntohs(portNumber);
    }
    inline bool IsValid() const {
        return ( (Type() != TYPE_UNSPECIFIED) && (Type() != TYPE_EMPTY) );
    }
    inline bool IsEmpty() const {
        return (Type() == TYPE_EMPTY);
    }
    inline unsigned short Size() const {
        #ifndef __WIN32__
        return (m_structInfo.IPV4Socket.sin_family == AF_INET ? sizeof(struct sockaddr_in) : (m_structInfo.IPV6Socket.sin6_family == AF_INET6 ? sizeof(struct sockaddr_in6) : (m_structInfo.NetlinkSocket.nl_family == AF_NETLINK ? sizeof(struct sockaddr_nl) : sizeof(struct sockaddr_un))));
        #else
        return (m_structInfo.IPV4Socket.sin_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
        #endif
    }
    inline operator const struct sockaddr* () const {
        return (reinterpret_cast<const struct sockaddr*>(&(m_structInfo.IPV4Socket)));
    }
    inline operator const union SocketInfo& () const {
        return (m_structInfo);
    }
    inline bool operator!= (const NodeId& rInfo) const {
        return (!NodeId::operator== (rInfo));
    }
    inline bool IsUnicast() const {
        return ((IsMulticast() == false) && (IsLocalInterface() == false) && (IsAnyInterface() == false));
    }

    string              HostName() const;
    void                HostName(const TCHAR strHostName[]);

    NodeId		AnyInterface() const;
    string              HostAddress() const;
    string              QualifiedName() const;
    NodeId              Origin() const;
    bool                IsLocalInterface() const;
    bool                IsAnyInterface() const;
    bool                IsMulticast() const;
    uint8_t               DefaultMask() const;

    bool                operator== (const NodeId& rInfo) const;

    NodeId&             operator= (const NodeId& rInfo);
    NodeId&             operator= (const struct sockaddr_in& rInfo);
    NodeId&             operator= (const struct sockaddr_in6& rInfo);
    NodeId&             operator= (const union SocketInfo& rInfo);

    #ifndef __WIN32__
    NodeId&             operator= (const struct sockaddr_un& rInfo);
    NodeId&             operator= (const struct sockaddr_nl& rInfo);
    #endif


    //------------------------------------------------------------------------
    // Protected Methods
    //------------------------------------------------------------------------
protected:
    void                Resolve(const TextFragment& strName, const enumType defaultType);

private:
    friend class IPNode; 
    inline operator struct sockaddr* () {
        return (reinterpret_cast<struct sockaddr*>(&(m_structInfo.IPV4Socket)));
    }

    mutable string	m_hostName;
    SocketInfo		m_structInfo;
    static bool         m_isIPV6Enabled;
};

class EXTERNAL IPNode : public Core::NodeId {
public:
    IPNode() : Core::NodeId(), _mask(0) {
    }
    IPNode(const NodeId& node, const uint8_t mask) : Core::NodeId(node), _mask(mask) {
    }
    IPNode(const IPNode& copy) : Core::NodeId(copy), _mask(copy._mask) {
    }
    ~IPNode() {
    }

public:
    uint8_t Mask() const {
        return (_mask == static_cast<uint8_t>(~0) ? (Type() == Core::NodeId::TYPE_IPV4 ? 32 : 128) : _mask);
    }
    NodeId Broadcast() const;

private:
    uint8_t _mask;    
};

}
} // namespace Core

#endif // __NODEID_H
