#ifndef __IBROWSER_H
#define __IBROWSER_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a Browser to change
    // Browser specific properties like displayed URL.
    struct IBrowser : virtual public Core::IUnknown {
        enum { ID = ID_BROWSER };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_BROWSER_NOTIFICATION };

            virtual ~INotification() {}

            // Signal changes on the subscribed namespace..
            virtual void LoadFinished(const string& URL) = 0;
            virtual void URLChanged(const string& URL) = 0;
            virtual void Hidden(const bool hidden) = 0;
            virtual void Closure() = 0;
        };

        struct IMetadata : virtual public Core::IUnknown {
            enum { ID = ID_BROWSER_METADATA };

            virtual ~IMetadata() {}

            virtual string LocalCache() const = 0;
            virtual string CookieStore() const = 0;
            virtual void GarbageCollect() = 0;
        };

        virtual ~IBrowser() {}

        virtual void Register(IBrowser::INotification* sink) = 0;
        virtual void Unregister(IBrowser::INotification* sink) = 0;

        // Change the currenly displayed URL by the browser.
        virtual void SetURL(const string& URL) = 0;
        virtual string GetURL() const = 0;
        virtual uint32_t GetFPS() const = 0;

        virtual void Hide(const bool hidden) = 0;
    };
}
}

#endif // __IBROWSER_H
