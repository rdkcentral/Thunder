#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IVoiceHandler;

    /*
     * Interface responsible for producing audio data
     * The data that is produced must be signed big endian
     */
    struct IVoiceProducer : virtual public Core::IUnknown {
        enum { ID = ID_VOICEPRODUCER };

        virtual ~IVoiceProducer(){};

        struct IProfile : virtual public Core::IUnknown {
            enum { ID = ID_VOICEPRODUCER_PROFILE };
            virtual ~IProfile(){};

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

    struct IVoiceHandler : virtual public Core::IUnknown {
        enum { ID = ID_VOICEHANDLER };

        virtual ~IVoiceHandler(){};

        virtual void Start(const IVoiceProducer::IProfile* profile) = 0;
        virtual void Stop() = 0;
        virtual void Data(const uint32_t sequenceNo, const uint8_t data[] /* @length:length */, const uint16_t length) = 0;
    };

} // Exchange
} // WPEFramework
