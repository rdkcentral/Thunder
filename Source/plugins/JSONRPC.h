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

namespace Thunder {

namespace PluginHost {

    namespace {

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

    class EXTERNAL JSONRPC : public IDispatcher {
    public:
        using SendIfMethod = std::function<bool(const string&)>;

    private:
        class Notification : public IShell::IConnectionServer::INotification {
        public:
            Notification(JSONRPC& parent)
                : _parent(parent)
            {
            }
            ~Notification() = default;

            Notification(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification& operator=(Notification&&) = delete;

        public:
            void Opened(const uint32_t channelId VARIABLE_IS_NOT_USED) override
            {
            }
            void Closed(const uint32_t channelId) override
            {
                _parent.ChannelClosed(channelId);
            }

        public:
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(IShell::IConnectionServer::INotification)
            END_INTERFACE_MAP

        private:
            JSONRPC& _parent;
        };

    private:
        class Observer {
        private:
            class Destination {
            public:
                Destination() = delete;
                Destination(uint32_t channelId, const string& designator, const bool oneShot = false)
                    : _callback(nullptr)
                    , _channelId(channelId)
                    , _designator(designator)
                    , _oneShot(oneShot) {
                }
                Destination(IDispatcher::ICallback* callback, const string& designator)
                    : _callback(callback)
                    , _channelId(~0)
                    , _designator(designator)
                    , _oneShot(false) {
                    if (_callback != nullptr) {
                        _callback->AddRef();
                    }
                }
                Destination(Destination&& move) noexcept
                    : _callback(move._callback)
                    , _channelId(move._channelId)
                    , _designator(std::move(move._designator))
                    , _oneShot(move._oneShot) {
                    move._callback = nullptr;
                    move._channelId = ~0;
                }
                Destination(const Destination& copy)
                    : _callback(copy._callback)
                    , _channelId(copy._channelId)
                    , _designator(copy._designator)
                    , _oneShot(copy._oneShot) {
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
                    _oneShot = copy._oneShot;
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
                inline bool IsOneShot() const {
                    return (_oneShot);
                }

            private:
                IDispatcher::ICallback* _callback;
                uint32_t _channelId;
                string _designator;
                bool _oneShot;
            };
            using Destinations = std::vector<Destination>;

        public:
            Observer& operator=(Observer&&) = delete;
            Observer& operator=(const Observer&) = delete;

            Observer()
                : _designators() {
            }
            Observer(Observer&& move) noexcept
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
            uint32_t Subscribe(const uint32_t id, const string& designator, const bool oneShot) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->ChannelId() != id) || (index->Designator() != designator))) {
                    index++;
                }

                if (index == _designators.end()) {
                    _designators.emplace_back(id, designator, oneShot);
                }
                else {
                    result = Core::ERROR_DUPLICATE_KEY;
                }

