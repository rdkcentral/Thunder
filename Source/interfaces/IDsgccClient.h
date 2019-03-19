#ifndef __IDSGCCCLIENT_H
#define __IDSGCCCLIENT_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IDsgccClient : virtual public Core::IUnknown {

        enum { ID = 0x00000081 };

        virtual ~IDsgccClient() {}

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        virtual void DsgccClientSet(const string& str) = 0;
        virtual string GetChannels() const = 0;
    };
}
}

#endif // __IDSGCCCLIENT_H
