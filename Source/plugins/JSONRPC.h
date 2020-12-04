/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#pragma once

#include "Module.h"
#include "System.h"
#include "IShell.h"
#include "IPlugin.h"

namespace WPEFramework {

namespace PluginHost {

    struct EXTERNAL IDispatcher : public virtual Core::IUnknown {
        virtual ~IDispatcher() {}

        enum { ID = RPC::ID_DISPATCHER };

        virtual Core::ProxyType<Core::JSONRPC::Message> Invoke(const string& token, const uint32_t channelId, const Core::JSONRPC::Message& message) = 0;

        // Methods used directly by the Framework to handle MetaData requirements.
        // There should be no need to call these methods from the implementation directly.
        virtual void Activate(IShell* service) = 0;
        virtual void Deactivate() = 0;
    };

    class EXTERNAL JSONRPC : public IDispatcher {
    private:
        typedef std::list<Core::JSONRPC::Handler> HandlerList;

        class EXTERNAL Registration : public Core::JSON::Container {
        private:
            Registration(const Registration&) = delete;
            Registration& operator=(const Registration&) = delete;

        public:
            Registration()
                : Core::JSON::Container()
                , Event()
                , Callsign()
            {
                Add(_T("event"), &Event);
                Add(_T("id"), &Callsign);
            }
            ~Registration()
            {
            }

        public:
            Core::JSON::String Event;
            Core::JSON::String Callsign;
        };

        enum state {
            STATE_INCORRECT_HANDLER,
            STATE_INCORRECT_VERSION,
            STATE_UNKNOWN_METHOD,
            STATE_REGISTRATION,
            STATE_UNREGISTRATION,
            STATE_EXISTS,
            STATE_CUSTOM
        };

    public:
        typedef std::function<bool(const string& token, const string& method, const string& parameters)> TokenCheckFunction;

        JSONRPC(const JSONRPC&) = delete;
        JSONRPC& operator=(const JSONRPC&) = delete;
        JSONRPC();
        JSONRPC(const TokenCheckFunction& validation);
        JSONRPC(const std::vector<uint8_t> versions);
        JSONRPC(const std::vector<uint8_t> versions, const TokenCheckFunction& validation);
        ~JSONRPC() override;

    public:
        //
        // Factory methods to aquire a JSONRPC Message
        // ------------------------------------------------------------------------------------------------------------------------------
        Core::ProxyType<Core::JSONRPC::Message> Message() const
        {
            return (Core::ProxyType<Core::JSONRPC::Message>(IFactories::Instance().JSONRPC()));
        }

        //
        // Get metadata for this handler. What is the callsign associated with this handler.
        // ------------------------------------------------------------------------------------------------------------------------------
        const string& Callsign() const
        {
            return (_callsign);
        }

        //
        // Methods to enable versioning, where interfaces are *changed* in stead of *extended*. Usage of these methods is not preferred,
        // methods are here for backwards compatibility with functionality out there in the field !!! Use these methods with caution!!!!
        // ------------------------------------------------------------------------------------------------------------------------------
		operator Core::JSONRPC::Handler& () {
            return (_handlers.front());
		}
        operator const Core::JSONRPC::Handler&() const
        {
            return (_handlers.front());
        }
        Core::JSONRPC::Handler& CreateHandler(const std::vector<uint8_t>& versions)
        {
            _handlers.emplace_back([&](const uint32_t id, const string& designator, const string& data) { Notify(id, designator, data); }, versions);
            return (_handlers.back());
        }
        Core::JSONRPC::Handler& CreateHandler(const std::vector<uint8_t>& versions, const Core::JSONRPC::Handler& source)
        {
            _handlers.emplace_back([&](const uint32_t id, const string& designator, const string& data) { Notify(id, designator, data); }, versions, source);
            return (_handlers.back());
        }
        Core::JSONRPC::Handler* GetHandler(uint8_t version)
        {
            Core::JSONRPC::Handler* result = (version == static_cast<uint8_t>(~0) ? &_handlers.front() : nullptr);
            HandlerList::iterator index(_handlers.begin());

            while ((index != _handlers.end()) && (result == nullptr)) {
                if (index->HasVersionSupport(version) == true) {
                    result = &(*index);
				} 
				else {
                    index++;
                }
            }

			return (result);
        }

        //
        // Register/Unregister methods for incoming method handling on the "base" handler elements.
        // ------------------------------------------------------------------------------------------------------------------------------
        template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
        void Property(const string& methodName, GET_METHOD getter, SET_METHOD setter, REALOBJECT* objectPtr)
        {
            _handlers.front().Property<PARAMETER>(methodName, getter, setter, objectPtr);
        }
        template <typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handlers.front().Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handlers.front().Register<INBOUND, OUTBOUND, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename INBOUND, typename OUTBOUND, typename METHOD>
        void Register(const string& methodName, const METHOD& method)
        {
            _handlers.front().Register<INBOUND, OUTBOUND, METHOD>(methodName, method);
        }
        template <typename INBOUND, typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handlers.front().Register<INBOUND, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename INBOUND, typename METHOD>
        void Register(const string& methodName, const METHOD& method)
        {
            _handlers.front().Register<INBOUND, METHOD>(methodName, method);
        }
        void Register(const string& methodName, const Core::JSONRPC::InvokeFunction& lambda) 
        { 
            _handlers.front().Register(methodName, lambda);
        } 
        void Register(const string& methodName, const Core::JSONRPC::CallbackFunction& lambda) 
        { 
            _handlers.front().Register(methodName, lambda);
        } 
        void Unregister(const string& methodName)
        {
            _handlers.front().Unregister(methodName);
        }

