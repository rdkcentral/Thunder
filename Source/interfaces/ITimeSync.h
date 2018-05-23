#ifndef __INTERFACE_TIMESYNC_H
#define __INTERFACE_TIMESYNC_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a time synchronize / update
    struct ITimeSync : virtual public Core::IUnknown {
        enum { ID = 0x0000005C };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x0000005D };

            virtual ~INotification() {}

            // Some change happened with respect to the Network..
            virtual void Completed() = 0;
        };

        virtual ~ITimeSync() {}

        virtual void Register(ITimeSync::INotification* notification) = 0;
        virtual void Unregister(ITimeSync::INotification* notification) = 0;

        virtual uint32_t Synchronize() = 0;
        virtual void Cancel() = 0;
        virtual string Source() const = 0;
        virtual uint64_t SyncTime() const = 0;
    };

} // namespace Exchange
} // namespace WPEFramework

#endif // __INTERFACE_TIMESYNC_H