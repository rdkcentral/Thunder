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

    struct EXTERNAL ILocalDispatcher : public IDispatcher {
        virtual ~ILocalDispatcher() = default;

        virtual void Activate(IShell* service) = 0;
        virtual void Deactivate() = 0;
        virtual void Dropped(const IDispatcher::ICallback* callback) = 0;
    };

    class EXTERNAL JSONRPC : public ILocalDispatcher {
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
                Destination(uint32_t channelId, const string& designator)
                    : _callback(nullptr)
                    , _channelId(channelId)
                    , _designator(designator) {
                }
                Destination(IDispatcher::ICallback* callback, const string& designator)
                    : _callback(callback)
                    , _channelId(~0)
                    , _designator(designator) {
                    if (_callback != nullptr) {
                        _callback->AddRef();
                    }
                }
                Destination(Destination&& move) noexcept
                    : _callback(move._callback)
                    , _channelId(move._channelId)
                    , _designator(move._designator) {
                    move._callback = nullptr;
                }
                Destination(const Destination& copy)
                    : _callback(copy._callback)
                    , _channelId(copy._channelId)
                    , _designator(copy._designator) {
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
                    _designator = move._designator;
                    move._callback = nullptr;
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

            private:
                IDispatcher::ICallback* _callback;
                uint32_t _channelId;
                string _designator;
            };
            using Destinations = std::vector<Destination>;

        public:
            Observer& operator=(Observer&&) = delete;
            Observer& operator=(const Observer&) = delete;

            Observer()
                : _designators() {
            }
            Observer(Observer&& move) noexcept
                : _designators(move._designators) {
            }
            Observer(const Observer& copy)
                : _designators(copy._designators) {
            }
            ~Observer() = default;

        public:
            bool IsEmpty() const {
                return ( _designators.empty() );
            }
            uint32_t Subscribe(const uint32_t id, const string& designator) {
                uint32_t result = Core::ERROR_NONE;

                Destinations::iterator index(_designators.begin());
                while ((index != _designators.end()) && ((index->ChannelId() != id) || (index->Designator() != designator))) {
                    index++;
                }

                if (index == _designators.end()) {
                    _designators.emplace_back(id, designator);
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
            void Dropped(const uint32_t channelId) {
                Destinations::iterator index = _designators.begin();
                while (index != _designators.end()) {
                    if ( (index->ChannelId() == channelId) && (index->Callback() == nullptr) ) {
                        index = _designators.erase(index);
                    }
                    else {
                        index++;
                    }
                }
            }
            void Event(JSONRPC& parent, const string event, const string& parameter, const SendIfMethod& sendifmethod) {
                for (Destination& entry : _designators) {
                    if (!sendifmethod || sendifmethod(entry.Designator())) {
                        if (entry.Callback() == nullptr) {
                            parent.Notify(entry.ChannelId(), entry.Designator() + '.' + event, parameter);
                        }
                        else {
                            entry.Callback()->Event(event, entry.Designator(), parameter);
                        }
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
            VersionInfo(VersionInfo&& move)
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
            VersionInfo& operator=(VersionInfo&& move)
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
        uint32_t Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response) override {
            uint32_t result = Core::ERROR_PARSE_FAILURE;

            if (method.empty() == false) {

                ASSERT(Core::JSONRPC::Message::Callsign(method).empty() || (Core::JSONRPC::Message::Callsign(method) == _callsign));

                string realMethod(Core::JSONRPC::Message::Method(method));

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
                    else if (realMethod == _T("register")) {
                        Registration info;  info.FromString(parameters);

                        result = Subscribe(channelId, info.Event.Value(), info.Callsign.Value());
                        if (result == Core::ERROR_NONE) {
                            response = _T("0");
                        }
                        else {
                            result = Core::ERROR_FAILED_REGISTERED;
                        }
                    }
                    else if (realMethod == _T("unregister")) {
                        Registration info;  info.FromString(parameters);

                        result = Unsubscribe(channelId, info.Event.Value(), info.Callsign.Value());
                        if (result == Core::ERROR_NONE) {
                            response = _T("0");
                        }
                        else {
                            result = Core::ERROR_FAILED_UNREGISTERED;
                        }
                    }
                    else {
                        Core::JSONRPC::Handler* handler(Handler(realMethod));

                        if (handler == nullptr) {
                            result = Core::ERROR_INCORRECT_URL;
                        }
                        else {
                            Core::JSONRPC::Context context(channelId, id, token);
                            result = handler->Invoke(context, Core::JSONRPC::Message::FullMethod(method), parameters, response);
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
        ILocalDispatcher* Local() override {
            return (this);
        }

        // Inherited via ILocalDispatcher
        // ---------------------------------------------------------------------------------
        void Activate(IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);

            _adminLock.Lock();

            _service = service;
            _service->AddRef();
            _callsign = _service->Callsign();

            _service->Register(&_notification);

            _adminLock.Unlock();
        }
        void Deactivate() override
        {
            _adminLock.Lock();

            if (_service != nullptr) {
                _service->Unregister(&_notification);
                _service->Release();
                _service = nullptr;
            }

            _callsign.clear();
            _observers.clear();

            _adminLock.Unlock();
        }

        // Inherited via IDispatcher::ICallback
        // ---------------------------------------------------------------------------------
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

        virtual uint32_t Subscribe(const uint32_t channelId, const string& eventId, const string& designator)
        {
            uint32_t result;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index == _observers.end()) {
                index = _observers.emplace(std::piecewise_construct,
                    std::forward_as_tuple(eventId),
                    std::forward_as_tuple()).first;
            }

            result = index->second.Subscribe(channelId, designator);

            _adminLock.Unlock();

            return (result);
        }
        virtual uint32_t Unsubscribe(const uint32_t channelId, const string& eventId, const string& designator)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(eventId);

            if (index != _observers.end()) {
                result = index->second.Unsubscribe(channelId, designator);

                if ((result == Core::ERROR_NONE) && (index->second.IsEmpty() == true)) {
                    _observers.erase(index);
                }
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        void ChannelClosed(const uint32_t channelId)
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

    private:
        uint32_t InternalNotify(const string& event, const string& parameters, const SendIfMethod& sendifmethod = nullptr) const
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::const_iterator index = _observers.find(event);

            if (index != _observers.end()) {
                const_cast<Observer&>(index->second).Event(const_cast<JSONRPC&>(*this), event, parameters, sendifmethod);
            }

            // See if this is perhaps a registered alias for an event...

            EventAliasesMap::const_iterator iter = _eventAliases.find(event);

            if (iter != _eventAliases.end()) {

                for (const string& alias : iter->second) {
                    ObserverMap::const_iterator index = _observers.find(alias);

                    if (index != _observers.end()) {
                        const_cast<Observer&>(index->second).Event(const_cast<JSONRPC&>(*this), alias, parameters, sendifmethod);
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
        ObserverMap _observers;
        EventAliasesMap _eventAliases;
        Core::SinkType<Notification> _notification;
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

    public:
        uint32_t Subscribe(const uint32_t channel, const string& eventId, const string& designator) override
        {
            const Core::hresult result = JSONRPC::Subscribe(channel, eventId, designator);

            if (result == Core::ERROR_NONE) {
                NotifyObservers(eventId, designator, Status::registered);
            }

            return (result);
        }
        uint32_t Unsubscribe(const uint32_t channel, const string& eventId, const string& designator) override
        {
            const Core::hresult result = JSONRPC::Unsubscribe(channel, eventId, designator);

            if (result == Core::ERROR_NONE) {
                NotifyObservers(eventId, designator, Status::unregistered);
            }

            return (result);
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

} // namespace Thunder::PluginHost
}