                return (result);
            }
            uint32_t Unsubscribe(const uint32_t id, const string& designator) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->ChannelId() != id) || (index->Designator() != designator))) {
                    index++;
                }

                if (index != _designators.end()) {
                    _designators.erase(index);
                }
                else {
                    result = Core::ERROR_BAD_REQUEST;
                }

                return (result);
            }
            uint32_t Subscribe(IDispatcher::ICallback* callback, const string& designator) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->Designator() != designator) || (index->Callback() == callback))) {
                    index++;
                }

                if (index == _designators.end()) {
                    _designators.emplace_back(callback, designator);
                }
                else {
                    result = Core::ERROR_DUPLICATE_KEY;
                }

                return (result);
            }
            uint32_t Unsubscribe(IDispatcher::ICallback* callback, const string& designator) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->Designator() != designator) || (index->Callback() == callback))) {
                    index++;
                }

                if (index != _designators.end()) {
                    _designators.erase(index);
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
                        unregistered(index->Designator());
                        index = _designators.erase(index);
                    }
                    else {
                        index++;
                    }
                }
            }
            void Event(JSONRPC& parent, const string event, const string& parameter, const SendIfMethod& sendifmethod) {
                Destinations::iterator index(_designators.begin());

                while (index != _designators.end()) {
                    Destination& entry = (*index);

                    if (!sendifmethod || sendifmethod(entry.Designator())) {
                        if (entry.Callback() == nullptr) {
                            parent.Notify(entry.ChannelId(), entry.Designator() + '.' + event, parameter);
                        }
                        else {
                            entry.Callback()->Event(event, entry.Designator(), parameter);
                        }
                    }

                    if (entry.IsOneShot() == true) {
                        index = _designators.erase(index);
                    }
                    else {
                        ++index;
                    }
                }
            }

        private:
            Destinations _designators;
        };

        using HandlerList = std::list<Core::JSONRPC::Handler>;
        using ObserverMap = std::unordered_map<string, Observer>;
        using EventAliasesMap = std::map<string, std::vector<string>>;

        class VersionInfo : public Core::JSON::Container {
        public:
            VersionInfo()
                : Core::JSON::Container()
                , Name()
                , Major()
                , Minor()
                , Patch()
            {
                Init();
            }
            VersionInfo(const VersionInfo& copy)
                : Core::JSON::Container()
                , Name(copy.Name)
                , Major(copy.Major)
                , Minor(copy.Minor)
                , Patch(copy.Patch)
            {
                Init();
            }
            VersionInfo(VersionInfo&& move) noexcept
                : Core::JSON::Container()
                , Name(std::move(move.Name))
                , Major(std::move(move.Major))
                , Minor(std::move(move.Minor))
                , Patch(std::move(move.Patch))
            {
                Init();
            }
            VersionInfo(const string& interfaceName, const uint8_t major, const uint8_t minor, const uint8_t patch)
                : VersionInfo()
            {
                Name = interfaceName;
                Major = major;
                Minor = minor;
                Patch = patch;
            }
            VersionInfo& operator=(const VersionInfo& rhs)
            {
                Name = rhs.Name;
                Major = rhs.Major;
                Minor = rhs.Minor;
                Patch = rhs.Patch;
                return (*this);
            }
            VersionInfo& operator=(VersionInfo&& move) noexcept
            {
                if (this != &move) {
                    Name = std::move(move.Name);
                    Major = std::move(move.Major);
                    Minor = std::move(move.Minor);
                    Patch = std::move(move.Patch);
		}
                return (*this);
            }
            ~VersionInfo() = default;

        private:
            void Init()
            {
                Add(_T("name"), &Name);
                Add(_T("major"), &Major);
                Add(_T("minor"), &Minor);
                Add(_T("patch"), &Patch);
            }

        public:
            Core::JSON::String Name;
            Core::JSON::DecUInt8 Major;
            Core::JSON::DecUInt8 Minor;
            Core::JSON::DecUInt8 Patch;
        };

        using VersionList = Core::JSON::ArrayType<VersionInfo>;

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

#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__

        template <typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handlers.front().Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }

