#pragma once

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IDropbearServer : virtual public Core::IUnknown {
        enum { ID = ID_DROPBEARSERVER };

        virtual uint32_t StartService(const std::string& host_keys, const std::string& port_flag, const std::string& port) = 0;
        virtual uint32_t StopService() = 0;
        virtual uint32_t GetTotalSessions() = 0;
        virtual uint32_t GetSessionsCount() = 0;
    };
}
}
