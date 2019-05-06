
// C++ classes for Time Sync JSON-RPC API.
// Generated automatically from 'TimeSyncAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace TimeSync {

        // Method params/result classes
        //

        class SetParamsData : public Core::JSON::Container {
        public:
            SetParamsData()
                : Core::JSON::Container()
            {
                Add(_T("time"), &Time);
            }

            SetParamsData(const SetParamsData&) = delete;
            SetParamsData& operator=(const SetParamsData&) = delete;

        public:
            Core::JSON::DecUInt64 Time; // New system time (in microseconds since midnight 1/Jan/1970); if this parameter is omitted then the *time* subsystem will still become activated but without setting the time
        }; // class SetParamsData

        class TimeResultData : public Core::JSON::Container {
        public:
            TimeResultData()
                : Core::JSON::Container()
            {
                Add(_T("time"), &Time);
                Add(_T("source"), &Source);
            }

            TimeResultData(const TimeResultData&) = delete;
            TimeResultData& operator=(const TimeResultData&) = delete;

        public:
            Core::JSON::DecUInt64 Time; // Synchronized time (in microseconds since midnight 1/Jan/1970); *0* if the time has never been synchronized
            Core::JSON::String Source; // The synchronization source e.g. an NTP server
        }; // class TimeResultData

    } // namespace TimeSync

} // namespace JsonData

}

