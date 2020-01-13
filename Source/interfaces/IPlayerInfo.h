#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IPlayerProperties : virtual public Core::IUnknown {
        enum { ID = ID_PLAYER_PROPERTIES };
        virtual ~IPlayerProperties() {}

        struct IAudioIterator : virtual public Core::IUnknown {
            enum { ID = ID_PLAYER_PROPERTIES_AUDIO };
            virtual ~IAudioIterator() {}

            enum AudioCodec : uint8_t {
                UNDEFINED = 0,
                AUDIO_AAC,
                AUDIO_AC3,
                AUDIO_AC3_PLUS,
                AUDIO_DTS,
                AUDIO_MPEG1,
                AUDIO_MPEG2,
                AUDIO_MPEG3,
                AUDIO_MPEG4,
                AUDIO_OPUS,
                AUDIO_VORBIS_OGG,
                AUDIO_WAV
            };

            virtual bool IsValid() const = 0;
            virtual bool Next() = 0;
            virtual void Reset() = 0;
            virtual AudioCodec Codec() const = 0;
        };

        struct IVideoIterator : virtual public Core::IUnknown {
            enum { ID = ID_PLAYER_PROPERTIES_VIDEO };
            virtual ~IVideoIterator() {}

            enum VideoCodec : uint8_t {
                UNDEFINED = 0,
                VIDEO_H263,
                VIDEO_H264,
                VIDEO_H265,
                VIDEO_H265_10,
                VIDEO_MPEG,
                VIDEO_VP8,
                VIDEO_VP9,
                VIDEO_VP10
            };

            virtual bool IsValid() const = 0;
            virtual bool Next() = 0;
            virtual void Reset() = 0;
            virtual VideoCodec Codec() const = 0;
        };

        virtual IAudioIterator* AudioCodec() const = 0;
        virtual IVideoIterator* VideoCodec() const = 0;
    };
}
}
