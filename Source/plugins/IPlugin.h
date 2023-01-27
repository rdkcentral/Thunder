/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __IPLUGIN_H__
#define __IPLUGIN_H__

#include <com/ICOM.h>

namespace WPEFramework {

namespace Web {

    class Request;

    class Response;
}

namespace PluginHost {

    struct IShell;

    class Channel;

    struct EXTERNAL IPlugin : public virtual Core::IUnknown {

        enum { ID = RPC::ID_PLUGIN };

        struct INotification : virtual public Core::IUnknown {

            enum { ID = RPC::ID_PLUGIN_NOTIFICATION };

            ~INotification() override = default;

            //! @{
            //! ================================== CALLED ON THREADPOOL THREAD =====================================
            //! Whenever a plugin changes state, this is reported to an observer so proper actions could be taken
            //! on this state change.
            //! @}
            virtual void Activated(const string& callsign, IShell* plugin) = 0;
            virtual void Deactivated(const string& callsign, IShell* plugin) = 0;
            virtual void Unavailable(const string& callsign, IShell* plugin) = 0;
        };

        struct ILifeTime : virtual public Core::IUnknown {

            enum { ID = RPC::ID_PLUGIN_LIFETIME };

            ~ILifeTime() override = default;

            virtual void Initialize(const string& callsign, IShell* plugin) = 0;
            virtual void Deinitialized(const string& callsign, IShell* plugin) = 0;
        };

        ~IPlugin() override = default;

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

    /* @stubgen:omit */
    struct IPluginExtended : public IPlugin {

        enum { ID = RPC::ID_PLUGINEXTENDED };

        ~IPluginExtended() override = default;

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
        //! Whenever the channel is closed, it is reported via the detach method.
        //! @}
        virtual bool Attach(PluginHost::Channel& channel) = 0;
        virtual void Detach(PluginHost::Channel& channel) = 0;
    };

    struct EXTERNAL ICompositPlugin : public virtual Core::IUnknown {
        enum { ID = RPC::ID_COMPOSIT_PLUGIN };

        struct EXTERNAL INotification : public virtual Core::IUnknown {
            enum { ID = RPC::ID_COMPOSIT_PLUGIN_NOTIFICATION };

            ~INotification() override = default;

            virtual uint32_t Activated(const string& callsign, IShell* plugin) = 0;
            virtual uint32_t Deactivated(const string& callsign, IShell* plugin) = 0;
        };

        ~ICompositPlugin() override = default;

        virtual uint32_t Register(INotification*) = 0;
        virtual uint32_t Unregister(INotification*) = 0;
    };

    /* @stubgen:omit */
    struct IWeb : virtual public Core::IUnknown {

        enum { ID = RPC::ID_WEB };

        ~IWeb() override = default;

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

    /* @stubgen:omit */
    struct IWebSocket : virtual public Core::IUnknown {

        enum { ID = RPC::ID_WEBSOCKET };

        ~IWebSocket() override = default;

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
        //! and to form an answer on the incoming JSON message(if needed).
        //! @}
        virtual Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element) = 0;
    };

    /* @stubgen:omit */
    struct ITextSocket : virtual public Core::IUnknown {

        enum { ID = RPC::ID_TEXTSOCKET };

        ~ITextSocket() override = default;

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! Once the passed object from the previous method is filled (completed), this method allows it to be handled
        //! and to form an answer on the incoming JSON messagev(if needed).
        //! @}
        virtual string Inbound(const uint32_t ID, const string& value) = 0;
    };

    /* @stubgen:omit */
    struct IChannel : virtual public Core::IUnknown {

        enum { ID = RPC::ID_CHANNEL };

        ~IChannel() override = default;

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

    /* @stubgen:omit */
    struct ISecurity : virtual public Core::IUnknown {

        enum { ID = RPC::ID_SECURITY };

        ~ISecurity() override = default;

        //! Allow a websocket upgrade to be checked if it is allowed to be opened.
        virtual bool Allowed(const string& path) const = 0;

        //! Allow a request to be checked before it is offered for processing.
        virtual bool Allowed(const Web::Request& request) const = 0;

        //! Allow a JSONRPC message to be checked before it is offered for processing.
        virtual bool Allowed(const Core::JSONRPC::Message& message) const = 0;

        // The security validation is based on a token. This token holds a payload.
        // The payload is used to determine the authorization that goes with this
        // security interface. Others (plugins) that would like to know the payload 
        // for their authentication/authorization might requre this payload as well.
        // Hereby we give access to the payload of the Token that identifies the 
        // source/user of this ISecurity interface.
        virtual string Token() const = 0;
    };

    struct IAuthenticate : virtual public Core::IUnknown {

        enum { ID = RPC::ID_AUTHENTICATE };

        ~IAuthenticate() override = default;

        virtual Core::hresult CreateToken(const uint16_t length /* @in */, const uint8_t buffer[] /* @in @length:length */, string& token /* @out */) = 0;
        virtual ISecurity* Officer(const string& token) = 0;
    };

} // namespace PluginHost
} // namespace WPEFramework


#include "IShell.h" // needed for the proxy/stub generation

#endif
