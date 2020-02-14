#ifndef __ICOMPOSITION_H
#define __ICOMPOSITION_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IComposition : virtual public Core::IUnknown {
        enum { ID = ID_COMPOSITION };

        static constexpr uint32_t maxOpacity = 255;
        static constexpr uint32_t minOpacity = 0;

        static constexpr uint32_t maxZOrder = 255;
        static constexpr uint32_t minZOrder = 0;

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

            enum { ID = ID_COMPOSITION_CLIENT };

            virtual ~IClient() {}

            virtual string Name() const = 0;
            virtual void Kill() = 0;
            virtual void Opacity(const uint32_t value) = 0;
            virtual uint32_t Geometry(const Rectangle& rectangle) = 0;
            virtual Rectangle Geometry() const = 0; 
            virtual uint32_t ZOrder(const uint16_t index) = 0;
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_COMPOSITION_NOTIFICATION };

            virtual ~INotification() {}

            virtual void Attached(const string& name, IClient* client) = 0;
            virtual void Detached(const string& name) = 0;
        };

        virtual ~IComposition() {}

        virtual void Register(IComposition::INotification* notification) = 0;
        virtual void Unregister(IComposition::INotification* notification) = 0;

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;

        // Set and get output resolution
        virtual void Resolution(const ScreenResolution) = 0;
        virtual ScreenResolution Resolution() const = 0;
    };
}
}

#endif // __ICOMPOSITION_H
