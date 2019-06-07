#ifndef __IDSGCCCLIENT_H
#define __IDSGCCCLIENT_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IDsgccClient : virtual public Core::IUnknown {

        enum { ID = ID_DSGCC_CLIENT };

        enum state {
            Unknown = 0,
            Ready   = 1,
            Changed = 2,
            Error   = 3
        };

        virtual ~IDsgccClient() {}

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_DSGCC_CLIENT_NOTIFICATION };

            virtual ~INotification() {}

            virtual void StateChange(const state newState) = 0;
        };

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        virtual string GetChannels() const = 0;
        virtual string State() const = 0;
        virtual void Restart() = 0;
        virtual void Callback(IDsgccClient::INotification* callback) = 0;

        virtual void DsgccClientSet(const string& str) = 0;
    };
}
}

#endif // __IDSGCCCLIENT_H
