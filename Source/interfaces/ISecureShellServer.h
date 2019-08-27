#pragma once

// @stubgen:skip

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ISecureShellServer : virtual public Core::IUnknown {
        enum { ID = ID_SECURESHELLSERVER };

/*	 struct IClients : virtual public Core::IUnknown {
		enum { ID = ID_SECURESHELLSERVER_CLIENT_ITERATOR };

                virtual ~IClients() {}

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;
                virtual string RemoteId() const = 0;
                virtual void Close() = 0;
                virtual void Count() const = 0;
            };
*/

        virtual uint32_t GetSessionsCount() = 0;
    };
}
}
