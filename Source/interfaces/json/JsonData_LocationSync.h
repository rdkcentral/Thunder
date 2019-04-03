
// C++ classes for Location Sync JSON-RPC API.
// Generated automatically from 'LocationSyncAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace LocationSync {

        // Method params/result classes
        //

        class LocationResultData : public Core::JSON::Container {
        public:
            LocationResultData()
                : Core::JSON::Container()
            {
                Add(_T("city"), &City);
                Add(_T("country"), &Country);
                Add(_T("region"), &Region);
                Add(_T("timezone"), &Timezone);
                Add(_T("publicip"), &Publicip);
            }

            LocationResultData(const LocationResultData&) = delete;
            LocationResultData& operator=(const LocationResultData&) = delete;

        public:
            Core::JSON::String City; // City name
            Core::JSON::String Country; // Country name
            Core::JSON::String Region; // Region name
            Core::JSON::String Timezone; // Time zone information
            Core::JSON::String Publicip; // Public IP
        }; // class LocationResultData

    } // namespace LocationSync

} // namespace JsonData

} // namespace WPEFramework

