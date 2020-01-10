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

        virtual ~IBrowser() {}

        virtual void Register(IBrowser::INotification* sink) = 0;
        virtual void Unregister(IBrowser::INotification* sink) = 0;

        // Change the currenly displayed URL by the browser.
        virtual void SetURL(const string& URL) = 0;
        virtual string GetURL() const = 0;

        virtual uint32_t GetFPS() const = 0;

        virtual void Hide(const bool hidden) = 0;
    };

    /* @json */
    struct IWebBrowser : virtual public Core::IUnknown {
        enum { ID = ID_WEB_BROWSER };

        /* @event */
        using INotification = IBrowser::INotification;

        virtual ~IWebBrowser() { }

        virtual void Register(INotification* sink) = 0;
        virtual void Unregister(INotification* sink) = 0;

        /* @property */
        /* @brief Page loaded in the browser */
        /* @param url Loaded URL */
        virtual uint32_t URL(string& url /* @out */) const = 0;
        virtual uint32_t URL(const string& url) = 0;

        /* @property */
        /* @brief Browser window visibility state */
        /* @param visible Visiblity state */
        virtual uint32_t Visible(bool visible /* @out */) const = 0;
        virtual uint32_t Visible(const bool visible) = 0;

        /* @property*/
        /* @brief Current framerate the browser is rendering at */
        /* @param fps Current FPS */
        virtual uint32_t FPS(uint8_t& fps /* @out */) const = 0;
    };

}
}

#endif // __IBROWSER_H
