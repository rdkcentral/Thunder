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
