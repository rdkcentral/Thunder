#ifndef __IRTSPCLIENBT_H
#define __IRTSPCLIENBT_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {
    struct IRtspClient : virtual public Core::IUnknown {

        enum { ID = 0x00000071 };

        virtual ~IRtspClient() {}

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;

        virtual uint32_t Setup(const string& assetId, uint32_t position);
        virtual uint32_t Play(int16_t scale, uint32_t position);
        virtual uint32_t Teardown();

        virtual void RtspClientSet(const string& str) = 0;
        virtual string RtspClientGet() const = 0;
    };
}
}

#endif // __IRTSPCLIENBT_H
