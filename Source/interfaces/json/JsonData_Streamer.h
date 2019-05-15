
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

        class DrmResultInfo : public Core::JSON::Container {
        public:
            DrmResultInfo()
                : Core::JSON::Container()
            {
                Add(_T("drm"), &Drm);
            }

            DrmResultInfo(const DrmResultInfo&) = delete;
            DrmResultInfo& operator=(const DrmResultInfo&) = delete;

        public:
            Core::JSON::EnumType<DrmType> Drm; // DRM used
        }; // class DrmResultInfo

        class StateResultInfo : public Core::JSON::Container {
        public:
            StateResultInfo()
                : Core::JSON::Container()
            {
                Add(_T("state"), &State);
            }

            StateResultInfo(const StateResultInfo&) = delete;
            StateResultInfo& operator=(const StateResultInfo&) = delete;

        public:
            Core::JSON::EnumType<StateType> State; // Stream state
        }; // class StateResultInfo

        class StatusParamsInfo : public Core::JSON::Container {
        public:
            StatusParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("id"), &Id);
            }

            StatusParamsInfo(const StatusParamsInfo&) = delete;
            StatusParamsInfo& operator=(const StatusParamsInfo&) = delete;

        public:
            Core::JSON::DecUInt8 Id; // ID of the streamer instance to get status of
        }; // class StatusParamsInfo

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
                Add(_T("id"), &Id);
                Add(_T("location"), &Location);
            }

            LoadParamsData(const LoadParamsData&) = delete;
            LoadParamsData& operator=(const LoadParamsData&) = delete;

        public:
            Core::JSON::DecUInt8 Id; // ID of the streamer instance to attach a decoder to
            Core::JSON::String Location; // Location of the source to load
        }; // class LoadParamsData

        class PositionParamsData : public Core::JSON::Container {
        public:
            PositionParamsData()
                : Core::JSON::Container()
            {
                Add(_T("id"), &Id);
                Add(_T("position"), &Position);
            }

            PositionParamsData(const PositionParamsData&) = delete;
            PositionParamsData& operator=(const PositionParamsData&) = delete;

        public:
            Core::JSON::DecUInt8 Id; // ID of the streamer instance to set position of
            Core::JSON::DecUInt64 Position; // Position to set (in milliseconds)
        }; // class PositionParamsData

        class SpeedParamsData : public Core::JSON::Container {
        public:
            SpeedParamsData()
                : Core::JSON::Container()
            {
                Add(_T("id"), &Id);
                Add(_T("speed"), &Speed);
            }

            SpeedParamsData(const SpeedParamsData&) = delete;
            SpeedParamsData& operator=(const SpeedParamsData&) = delete;

        public:
            Core::JSON::DecUInt8 Id; // ID of the streamer instance to set speed of
            Core::JSON::DecUInt32 Speed; // Speed to set; 0 - pause, 100 - normal playback forward, -100 - normal playback back, -200 - reverse at twice the speed, 50 - forward at half speed
        }; // class SpeedParamsData

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
            Core::JSON::DecUInt64 Position; // Stream position (in nanoseconds)
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
            Core::JSON::DecUInt64 Time; // Time in seconds
        }; // class TimeupdateParamsData

        class TypeResultData : public Core::JSON::Container {
        public:
            TypeResultData()
                : Core::JSON::Container()
            {
                Add(_T("stream"), &Stream);
            }

            TypeResultData(const TypeResultData&) = delete;
            TypeResultData& operator=(const TypeResultData&) = delete;

        public:
            Core::JSON::EnumType<TypeType> Stream; // Stream type
        }; // class TypeResultData

        class WindowParamsData : public Core::JSON::Container {
        public:
            WindowParamsData()
                : Core::JSON::Container()
            {
                Add(_T("id"), &Id);
                Add(_T("window"), &Window);
            }

            WindowParamsData(const WindowParamsData&) = delete;
            WindowParamsData& operator=(const WindowParamsData&) = delete;

        public:
            Core::JSON::DecUInt8 Id; // ID of the streamer instance to set window of
            WindowInfo Window; // Geometry of the window
        }; // class WindowParamsData

    } // namespace Streamer

} // namespace JsonData

}

