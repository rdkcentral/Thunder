
// C++ classes for StateControl JSON-RPC API.
// Generated automatically from 'StateControlAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace StateControl {

        // Common enums
        //

        enum class StateType {
            RESUMED,
            SUSPENDED
        };

        // Method params/result classes
        //

        class StatechangeParamsData : public Core::JSON::Container {
        public:
            StatechangeParamsData()
                : Core::JSON::Container()
            {
                Add(_T("suspended"), &Suspended);
            }

            StatechangeParamsData(const StatechangeParamsData&) = delete;
            StatechangeParamsData& operator=(const StatechangeParamsData&) = delete;

        public:
            Core::JSON::Boolean Suspended; // Determines if the service has entered suspended state (true) or resumed state (false)
        }; // class StatechangeParamsData

    } // namespace StateControl

} // namespace JsonData

}

