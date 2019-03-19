#ifndef _POWER_H
#define _POWER_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IPower : virtual public Core::IUnknown {
        enum { ID = ID_POWER };

        enum PCState {
            On = 1, // S0.
            ActiveStandby = 2, // S1.
            PassiveStandby = 3, // S2.
            SuspendToRAM = 4, // S3.
            Hibernate = 5, // S4.
            PowerOff = 6, // S5.
        };

        enum PCStatus {
            PCSuccess = 1,
            PCFailure = 2,
            PCSameMode = 3,
            PCInvalidState = 4,
            PCNotSupportedState = 5
        };

        struct INotification : virtual public Core::IUnknown {
            enum {
                ID = ID_POWER_NOTIFICATION
            };

            virtual ~INotification()
            {
            }

            virtual void StateChange(const PCState) = 0;
        };

        virtual void Register(IPower::INotification* sink) = 0;
        virtual void Unregister(IPower::INotification* sink) = 0;

        virtual PCState GetState() const = 0;
        virtual PCStatus SetState(const PCState, const uint32_t) = 0;
        virtual void PowerKey() = 0;
        virtual void Configure(const string& settings) = 0;
    };
}
}

#endif // _POWER_H
