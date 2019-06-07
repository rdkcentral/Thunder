
// C++ classes for Snapshot JSON-RPC API.
// Generated automatically from 'SnapshotAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Snapshot {

        // Method params/result classes
        //

        class CaptureResultData : public Core::JSON::Container {
        public:
            CaptureResultData()
                : Core::JSON::Container()
            {
                Add(_T("image"), &Image);
                Add(_T("device"), &Device);
            }

            CaptureResultData(const CaptureResultData&) = delete;
            CaptureResultData& operator=(const CaptureResultData&) = delete;

        public:
            Core::JSON::String Image; // Image file in PNG format (base64 encoded)
            Core::JSON::String Device; // Display device on which the capture was performed on
        }; // class CaptureResultData

    } // namespace Snapshot

} // namespace JsonData

}

