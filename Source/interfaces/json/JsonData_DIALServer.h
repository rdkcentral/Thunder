
// C++ classes for DIAL Server JSON-RPC API.
// Generated automatically from 'DIALServerPlugin.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace DIALServer {

        // Common classes
        //

        class StartParamsInfo : public Core::JSON::Container {
        public:
            StartParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("application"), &Application);
                Add(_T("parameters"), &Parameters);
            }

            StartParamsInfo(const StartParamsInfo&) = delete;
            StartParamsInfo& operator=(const StartParamsInfo&) = delete;

        public:
            Core::JSON::String Application; // Application name
            Core::JSON::String Parameters; // Additional application-specific parameters
        }; // class StartParamsInfo

        // Method params/result classes
        //

    } // namespace DIALServer

} // namespace JsonData

}

