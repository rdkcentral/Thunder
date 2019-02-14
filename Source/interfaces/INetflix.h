#ifndef __INETFLIX_H
#define __INETFLIX_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a Netflix instance
    struct INetflix : virtual public Core::IUnknown {

        enum { ID = ID_NETFLIX };

        enum state {
            PLAYING = 0x0001,
            STOPPED = 0x0002,
            SUSPENDING = 0x0004
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_NETFLIX_NOTIFICATION };

            virtual ~INotification() {}

            // Signal changes on the subscribed namespace..
            virtual void StateChange(const INetflix::state state) = 0;
        };

        virtual ~INetflix() {}

        virtual void Register(INetflix::INotification* netflix) = 0;
        virtual void Unregister(INetflix::INotification* netflix) = 0;

        virtual string GetESN() const = 0;

        virtual void FactoryReset() = 0;
        virtual void SystemCommand(const string& command) = 0;
        virtual void Language(const string& language) = 0;
        virtual void SetVisible(bool visibility) = 0;
    };
}
}

#endif // __INETFLIX_H
