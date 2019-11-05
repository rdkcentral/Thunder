#ifndef __IREMOTECONTROL_H__
#define __IREMOTECONTROL_H__

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IRemoteControl : virtual public Core::IUnknown {
        enum { ID = ID_REMOTECONTROL };

        virtual ~IRemoteControl(){};

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_REMOTECONTROL_NOTIFICATION };

            virtual ~INotification(){};
            virtual void Event(const string& producer, const uint32_t event) = 0;
        };

        virtual void RegisterEvents(INotification* sink) = 0;
        virtual void UnregisterEvents(INotification* sink) = 0;
    };
}
}

#endif // __IREMOTECONTROL_H__
