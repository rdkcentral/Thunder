#ifndef __IVOICEHANDLER_H__
#define __IVOICEHANDLER_H__

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IVoiceHandler : virtual public Core::IUnknown {
        enum { ID = ID_VOICEHANDLER };

        virtual ~IVoiceHandler(){};

        virtual void VoiceEvent(const uint8_t data[], const uint16_t length) = 0;
    };

    struct IVoiceProducer : virtual public Core::IUnknown {
        enum { ID = ID_VOICEPRODUCER };

        enum audioCodec {
            PCM,
            ADPCM
        };

        virtual ~IVoiceProducer(){};

        virtual const TCHAR* VoiceName() const = 0;
        virtual void VoiceCallback(IVoiceHandler* callback) = 0; //!< Sets a handle to send the VoiceEvent.
        virtual const Exchange::IVoiceProducer::audioCodec AudioCodec() = 0; //!< Type of audio codec of this producer
        virtual uint32_t VoiceError() const = 0;
    };
}
}
#endif // __IVOICEHANDLER_H__
