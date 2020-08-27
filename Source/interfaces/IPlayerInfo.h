/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {
    /* @json */
    struct EXTERNAL IPlayerProperties : virtual public Core::IUnknown {
        enum { ID = ID_PLAYER_PROPERTIES };
        virtual ~IPlayerProperties() {}

         enum AudioCodec : uint8_t {
            AUDIO_UNDEFINED,
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

        typedef RPC::IIteratorType<AudioCodec, ID_PLAYER_PROPERTIES_AUDIO> IAudioCodedIterator;

        enum VideoCodec : uint8_t {
            VIDEO_UNDEFINED,
            VIDEO_H263,
            VIDEO_H264,
            VIDEO_H265,
            VIDEO_H265_10,
            VIDEO_MPEG,
            VIDEO_VP8,
            VIDEO_VP9,
            VIDEO_VP10
        };

        typedef RPC::IIteratorType<VideoCodec, ID_PLAYER_PROPERTIES_VIDEO> IVideoCodecIterator;

        virtual uint32_t AudioCodec(IAudioCodedIterator& codec /* @out */) const;
        virtual uint32_t* VideoCodec(IVideoCodecIterator& codec /* @out */) const;

        enum PlaybackResolution : uint8_t {
            RESOLUTION_UNKNOWN,
            RESOLUTION_480I,
            RESOLUTION_480P,
            RESOLUTION_576I,
            RESOLUTION_576P,
            RESOLUTION_720P,
            RESOLUTION_1080I,
            RESOLUTION_1080P,
            RESOLUTION_2160P30,
            RESOLUTION_2160P60
        };

        // @property
        // @brief Current Video playback resolution
        // @param res: resolution
        virtual uint32_t Resolution(PlaybackResolution& res /* @out */) const = 0;

        // @property
        // @brief Checks Loudness Equivalence in platform
        // @param ae: enabled/disabled
        virtual uint32_t IsAudioEquivalenceEnabled(bool& ae /* @out */) const = 0;
    };
}
}
