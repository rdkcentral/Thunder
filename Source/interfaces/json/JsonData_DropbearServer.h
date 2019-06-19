
// C++ classes for Dropbear Server JSON-RPC API.
// Generated automatically from 'DropbearServerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace DropbearServer {

        // Method params/result classes
        //

        class StartserviceParamsData : public Core::JSON::Container {
        public:
            StartserviceParamsData()
                : Core::JSON::Container()
            {
                Add(_T("port"), &Port);
            }

            StartserviceParamsData(const StartserviceParamsData&) = delete;
            StartserviceParamsData& operator=(const StartserviceParamsData&) = delete;

        public:
            Core::JSON::String Port; // USB device to reset
        }; // class UsbresetParamsData

    } // namespace DropbearServer

} // namespace JsonData

}

