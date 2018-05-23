#ifndef __IPLUGIN_H
#define __IPLUGIN_H

#include "Module.h"

namespace WPEFramework {

namespace Web {
    class EXTERNAL Request;

    class EXTERNAL Response;
}

namespace PluginHost {

    struct EXTERNAL IShell;

    class EXTERNAL Channel;

    struct IPlugin
        : public virtual Core::IUnknown {

        enum {
            ID = 0x00000020
        };

        struct INotification
            : virtual public Core::IUnknown {

            enum {
                ID = 0x00000021
            };

            virtual ~INotification()
            {
            }

            //! @{
            //! ================================== CALLED ON THREADPOOL THREAD =====================================
            //! Whenever a plugin changes state, this is reported to an observer so proper actions could be taken
            //! on this state change.
            //! @}
            virtual void StateChange(PluginHost::IShell* plugin) = 0;
        };

        virtual ~IPlugin()
        {
        }

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        //! information and services for this particular plugin. The Service object contains configuration information that
        //! can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        //! If there is an error, return a string describing the issue why the initialisation failed.
        //! The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        //! The lifetime of the Service object is guaranteed till the deinitialize method is called.
        //! @}
        virtual const string Initialize(PluginHost::IShell* shell) = 0;

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! The plugin is unloaded from framework. This is call allows the module to notify clients
        //! or to persist information if needed. After this call the plugin will unlink from the service path
        //! and be deactivated. The Service object is the same as passed in during the Initialize.
        //! After theis call, the lifetime of the Service object ends.
        //! @}
        virtual void Deinitialize(PluginHost::IShell* shell) = 0;

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        //! to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        //! @}
        virtual string Information() const = 0;
    };

    struct IPluginExtended
        : public IPlugin {

        enum {
            ID = 0x00000022
        };

        virtual ~IPluginExtended()
        {
        }

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
        //! Whenever the channel is closed, it is reported via the detach method.
        //! @}
        virtual bool Attach(PluginHost::Channel& channel) = 0;
        virtual void Detach(PluginHost::Channel& channel) = 0;
    };

    struct IWeb
        : virtual public Core::IUnknown {
        enum {
            ID = 0x00000023
        };

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a request is received, it might carry some additional data in the body. This method allows
        //! the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        //! in the body of the request.
        //! @}
        virtual void Inbound(Web::Request& request) = 0;

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! If everything is received correctly, the request is passed to us, on a thread from the thread pool, to
        //! do our thing and to return the result in the response object. Here the actual specific module work,
        //! based on a a request is handled.
        //! @}
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) = 0;
    };

    struct IWebSocket
        : virtual public Core::IUnknown {
        enum {
            ID = 0x00000024
        };

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
        //! JSON object to us. This method allows the plugin to return a JSON object that will be used to deserialize
        //! the comming content on the communication channel. In case the content does not start with a { or [, the
        //! first keyword deserialized is passed as the identifier.
        //! @}
        virtual Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) = 0;

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! Once the passed object from the previous method is filled (completed), this method allows it to be handled
        //! and to form an answer on the incoming JSON messagev(if needed).
        //! @}
        virtual Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::JSON::IElement& element) = 0;
    };

    struct ITextSocket
        : virtual public Core::IUnknown {
        enum {
            ID = 0x00000025
        };

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! Once the passed object from the previous method is filled (completed), this method allows it to be handled
        //! and to form an answer on the incoming JSON messagev(if needed).
        //! @}
        virtual string Inbound(const uint32_t ID, const string& value) = 0;
    };

 
    struct IChannel
        : virtual public Core::IUnknown {
        enum {
            ID = 0x00000026
        };

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
        //! raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
        //! @}
        virtual uint32_t Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length) = 0;

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
        //! raw data to the initiator of the websocket.
        //! @}
        virtual uint32_t Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const = 0;
    };

    struct ISecurity
        : virtual public Core::IUnknown {
        enum {
            ID = 0x00000027
        };

        //! Allow a request to be checked before it is offered for processing.
        virtual bool Allowed(const Web::Request& request) = 0;

        //! What options are allowed to be passed to this service???
        virtual Core::ProxyType<Web::Response> Options(const Web::Request& request) = 0;
    };
} // namespace PluginHost
} // namespace WPEFramework

#endif // __IPLUGIN_H
