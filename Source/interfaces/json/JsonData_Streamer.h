
// C++ classes for Streamer JSON-RPC API.
// Generated automatically from 'StreamerPlugin.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Streamer {

        // Common classes
        //

        class GeometryInfo : public Core::JSON::Container {
        public:
            GeometryInfo()
                : Core::JSON::Container()
            {
                Add(_T("x"), &X);
                Add(_T("y"), &Y);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
            }

            GeometryInfo(const GeometryInfo&) = delete;
            GeometryInfo& operator=(const GeometryInfo&) = delete;

        public:
            Core::JSON::DecUInt32 X; // X co-ordinate
            Core::JSON::DecUInt32 Y; // Y co-ordinate
            Core::JSON::DecUInt32 Width; // Width of the window
            Core::JSON::DecUInt32 Height; // Height of the window
        }; // class GeometryInfo

        // Method params/result classes
        //

        class DRMResultData : public Core::JSON::Container {
        public:
            // DRM/CAS attached with stream
            enum class DrmType {
                UNKNOWN,
                CLEARKEY,
                PLAYREADY,
                WIDEVINE
            };

            DRMResultData()
                : Core::JSON::Container()
            {
                Add(_T("drm"), &Drm);
            }

            DRMResultData(const DRMResultData&) = delete;
            DRMResultData& operator=(const DRMResultData&) = delete;

        public:
            Core::JSON::EnumType<DrmType> Drm; // DRM/CAS attached with stream
        }; // class DRMResultData

        class LoadParamsData : public Core::JSON::Container {
        public:
            LoadParamsData()
                : Core::JSON::Container()
            {
                Add(_T("index"), &Index);
                Add(_T("url"), &Url);
            }

            LoadParamsData(const LoadParamsData&) = delete;
            LoadParamsData& operator=(const LoadParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Index; // Index of the streamer instance
            Core::JSON::String Url; // URL of the stream
        }; // class LoadParamsData

        class PositionParamsData : public Core::JSON::Container {
        public:
            PositionParamsData()
                : Core::JSON::Container()
            {
                Add(_T("index"), &Index);
                Add(_T("position"), &Position);
            }

            PositionParamsData(const PositionParamsData&) = delete;
            PositionParamsData& operator=(const PositionParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Index; // Index of the streamer instance
            Core::JSON::DecUInt32 Position; // Absolute position value to be set
        }; // class PositionParamsData

        class SpeedParamsData : public Core::JSON::Container {
        public:
            SpeedParamsData()
                : Core::JSON::Container()
            {
                Add(_T("index"), &Index);
                Add(_T("speed"), &Speed);
            }

            SpeedParamsData(const SpeedParamsData&) = delete;
            SpeedParamsData& operator=(const SpeedParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Index; // Index of the streamer instance
            Core::JSON::DecUInt32 Speed; // Speed value to be set
        }; // class SpeedParamsData

        class StateResultData : public Core::JSON::Container {
        public:
            // Player State
            enum class StateType {
                IDLE,
                LOADING,
                PREPARED,
                PAUSED,
                PLAYING,
                ERROR
            };

            StateResultData()
                : Core::JSON::Container()
            {
                Add(_T("state"), &State);
            }

            StateResultData(const StateResultData&) = delete;
            StateResultData& operator=(const StateResultData&) = delete;

        public:
            Core::JSON::EnumType<StateType> State; // Player State
        }; // class StateResultData

        class TypeResultData : public Core::JSON::Container {
        public:
            // Type of the stream
            enum class StreamType {
                STUBBED,
                DVB,
                VOD
            };

            TypeResultData()
                : Core::JSON::Container()
            {
                Add(_T("stream"), &Stream);
            }

            TypeResultData(const TypeResultData&) = delete;
            TypeResultData& operator=(const TypeResultData&) = delete;

        public:
            Core::JSON::EnumType<StreamType> Stream; // Type of the stream
        }; // class TypeResultData

        class WindowParamsData : public Core::JSON::Container {
        public:
            WindowParamsData()
                : Core::JSON::Container()
            {
                Add(_T("index"), &Index);
                Add(_T("geometry"), &Geometry);
            }

            WindowParamsData(const WindowParamsData&) = delete;
            WindowParamsData& operator=(const WindowParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Index; // Index of the streamer instance
            GeometryInfo Geometry; // Geometry value of the window
        }; // class WindowParamsData

    } // namespace Streamer

} // namespace JsonData

}

