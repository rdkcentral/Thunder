
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

        class SynctimeParamsData : public Core::JSON::Container {
        public:
            SynctimeParamsData()
                : Core::JSON::Container()
            {
                Add(_T("time"), &Time);
                Add(_T("source"), &Source);
            }

            SynctimeParamsData(const SynctimeParamsData&) = delete;
            SynctimeParamsData& operator=(const SynctimeParamsData&) = delete;

        public:
            Core::JSON::String Time; // Synchronized time (in ISO8601 format); empty string if the time has never been synchronized
            Core::JSON::String Source; // The synchronization source e.g. an NTP server
        }; // class SynctimeParamsData

    } // namespace TimeSync

} // namespace JsonData

}

