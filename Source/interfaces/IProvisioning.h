#ifndef __IPROVISIONING_H
#define __IPROVISIONING_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IProvisioning : virtual public Core::IUnknown {
        enum { ID = 0x00000056 };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000057 };

            virtual ~INotification() {}

            // Signal changes on the subscribed namespace..
            virtual void Provisioned(const string& component) = 0;
        };

        virtual ~IProvisioning() {}

        virtual void Register(IProvisioning::INotification* provisioning) = 0;
        virtual void Unregister(IProvisioning::INotification* provisioning) = 0;
    };
}
}

#endif // __IPROVISIONING_H
