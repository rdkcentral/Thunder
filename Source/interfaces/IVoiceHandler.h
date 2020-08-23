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

    struct EXTERNAL IVoiceHandler;

    /*
     * Interface responsible for producing audio data
     * The data that is produced must be signed big endian
     */
    struct EXTERNAL IVoiceProducer : virtual public Core::IUnknown {
        enum { ID = ID_VOICEPRODUCER };

        struct EXTERNAL IProfile : virtual public Core::IUnknown {
            enum { ID = ID_VOICEPRODUCER_PROFILE };

            enum codec : uint8_t {
                UNDEFINED = 0,
                PCM,
                ADPCM
            };

            virtual codec Codec() const = 0;
            virtual uint8_t Channels() const = 0;
            virtual uint32_t SampleRate() const = 0;
            virtual uint8_t Resolution() const = 0;
        };

        virtual string Name() const = 0;
        virtual uint32_t Callback(IVoiceHandler* callback) = 0;
        virtual uint32_t Error() const = 0;
        virtual string MetaData() const = 0;
        virtual void Configure(const string& settings) = 0;
    };

    struct EXTERNAL IVoiceHandler : virtual public Core::IUnknown {
        enum { ID = ID_VOICEHANDLER };

        virtual void Start(const IVoiceProducer::IProfile* profile) = 0;
        virtual void Stop() = 0;
        virtual void Data(const uint32_t sequenceNo, const uint8_t data[] /* @length:length */, const uint16_t length) = 0;
    };

} // Exchange
} // WPEFramework
