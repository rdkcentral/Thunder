
// C++ classes for Spark Browser JSON-RPC API.
// Generated automatically from 'SparkPlugin.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Spark {

        // Method params/result classes
        //

        class SeturlParamsData : public Core::JSON::Container {
        public:
            SeturlParamsData()
                : Core::JSON::Container()
            {
                Add(_T("url"), &Url);
            }

            SeturlParamsData(const SeturlParamsData&) = delete;
            SeturlParamsData& operator=(const SeturlParamsData&) = delete;

        public:
            Core::JSON::String Url; // The URL to load
        }; // class SeturlParamsData

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
            Core::JSON::Boolean Suspended; // Determines if the browser has reached suspended state (true) or resumed state (false)
        }; // class StatechangeParamsData

        class StatusResultData : public Core::JSON::Container {
        public:
            StatusResultData()
                : Core::JSON::Container()
            {
                Add(_T("url"), &Url);
                Add(_T("fps"), &Fps);
                Add(_T("suspended"), &Suspended);
                Add(_T("hidden"), &Hidden);
            }

            StatusResultData(const StatusResultData&) = delete;
            StatusResultData& operator=(const StatusResultData&) = delete;

        public:
            Core::JSON::String Url; // The currently loaded URL in the Spark browser
            Core::JSON::DecUInt32 Fps; // The current number of frames per second the browser is rendering
            Core::JSON::Boolean Suspended; // Determines if the browser is in suspended mode (true) or resumed mode (false)
            Core::JSON::Boolean Hidden; // Determines if the browser is hidden (true) or visible (false)
        }; // class StatusResultData

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

    } // namespace Spark

} // namespace JsonData

}

