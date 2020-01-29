#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // @json
    struct IVolumeControl : virtual public Core::IUnknown {
        enum { ID = ID_VOLUMECONTROL };

        // @event
        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_VOLUMECONTROL_NOTIFICATION };

            virtual ~INotification() {}

            // @brief Signals volume change
            // @param volume New bolume level in percent (e.g. 100)
            virtual void Volume(const uint8_t volume) = 0;

            // @brief Signals mute state change
            // @param muted New mute state (true: muted, false: un-muted)
            virtual void Muted(const bool muted) = 0;
        };

        virtual ~IVolumeControl() {}

        virtual void Register(IVolumeControl::INotification* sink) = 0;
        virtual void Unregister(const IVolumeControl::INotification* sink) = 0;

        // @property
        // @brief Audio mute state
        // @param muted Mute state (true: muted, false: un-muted)
        // @retval ERROR_GENERAL Failed to set/retrieve muting state
        virtual uint32_t Muted(const bool muted) = 0;
        virtual uint32_t Muted(bool& muted /* @out */) const = 0;

        // @property
        // @brief Audio volume level
        // @param volume Volume level in percent (e.g. 100)
        // @retval ERROR_GENERAL Failed to set/retrieve audio volume
        virtual uint32_t Volume(const uint8_t volume) = 0;
        virtual uint32_t Volume(uint8_t& volume /* @out */) const = 0;
    };
}
}
