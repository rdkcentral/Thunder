#ifndef __ICOMPOSITION_H
#define __ICOMPOSITION_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IComposition : virtual public Core::IUnknown {
        enum { ID = 0x00000046 };

        struct IClient : virtual public Core::IUnknown {
            enum { ID = 0x00000047 };

            virtual ~IClient() {}

            virtual string Name() const = 0;
            virtual void Kill() = 0;
            virtual void Opacity(const uint32_t value) = 0;
            virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height) = 0;
            virtual void Visible(const bool visible) = 0;
            virtual void SetTop() = 0;
            virtual void SetInput() = 0;
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000048 };

            virtual ~INotification() {}

            virtual void Attached(IClient* client) = 0;
            virtual void Detached(IClient* client) = 0;
        };

        virtual ~IComposition() {}

        virtual void Register(IComposition::INotification* notification) = 0;
        virtual void Unregister(IComposition::INotification* notification) = 0;

        // Index is a virtual number indicating the spot in a list of clients.
        // 0 is always the first client, the last client is found if the index
        // incremented by 1 and the Client returns a nullptr for the IClient
        // interface.
        virtual IClient* Client(const uint8_t index) = 0;

        // As the previous method is just to iterate over all clients, it is
        // expected that the next method is used to actually aquire a IClient.
        virtual IClient* Client(const string& name) = 0;
    };
}
}

#endif // __ICOMPOSITION_H
