#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IVolumeControl : virtual public Core::IUnknown {
        enum { ID = ID_VOLUMECONTROL };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_VOLUMECONTROL_NOTIFICATION };

            virtual ~INotification() {}

            virtual void Volume(const uint8_t volume) = 0;
            virtual void Muted(const bool muted) = 0;
        };

        virtual ~IVolumeControl() {}

        virtual void Register(IVolumeControl::INotification* sink) = 0;
        virtual void Unregister(const IVolumeControl::INotification* sink) = 0;
        virtual uint32_t Muted(const bool muted) = 0;
        virtual uint32_t Muted(bool& muted /* @out */) const = 0;
        // In percents i.e. 0 - 100.
        virtual uint32_t Volume(const uint8_t volume) = 0;
        virtual uint32_t Volume(uint8_t& volume /* @out */) const = 0;
    };
}
}
