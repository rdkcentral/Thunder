
// C++ classes for Browser JSON-RPC API.
// Generated automatically from 'BrowserAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Browser {

        // Common enums
        //

        enum class VisibilityType {
            VISIBLE,
            HIDDEN
        };

        // Method params/result classes
        //

        class UrlchangeParamsData : public Core::JSON::Container {
        public:
            UrlchangeParamsData()
                : Core::JSON::Container()
            {
                Add(_T("url"), &Url);
                Add(_T("loaded"), &Loaded);
            }

            UrlchangeParamsData(const UrlchangeParamsData&) = delete;
            UrlchangeParamsData& operator=(const UrlchangeParamsData&) = delete;

        public:
            Core::JSON::String Url; // The URL that has been loaded or requested
            Core::JSON::Boolean Loaded; // Determines if the URL has just been loaded (true) or if URL change has been requested (false)
        }; // class UrlchangeParamsData

        class VisibilitychangeParamsData : public Core::JSON::Container {
        public:
            VisibilitychangeParamsData()
                : Core::JSON::Container()
            {
                Add(_T("hidden"), &Hidden);
            }

            VisibilitychangeParamsData(const VisibilitychangeParamsData&) = delete;
            VisibilitychangeParamsData& operator=(const VisibilitychangeParamsData&) = delete;

        public:
            Core::JSON::Boolean Hidden; // Determines if the browser has been hidden (true) or made visible (false)
        }; // class VisibilitychangeParamsData

    } // namespace Browser

} // namespace JsonData

}

