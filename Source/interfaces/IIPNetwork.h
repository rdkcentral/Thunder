#ifndef _IPNETWORK_H
#define _IPNETWORK_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IIPNetwork : virtual public Core::IUnknown {
        enum { ID = 0x0000005E };

        struct IDNSServers : virtual public Core::IUnknown {
            enum { ID = 0x0000005F };

            virtual ~IDNSServers() {}

            virtual void Reset() = 0;
            virtual bool Next() = 0;
            virtual string Server() const = 0;
        };


        virtual uint32_t AddAddress    (const string& interfaceName) = 0;
        virtual uint32_t AddAddress    (const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast, const uint8_t netmask) = 0;
        virtual uint32_t RemoveAddress (const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast) = 0;
        virtual uint32_t AddDNS        (IDNSServers* dnsServers) = 0;
        virtual uint32_t RemoveDNS     (IDNSServers* dnsServers) = 0;
    };
}
}

#endif // _IPNETWORK_H