        //
        // Methods to send outbound event messages
        // ------------------------------------------------------------------------------------------------------------------------------
        uint32_t Notify(const string& event)
        {
            return (_handlers.front().Notify(event, Core::JSON::String()));
        }
        template <typename JSONOBJECT>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters)
        {
            return (_handlers.front().Notify(event, parameters));
        }
        template <typename JSONOBJECT, typename SENDIFMETHOD>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters, SENDIFMETHOD method)
        {
            return (_handlers.front().Notify(event, parameters, method));
        }

        //
        // Methods to send responses to inbound invokaction methods (a-synchronous callbacks)
        // ------------------------------------------------------------------------------------------------------------------------------
        template <typename JSONOBJECT>
        uint32_t Response(const Core::JSONRPC::Connection& channel, const JSONOBJECT& parameters)
        {
            string subject;
            parameters.ToString(subject);
            return (Response(channel, subject));
        }
        uint32_t Response(const Core::JSONRPC::Connection& channel, const string& result)
        {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = IFactories::Instance().JSONRPC();

            ASSERT(_service != nullptr);

            message->Result = result;
            message->Id = channel.Sequence();
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel.ChannelId(), Core::ProxyType<Core::JSON::IElement>(message)));
        }
        uint32_t Response(const Core::JSONRPC::Connection& channel, const Core::JSONRPC::Error& result)
        {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = IFactories::Instance().JSONRPC();

            ASSERT(_service != nullptr);

            message->Error = result;
            message->Id = channel.Sequence();
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel.ChannelId(), Core::ProxyType<Core::JSON::IElement>(message)));
        }

    protected:
        virtual bool Exists(Core::JSONRPC::Handler& handler, const string& parameters)
        {
            return (handler.Exists(parameters));
        }
        virtual void Subscribe(Core::JSONRPC::Handler& handler, const uint32_t channelId, const string& eventName, const string& callsign, Core::JSONRPC::Message& response)
        {
            handler.Subscribe(channelId, eventName, callsign, response);
        }
        virtual void Unsubscribe(Core::JSONRPC::Handler& handler, const uint32_t channelId, const string& eventName, const string& callsign, Core::JSONRPC::Message& response)
        {
            handler.Unsubscribe(channelId, eventName, callsign, response);
        }
        Core::ProxyType<Core::JSONRPC::Message> Invoke(const string& token, const uint32_t channelId, const Core::JSONRPC::Message& inbound) override
        {
            Registration info;
            Core::ProxyType<Core::JSONRPC::Message> response(Message());
            Core::JSONRPC::Handler* source = nullptr;
            string method(inbound.Designator.Value());

            if (inbound.Id.IsSet() == true) {
                response->JSONRPC = Core::JSONRPC::Message::DefaultVersion;
                response->Id = inbound.Id.Value();
            }

            if ((_validate != nullptr) && (_validate(token, Core::JSONRPC::Message::Method(method), inbound.Parameters.Value()) == false)) {
                response->Error.SetError(Core::ERROR_PRIVILIGED_REQUEST);
                response->Error.Text = _T("method invokation not allowed.");
            } 
            else {
                switch (Destination(method, source)) {
                case STATE_INCORRECT_HANDLER:
                    response->Error.SetError(Core::ERROR_INVALID_DESIGNATOR);
                    response->Error.Text = _T("Destined invoke failed.");
                    break;
                case STATE_INCORRECT_VERSION:
                    response->Error.SetError(Core::ERROR_INVALID_SIGNATURE);
                    response->Error.Text = _T("Requested version is not supported.");
                    break;
                case STATE_UNKNOWN_METHOD:
                    response->Error.SetError(Core::ERROR_UNKNOWN_KEY);
                    response->Error.Text = _T("Unknown method.");
                    break;
                case STATE_REGISTRATION:
                    info.FromString(inbound.Parameters.Value());
                    Subscribe(*source, channelId, info.Event.Value(), info.Callsign.Value(), *response);
                    break;
                case STATE_UNREGISTRATION:
                    info.FromString(inbound.Parameters.Value());
                    Unsubscribe(*source, channelId, info.Event.Value(), info.Callsign.Value(), *response);
                    break;
                case STATE_EXISTS:
                    if (Exists(*source, inbound.Parameters.Value()) == true) {
                        response->Result = Core::NumberType<uint32_t>(Core::ERROR_NONE).Text();
                    } else {
                        response->Result = Core::NumberType<uint32_t>(Core::ERROR_UNKNOWN_KEY).Text();
                    }
                    break;
                case STATE_CUSTOM:
                    string result;
                    uint32_t code = source->Invoke(Core::JSONRPC::Connection(channelId, inbound.Id.Value()), inbound.FullMethod(), inbound.Parameters.Value(), result);
                    if (response.IsValid() == true) {
                        if (code == static_cast<uint32_t>(~0)) {
                            response.Release();
                        } else if (code == Core::ERROR_NONE) {
                            response->Result = result;
                        } else {
                            response->Error.Code = code;
                            response->Error.Text = Core::ErrorToString(code);
                        }
                    }
                }
            }

            return response;
        }

    private:
        state Destination(const string& designator, Core::JSONRPC::Handler*& source)
        {
            state result = STATE_INCORRECT_HANDLER;
            string callsign(Core::JSONRPC::Message::Callsign(designator));

            if (callsign.empty() || (callsign == _callsign)) {
                // Seems we are on the right handler..
                // now see if someone supports this version
                uint8_t version = Core::JSONRPC::Message::Version(designator);
                HandlerList::iterator index(_handlers.begin());

                if (version != static_cast<uint8_t>(~0)) {
                    while ((index != _handlers.end()) && (index->HasVersionSupport(version) == false)) {
                        index++;
                    }
                }

                if (index == _handlers.end()) {
                    result = STATE_INCORRECT_VERSION;
                } else {
                    string method(Core::JSONRPC::Message::Method(designator));

                    if (method == _T("register")) {
                        result = STATE_REGISTRATION;
                        source = &(*index);
                    } else if (method == _T("unregister")) {
                        result = STATE_UNREGISTRATION;
                        source = &(*index);
                    } else if (method == _T("exists")) {
                        result = STATE_EXISTS;
                        source = &(*index);
                    } else if (index->Exists(method) == Core::ERROR_NONE) {
                        source = &(*index);
                        result = STATE_CUSTOM;
                    } else {
                        result = STATE_UNKNOWN_METHOD;
                    }
                }
            }
            return (result);
        }
        void Notify(const uint32_t id, const string& designator, const string& parameters)
        {
            Core::ProxyType<Core::JSONRPC::Message> message(Message());

            ASSERT(_service != nullptr);

            if (!parameters.empty()) {
                message->Parameters = parameters;
            }

            message->Designator = designator;
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            _service->Submit(id, Core::ProxyType<Core::JSON::IElement>(message));
        }
        void Activate(IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);

            _service = service;
            _callsign = _service->Callsign();
        }
        void Deactivate() override
        {
            HandlerList::iterator index(_handlers.begin());

            while (index != _handlers.end()) {
                index->Close();
                index++;
            }

            _handlers.front().Close();
            _service = nullptr;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        std::list<Core::JSONRPC::Handler> _handlers;
        IShell* _service;
        string _callsign;
        TokenCheckFunction _validate;
    };

    class EXTERNAL JSONRPCSupportsEventStatus : public JSONRPC {
    public:
        JSONRPCSupportsEventStatus(const JSONRPCSupportsEventStatus&) = delete;
        JSONRPCSupportsEventStatus& operator=(const JSONRPCSupportsEventStatus&) = delete;

        JSONRPCSupportsEventStatus() = default;
        virtual ~JSONRPCSupportsEventStatus() = default;

        enum class Status { registered,
            unregistered };

        template <typename METHOD>
        void RegisterEventStatusListener(const string& event, METHOD method)
        {
            _adminLock.Lock();

            ASSERT(_observers.find(event) == _observers.end());

            _observers[event] = method;

            _adminLock.Unlock();
        }

        void UnregisterEventStatusListener(const string& event)
        {
            _adminLock.Lock();

            ASSERT(_observers.find(event) != _observers.end());

            _observers.erase(event);

            _adminLock.Unlock();
        }

    protected:
        void NotifyObservers(const string& event, const string& client, const Status status) const
        {
            _adminLock.Lock();

            StatusCallbackMap::const_iterator it = _observers.find(event);
            if (it != _observers.cend()) {
                it->second(client, status);
            }

            _adminLock.Unlock();
        }
        virtual void Subscribe(Core::JSONRPC::Handler& handler, const uint32_t channelId, const string& eventName, const string& callsign, Core::JSONRPC::Message& response)
        {
            JSONRPC::Subscribe(handler, channelId, eventName, callsign, response);
            NotifyObservers(eventName, callsign, Status::registered);
        }
        virtual void Unsubscribe(Core::JSONRPC::Handler& handler, const uint32_t channelId, const string& eventName, const string& callsign, Core::JSONRPC::Message& response)
        {
            NotifyObservers(eventName, callsign, Status::unregistered);
            JSONRPC::Subscribe(handler, channelId, eventName, callsign, response);
        }

    private:
        using EventStatusCallback = std::function<void(const string&, Status status)>;
        using StatusCallbackMap = std::map<string, EventStatusCallback>;

        mutable Core::CriticalSection _adminLock;
        StatusCallbackMap _observers;
    };
} // namespace WPEFramework::PluginHost
}
