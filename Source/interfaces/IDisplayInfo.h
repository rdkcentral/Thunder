#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IDeviceProperties : virtual public Core::IUnknown {
        enum { ID = ID_DEVICE_PROPERTIES };

        virtual ~IDeviceProperties() { }

        virtual const string Chipset() const = 0;
        virtual const string FirmwareVersion() const = 0;
    };

    struct IGraphicsProperties : virtual public Core::IUnknown {
        enum { ID = ID_GRAPHICS_PROPERTIES };

        virtual ~IGraphicsProperties() {}

        virtual uint64_t TotalGpuRam() const = 0;
        virtual uint64_t FreeGpuRam() const = 0;
    };

    struct IConnectionProperties : virtual public Core::IUnknown {
        enum { ID = ID_CONNECTION_PROPERTIES };

        virtual ~IConnectionProperties() { }

        enum HDRType {
            HDR_OFF,
            HDR_10,
            HDR_10PLUS,
            HDR_DOLBYVISION,
            HDR_TECHNICOLOR
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_CONNECTION_PROPERTIES_NOTIFICATION };

            virtual ~INotification() {}

            virtual void Updated() = 0;
        };

        virtual uint32_t Register(INotification*) = 0;
        virtual uint32_t Unregister(INotification*) = 0;

        virtual bool IsAudioPassthrough () const = 0;
        virtual bool Connected() const = 0;
        virtual uint32_t Width() const = 0;
        virtual uint32_t Height() const = 0;
        virtual uint8_t HDCPMajor() const = 0;
        virtual uint8_t HDCPMinor() const = 0;
        virtual HDRType Type() const = 0;
    };
}
}
