#ifndef __ISWITCHBOARD_H
#define __ISWITCHBOARD_H

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a switchboard
    struct ISwitchBoard : virtual public Core::IUnknown {

        enum { ID = ID_SWITCHBOARD };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_SWITCHBOARD_NOTIFICATION };

            virtual ~INotification() {}

            // Signal which callsign has been switched on
            virtual void Activated(const string& callsign) = 0;
        };

        virtual ~ISwitchBoard() {}

        virtual void Register(ISwitchBoard::INotification* notification) = 0;
        virtual void Unregister(ISwitchBoard::INotification* notification) = 0;

        virtual bool IsActive(const string& callsign) const = 0;
        virtual uint32_t Activate(const string& callsign) = 0;
        virtual uint32_t Deactivate(const string& callsign) = 0;
    };
}
}

#endif // __ISWITCHBOARD_H
