#pragma once

#include "Module.h"
 
namespace WPEFramework {
namespace PluginHost {

    struct IRemoteInstantiation : virtual public Core::IUnknown {

        enum { ID = RPC::IDS::ID_REMOTE_INSTANTIATION };

        virtual ~IRemoteInstantiation() {}

        // Overview:
        //          HOST                                      TARGET
        //
        //      +------------+                            +------------+
        //      | WEBSERVER' | [OOP]                      | WEBSERVER' | [OOH]
        //      +-----+------+                            +--+---------+
        //            |                                      |   ^
        //            |+-------------------------------------+   |
        //            ||                                         |
        //      +-----++-----+                                SPAWNED
        //      | WEBSERVER  |                                   |
        //  +---+------------+---+                     +---------+---------+
        //  |     Thunder [A]    |                     |    Thunder [B]    |
        //  +--------------------+                     +-------------------+
        virtual uint32_t Instantiate(
            const uint32_t requestId,
            const string& libraryName,
            const string& className,
            const string& callsign,
            const uint32_t interfaceId,
            const uint32_t version,
            const string& user,
            const string& group,
            const uint8_t threads,
            const int8_t priority,
            const string configuration) = 0;
    };
}
}