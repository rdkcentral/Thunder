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

  namespace {

        static string Pack(const string& event, const string& index)
        {
            string packed = event;
            if (index.empty() == false) {
                packed += TCHAR('@') + index;
            }
            return packed;
        }

        template<typename JSONRPCERRORASSESSORTYPE>
        uint32_t InvokeOnHandler(const Core::JSONRPC::Context& context, const string& method, const string& parameters, string& response, Core::JSONRPC::Handler& handler, JSONRPCERRORASSESSORTYPE errorhandler) 
        {
            uint32_t result = handler.Invoke(context, method, parameters, response);
            if(result != Core::ERROR_NONE) {
                result = errorhandler(context, method, parameters, result, response);
            }
            return result;
        }
        template<>
        uint32_t InvokeOnHandler<void*>(const Core::JSONRPC::Context& context, const string& method, const string& parameters, string& response, Core::JSONRPC::Handler& handler, void*)
        {
            return handler.Invoke(context, method, parameters, response);
        }
    }

    struct EXTERNAL ILocalDispatcher : public IDispatcher {
        virtual ~ILocalDispatcher() override = default;

        virtual uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) = 0;

        virtual void Activate(IShell* service) = 0;
        virtual void Deactivate() = 0;
        virtual void Dropped(const uint32_t channelId) = 0;
    };

    class EXTERNAL JSONRPC : public ILocalDispatcher, public IDispatcher::ICallback {
    public:
        using SendIfMethod = std::function<bool(const string&)>;
        using SendIfMethodIndexed = std::function<bool(const string&, const string&)>;

    private:
        class Observer {
        private:
            class Destination {
            public:
                Destination() = delete;
                Destination(uint32_t channelId, const string& designator)
                    : _callback(nullptr)
                    , _channelId(channelId)
                    , _designator(designator)
                    , _index() {
                }
                Destination(uint32_t channelId, const string& designator, const string& index)
                    : _callback(nullptr)
                    , _channelId(channelId)
                    , _designator(designator)
                    , _index(index) {
                }
                Destination(IDispatcher::ICallback* callback, const string& designator)
                    : _callback(callback)
                    , _channelId(~0)
                    , _designator(designator)
                    , _index() {
                    if (_callback != nullptr) {
                        _callback->AddRef();
                    }
                }
                Destination(IDispatcher::ICallback* callback, const string& designator, const string& index)
                    : _callback(callback)
                    , _channelId(~0)
                    , _designator(designator)
                    , _index(index) {
                    if (_callback != nullptr) {
                        _callback->AddRef();
                    }
                }
                Destination(Destination&& move) noexcept
                    : _callback(move._callback)
                    , _channelId(move._channelId)
                    , _designator(std::move(move._designator))
                    , _index(std::move(move._index)) {
                    move._callback = nullptr;
                    move._channelId = ~0;
                }
                Destination(const Destination& copy)
                    : _callback(copy._callback)
                    , _channelId(copy._channelId)
                    , _designator(copy._designator)
                    , _index(copy._index) {
                    if (_callback != nullptr) {
                        _callback->AddRef();
                    }
                }
                ~Destination() {
                    if (_callback != nullptr) {
                        _callback->Release();
                    }
                }

                Destination& operator=(Destination&& move) noexcept
                {
                    if (_callback != nullptr) {
                        _callback->Release();
                    }
                    _callback = move._callback;
                    _channelId = move._channelId;
                    _designator = std::move(move._designator);
                    _index = std::move(move._index);
                    move._callback = nullptr;
                    move._channelId = ~0;
                    return (*this);
                }
                Destination& operator=(const Destination& copy)
                {
                    if (_callback != nullptr) {
                        _callback->Release();
                    }
                    _callback = copy._callback;
                    _channelId = copy._channelId;
                    _designator = copy._designator;
                    _index = copy._index;
                    if (_callback != nullptr) {
                        _callback->AddRef();
                    }
                    return (*this);
                }

            public:
                inline IDispatcher::ICallback* Callback() {
                    return (_callback);
                }
                inline uint32_t ChannelId() const {
                    return (_channelId);
                }
                inline const string& Designator() const {
                    return (_designator);
                }
                inline const string& Index() const {
                    return (_index);
                }

            private:
                IDispatcher::ICallback* _callback;
                uint32_t _channelId;
                string _designator;
                string _index;
            };

            using Destinations = std::vector<Destination>;

        public:
            Observer& operator=(Observer&&) = delete;
            Observer& operator=(const Observer&) = delete;

            Observer()
                : _designators() {
            }
            Observer(Observer&& move)
                : _designators(std::move(move._designators)) {
            }
            Observer(const Observer& copy)
                : _designators(copy._designators) {
            }
            ~Observer() = default;

        public:
            bool IsEmpty() const {
                return ( _designators.empty() );
            }
            uint32_t Subscribe(const uint32_t id, const string& designator, const string& index) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator it(_designators.begin());
                while ((it != _designators.end()) && ((it->ChannelId() != id) || (it->Designator() != designator) || (it->Index() != index))) {
                    it++;
                }

                if (it == _designators.end()) {
                    _designators.emplace_back(id, designator, index);
                }
                else {
                    result = Core::ERROR_DUPLICATE_KEY;
                }

                return (result);
            }
            uint32_t Unsubscribe(const uint32_t id, const string& designator, const string& index) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator it(_designators.begin());
                while ((it != _designators.end()) && ((it->ChannelId() != id) || (it->Designator() != designator) || (it->Index() != index))) {
                    it++;
                }

                if (it != _designators.end()) {
                    _designators.erase(it);
                }
                else {
                    result = Core::ERROR_BAD_REQUEST;
                }

                return (result);
            }
            uint32_t Subscribe(IDispatcher::ICallback* callback, const string& designator, const string& index) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator it(_designators.begin());
                while ((it != _designators.end()) && ((it->Designator() != designator) || (it->Index() != index) || (it->Callback() == callback))) {
                    it++;
                }

                if (it == _designators.end()) {
                    _designators.emplace_back(callback, designator, index);
                }
                else {
                    result = Core::ERROR_DUPLICATE_KEY;
                }

                return (result);
            }
            uint32_t Unsubscribe(IDispatcher::ICallback* callback, const string& designator, const string& index) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator it(_designators.begin());
                while ((it != _designators.end()) && ((it->Designator() != designator) || (it->Index() != index) || (it->Callback() == callback))) {
                    it++;
                }

                if (it != _designators.end()) {
                    _designators.erase(it);
                }
                else {
                    result = Core::ERROR_BAD_REQUEST;
                }

                return (result);
            }
            void Dropped(const IDispatcher::ICallback* callback) {
                Destinations::iterator index = _designators.begin();
                while (index != _designators.end()) {
                    if (index->Callback() == callback) {
                        index = _designators.erase(index);
                    }
                    else {
                        index++;
                    }
                }
            }
            template<typename METHOD>
            void Dropped(const uint32_t channelId, METHOD unregistered) {
                Destinations::iterator index = _designators.begin();
                while (index != _designators.end()) {
                    if ( (index->ChannelId() == channelId) && (index->Callback() == nullptr) ) {
                        unregistered(index->Designator(), index->Index());
                        index = _designators.erase(index);
                    } 
                    else {
                        index++;
                    }
                }
            }
            void Event(const JSONRPC& parent, const string event, const string& parameter, const SendIfMethod& sendifmethod) {
                Destinations::iterator index(_designators.begin());

                while (index != _designators.end()) {
                    Destination& entry = (*index);

                    if (!sendifmethod || sendifmethod(entry.Designator())) {
                        if (entry.Callback() == nullptr) {
                            parent.Notify(entry.ChannelId(), (entry.Designator() + '.' + event), parameter);
                        }
                        else {
                            entry.Callback()->Event(event, parameter);
                        }
                    }

                    ++index;
                }
            }
            void Event(const JSONRPC& parent, const string event, const string& parameter, const SendIfMethodIndexed& sendifmethod) {
                Destinations::iterator index(_designators.begin());

                while (index != _designators.end()) {
                    Destination& entry = (*index);

                    if (!sendifmethod || sendifmethod(entry.Designator(), entry.Index())) {
                        if (entry.Callback() == nullptr) {
                            parent.Notify(entry.ChannelId(), Pack((entry.Designator() + '.' + event), entry.Index()), parameter);
                        }
                        else {
                            // Have to sneak in the index into event name
                            entry.Callback()->Event(Pack(event, entry.Index()), parameter);
                        }
                    }

                    ++index;
                }
            }

        private:
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
                , Id()
            {
                Add(_T("event"), &Event);
                Add(_T("id"), &Id);
            }
            ~Registration() override = default;

        public:
            Core::JSON::String Event;
            Core::JSON::String Id;
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
        template <typename JSONOBJECT, typename std::enable_if<!std::is_convertible<JSONOBJECT, SendIfMethod>::value && !std::is_convertible<JSONOBJECT, SendIfMethodIndexed>::value, int>::type = 0>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters) const
        {
            string subject;
            parameters.ToString(subject);
            return (InternalNotify(event, subject));
        }
        template <typename SENDIFMETHOD, typename std::enable_if<std::is_convertible<SENDIFMETHOD, SendIfMethod>::value || std::is_convertible<SENDIFMETHOD, SendIfMethodIndexed>::value, int>::type = 0>
        uint32_t Notify(const string& event, SENDIFMETHOD method) const
        {
            return InternalNotify(event, _T(""), std::move(method));
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
                    return (Core::ERROR_PRIVILIGED_DEFERRED);
                }
            }
            return (Core::ERROR_NONE);
        }

        Core::hresult Invoke(IDispatcher::ICallback* callback, const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override 
        {
            return InvokeHandler(callback, channelId, id, token, method, parameters, response);
        }

        template<typename JSONRPCERRORASSESSORTYPE = void*>
        Core::hresult InvokeHandler(IDispatcher::ICallback*, const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response, JSONRPCERRORASSESSORTYPE errorhandler = nullptr) 
        {
            uint32_t result(Core::ERROR_UNKNOWN_METHOD);
            Core::JSONRPC::Handler* handler(Handler(method));
            string realMethod(Core::JSONRPC::Message::Method(method));

            if (handler == nullptr) {
                result = Core::ERROR_INVALID_RANGE;
            }
            else if (realMethod == _T("exists")) {
                result = Core::ERROR_NONE;
                if (handler->Exists(parameters) == Core::ERROR_NONE) {
                    response = Core::NumberType<uint32_t>(Core::ERROR_NONE).Text();
                }
                else {
                    response = Core::NumberType<uint32_t>(Core::ERROR_UNKNOWN_KEY).Text();
                }
            }
            else if (handler->Exists(realMethod) == Core::ERROR_NONE) {
                Core::JSONRPC::Context context(channelId, id, token);
                result = InvokeOnHandler<JSONRPCERRORASSESSORTYPE>(context, Core::JSONRPC::Message::FullMethod(method), parameters, response, *handler, errorhandler);
            }
            return (result);
        }
        Core::hresult Revoke(IDispatcher::ICallback* callback) override {
            // See if we re using this callback, we need to abort its use..
            for (std::pair<const string, Observer>& entry : _observers) {
                entry.second.Dropped(callback);
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

            string realMethod = Core::JSONRPC::Message::Method(method);
            string callsign = Core::JSONRPC::Message::Callsign(method);

            ASSERT((callsign.empty() == true) || (callsign == _callsign));

            if (realMethod == _T("register")) {
                Registration info;  info.FromString(parameters);
                string index = Core::JSONRPC::Message::Index(method);

                result = Subscribe(this, channelId, info.Event.Value(), info.Id.Value(), index);
                if (result == Core::ERROR_NONE) {
                    response = _T("0");
                }
                else {
                    result = Core::ERROR_FAILED_REGISTERED;
                }
            }
            else if (realMethod == _T("unregister")) {
                Registration info;  info.FromString(parameters);
                string index = Core::JSONRPC::Message::Index(method);

                result = Unsubscribe(this, channelId, info.Event.Value(), info.Id.Value(), index);
                if (result == Core::ERROR_NONE) {
                    response = _T("0");
                }
                else {
                    result = Core::ERROR_FAILED_UNREGISTERED;
                }
            }
            else {
                result = Invoke(this, channelId, id, token, Core::JSONRPC::Message::FullMethod(method), parameters, response);
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

                const string& eventId = index->first;
                index->second.Dropped(channelId, [this, channelId, &eventId](const string& clientId, const string& index) { ProcessUnsubscribed(channelId, eventId, clientId, index); });

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
                index->second.Event(*this, eventId, parameters, SendIfMethod());
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
        Core::hresult Subscribe(const uint32_t channel, const string& event, const string& clientId) final
        {
            // id and index are packed into event name
            const string eventId = Core::JSONRPC::Message::Method(event);
            const string index = Core::JSONRPC::Message::Index(event);

            return ProcessSubscribe(channel, eventId, clientId, index);
        }
        Core::hresult Unsubscribe(const uint32_t channel, const string& event, const string& clientId) final
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            const string eventId = Core::JSONRPC::Message::Method(event);

            _adminLock.Lock();

            ObserverMap::iterator it = _observers.find(eventId);

            if (it != _observers.end()) {
                const string index = Core::JSONRPC::Message::Index(event);

                result = it->second.Unsubscribe(channel, clientId, index);

                if (result == Core::ERROR_NONE) {

                    ProcessUnsubscribed(channel, eventId, clientId, index);

                    if (it->second.IsEmpty() == true) {
                        _observers.erase(it);
                    }
                }
            }
            _adminLock.Unlock();

            return (result);
        }

    protected:
        uint32_t DoSubscribe(const uint32_t channelId, const string& event, const string& clientId, const string& index)
        {
            _adminLock.Lock();

            ObserverMap::iterator it = _observers.find(event);

            if (it == _observers.end()) {
                it = _observers.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(event),
                                      std::forward_as_tuple())
                            .first;
            }

            uint32_t result = it->second.Subscribe(channelId, clientId, index);

            if ((result != Core::ERROR_NONE) && (it->second.IsEmpty() == true)) {
                _observers.erase(it);
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        virtual uint32_t ProcessSubscribe(const uint32_t channelId, const string& event, const string& clientId, const string& index)
        {
            return DoSubscribe(channelId, event, clientId, index);
        }

        virtual void ProcessUnsubscribed(const uint32_t channelId VARIABLE_IS_NOT_USED, const string& event VARIABLE_IS_NOT_USED, const string& clientId VARIABLE_IS_NOT_USED, const string& index VARIABLE_IS_NOT_USED) 
        {
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
        uint32_t Subscribe(IDispatcher::ICallback* callback, const uint32_t channelId, const string& eventId, const string& clientId, const string& index) {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            // This is to make sure that the actuall location (there weher the channels really end) are
            // aware of distributing the event.
            if (callback->Subscribe(channelId, Pack(eventId, index), clientId) == Core::ERROR_NONE) {

                if (callback != this) {
                    // Oops the real location is somewhere else. Register this event also for callbacks
                    // to be forwarded to that actual location
                    _adminLock.Lock();

                    ObserverMap::iterator it = _observers.find(eventId);

                    if (it == _observers.end()) {
                        it = _observers.emplace(std::piecewise_construct,
                            std::forward_as_tuple(eventId),
                            std::forward_as_tuple()).first;
                    }

                    it->second.Subscribe(callback, clientId, index);

                    _adminLock.Unlock();
                }
                result = Core::ERROR_NONE;
            }
            return (result);
        }
        uint32_t Unsubscribe(IDispatcher::ICallback* callback, const uint32_t channelId, const string& eventId, const string& clientId, const string& index) {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            if (callback->Unsubscribe(channelId, Pack(eventId, index), clientId) == Core::ERROR_NONE) {

                if (callback != this) {
                    // Oops the real location was somewhere else. Unregister this event also for callbacks
                    // to be forwarded to that actual location
                    _adminLock.Lock();

                    ObserverMap::iterator it = _observers.find(eventId);

                    if (it != _observers.end()) {
                        it->second.Unsubscribe(callback, clientId, index);

                        if ((result == Core::ERROR_NONE) && (it->second.IsEmpty() == true)) {
                            _observers.erase(it);
                        }
                    }
                }
                result = Core::ERROR_NONE;
            }
            return (result);
        }
        template<typename SENDIFMETHOD = SendIfMethod>
        uint32_t InternalNotify(const string& event, const string& parameters, SENDIFMETHOD sendifmethod = nullptr) const
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(event);

            if (index != _observers.end()) {
               index->second.Event(*this, event, parameters, std::move(sendifmethod));
            }

            _adminLock.Unlock();

            return (result);
        }
        void Notify(const uint32_t channelId, const string& designator, const string& parameters) const
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
        mutable ObserverMap _observers;
    };

    class EXTERNAL JSONRPCSupportsEventStatus : virtual public PluginHost::JSONRPC {
    public:
        JSONRPCSupportsEventStatus(const JSONRPCSupportsEventStatus&) = delete;
        JSONRPCSupportsEventStatus& operator=(const JSONRPCSupportsEventStatus&) = delete;
        JSONRPCSupportsEventStatus(JSONRPCSupportsEventStatus&&) = delete;
        JSONRPCSupportsEventStatus& operator=(JSONRPCSupportsEventStatus&&) = delete;

        JSONRPCSupportsEventStatus()
            : _adminLock()
            , _observers()
        {
        }
        JSONRPCSupportsEventStatus(const PluginHost::JSONRPC::TokenCheckFunction& validation)
            : JSONRPC(validation)
            , _adminLock()
            , _observers()
        {
        }
        JSONRPCSupportsEventStatus(const std::vector<uint8_t>& versions)
            : JSONRPC(versions)
            , _adminLock()
            , _observers()
        {
        }
        JSONRPCSupportsEventStatus(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation)
            : JSONRPC(versions, validation)
            , _adminLock()
            , _observers()
        {
        }
        ~JSONRPCSupportsEventStatus() override = default;

        enum class Status {
            registered,
            unregistered
        };

    public:
        template<typename METHOD>
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


    private:
        uint32_t ProcessSubscribe(const uint32_t channel, const string& event, const string& clientId, const string& index) override
        {
            Core::hresult result = Core::ERROR_PRIVILIGED_REQUEST;

            _adminLock.Lock();

            result = JSONRPC::DoSubscribe(channel, event, clientId, index);

            if (result == Core::ERROR_NONE) {
                NotifyObservers(channel, event, clientId, index, Status::registered);
            }

            _adminLock.Unlock();

            return (result);
        }
        void ProcessUnsubscribed(const uint32_t channel, const string& event, const string& clientId, const string& index) override
        {
            _adminLock.Lock();

            NotifyObservers(channel, event, clientId, index, Status::unregistered);

            _adminLock.Unlock();
        }

    private:
        void NotifyObservers(const uint32_t channel, const string event, const string& clientId, const string& index, const Status status) const
        {
            StatusCallbackMap::const_iterator it = _observers.find(event);
            if (it != _observers.cend()) {
                it->second(channel, clientId, index, status);
            }
        }

    private:
        using EventStatusCallback = std::function<void(const uint32_t, const string&, const string&, Status status)>;
        using StatusCallbackMap = std::map<string, EventStatusCallback>;

        mutable Core::CriticalSection _adminLock;
        StatusCallbackMap _observers;
    };

    namespace JSONRPCErrorAssessorTypes {
        using FunctionCallbackType  = uint32_t (*) (const Core::JSONRPC::Context&, const string&, const string&, const uint32_t errorcode, string&);
        using StdFunctionCallbackType = std::function<int32_t(const Core::JSONRPC::Context&, const string&, const string&, const uint32_t errorcode, string&)>;
    }

    template<typename JSONRPCERRORASSESSORTYPE>
    class EXTERNAL JSONRPCErrorAssessor : public JSONRPC {
    public:
        JSONRPCErrorAssessor(JSONRPCERRORASSESSORTYPE errorhandler) 
            : JSONRPC()
            , _errorhandler(errorhandler)
            {
            }
        ~JSONRPCErrorAssessor() override = default;
        JSONRPCErrorAssessor(const JSONRPCErrorAssessor&) = delete;
        JSONRPCErrorAssessor &operator=(const JSONRPCErrorAssessor&) = delete;
        JSONRPCErrorAssessor(JSONRPCErrorAssessor&&) = delete;
        JSONRPCErrorAssessor &operator=(JSONRPCErrorAssessor&&) = delete;
        Core::hresult Invoke(IDispatcher::ICallback* callback, const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override 
        {
            return JSONRPC::InvokeHandler<JSONRPCERRORASSESSORTYPE>(callback, channelId, id, token, method, parameters, response, _errorhandler);
        }
        private:
            JSONRPCERRORASSESSORTYPE _errorhandler;
    };
    template<>
    class EXTERNAL JSONRPCErrorAssessor<JSONRPCErrorAssessorTypes::StdFunctionCallbackType> : public JSONRPC {
    public:
        JSONRPCErrorAssessor(const JSONRPCErrorAssessorTypes::StdFunctionCallbackType& errorhandler) 
            : JSONRPC()
            , _errorhandler(errorhandler)
            {
            }
        JSONRPCErrorAssessor(JSONRPCErrorAssessorTypes::StdFunctionCallbackType&& errorhandler) 
            : JSONRPC()
            , _errorhandler(std::move(errorhandler))
            {
            }
        ~JSONRPCErrorAssessor() override = default;
        JSONRPCErrorAssessor(const JSONRPCErrorAssessor&) = delete;
        JSONRPCErrorAssessor &operator=(const JSONRPCErrorAssessor&) = delete;
        JSONRPCErrorAssessor(JSONRPCErrorAssessor&&) = delete;
        JSONRPCErrorAssessor &operator=(JSONRPCErrorAssessor&&) = delete;
        Core::hresult Invoke(IDispatcher::ICallback* callback, const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override 
        {
            return JSONRPC::InvokeHandler<const JSONRPCErrorAssessorTypes::StdFunctionCallbackType&>(callback, channelId, id, token, method, parameters, response, _errorhandler);
        }
        private:
            JSONRPCErrorAssessorTypes::StdFunctionCallbackType _errorhandler;
    };

} // namespace WPEFramework::PluginHost
}
