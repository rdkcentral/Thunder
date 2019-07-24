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
        //virtual uint32_t GetSessionsInfo(Core::JSON::ArrayType<JsonData::DropbearServer::SessioninfoResultData>& response) = 0;
        virtual uint32_t CloseClientSession(uint32_t client_pid) = 0;
    };
}
}
