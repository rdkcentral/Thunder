
#ifndef __ICAPTURE_H__
#define __ICAPTURE_H__

#include "Module.h"

namespace WPEFramework {
    namespace Exchange {

        struct ICapture : virtual public Core::IUnknown {
            enum { ID = 0x00000069 };

            struct IStore {

                virtual ~IStore() {};

                virtual bool R8_G8_B8_A8(const unsigned char *buffer, const unsigned int width, const unsigned int height) = 0;
            };

            virtual ~ICapture(){};

            virtual const TCHAR* Name() const = 0;
            virtual bool Capture(ICapture::IStore& storer) = 0;

            // Get the interface so we can Capture a screen
            static ICapture* Instance();
        };
    }
}

#endif //__ICAPTURE_H__
