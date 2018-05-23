#ifndef __INTERFACE_RPCLINK_H
#define __INTERFACE_RPCLINK_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface show the use case of communicating transparently over process boundaries
    struct IRPCLink : virtual public Core::IUnknown {
        enum { ID = 0x0000006B };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x0000006A };

            virtual ~INotification() {}

            // Some change happened with respect to the test case ..
            virtual void Completed(const uint32_t id, const string& name) = 0;
        };

        virtual ~IRPCLink() {}

        virtual void Register(INotification* notification) = 0;
        virtual void Unregister(INotification* notification) = 0;

        virtual uint32_t Start(const uint32_t id, const string& name) = 0;
        virtual uint32_t Stop() = 0;
        virtual uint32_t ForceCallback() = 0;
    };

} // namespace Exchange
} // namespace WPEFramework

#endif // __INTERFACE_RPCLINK_H
