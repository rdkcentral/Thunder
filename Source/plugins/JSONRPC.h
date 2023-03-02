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

#pragma once

#include "Module.h"
#include "System.h"
#include "IShell.h"
#include "IPlugin.h"
#include "IDispatcher.h"

namespace WPEFramework {

namespace PluginHost {

    struct EXTERNAL ILocalDispatcher : public IDispatcher {
        virtual ~ILocalDispatcher() override = default;

        virtual uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) = 0;

        virtual void Activate(IShell* service) = 0;
        virtual void Deactivate() = 0;
        virtual void Dropped(const uint32_t channelId) = 0;
    };

    class EXTERNAL JSONRPC : public ILocalDispatcher, public IDispatcher::ICallback {
    private:
        class Observer {
        private:
            using Destination = std::pair<uint32_t, string>;
            using Destinations = std::vector<Destination>;
            using Remotes = std::vector<IDispatcher::ICallback*>;

        public:
            Observer& operator= (const Observer& copy) = delete;

            Observer()
                : _callbacks()
                , _designators() {
            }
            Observer(Observer&& move)
                : _callbacks(move._callbacks)
                , _designators(move._designators) {
            }
            Observer(const Observer& copy)
                : _callbacks(copy._callbacks)
                , _designators(copy._designators) {
                for (IDispatcher::ICallback*& callback : _callbacks) {
                    callback->AddRef();
                }
            }
            ~Observer() {
                for (IDispatcher::ICallback*& callback : _callbacks) {
                    callback->Release();
                }
            }

