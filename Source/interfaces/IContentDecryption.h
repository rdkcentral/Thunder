#ifndef _CONTENT_DECRYPTION_H
#define _CONTENT_DECRYPTION_H

#include "Module.h"

namespace WPEFramework {

namespace Exchange {

    // This interface gives direct access to a OpenCDMi server instance, running as a plugin in the framework.
    struct IContentDecryption : virtual public Core::IUnknown {

        enum { ID = ID_CONTENTDECRYPTION };

        virtual ~IContentDecryption() {}
        virtual uint32_t Initialize(PluginHost::IShell* service) = 0;
        virtual void Deinitialize(PluginHost::IShell* service) = 0;
        virtual uint32_t Reset() = 0;
        virtual RPC::IStringIterator* Systems() const = 0;
        virtual RPC::IStringIterator* Designators(const string& keySystem) const = 0;
        virtual RPC::IStringIterator* Sessions(const string& keySystem) const = 0;
    };
}
}

#endif // _CONTENT_DECRYPTION_H
