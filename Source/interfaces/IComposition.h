#ifndef __ICOMPOSITION_H
#define __ICOMPOSITION_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IComposition : virtual public Core::IUnknown {
        enum { ID = 0x00000046 };

        static const uint32_t maxOpacity = 255;
        static const uint32_t minOpacity = 0;

        enum ScreenResolution {
            ScreenResolution_Unknown = 0,
            ScreenResolution_480i = 1,
            ScreenResolution_480p = 2,
            ScreenResolution_720p = 3,
            ScreenResolution_720p50Hz = 4,
            ScreenResolution_1080p24Hz = 5,
            ScreenResolution_1080i50Hz = 6,
            ScreenResolution_1080p50Hz = 7,
            ScreenResolution_1080p60Hz = 8,
            ScreenResolution_2160p50Hz = 9,
            ScreenResolution_2160p60Hz = 10
        };

        static uint32_t WidthFromResolution(const ScreenResolution resolution);
        static uint32_t HeightFromResolution(const ScreenResolution resolution);

        struct Rectangle {
            uint32_t x;
            uint32_t y;
            uint32_t width;
            uint32_t height;    
        };

        struct IClient : virtual public Core::IUnknown {
            enum { ID = 0x00000047 };

            virtual ~IClient() {}

            virtual string Name() const = 0;
            virtual void Kill() = 0;
            virtual void Opacity(const uint32_t value) = 0;
            virtual void ChangedGeometry(const Rectangle& rectangle) = 0;
            virtual void SetTop() = 0;
            virtual void SetInput() = 0;
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000048 };

            virtual ~INotification() {}

            virtual void Attached(IClient* client) = 0;
            virtual void Detached(IClient* client) = 0;
        };

        virtual ~IComposition() {}

        virtual void Register(IComposition::INotification* notification) = 0;
        virtual void Unregister(IComposition::INotification* notification) = 0;

        // Index is a virtual number indicating the spot in a list of clients.
        // 0 is always the first client, the last client is found if the index
        // incremented by 1 and the Client returns a nullptr for the IClient
        // interface.
        virtual IClient* Client(const uint8_t index) = 0;

        // As the previous method is just to iterate over all clients, it is
        // expected that the next method is used to actually aquire a IClient.
        virtual IClient* Client(const string& callsign) = 0;
        virtual void      Geometry(const string& callsign, const Rectangle& rectangle) = 0;
        virtual Rectangle Geometry(const string& callsign) const = 0;

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;

        // Set and get output resolution
        virtual void Resolution(const ScreenResolution) = 0;
        virtual ScreenResolution Resolution() const = 0;

    };
}
}

#endif // __ICOMPOSITION_H
