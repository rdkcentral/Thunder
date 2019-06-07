
// C++ classes for Streamer JSON-RPC API.
// Generated automatically from 'StreamerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Streamer {

        // Common enums
        //

        // DRM used
        enum class DrmType {
            UNKNOWN,
            CLEARKEY,
            PLAYREADY,
            WIDEVINE
        };

        // Stream state
        enum class StateType {
            IDLE,
            LOADING,
            PREPARED,
            PAUSED,
            PLAYING,
            ERROR
        };

        // Stream type
        enum class TypeType {
            STUBBED,
            DVB,
            ATSC,
            VOD
        };

        // Common classes
        //

        class DrmInfo : public Core::JSON::Container {
        public:
            DrmInfo()
                : Core::JSON::Container()
            {
                Add(_T("drm"), &Drm);
            }

            DrmInfo(const DrmInfo&) = delete;
            DrmInfo& operator=(const DrmInfo&) = delete;

        public:
            Core::JSON::EnumType<DrmType> Drm; // DRM used
        }; // class DrmInfo

        class StateInfo : public Core::JSON::Container {
        public:
            StateInfo()
                : Core::JSON::Container()
            {
                Add(_T("state"), &State);
            }

            StateInfo(const StateInfo&) = delete;
            StateInfo& operator=(const StateInfo&) = delete;

        public:
            Core::JSON::EnumType<StateType> State; // Stream state
        }; // class StateInfo

        class WindowInfo : public Core::JSON::Container {
        public:
            WindowInfo()
                : Core::JSON::Container()
            {
                Add(_T("x"), &X);
                Add(_T("y"), &Y);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
            }

            WindowInfo(const WindowInfo&) = delete;
            WindowInfo& operator=(const WindowInfo&) = delete;

        public:
            Core::JSON::DecUInt32 X; // Horizontal position of the window (in pixels)
            Core::JSON::DecUInt32 Y; // Vertical position of the window (in pixels)
            Core::JSON::DecUInt32 Width; // Width of the window (in pixels)
            Core::JSON::DecUInt32 Height; // Height of the window (in pixels)
        }; // class WindowInfo

        // Method params/result classes
        //

        class CreateParamsData : public Core::JSON::Container {
        public:
            CreateParamsData()
                : Core::JSON::Container()
            {
                Add(_T("type"), &Type);
            }

            CreateParamsData(const CreateParamsData&) = delete;
            CreateParamsData& operator=(const CreateParamsData&) = delete;

        public:
            Core::JSON::EnumType<TypeType> Type; // Stream type
        }; // class CreateParamsData

        class LoadParamsData : public Core::JSON::Container {
        public:
            LoadParamsData()
                : Core::JSON::Container()
            {
                Add(_T("index"), &Index);
                Add(_T("location"), &Location);
            }

            LoadParamsData(const LoadParamsData&) = delete;
            LoadParamsData& operator=(const LoadParamsData&) = delete;

        public:
            Core::JSON::DecUInt8 Index; // ID of the streamer instance
            Core::JSON::String Location; // Location of the source to load
        }; // class LoadParamsData

        class StatusResultData : public Core::JSON::Container {
        public:
            StatusResultData()
                : Core::JSON::Container()
            {
                Add(_T("type"), &Type);
                Add(_T("state"), &State);
                Add(_T("metadata"), &Metadata);
                Add(_T("drm"), &Drm);
                Add(_T("position"), &Position);
                Add(_T("window"), &Window);
            }

            StatusResultData(const StatusResultData&) = delete;
            StatusResultData& operator=(const StatusResultData&) = delete;

        public:
            Core::JSON::EnumType<TypeType> Type; // Stream type
            Core::JSON::EnumType<StateType> State; // Stream state
            Core::JSON::String Metadata; // Custom metadata associated with the stream
            Core::JSON::EnumType<DrmType> Drm; // DRM used
            Core::JSON::DecUInt64 Position; // Stream position (in milliseconds)
            WindowInfo Window; // Geometry of the window
        }; // class StatusResultData

        class TimeupdateParamsData : public Core::JSON::Container {
        public:
            TimeupdateParamsData()
                : Core::JSON::Container()
            {
                Add(_T("time"), &Time);
            }

            TimeupdateParamsData(const TimeupdateParamsData&) = delete;
            TimeupdateParamsData& operator=(const TimeupdateParamsData&) = delete;

        public:
            Core::JSON::DecUInt64 Time; // Position in seconds
        }; // class TimeupdateParamsData

        class TypeData : public Core::JSON::Container {
        public:
            TypeData()
                : Core::JSON::Container()
            {
                Add(_T("stream"), &Stream);
            }

            TypeData(const TypeData&) = delete;
            TypeData& operator=(const TypeData&) = delete;

        public:
            Core::JSON::EnumType<TypeType> Stream; // Stream type
        }; // class TypeData

        class WindowData : public Core::JSON::Container {
        public:
            WindowData()
                : Core::JSON::Container()
            {
                Add(_T("window"), &Window);
            }

            WindowData(const WindowData&) = delete;
            WindowData& operator=(const WindowData&) = delete;

        public:
            WindowInfo Window; // Geometry of the window
        }; // class WindowData

    } // namespace Streamer

} // namespace JsonData

}