        public:
            bool IsEmpty() const {
                return ( (_designators.empty()) && (_callbacks.empty()) );
            }
            uint32_t Subscribe(const uint32_t id, const string& event) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->first != id) || (index->second != event))) {
                    index++;
                }
                ASSERT(index == _designators.end());
                if (index == _designators.end()) {
                    _designators.emplace_back(Destination(id, event));
                }
                else {
                    result = Core::ERROR_DUPLICATE_KEY;
                }

                return (result);
            }
            uint32_t Unsubscribe(const uint32_t id, const string& event) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->first != id) || (index->second != event))) {
                    index++;
                }
                ASSERT(index != _designators.end());
                if (index != _designators.end()) {
                    _designators.erase(index);
                }
                else {
                    result = Core::ERROR_BAD_REQUEST;
                }

                return (result);
            }
            void Subscribe(IDispatcher::ICallback* callback) {
                Remotes::iterator index = std::find(_callbacks.begin(), _callbacks.end(), callback);

                ASSERT(index == _callbacks.end());

                if (index == _callbacks.end()) {
                    callback->AddRef();
                    _callbacks.emplace_back(callback);
                }
            }
            void Unsubscribe(const IDispatcher::ICallback* callback) {
                Remotes::iterator index = std::find(_callbacks.begin(), _callbacks.end(), callback);
                
                ASSERT(index != _callbacks.end());

                if (index != _callbacks.end()) {
                    (*index)->Release();
                    _callbacks.erase(index);
                }
            }
            void Dropped(const uint32_t channelId) {
                Destinations::iterator index = _designators.begin();
                while (index != _designators.end()) {
                    if (index->first == channelId) {
                        index = _designators.erase(index);
                    }
                    else {
                        index++;
                    }
                }
            }
            void Event(JSONRPC& parent, const string event, const string& parameter, std::function<bool(const string&)>&& sendifmethod) {
                for (const Destination& entry : _designators) {
                    if (!sendifmethod || sendifmethod(entry.second)) {
                        parent.Notify(entry.first, entry.second + '.' + event, parameter);
                    }
                }
                for (IDispatcher::ICallback*& callback : _callbacks) {
                    callback->Event(event, parameter);
                }
            }

        private:
            Remotes _callbacks;
            Destinations _designators;
        };
        using HandlerList = std::list<Core::JSONRPC::Handler>;
        using ObserverMap = std::unordered_map<string, Observer>;

        class VersionInfo {
        public:
            VersionInfo() = delete;
            VersionInfo& operator= (const VersionInfo&) = delete;

            VersionInfo(const string& interfaceName, const uint8_t major, const uint8_t minor, const uint8_t patch)
                : _name(interfaceName)
                , _major(major)
                , _minor(minor)
                , _patch(patch) {
            }
            VersionInfo(VersionInfo&& move)
                : _name(move._name)
                , _major(move._major)
                , _minor(move._minor)
                , _patch(move._patch) {
            }
            VersionInfo(const VersionInfo& copy)
                : _name(copy._name)
                , _major(copy._major)
                , _minor(copy._minor)
                , _patch(copy._patch) {
            }
            ~VersionInfo() = default;

        public:
            const string& Name() const {
                return(_name);
            }
            uint8_t Major() const {
                return(_major);
            }
            uint8_t Minor() const {
                return(_minor);
            }
            uint8_t Patch() const {
                return(_patch);
            }

        private:
            const string  _name;
            const uint8_t _major;
            const uint8_t _minor;
            const uint8_t _patch;
        };

        using VersionList = std::vector<VersionInfo>;

        class Registration : public Core::JSON::Container {
        public:
            Registration(const Registration&) = delete;
            Registration& operator=(const Registration&) = delete;

            Registration()
                : Core::JSON::Container()
                , Event()
                , Callsign()
            {
                Add(_T("event"), &Event);
                Add(_T("id"), &Callsign);
            }
            ~Registration() override = default;

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
        enum classification {
            INVALID  = 0,
            VALID    = 1,
            DEFERRED = 3
        };

        typedef std::function<classification(const string& token, const string& method, const string& parameters)> TokenCheckFunction;

        JSONRPC(const JSONRPC&) = delete;
        JSONRPC& operator=(const JSONRPC&) = delete;

        JSONRPC();
        JSONRPC(const TokenCheckFunction& validation);
        JSONRPC(const std::vector<uint8_t>& versions);
        JSONRPC(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation);
        ~JSONRPC() override;

    public:
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
            _handlers.emplace_back(versions);
            return (_handlers.back());
        }
        Core::JSONRPC::Handler& CreateHandler(const std::vector<uint8_t>& versions, const Core::JSONRPC::Handler& source)
        {
            _handlers.emplace_back(versions, source);
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
        // Register/Unregister methods for interface versioning..
        // ------------------------------------------------------------------------------------------------------------------------------
        void RegisterVersion(const string& name, const uint8_t major, const uint8_t minor, const uint8_t patch) {
            _versions.emplace_back(name, major, minor, patch);
        }

        //
        // Methods to send outbound event messages
        // ------------------------------------------------------------------------------------------------------------------------------
        uint32_t Notify(const string& event) const
        {
            return (InternalNotify(event, _T("")));
        }
        template <typename JSONOBJECT>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters) const
        {
            string subject;
            parameters.ToString(subject);
            return (InternalNotify(event, subject));
        }
        template <typename JSONOBJECT, typename SENDIFMETHOD>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters, SENDIFMETHOD method) const
        {
            string subject;
            parameters.ToString(subject);
            return InternalNotify(event, subject, std::move(method));
        }

        //
        // Methods to send responses to inbound invokaction methods (a-synchronous callbacks)
        // ------------------------------------------------------------------------------------------------------------------------------
        template <typename JSONOBJECT>
        uint32_t Response(const Core::JSONRPC::Context& channel, const JSONOBJECT& parameters)
        {
            string subject;
            parameters.ToString(subject);
            return (Response(channel, subject));
        }
        uint32_t Response(const Core::JSONRPC::Context& context, const string& result)
        {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = IFactories::Instance().JSONRPC();

            ASSERT(_service != nullptr);

            message->Result = result;
            message->Id = context.Sequence();
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(context.ChannelId(), Core::ProxyType<Core::JSON::IElement>(message)));
        }
        uint32_t Response(const Core::JSONRPC::Context& channel, const Core::JSONRPC::Error& result)
        {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = IFactories::Instance().JSONRPC();

            ASSERT(_service != nullptr);

            message->Error = result;
            message->Id = channel.Sequence();
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel.ChannelId(), Core::ProxyType<Core::JSON::IElement>(message)));
        }
 
        // Inherited via IDispatcher
        // ---------------------------------------------------------------------------------
        Core::hresult Validate(const string& token, const string& method, const string& parameters) const override {
            classification result;
            if (_validate != nullptr) {
                result = _validate(token, method, parameters);
                if (result == classification::INVALID) {
                    return (Core::ERROR_PRIVILIGED_REQUEST);
                }
                else if (result == classification::DEFERRED) {
                    return (Core::ERROR_UNAVAILABLE);
                }
            }
            return (Core::ERROR_NONE);
        }
        Core::hresult Invoke(IDispatcher::ICallback*, const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override {
            uint32_t result(Core::ERROR_BAD_REQUEST);
            Core::JSONRPC::Handler* handler(Handler(method));
            string realMethod(Core::JSONRPC::Message::Method(method));

            if (handler == nullptr) {
                result = Core::ERROR_INVALID_RANGE;
            }
            else if (realMethod == _T("exists")) {
                result = Core::ERROR_NONE;
                if (handler->Exists(realMethod) == Core::ERROR_NONE) {
                    response = _T("1");
                }
                else {
                    response = _T("0");
                }
            }
            else if (handler->Exists(realMethod) == Core::ERROR_NONE) {
                Core::JSONRPC::Context context(channelId, id, token);
                result = handler->Invoke(context, Core::JSONRPC::Message::FullMethod(method), parameters, response);
            }
            return (result);
        }
        Core::hresult Revoke(IDispatcher::ICallback* callback) override {
            // See if we re using this callback, we need to abort its use..
            for (std::pair<const string, Observer>& entry : _observers) {
                entry.second.Unsubscribe(callback);
            }
            return (Core::ERROR_NONE);
        }
        ILocalDispatcher* Local() override {
            return (this);
        }

        // Inherited via ILocalDispatcher
        // ---------------------------------------------------------------------------------
        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override {
            uint32_t result = Core::ERROR_INCORRECT_URL;

            ASSERT(Core::JSONRPC::Message::Callsign(method).empty() || (Core::JSONRPC::Message::Callsign(method) == _callsign));

            // Seems we are on the right handler..
            // now see if someone supports this version
            string realMethod(Core::JSONRPC::Message::Method(method));

            if (realMethod == _T("register")) {
                Registration info;  info.FromString(parameters);

                result = Subscribe(this, channelId, info.Event.Value(), info.Callsign.Value());
                if (result == Core::ERROR_NONE) {
                    response = _T("0");
                }
                else {
                    result = Core::ERROR_FAILED_REGISTERED;
                }
            }
            else if (realMethod == _T("unregister")) {
                Registration info;  info.FromString(parameters);

                result = Unsubscribe(this, channelId, info.Event.Value(), info.Callsign.Value());
                if (result == Core::ERROR_NONE) {
                    response = _T("0");
                }
                else {
                    result = Core::ERROR_FAILED_UNREGISTERED;
                }
            }
            else {
                result = Invoke(this, channelId, id, token, method, parameters, response);
            }

            return (result);
        }
        void Activate(IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);

            _service = service;
            _service->AddRef();
            _callsign = _service->Callsign();
        }
        void Deactivate() override
        {
            _adminLock.Lock();
            _observers.clear();
            _adminLock.Unlock();

            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
        }
        void Dropped(const uint32_t channelId) override
        {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.begin();

            while (index != _observers.end()) {

                index->second.Dropped(channelId);

                if (index->second.IsEmpty() == true) {
                    index = _observers.erase(index);
                }
                else {
                    index++;
                }
            }


            _adminLock.Unlock();
        }

        // Inherited via IDispatcher::ICallback
        // ---------------------------------------------------------------------------------
        Core::hresult Event(const string& eventId, const string& parameters) override {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index != _observers.end()) {
                index->second.Event(*this, eventId, parameters, std::function<bool(const string&)>());
            }

            _adminLock.Unlock();

            return (Core::ERROR_NONE);

        }
        Core::hresult Error(const uint32_t channel, const uint32_t id, const uint32_t code, const string& errorText) override {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = IFactories::Instance().JSONRPC();

            ASSERT(_service != nullptr);

            message->Error.Text = errorText;
            message->Error.Code = code;
            message->Id = id;
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel, Core::ProxyType<Core::JSON::IElement>(message)));
        }
        Core::hresult Response(const uint32_t channel, const uint32_t id, const string& response) override {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = IFactories::Instance().JSONRPC();

            ASSERT(_service != nullptr);

            message->Result = response;
            message->Id = id;
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel, Core::ProxyType<Core::JSON::IElement>(message)));
        }
        Core::hresult Subscribe(const uint32_t channel, const string& eventId, const string& designator) override
        {
            uint32_t result;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index == _observers.end()) {
                index = _observers.emplace(std::piecewise_construct,
                    std::forward_as_tuple(eventId),
                    std::forward_as_tuple()).first;
            }

            result = index->second.Subscribe(channel, designator);

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Unsubscribe(const uint32_t channel, const string& eventId, const string& designator) override
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index != _observers.end()) {
                result = index->second.Unsubscribe(channel, designator);

                if ((result == Core::ERROR_NONE) && (index->second.IsEmpty() == true)) {
                    _observers.erase(index);
                }
            }
            _adminLock.Unlock();

            return (result);
        }

    protected:
        uint32_t RegisterMethod(const uint8_t version, const string& methodName) {
            _adminLock.Lock();

            HandlerList::iterator index(_handlers.begin());

            if (version != static_cast<uint8_t>(~0)) {
                while ((index != _handlers.end()) && (index->HasVersionSupport(version) == false)) {
                    index++;
                }
            }
            if (index == _handlers.end()) {
                std::vector<uint8_t> versions({ version });
                _handlers.emplace_front(versions);
                index = _handlers.begin();
            } 
            index->Register(methodName, Core::JSONRPC::InvokeFunction());

            _adminLock.Unlock();

            return (Core::ERROR_NONE);
        }
        Core::JSONRPC::Handler* Handler(const string& methodName) {
            uint8_t version(Core::JSONRPC::Message::Version(methodName));

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            HandlerList::iterator index(_handlers.begin());

            if (version != static_cast<uint8_t>(~0)) {
                while ((index != _handlers.end()) && (index->HasVersionSupport(version) == false)) {
                    index++;
                }
            }
            return (index == _handlers.end() ? nullptr : &(*index));
        }

    private:
        uint32_t Subscribe(IDispatcher::ICallback* callback, const uint32_t channelId, const string& event, const string& designator) {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            // This is to make sure that the actuall location (there weher the channels really end) are
            // aware of distributing the event.
            if (callback->Subscribe(channelId, event, designator) == Core::ERROR_NONE) {

                if (callback != this) {
                    // Oops the real location is somewhere else. Register this event also for callbacks
                    // to be forwarded to that actual location
                    _adminLock.Lock();

                    ObserverMap::iterator index = _observers.find(event);

                    if (index == _observers.end()) {
                        index = _observers.emplace(std::piecewise_construct,
                            std::forward_as_tuple(event),
                            std::forward_as_tuple()).first;
                    }

                    index->second.Subscribe(callback);

                    _adminLock.Unlock();
                }
                result = Core::ERROR_NONE;
            }
            return (result);
        }
        uint32_t Unsubscribe(IDispatcher::ICallback* callback, const uint32_t channelId, const string& event, const string& designator) {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            if (callback->Unsubscribe(channelId, event, designator) == Core::ERROR_NONE) {

                if (callback != this) {
                    // Oops the real location was somewhere else. Unregister this event also for callbacks
                    // to be forwarded to that actual location
                    _adminLock.Lock();

                    ObserverMap::iterator index = _observers.find(event);

                    if (index != _observers.end()) {
                        index->second.Unsubscribe(callback);

                        if ((result == Core::ERROR_NONE) && (index->second.IsEmpty() == true)) {
                            _observers.erase(index);
                        }
                    }
                }
                result = Core::ERROR_NONE;
            }
            return (result);
        }
        uint32_t InternalNotify(const string& event, const string& parameters, std::function<bool(const string&)>&& sendifmethod = std::function<bool(const string&)>()) const
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::const_iterator index = _observers.find(event);

            if (index != _observers.end()) {
                const_cast<Observer&>(index->second).Event(const_cast<JSONRPC&>(*this), event, parameters, std::move(sendifmethod));
            }

            _adminLock.Unlock();

            return (result);
        }
        void Notify(const uint32_t channelId, const string& designator, const string& parameters)
        {
            Core::ProxyType<Core::JSONRPC::Message> message(Core::ProxyType<Core::JSONRPC::Message>(IFactories::Instance().JSONRPC()));

            ASSERT(_service != nullptr);

            if (!parameters.empty()) {
                message->Parameters = parameters;
            }

            message->Designator = designator;
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            _service->Submit(channelId, Core::ProxyType<Core::JSON::IElement>(message));
        }

    private:
        mutable Core::CriticalSection _adminLock;
        HandlerList _handlers;
        IShell* _service;
        string _callsign;
        TokenCheckFunction _validate;
        VersionList _versions;
        ObserverMap _observers;
    };

    class EXTERNAL JSONRPCSupportsEventStatus : public PluginHost::JSONRPC {
    public:
        JSONRPCSupportsEventStatus(const JSONRPCSupportsEventStatus&) = delete;
        JSONRPCSupportsEventStatus& operator=(const JSONRPCSupportsEventStatus&) = delete;

        JSONRPCSupportsEventStatus() = default;
        JSONRPCSupportsEventStatus(const PluginHost::JSONRPC::TokenCheckFunction& validation) : JSONRPC(validation) {}
        JSONRPCSupportsEventStatus(const std::vector<uint8_t>& versions) : JSONRPC(versions) {}
        JSONRPCSupportsEventStatus(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation) : JSONRPC(versions, validation) {}
        virtual ~JSONRPCSupportsEventStatus() = default;

        enum class Status {
            registered,
            unregistered
        };

    public:
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

    private:
        using EventStatusCallback = std::function<void(const string&, Status status)>;
        using StatusCallbackMap = std::map<string, EventStatusCallback>;

        mutable Core::CriticalSection _adminLock;
        StatusCallbackMap _observers;
    };

} // namespace WPEFramework::PluginHost
}
