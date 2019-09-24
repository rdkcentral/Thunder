#pragma once

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ISecureShellServer : virtual public Core::IUnknown {
        enum { ID = ID_SECURESHELLSERVER };

	virtual ~ISecureShellServer() {}

	struct IClient : virtual public Core::IUnknown {

            enum { ID = ID_SECURESHELLSERVER_CLIENT};

            struct IIterator : virtual public Core::IUnknown {

                enum { ID = ID_SECURESHELLSERVER_CLIENT_ITERATOR };

		virtual ~IIterator() {}

                virtual uint32_t Count() const = 0;
                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;
                virtual IClient* Current() = 0;
            };

            virtual ~IClient() {}

            virtual string RemoteId() const = 0;
            virtual string TimeStamp() const = 0;
            virtual string IpAddress() const = 0;
            virtual void Close() = 0;
        };

        virtual IClient::IIterator* Clients() = 0;
    };
}
}
