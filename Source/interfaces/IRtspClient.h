#ifndef __IRTSPCLIENBT_H
#define __IRTSPCLIENBT_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {
    struct IRtspClient : virtual public Core::IUnknown {

        enum { ID = ID_RTSPCLIENT };

        virtual ~IRtspClient() {}

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;

        virtual uint32_t Setup(const string& assetId, uint32_t position) = 0;
        virtual uint32_t Play(int32_t scale, uint32_t position) = 0;
        virtual uint32_t Teardown() = 0;

        virtual void Set(const string& name, const string& value) = 0;
        virtual string Get(const string& name) const = 0;
    };
}
}

#endif // __IRTSPCLIENBT_H
