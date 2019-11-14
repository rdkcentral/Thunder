#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IInputPin : virtual public Core::IUnknown {
        enum { ID = ID_INPUT_PIN };

        struct INotification : virtual public Core::IUnknown {
            enum {
                ID = ID_INPUT_PIN_NOTIFICATION
            };

            virtual ~INotification()
            {
            }

            virtual void Marker(const uint32_t marker) = 0;
        };

        virtual void Register(IInputPin::INotification* sink) = 0;
        virtual void Unregister(IInputPin::INotification* sink) = 0;

        virtual uint32_t AddMarker(const IInputPin::INotification* sink, const uint32_t marker) = 0;
        virtual uint32_t RemoveMarker(const IInputPin::INotification* sink, const uint32_t marker) = 0;
    };
}
}