#endif // __DISABLE_USE_COMPLEMENTARY_CODE_SET__

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
        void Register(const string& methodName, const string& originalName)
        {
            _handlers.front().Register(methodName, originalName);
        }
        void Unregister(const string& methodName)
        {
            _handlers.front().Unregister(methodName);
        }

        //
        // Register/Unregister methods for interface versioning..
        // ------------------------------------------------------------------------------------------------------------------------------
        void RegisterVersion(const string& name, const uint8_t major, const uint8_t minor, const uint8_t patch)
        {
            VersionInfo& version = _versions.Add();
            version.Name = name;
            version.Major = major;
            version.Minor = minor;
            version.Patch = patch;
        }

        //
        // Methods to register alternative event names
        // ------------------------------------------------------------------------------------------------------------------------------
        void RegisterEventAlias(const string& event, const string& primary)
        {
            ASSERT(event.empty() == false);
            ASSERT(primary.empty() == false);

            _adminLock.Lock();

            _eventAliases[primary].emplace_back(event);

            ASSERT(std::count(_eventAliases[primary].begin(), _eventAliases[primary].end(), event) == 1);

            _adminLock.Unlock();
        }
        void UnregisterEventAlias(const string& event, const string& primary)
        {
            ASSERT(event.empty() == false);
            ASSERT(primary.empty() == false);

            _adminLock.Lock();

            auto const& entry = _eventAliases.find(primary);
            ASSERT(entry != _eventAliases.end());

            if (entry != _eventAliases.end()) {
                auto it = std::find(entry->second.begin(), entry->second.end(), event);
                ASSERT(it != entry->second.end());

                if (it != entry->second.end()) {
                    entry->second.erase(it);

                    if (entry->second.empty() == true) {
                        _eventAliases.erase(entry);
                    }
                }
            }

            _adminLock.Unlock();
        }

        //
        // Methods to send outbound event messages
        // ------------------------------------------------------------------------------------------------------------------------------
        uint32_t Notify(const string& event) const
        {
            return (InternalNotify(event, _T("")));
        }
        template <typename JSONOBJECT, typename std::enable_if<!std::is_convertible<JSONOBJECT, SendIfMethod>::value, int>::type = 0>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters) const
        {
            string subject;
            parameters.ToString(subject);
            return (InternalNotify(event, subject));
        }
        template <typename SENDIFMETHOD, typename std::enable_if<std::is_convertible<SENDIFMETHOD, SendIfMethod>::value, int>::type = 0>
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
        Core::hresult Event(const string& eventId, const string& parameters) {
            return (InternalNotify(eventId, parameters));
        }
        void Response(const uint32_t channelId, const Core::ProxyType<Core::JSON::IElement>& message) {
            _service->Submit(channelId, message);
        }

        // Inherited via IDispatcher
        // ---------------------------------------------------------------------------------
        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override
        {
            return InvokeHandler(channelId, id, token, method, parameters, response);
        }

        template<typename JSONRPCERRORASSESSORTYPE = void*>
        uint32_t InvokeHandler(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response, JSONRPCERRORASSESSORTYPE errorhandler = nullptr)         {
            uint32_t result = Core::ERROR_PARSE_FAILURE;

            if (method.empty() == false) {

                string callsign;
                string prefix;
                string instanceId;
                string methodName;

                Core::JSONRPC::Message::Split(method, &callsign, nullptr, &prefix, &instanceId, &methodName, nullptr);

                const string realMethod = Core::JSONRPC::Message::Join(prefix, methodName);

                ASSERT((callsign.empty() == true) || (callsign == _callsign));

                result = Core::ERROR_NONE;

                if (_validate != nullptr) {
                    classification validation = _validate(token, realMethod, parameters);
                    if (validation == classification::INVALID) {
                        result = Core::ERROR_PRIVILIGED_REQUEST;
                    }
                    else if (validation == classification::DEFERRED) {
                        result = Core::ERROR_PRIVILIGED_DEFERRED;
                    }
                }

                if (result == Core::ERROR_NONE) {

                    // Seems we are on the right handler..
                    // now see if someone supports this version

                    if (realMethod == _T("versions")) {
                        _versions.ToString(response);
                        result = Core::ERROR_NONE;
                    }
                    else if (realMethod == _T("exists")) {
                        if (Handler(parameters) == nullptr) {
                            response = _T("0");
                        }
                        else {
                            response = _T("1");
                        }
                    }
                    else if (methodName == _T("register")) {
                        Registration info;  info.FromString(parameters);

                        result = Subscribe(channelId, Core::JSONRPC::Message::Join(prefix, instanceId, info.Event.Value()), info.Id.Value());
                        if (result == Core::ERROR_NONE) {
                            response = _T("0");
                        }
                        else {
                            result = Core::ERROR_FAILED_REGISTERED;
                        }
                    }
                    else if (methodName == _T("unregister")) {
                        Registration info;  info.FromString(parameters);

                        result = Unsubscribe(channelId, Core::JSONRPC::Message::Join(prefix, instanceId, info.Event.Value()), info.Id.Value());
                        if (result == Core::ERROR_NONE) {
                            response = _T("0");
                        }
                        else {
                            result = Core::ERROR_FAILED_UNREGISTERED;
                        }
                    }
                    else {
                        Core::JSONRPC::Handler* handler(Handler(realMethod));

                        if (handler != nullptr) {
                            Core::JSONRPC::Context context(channelId, id, token);
                            result = InvokeOnHandler<JSONRPCERRORASSESSORTYPE>(context, Core::JSONRPC::Message::FullMethod(method), parameters, response, *handler, errorhandler);
                        } else {
                            result = Core::ERROR_INCORRECT_URL;
                        }
                    }
                }
            }

            return (result);
        }

        Core::hresult Subscribe(ICallback* callback, const string& eventId, const string& designator) override
        {
            uint32_t result;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index == _observers.end()) {
                index = _observers.emplace(std::piecewise_construct,
                    std::forward_as_tuple(eventId),
                    std::forward_as_tuple()).first;
            }

            result = index->second.Subscribe(callback, designator);

            if ((result != Core::ERROR_NONE) && (index->second.IsEmpty() == true)) {
                _observers.erase(index);
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Unsubscribe(ICallback* callback, const string& eventId, const string& designator) override
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index != _observers.end()) {
                result = index->second.Unsubscribe(callback, designator);

                if ((result == Core::ERROR_NONE) && (index->second.IsEmpty() == true)) {
                    _observers.erase(index);
                }
            }
            _adminLock.Unlock();

            return (result);
        }

        Core::hresult Attach(IShell::IConnectionServer::INotification*& sink /* @out */, IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);

            _adminLock.Lock();

            _service = service;
            _service->AddRef();
            _callsign = _service->Callsign();

            sink = &_notification;
            sink->AddRef();

            _adminLock.Unlock();

            return (Core::ERROR_NONE);
        }
        Core::hresult Detach(IShell::IConnectionServer::INotification*& sink /* @out */) override
        {
            _adminLock.Lock();

            sink = &_notification;
            sink->AddRef();

            _callsign.clear();
            _observers.clear();

            _adminLock.Unlock();

            return (Core::ERROR_NONE);
        }

        void Dropped(const IDispatcher::ICallback* callback) override {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.begin();

            while (index != _observers.end()) {

                index->second.Dropped(callback);

                if (index->second.IsEmpty() == true) {
                    index = _observers.erase(index);
                }
                else {
                    index++;
                }
            }
            _adminLock.Unlock();
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

    public:
        uint32_t Subscribe(const uint32_t channelId, const string& eventId, const string& designator, const bool oneShot = false)
        {
            uint32_t result = ProcessSubscribe(channelId, eventId, designator, oneShot);

            return (result);
        }

        uint32_t Unsubscribe(const uint32_t channelId, const string& eventId, const string& designator)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index != _observers.end()) {
                result = index->second.Unsubscribe(channelId, designator);

                if (result == Core::ERROR_NONE) {

                    ProcessUnsubscribed(channelId, eventId, designator);

                    if (index->second.IsEmpty() == true) {
                        _observers.erase(index);
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }

    protected:
        uint32_t DoSubscribe(const uint32_t channelId, const string& eventId, const string& designator, const bool oneShot)
        {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index == _observers.end()) {
                index = _observers.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(eventId),
                                      std::forward_as_tuple())
                            .first;
            }

            uint32_t result = index->second.Subscribe(channelId, designator, oneShot);

            if ((result != Core::ERROR_NONE) && (index->second.IsEmpty() == true)) {
                _observers.erase(index);
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        virtual uint32_t ProcessSubscribe(const uint32_t channelId, const string& eventId, const string& designator, const bool oneShot)
        {
            return DoSubscribe(channelId, eventId, designator, oneShot);
        }

        virtual void ProcessUnsubscribed(const uint32_t channelId VARIABLE_IS_NOT_USED, const string& eventId VARIABLE_IS_NOT_USED, const string& designator VARIABLE_IS_NOT_USED) 
        {
        }

        void ChannelClosed(const uint32_t channelId)
        {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.begin();

            while (index != _observers.end()) {
                const string& eventId = index->first;
                index->second.Dropped(channelId, [this, channelId, &eventId](const string& designator) { ProcessUnsubscribed(channelId, eventId, designator); });

                if (index->second.IsEmpty() == true) {
                    index = _observers.erase(index);
                }
                else {
                    index++;
                }
            }

            _adminLock.Unlock();
        }

    private:
        uint32_t InternalNotify(const string& event, const string& parameters, const SendIfMethod& sendifmethod = nullptr) const
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(event);

            if (index != _observers.end()) {
                index->second.Event(const_cast<JSONRPC&>(*this), event, parameters, sendifmethod);

                if (index->second.IsEmpty() == true) {
                    // A one-shot observer might've removed itself, so remove the event from being observed
                    _observers.erase(index);
                }
            }

            // See if this is perhaps a registered alias for an event...

            EventAliasesMap::const_iterator iter = _eventAliases.find(event);

            if (iter != _eventAliases.end()) {

                for (const string& alias : iter->second) {
                    ObserverMap::iterator index = _observers.find(alias);

                    if (index != _observers.end()) {
                        index->second.Event(const_cast<JSONRPC&>(*this), alias, parameters, sendifmethod);
                    }
                }
            }

            if (_service != nullptr) {
                _service->Notify(event, parameters);
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
        mutable ObserverMap _observers;
        EventAliasesMap _eventAliases;
        Core::SinkType<Notification> _notification;
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
            , _subscribeAssessor()
        {
        }
        JSONRPCSupportsEventStatus(const PluginHost::JSONRPC::TokenCheckFunction& validation)
            : JSONRPC(validation)
            , _adminLock()
            , _observers()
            , _subscribeAssessor()
        {
        }
        JSONRPCSupportsEventStatus(const std::vector<uint8_t>& versions)
            : JSONRPC(versions)
            , _adminLock()
            , _observers()
            , _subscribeAssessor()
        {
        }
        JSONRPCSupportsEventStatus(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation)
            : JSONRPC(versions, validation)
            , _adminLock()
            , _observers()
            , _subscribeAssessor()
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

            // Expect event (or prefix+event) without instance id.
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

        template<typename METHOD>
        void SetSubscribeAssessor(METHOD method)
        {
            _adminLock.Lock();

            _subscribeAssessor = method;

            _adminLock.Unlock();
        }

    private:
        uint32_t ProcessSubscribe(const uint32_t channel, const string& designator, const string& clientId, const bool oneShot) override
        {
            Core::hresult result = Core::ERROR_PRIVILIGED_REQUEST;

            string prefix;
            string instanceId;
            string event;

            Core::JSONRPC::Message::Split(designator, nullptr, nullptr, &prefix, &instanceId, &event, nullptr);

            _adminLock.Lock();

            if ((_subscribeAssessor == nullptr) || (_subscribeAssessor(channel, prefix, instanceId, event, clientId) == true)) {

                result = JSONRPC::DoSubscribe(channel, designator, clientId, oneShot);

                if (result == Core::ERROR_NONE) {
                    NotifyObservers(channel, Core::JSONRPC::Message::Join(prefix, event), instanceId, clientId, Status::registered);
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        void ProcessUnsubscribed(const uint32_t channel, const string& designator, const string& clientId) override
        {
            string prefix;
            string instanceId;
            string event;

            Core::JSONRPC::Message::Split(designator, nullptr, nullptr, &prefix, &instanceId, &event, nullptr);

            _adminLock.Lock();

            NotifyObservers(channel, Core::JSONRPC::Message::Join(prefix, event), instanceId, clientId, Status::unregistered);

            _adminLock.Unlock();
        }

    private:
        void NotifyObservers(const uint32_t channel, const string event, const string& instanceId, const string& client, const Status status) const
        {
            StatusCallbackMap::const_iterator it = _observers.find(event);
            if (it != _observers.cend()) {
                it->second(channel, instanceId, client, status);
            }
        }

    private:
        using EventStatusCallback = std::function<void(const uint32_t, const string&, const string&, Status status)>;
        using SubscribeCallback = std::function<bool(const uint32_t, const string&, const string&, const string&, const string&)>;
        using StatusCallbackMap = std::map<string, EventStatusCallback>;

        mutable Core::CriticalSection _adminLock;
        StatusCallbackMap _observers;
        SubscribeCallback _subscribeAssessor;
    };

    class EXTERNAL JSONRPCSupportsObjectLookup : virtual public PluginHost::JSONRPC {
    private:
        using ToIDCallback = std::function<string(const Core::JSONRPC::Context&, const Core::IUnknown*)>;
        using FromIDCallback = std::function<Core::IUnknown* (const Core::JSONRPC::Context&, const string&)>;
        using Handlers = std::pair<ToIDCallback, FromIDCallback>;

    public:
        JSONRPCSupportsObjectLookup(const JSONRPCSupportsObjectLookup&) = delete;
        JSONRPCSupportsObjectLookup& operator=(const JSONRPCSupportsObjectLookup&) = delete;
        JSONRPCSupportsObjectLookup(JSONRPCSupportsObjectLookup&&) = delete;
        JSONRPCSupportsObjectLookup& operator=(JSONRPCSupportsObjectLookup&&) = delete;

        JSONRPCSupportsObjectLookup()
            : JSONRPC()
            , _adminLock()
            , _handlers()
        {
        }
        JSONRPCSupportsObjectLookup(const PluginHost::JSONRPC::TokenCheckFunction& validation)
            : JSONRPC(validation)
            , _adminLock()
            , _handlers()
        {
        }
        JSONRPCSupportsObjectLookup(const std::vector<uint8_t>& versions)
            : JSONRPC(versions)
            , _adminLock()
            , _handlers()
        {
        }
        JSONRPCSupportsObjectLookup(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation)
            : JSONRPC(versions, validation)
            , _adminLock()
            , _handlers()
        {
        }
        ~JSONRPCSupportsObjectLookup() override
        {
            ASSERT(_handlers.empty() == true);
        }

    public:
        template<typename T, typename METHOD1, typename METHOD2>
        void Add(METHOD1 toIdCb, METHOD2 fromIdCb)
        {
            using FI1 = typename Core::TypeTraits::lambda_traits<METHOD1>;
            using FI1_ARG0 = typename std::remove_cv<typename std::remove_reference<typename FI1::template argument<0>::type>::type>::type;
            using FI2 = typename Core::TypeTraits::lambda_traits<METHOD2>;
            using FI2_ARG0 = typename std::remove_cv<typename std::remove_reference<typename FI2::template argument<0>::type>::type>::type;


            ToIDCallback toIdCallback([this, toIdCb](const Core::JSONRPC::Context& context, const Core::IUnknown* obj) -> string {

                string returnValue;
                const T* realInterface(obj->QueryInterface<T>());
                if (realInterface != nullptr) {
                    returnValue = Execute<T, METHOD1>(
                        context,
                        realInterface,
                        toIdCb,
                        TemplateIntToType<std::is_same<FI1_ARG0, Core::JSONRPC::Context>::value>());

                    realInterface->Release();
                }
                return (returnValue);
            });

            FromIDCallback fromIdCallback([this, fromIdCb](const Core::JSONRPC::Context& context, const string& id) -> Core::IUnknown* {

                return (Execute<METHOD2>(
                    context,
                    id,
                    fromIdCb,
                    TemplateIntToType<std::is_same<FI2_ARG0, Core::JSONRPC::Context>::value>()));
            });

            _adminLock.Lock();

            ASSERT(_handlers.find(T::ID) == _handlers.end());

            _handlers.emplace(std::piecewise_construct,
                std::forward_as_tuple(T::ID),
                std::forward_as_tuple(toIdCallback, fromIdCallback));

            _adminLock.Unlock();
        }

        template<typename T>
        void Remove()
        {
            _adminLock.Lock();

            ASSERT(_handlers.find(T::ID) != _handlers.end());

            _handlers.erase(T::ID);

            _adminLock.Unlock();
        }

        void Clear()
        {
            _adminLock.Lock();

            _handlers.clear();

            _adminLock.Unlock();
        }

        template<typename T>
        bool Exists() const
        {
            bool result = false;

            _adminLock.Lock();

            result = (_handlers.find(T::ID) != _handlers.end());

            _adminLock.Unlock();

            return (result);
        }

        template<typename T>
        T* LookUp(const string& id, const Core::JSONRPC::Context& context)
        {
            T* obj{};

            _adminLock.Lock();

            auto translator = _handlers.find(T::ID);

            if (translator != _handlers.end()) {
                Core::IUnknown* unknown = (*translator).second.second(context, id);

                if (unknown != nullptr) {
                    obj = unknown->QueryInterface<T>();
                    unknown->Release();
                }
            }

            _adminLock.Unlock();

            return (obj);
        }

        template<typename T>
        string InstanceId(const T* const object, const Core::JSONRPC::Context& context) const
        {
            string id;

            _adminLock.Lock();

            auto translator = _handlers.find(T::ID);

            if (translator != _handlers.end()) {
                id = (*translator).second.first(context, object);
            }

            _adminLock.Unlock();

            return (id);
        }

        template<typename T>
        string InstanceId(const T* const object) const
        {
            Core::JSONRPC::Context context{};
            return (InstanceId<T>(object, context));
        }

    private:
        template<typename T, typename METHOD>
        string Execute(const Core::JSONRPC::Context&, const T* obj, METHOD method, const TemplateIntToType<false>&)
        {
            using INFO = typename Core::TypeTraits::lambda_traits<METHOD>;
            return (ConvertToId<T, METHOD, typename INFO::result_type>(obj, method, TemplateIntToType<std::is_integral<typename INFO::result_type>::value>()));
        }
        template<typename T, typename METHOD, typename INDEX>
        string ConvertToId(const T* obj, METHOD method, const TemplateIntToType<false>&)
        {
            return (method(obj));
        }
        template<typename T, typename METHOD, typename INDEX>
        string ConvertToId(const T* obj, METHOD method, const TemplateIntToType<true>&)
        {
            Core::NumberType<INDEX> number(method(obj));
            return (number.Text());
        }
        template<typename T, typename METHOD>
        string Execute(const Core::JSONRPC::Context& context, const T* obj, METHOD method, const TemplateIntToType<true>&)
        {
            using INFO = typename Core::TypeTraits::lambda_traits<METHOD>;
            return (ConvertToId<T, METHOD, typename INFO::result_type>(context, obj, method, TemplateIntToType<std::is_integral<typename INFO::result_type>::value>()));
        }
        template<typename T, typename METHOD, typename INDEX>
        string ConvertToId(const Core::JSONRPC::Context& context, const T* obj, METHOD method, const TemplateIntToType<false>&)
        {
            return (method(context, obj));
        }
        template<typename T, typename METHOD, typename INDEX>
        string ConvertToId(const Core::JSONRPC::Context& context, const T* obj, METHOD method, const TemplateIntToType<true>&)
        {
            Core::NumberType<INDEX> number(method(context, obj));
            return (number.Text());
        }

    private:
        template<typename METHOD>
        Core::IUnknown* Execute(const Core::JSONRPC::Context&, const string& id, METHOD method, const TemplateIntToType<false>&)
        {
            using INDEX = typename Core::TypeTraits::lambda_traits<METHOD>::template argument<0>::type;
            return (ConvertFromId<METHOD, INDEX>(id, method, TemplateIntToType<std::is_integral<INDEX>::value>()));
        }
        template<typename METHOD, typename INDEX>
        Core::IUnknown* ConvertFromId(const string& id, METHOD method, const TemplateIntToType<false>&)
        {
            return (method(id));
        }
        template<typename METHOD, typename INDEX>
        Core::IUnknown* ConvertFromId(const string& id, METHOD method, const TemplateIntToType<true>&)
        {
            Core::NumberType<INDEX> number(id.c_str(), static_cast<uint32_t>(id.length()));
            return (method(number.Value()));
        }
        template<typename METHOD>
        Core::IUnknown* Execute(const Core::JSONRPC::Context& context, const string& id, METHOD method, const TemplateIntToType<true>&)
        {
            using INDEX = typename Core::TypeTraits::lambda_traits<METHOD>::template argument<1>::type;
            return (ConvertFromId<METHOD,INDEX>(context, id, method, TemplateIntToType<std::is_integral<INDEX>::value>()));
        }
        template<typename METHOD, typename INDEX>
        Core::IUnknown* ConvertFromId(const Core::JSONRPC::Context& context, const string& id, METHOD method, const TemplateIntToType<false>&)
        {
            return (method(context, id));
        }
        template<typename METHOD, typename INDEX>
        Core::IUnknown* ConvertFromId(const Core::JSONRPC::Context& context, const string& id, METHOD method, const TemplateIntToType<true>&)
        {
            Core::NumberType<INDEX> number(id.c_str(), static_cast<uint32_t>(id.length()));
            return (method(context, number.Value()));
        }

    private:
        mutable Core::CriticalSection _adminLock;
        std::unordered_map<uint32_t, Handlers> _handlers;
    };

    class JSONRPCSupportsAutoObjectLookup : virtual public JSONRPCSupportsObjectLookup {
    public:
        JSONRPCSupportsAutoObjectLookup(const JSONRPCSupportsAutoObjectLookup&) = delete;
        JSONRPCSupportsAutoObjectLookup& operator=(const JSONRPCSupportsAutoObjectLookup&) = delete;
        JSONRPCSupportsAutoObjectLookup(JSONRPCSupportsAutoObjectLookup&&) = delete;
        JSONRPCSupportsAutoObjectLookup& operator=(JSONRPCSupportsAutoObjectLookup&&) = delete;

        JSONRPCSupportsAutoObjectLookup()
            : JSONRPCSupportsObjectLookup()
            , _lock()
            , _interfaces()
            , _callback()
            , _nextId(1)
        {
        }
        JSONRPCSupportsAutoObjectLookup(const PluginHost::JSONRPC::TokenCheckFunction& validation)
            : JSONRPCSupportsObjectLookup(validation)
            , _lock()
            , _interfaces()
            , _callback()
            , _nextId(1)
        {
        }
        JSONRPCSupportsAutoObjectLookup(const std::vector<uint8_t>& versions)
            : JSONRPCSupportsObjectLookup(versions)
            , _lock()
            , _interfaces()
            , _callback()
            , _nextId(1)
        {
        }
        JSONRPCSupportsAutoObjectLookup(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation)
            : JSONRPCSupportsObjectLookup(versions, validation)
            , _lock()
            , _interfaces()
            , _callback()
            , _nextId(1)
        {
        }
        ~JSONRPCSupportsAutoObjectLookup() override
        {
            ASSERT(_interfaces.empty() == true);
        }

    public:
        using LifetimeCallback = std::function<void(const bool acquired, const uint32_t, Core::IUnknown*)>;

    public:
        template<typename METHOD>
        void Callback(METHOD cb)
        {
            _lock.Lock();

            _callback = cb;

            _lock.Unlock();
        }

    public:
        template<typename T>
        void InstallHandler()
        {
            _lock.Lock();

            const uint32_t interfaceId = T::ID;
            ASSERT(interfaceId != 0);

            ASSERT(_interfaces.find(interfaceId) == _interfaces.end());

            _interfaces.emplace(std::piecewise_construct, std::make_tuple(interfaceId), std::make_tuple());

            JSONRPCSupportsObjectLookup::Add<T>(
                [this, interfaceId](const Core::JSONRPC::Context& context, const Core::IUnknown* obj) -> uint32_t {
                    return (LookUpImpl(obj, interfaceId, context));
                },
                [this, interfaceId](const Core::JSONRPC::Context& context, const uint32_t instanceId) {
                    return (LookUpImpl(instanceId, interfaceId, context));
                }
            );

            _lock.Unlock();
        }

        template<typename T>
        void UninstallHandler()
        {
            _lock.Lock();

            JSONRPCSupportsObjectLookup::Remove<T>();

            auto faceIt = _interfaces.find(T::ID);
            ASSERT(faceIt != _interfaces.end());

            auto& storage = (*faceIt).second;
            ASSERT(storage.empty() == true); // otherwise it's a leak!

            _interfaces.erase(T::ID);

            _lock.Unlock();
        }

        void ClearHandlers()
        {
            _lock.Lock();

            JSONRPCSupportsObjectLookup::Clear();

            for (auto& entry : _interfaces) {
                DEBUG_VARIABLE(entry);
                ASSERT(entry.second.empty() == true);
            }

            _interfaces.clear();

            _lock.Unlock();
        }

    public:
        template<typename T>
        uint32_t Store(Core::IUnknown* obj, const Core::JSONRPC::Context& context)
        {
            uint32_t instanceId = 0;

            ASSERT(obj != nullptr);
            obj->AddRef();

            _lock.Lock();

            auto faceIt = _interfaces.find(T::ID);
            ASSERT(faceIt != _interfaces.end());

            auto& storage = (*faceIt).second;

            instanceId = _nextId++;

            storage.emplace(std::piecewise_construct,
                std::forward_as_tuple(instanceId),
                std::forward_as_tuple(context.ChannelId(), obj));

            if (_callback != nullptr) {
                _callback(true, T::ID, obj);
            }

            _lock.Unlock();

            return (instanceId);
        }

        template<typename T>
        T* Dispose(const Core::JSONRPC::Context& context, const string& instanceId)
        {
            uint32_t intId = 0;
            Core::FromString(instanceId, intId);
            return (Dispose<T>(context, instanceId));
        }

        template<typename T>
        T* Dispose(const Core::JSONRPC::Context& context, const uint32_t instanceId)
        {
            T* obj{};

            _lock.Lock();

            auto faceIt = _interfaces.find(T::ID);
            ASSERT(faceIt != _interfaces.end());

            auto& storage = (*faceIt).second;
            auto it = storage.find(instanceId);

            if (it != storage.end() && ((*it).second.first == context.ChannelId())) {
                obj = (*it).second.second->template QueryInterface<T>();
                ASSERT(obj != nullptr);

                if (_callback != nullptr) {
                    _callback(false, T::ID, obj);
                }

                obj->Release();
                // The caller should release one last time to destroy.

                storage.erase(it);
            }

            _lock.Unlock();

            return (obj);
        }

        template<typename T>
        bool Check(const string& instanceId, const uint32_t channelId) const
        {
            uint32_t intId = 0;
            Core::FromString(instanceId, intId);
            return (Check<T>(intId, channelId));
        }

        template<typename T>
        bool Check(const uint32_t instanceId, const uint32_t channelId) const
        {
            bool result = false;

            _lock.Lock();

            auto faceIt = _interfaces.find(T::ID);
            ASSERT(faceIt != _interfaces.end());

            auto& storage = (*faceIt).second;

            auto it = storage.find(instanceId);

            if ((it != storage.end()) && ((*it).second.first == channelId)) {
                result = true;
            }

            _lock.Unlock();

            return (result);
        }

    public:
        void Closed(const Core::JSONRPC::Context& context)
        {
            Closed(context.ChannelId());
        }

        void Closed(const uint32_t channel)
        {
            _lock.Lock();

            for (auto& interface : _interfaces) {
                auto& storage = interface.second;
                auto it = storage.begin();

                while (it != storage.end()) {
                    if ((*it).second.first == channel) {
                        Core::IUnknown* obj = (*it).second.second;
                        ASSERT(obj != nullptr);

                        if (_callback != nullptr) {
                            _callback(false, interface.first, obj);
                        }

                        obj->Release();
                        it = storage.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            _lock.Unlock();
        }

    private:
        uint32_t LookUpImpl(const Core::IUnknown* const obj, const uint32_t interfaceId, const Core::JSONRPC::Context& context) const
        {
            uint32_t id = 0;

            ASSERT(interfaceId != 0);

            if (obj != nullptr) {
                _lock.Lock();

                auto faceIt = _interfaces.find(interfaceId);
                ASSERT(faceIt != _interfaces.end());

                auto& storage = (*faceIt).second;

                for (auto const& entry : storage) {
                    if (entry.second.second == obj) {
                        if ((context.ChannelId() == static_cast<uint32_t>(~0)) || (entry.second.first == context.ChannelId())) {
                            id = entry.first;
                        }

                        break;
                    }
                }

                _lock.Unlock();
            }

            return (id);
        }

        Core::IUnknown* LookUpImpl(const uint32_t instanceId, const uint32_t interfaceId, const Core::JSONRPC::Context& context) const
        {
            Core::IUnknown* obj{};

            ASSERT(interfaceId != 0);

            if (instanceId != 0) {
                _lock.Lock();

                auto faceIt = _interfaces.find(interfaceId);
                ASSERT(faceIt != _interfaces.end());

                auto& storage = (*faceIt).second;

                auto const it = storage.find(instanceId);
                if ((it != storage.cend()) && (((*it).second.first == context.ChannelId()) || (context.ChannelId() == static_cast<uint32_t>(~0)))) {

                    obj = (*it).second.second;
                    ASSERT(obj != nullptr);

                    obj->AddRef();
                }

                _lock.Unlock();
            }

            return (obj);
        }

    private:
        mutable Core::CriticalSection _lock;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::pair<uint32_t, Core::IUnknown*>>> _interfaces;
        LifetimeCallback _callback;
        uint32_t _nextId;
    };

#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__

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

        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override
        {
            return JSONRPC::InvokeHandler<JSONRPCERRORASSESSORTYPE>(channelId, id, token, method, parameters, response, _errorhandler);
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

        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override
        {
            return JSONRPC::InvokeHandler<const JSONRPCErrorAssessorTypes::StdFunctionCallbackType&>(channelId, id, token, method, parameters, response, _errorhandler);
        }

    private:
        JSONRPCErrorAssessorTypes::StdFunctionCallbackType _errorhandler;
    };

#endif // __DISABLE_USE_COMPLEMENTARY_CODE_SET__

} // namespace Thunder::PluginHost
}

