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

// @stubgen:skip

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IIPNetwork : virtual public Core::IUnknown {
        enum { ID = ID_IPNETWORK };

        struct EXTERNAL IDNSServers : virtual public Core::IUnknown {
            enum { ID = ID_IPNETWORK_DNSSERVER };

            virtual void Reset() = 0;
            virtual bool Next() = 0;
            virtual string Server() const = 0;
        };

        virtual uint32_t AddAddress(const string& interfaceName) = 0;
        virtual uint32_t AddAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast, const uint8_t netmask) = 0;
        virtual uint32_t RemoveAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast) = 0;
        virtual uint32_t AddDNS(IDNSServers* dnsServers) = 0;
        virtual uint32_t RemoveDNS(IDNSServers* dnsServers) = 0;
    };
}
}
