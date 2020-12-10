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

#include "JSON.h"
#include "Module.h"
#include "TypeTraits.h"

#include <cctype>
#include <functional>
#include <vector>

namespace WPEFramework {

namespace Core {

    namespace JSONRPC {

        class EXTERNAL Message : public Core::JSON::Container {
        private:
            Message(const Message&) = delete;
            Message& operator=(const Message&) = delete;

        public:
            class Info : public Core::JSON::Container {
            public:
                Info()
                    : Core::JSON::Container()
                    , Code(0)
                    , Text()
                    , Data(false)
                {
                    Add(_T("code"), &Code);
                    Add(_T("message"), &Text);
                    Add(_T("data"), &Data);
                }
                Info(const Info& copy)
                    : Core::JSON::Container()
                    , Code(0)
                    , Text()
                    , Data(false)
                {
                    Add(_T("code"), &Code);
                    Add(_T("message"), &Text);
                    Add(_T("data"), &Data);
                    Code = copy.Code;
                    Text = copy.Text;
                    Data = copy.Data;
                }
                virtual ~Info()
                {
                }

                Info& operator=(const Info& RHS)
                {
                    Code = RHS.Code;
                    Text = RHS.Text;
                    Data = RHS.Data;
                    return (*this);
                }

            public:
                void Clear()
                {
                    Code.Clear();
                    Text.Clear();
                    Data.Clear();
                }
                void SetError(const uint32_t frameworkError)
                {
                    switch (frameworkError) {
                    case Core::ERROR_BAD_REQUEST:
                        Code = -32603; // Internal Error
                        break;
                    case Core::ERROR_INVALID_DESIGNATOR:
                        Code = -32600; // Invalid request
                        break;
                    case Core::ERROR_INVALID_SIGNATURE:
                        Code = -32602; // Invalid parameters
                        break;
                    case Core::ERROR_UNKNOWN_KEY:
                        Code = -32601; // Method not found
                        break;
                    case Core::ERROR_PRIVILIGED_REQUEST:
                        Code = -32604; // Priviliged
                        break;
                    case Core::ERROR_TIMEDOUT:
                        Code = -32000; // Server defined, now mapped to Timed out
                        break;
                    default:
                        Code = static_cast<int32_t>(frameworkError);
                        break;
                    }
                }
                Core::JSON::DecSInt32 Code;
                Core::JSON::String Text;
                Core::JSON::String Data;
            };

        public:
            static constexpr TCHAR DefaultVersion[] = _T("2.0");

            Message()
                : Core::JSON::Container()
                 , JSONRPC(DefaultVersion)
                 , Id(~0)
                 , Designator()
                 , Parameters(false)
                 , Result(false)
                 , Error()
            {
                Add(_T("jsonrpc"), &JSONRPC);
                Add(_T("id"), &Id);
                Add(_T("method"), &Designator);
                Add(_T("params"), &Parameters);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);

                Clear();
            }
            ~Message()
            {
            }

        public:
            static string Callsign(const string& designator)
            {
                size_t pos = designator.find_last_of('.', designator.find_last_of('@'));
                if ((pos != string::npos) && (pos > 0)) {
                    size_t index = pos - 1;
                    while ((index != 0) && (isdigit(designator[index]))) {
                        index--;
                    }
                    if ((index != 0) && (designator[index] == '.')) {
                        pos = index;
                    } else if ((index == 0) && (isdigit(designator[0]))) {
                        pos = string::npos;
                    }
                }
                return (pos == string::npos ? EMPTY_STRING : designator.substr(0, pos));
            }
            static string FullCallsign(const string& designator)
            {
                size_t pos = designator.find_last_of('.', designator.find_last_of('@'));
                return (pos == string::npos ? EMPTY_STRING : designator.substr(0, pos));
            }
            static string Method(const string& designator)
            {
                size_t end = designator.find_last_of('@');
                size_t begin = designator.find_last_of('.', end);

                return (designator.substr((begin == string::npos) ? 0 : begin + 1, (end == string::npos ? string::npos : (begin == string::npos) ? end : end - begin - 1)));
            }
            static string FullMethod(const string& designator)
            {
                size_t pos = designator.find_last_of('.', designator.find_last_of('@'));
                return (designator.substr(pos == string::npos ? 0 : pos + 1));
            }
            static string VersionedFullMethod(const string& designator)
            {
                size_t pos = designator.find_last_of('.', designator.find_last_of('@'));
                if ((pos != string::npos) && (pos > 0)) {
                    size_t index = pos - 1;
                    while ((index != 0) && (isdigit(designator[index]))) {
                        index--;
                    }
                    if ((index != 0) && (designator[index] == '.')) {
                        pos = index;
                    } else if ((index == 0) && (isdigit(designator[0]))) {
                        pos = string::npos;
                    }
                }
                return (designator.substr(pos == string::npos ? 0 : pos + 1));
            }
            static uint8_t Version(const string& designator)
            {
                uint8_t result = ~0;
                size_t pos = designator.find_last_of('.', designator.find_last_of('@'));

                if (pos != string::npos) {
                    size_t index = pos - 1;
                    while ((index != 0) && (isdigit(designator[index]))) {
                        index--;
                    }
                    if ((index != 0) && (designator[index] == '.')) {
                        index++;
                    } else if ((index == 0) && (isdigit(designator[0]) == false)) {
                        index = pos;
                    }

                    if (index < pos) {
                        result = static_cast<uint8_t>(atoi(designator.substr(index, pos - index).c_str()));
                    }
                }
                return (result);
            }
            static string Index(const string& designator)
            {
                size_t end = designator.find_last_of('@');

                return (end == string::npos ? EMPTY_STRING : designator.substr(end + 1, string::npos));
            }
            void Clear()
            {
                JSONRPC = DefaultVersion;
                Id.Clear();
                Designator.Clear();
                Parameters.Clear();
                Result.Clear();
                Error.Clear();
            }
            string Callsign() const
            {
                return (Callsign(Designator.Value()));
            }
            string FullCallsign() const
            {
                return (FullCallsign(Designator.Value()));
            }
            string Method() const
            {
                return (Method(Designator.Value()));
            }
            string FullMethod() const
            {
                return (FullMethod(Designator.Value()));
            }
            string VersionedFullMethod() const
            {
                return (VersionedFullMethod(Designator.Value()));
            }
            uint8_t Version() const
            {
                return (Version(Designator.Value()));
            }
            string Index() const
            {
                return (Index(Designator.Value()));
            }
            Core::JSON::String JSONRPC;
            Core::JSON::DecUInt32 Id;
            Core::JSON::String Designator;
            Core::JSON::String Parameters;
            Core::JSON::String Result;
            Info Error;
        };

        class EXTERNAL Connection {
        private:
            Connection() = delete;

        public:
            Connection(const uint32_t channelId, const uint32_t sequence)
                : _channelId(channelId)
                , _sequence(sequence)
            {
            }
            Connection(const Connection& copy)
                : _channelId(copy._channelId)
                , _sequence(copy._sequence)
            {
            }
            ~Connection()
            {
            }

            Connection& operator=(const Connection& rhs)
            {
                _channelId = rhs._channelId;
                _sequence = rhs._sequence;

                return (*this);
            }

        public:
            uint32_t ChannelId() const
            {
                return (_channelId);
            }
            uint32_t Sequence() const
            {
                return (_sequence);
            }

        private:
            uint32_t _channelId;
            uint32_t _sequence;
        };

        typedef std::function<void(const Connection& channel, const string& parameters)> CallbackFunction;
        typedef std::function<uint32_t(const string& method, const string& parameters, string& result)> InvokeFunction;

        class EXTERNAL Handler {
        private:
            class Entry {
            private:
                Entry() = delete;
                Entry& operator=(const Entry&) = delete;
                
                union Functions {
                    Functions(const Functions& function, const bool async)
                    {
                        if (async == true) {
                            new (&_callback) auto(function._callback);
                        } else {
                            new (&_invoke) auto(function._invoke);
                        }
                    }
                    Functions(const CallbackFunction& function)
                        : _callback(function)
                    {
                    }
                    Functions(const InvokeFunction& function)
                        : _invoke(function)
                    {
                    }
                    void Assign(const CallbackFunction& function, bool callbackactive) {
                        if( callbackactive ) {
                            _callback = function;
                        } else {
                            _invoke.~InvokeFunction();
                            new (&_callback) auto(function);
                        }
                    }
                    void Assign(const InvokeFunction& function, bool callbackactive) {
                        if( callbackactive ) {
                            _callback.~CallbackFunction();
                            new (&_invoke) auto(function);
                        } else {
                            _invoke = function;
                        }
                    }

                    ~Functions()
                    {
                    }

                    CallbackFunction _callback;
                    InvokeFunction _invoke;
                };

            public:
                Entry(const CallbackFunction& callback)
                    : _asynchronous(true)
                    , _info(callback)
                {
                }
                Entry(const InvokeFunction& callback)
                    : _asynchronous(false)
                    , _info(callback)
                {
                }
                Entry(const Entry& copy)
                    : _asynchronous(copy._asynchronous)
                    , _info(copy._info, copy._asynchronous)
                {
                }
                Entry& operator=(const CallbackFunction& callback) {
                    _info.Assign(callback, _asynchronous);
                    _asynchronous = true;
                    return *this;
                }
                Entry& operator=(const InvokeFunction& invokefunction) {
                    _info.Assign(invokefunction, _asynchronous);
                    _asynchronous = false;
                    return *this;
                }
                ~Entry()
                {
                    if (_asynchronous == true) {
                        _info._callback.~CallbackFunction();
                    } else {
                        _info._invoke.~InvokeFunction();
                    }
                }

            public:
                uint32_t Invoke(const Connection connection, const string& method, const string& parameters, string& response)
                {
                    uint32_t result = ~0;
                    if (_asynchronous == true) {
                        _info._callback(connection, parameters);
                    } else {
                        result = _info._invoke(method, parameters, response);
                    }
                    return (result);
                }

            private:
                bool _asynchronous;
                Functions _info;
            };

            class Observer {
            private:
                Observer(const Observer&) = delete;
                Observer& operator=(const Observer&) = delete;

            public:
                Observer(const uint32_t id, const string& designator)
                    : _id(id)
                    , _designator(designator)
                {
                }
                ~Observer()
                {
                }

                bool operator==(const Observer& rhs) const
                {
                    return ((rhs._id == _id) && (rhs._designator == _designator));
                }
                bool operator!=(const Observer& rhs) const
                {
                    return (!operator==(rhs));
                }

                uint32_t Id() const
                {
                    return (_id);
                }
                const string& Designator() const
                {
                    return (_designator);
                }

            private:
                uint32_t _id;
                string _designator;
            };

            typedef std::map<const string, Entry> HandlerMap;
            typedef std::list<Observer> ObserverList;
            typedef std::map<string, ObserverList> ObserverMap;

            typedef std::function<void(const uint32_t id, const string& designator, const string& data)> NotificationFunction;

        public:
            class EventIterator {
            public:
                EventIterator()
                    : _container(nullptr)
                    , _index()
                    , _position(~0)
                {
                }
                EventIterator(const HandlerMap& container)
                    : _container(&container)
                    , _index()
                    , _position(~0)
                {
                }
                EventIterator(const EventIterator& copy)
                    : _container(copy._container)
                    , _index(copy._index)
                    , _position(copy._position)
                {
                }
                ~EventIterator()
                {
                }

                EventIterator& operator=(const EventIterator& rhs)
                {
                    _container = rhs._container;
                    _index = rhs._index;
                    _position = rhs._position;

                    return (*this);
                }

            public:
                bool IsValid() const
                {
                    return ((_container != nullptr) && (_position < _container->size()));
                }
                void Reset()
                {
                    _position = ~0;
                }
                bool Next()
                {
                    if (_position == static_cast<uint16_t>(~0)) {
                        if (_container != nullptr) {
                            _position = 0;
                            _index = _container->cbegin();
                        }
                    } else if (_index != _container->cend()) {
                        _index++;
                        _position++;
                    }
                    return (IsValid());
                }
                const string& Event() const
                {
                    ASSERT(IsValid());
                    return (_index->first);
                }

            private:
                const HandlerMap* _container;
                HandlerMap::const_iterator _index;
                uint16_t _position;
            };

        public:
            Handler() = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

            Handler(const NotificationFunction& notificationFunction, const std::vector<uint8_t>& versions)
                : _adminLock()
                , _handlers()
                , _observers()
                , _notificationFunction(notificationFunction)
                , _versions(versions)
            {
            }
            Handler(const NotificationFunction& notificationFunction, const std::vector<uint8_t>& versions, const Handler& copy)
                : _adminLock()
                , _handlers(copy._handlers)
                , _observers()
                , _notificationFunction(notificationFunction)
                , _versions(versions)
            {
            }
            ~Handler()
            {
            }

        public:
            inline uint32_t Observers() const {
                return (static_cast<uint32_t>(_observers.size()));
            }
            inline EventIterator Events() const
            {
                return (EventIterator(_handlers));
            }
            inline bool Copy(const Handler& copy, const string& method)
            {
                bool copied = false;

                HandlerMap::const_iterator index = copy._handlers.find(method);

                if (index != copy._handlers.end()) {
                    copied = true;
                    const Entry& info(index->second);

                    _handlers.emplace(std::piecewise_construct,
                        std::forward_as_tuple(method),
                        std::forward_as_tuple(info));
                }

                return (copied);
            }
            // For now the version is not used for exist determination, but who knows what will happen in the future.
            // The interface is prepared.
            inline uint32_t Exists(const string& methodName) const
            {
                return ((_handlers.find(methodName) != _handlers.end()) ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY);
            }
            bool HasVersionSupport(const uint8_t number) const
            {
                return (std::find(_versions.begin(), _versions.end(), number) != _versions.end());
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            typename std::enable_if<(std::is_same<std::nullptr_t, typename std::remove_cv<GET_METHOD>::type>::value && !std::is_same<std::nullptr_t, typename std::remove_cv<SET_METHOD>::type>::value), void>::type
            Property(const string& methodName, GET_METHOD getMethod, SET_METHOD setMethod, REALOBJECT* objectPtr)
            {
                using COUNT = Core::TypeTraits::func_traits<SET_METHOD>;

                static_assert((COUNT::Arguments == 1) || (COUNT::Arguments == 2), "We need 1 (value to set) or 2 (index and value to set) arguments!!!");

                InternalProperty<PARAMETER, SET_METHOD, REALOBJECT>(::TemplateIntToType<COUNT::Arguments>(), methodName, objectPtr, setMethod);
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            typename std::enable_if<(!std::is_same<std::nullptr_t, typename std::remove_cv<GET_METHOD>::type>::value && std::is_same<std::nullptr_t, typename std::remove_cv<SET_METHOD>::type>::value), void>::type
            Property(const string& methodName, GET_METHOD getMethod, SET_METHOD setMethod, REALOBJECT* objectPtr)
            {
                using COUNT = Core::TypeTraits::func_traits<GET_METHOD>;

                static_assert((COUNT::Arguments == 1) || (COUNT::Arguments == 2), "We need 1 (value to set) or 2 (index and value to set) arguments!!!");

                InternalProperty<PARAMETER, GET_METHOD, REALOBJECT>(::TemplateIntToType<COUNT::Arguments>(), methodName, getMethod, objectPtr);
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            typename std::enable_if<(!std::is_same<std::nullptr_t, typename std::remove_cv<GET_METHOD>::type>::value && !std::is_same<std::nullptr_t, typename std::remove_cv<SET_METHOD>::type>::value), void>::type
            Property(const string& methodName, GET_METHOD getMethod, SET_METHOD setMethod, REALOBJECT* objectPtr)
            {
                using GET_COUNT = Core::TypeTraits::func_traits<SET_METHOD>;
                using SET_COUNT = Core::TypeTraits::func_traits<SET_METHOD>;

                static_assert((GET_COUNT::Arguments == 1) || (GET_COUNT::Arguments == 2), "We need 1 (value to set) or 2 (index and value to set) arguments!!!");
                static_assert((SET_COUNT::Arguments == 1) || (SET_COUNT::Arguments == 2), "We need 1 (value to set) or 2 (index and value to set) arguments!!!");
                static_assert((GET_COUNT::Arguments == SET_COUNT::Arguments), "The getter and the setter need the same amount of arguments !!");

                InternalProperty<PARAMETER, GET_METHOD, SET_METHOD, REALOBJECT>(::TemplateIntToType<SET_COUNT::Arguments>(), methodName, getMethod, setMethod, objectPtr);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void Register(const string& methodName, const METHOD& method)
            {
                InternalRegister<INBOUND, OUTBOUND, METHOD>(
                    ::TemplateIntToType<std::is_same<INBOUND, void>::value>(),
                    ::TemplateIntToType<std::is_same<OUTBOUND, void>::value>(),
                    methodName,
                    method);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                InternalRegister<INBOUND, OUTBOUND, METHOD, REALOBJECT>(
                    ::TemplateIntToType<std::is_same<INBOUND, void>::value>(),
                    ::TemplateIntToType<std::is_same<OUTBOUND, void>::value>(),
                    methodName,
                    method,
                    objectPtr);
            }
            template <typename INBOUND, typename METHOD>
            void Register(const string& methodName, const METHOD& method)
            {
                InternalAnnounce<INBOUND, METHOD>(
                    ::TemplateIntToType<std::is_same<INBOUND, void>::value>(),
                    methodName,
                    method);
            }
            template <typename INBOUND, typename METHOD, typename REALOBJECT>
            void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                InternalAnnounce<INBOUND, METHOD, REALOBJECT>(
                    ::TemplateIntToType<std::is_same<INBOUND, void>::value>(),
                    methodName,
                    method,
                    objectPtr);
            }
            void Register(const string& methodName, const InvokeFunction& lambda)
            {
                // Due to versioning, we do allow to overwrite methods that have been registered.
                // These are typically methods that are different from the preferred interface..
                auto retval = _handlers.emplace(std::piecewise_construct,
                                    std::make_tuple(methodName),
                                    std::make_tuple(lambda));
                    
                if ( retval.second == false ) {
                    retval.first->second = lambda;
                }
            }
            void Register(const string& methodName, const CallbackFunction& lambda)
            {
                // Due to versioning, we do allow to overwrite methods that have been registered.
                // These are typically methods that are different from the preferred interface..

                auto retval = _handlers.emplace(std::piecewise_construct,
                                    std::make_tuple(methodName),
                                    std::make_tuple(lambda));

                if ( retval.second == false ) {
                    retval.first->second = lambda;
                }
            }
            void Unregister(const string& methodName)
            {
                HandlerMap::iterator index = _handlers.find(methodName);

                ASSERT((index != _handlers.end()) && _T("Do not unregister methods that are not registered!!!"));

                if (index != _handlers.end()) {
                    _handlers.erase(index);
                }
            }
            uint32_t Invoke(const Connection connection, const string& method, const string& parameters, string& response)
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                response.clear();

                HandlerMap::iterator index = _handlers.find(Message::Method(method));
                if (index != _handlers.end()) {
                    result = index->second.Invoke(connection, method, parameters, response);
                }
                return (result);
            }
            void Subscribe(const uint32_t id, const string& eventId, const string& callsign, Core::JSONRPC::Message& response)
            {
                _adminLock.Lock();

                ObserverMap::iterator index = _observers.find(eventId);

                if (index == _observers.end()) {
                    _observers[eventId].emplace_back(id, callsign);
                    response.Result = _T("0");
                } else if (std::find(index->second.begin(), index->second.end(), Observer(id, callsign)) == index->second.end()) {
                    index->second.emplace_back(id, callsign);
                    response.Result = _T("0");
                } else {
                    response.Error.SetError(Core::ERROR_DUPLICATE_KEY);
                    response.Error.Text = _T("Duplicate registration. Only 1 remains!!!");
                }

                _adminLock.Unlock();
            }
            void Unsubscribe(const uint32_t id, const string& eventId, const string& callsign, Core::JSONRPC::Message& response)
            {
                _adminLock.Lock();

                ObserverMap::iterator index = _observers.find(eventId);

                if (index != _observers.end()) {
                    ObserverList& clients = index->second;
                    ObserverList::iterator loop = clients.begin();
                    Observer key(id, callsign);

                    while ((loop != clients.end()) && (*loop != key)) {
                        loop++;
                    }

                    if (loop != clients.end()) {
                        clients.erase(loop);
                        if (clients.empty() == true) {
                            _observers.erase(index);
                        }
                        response.Result = _T("0");
                    }
                }

                if (response.Result.IsSet() == false) {
                    response.Error.SetError(Core::ERROR_UNKNOWN_KEY);
                    response.Error.Text = _T("Registration not found!!!");
                }

                _adminLock.Unlock();
            }
            uint32_t Notify(const string& event)
            {
                return (InternalNotify(event, _T("")));
            }
            template <typename JSONOBJECT>
            uint32_t Notify(const string& event, const JSONOBJECT& parameters)
            {
                string subject;
                parameters.ToString(subject);
                return (InternalNotify(event, subject));
            }
            template <typename JSONOBJECT, typename SENDIFMETHOD>
            uint32_t Notify(const string& event, const JSONOBJECT& parameters, SENDIFMETHOD method)
            {
                string subject;
                parameters.ToString(subject);
                return InternalNotify(event, subject, std::move(method));
            }
            void Close(const uint32_t id)
            {
                _adminLock.Lock();

                ObserverMap::iterator index = _observers.begin();

                while (index != _observers.end()) {
                    ObserverList& clients = index->second;
                    ObserverList::iterator loop = clients.begin();

                    while (loop != clients.end()) {
                        if (loop->Id() != id) {
                            loop++;
                        } else {
                            loop = clients.erase(loop);
                        }
                    }
                    if (clients.empty() == true) {
                        index = _observers.erase(index);
                    } else {
                        index++;
                    }
                }

                _adminLock.Unlock();
            }
            void Close()
            {
                _adminLock.Lock();

                _observers.clear();

                _adminLock.Unlock();
            }

        private:
            template <typename PARAMETER, typename GET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<1>&, const string& methodName, const GET_METHOD& getMethod, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const REALOBJECT&, PARAMETER&)> getter = getMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, getter](const string&, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        code = Core::ERROR_UNAVAILABLE;
                    } else {
                        code = getter(*objectPtr, parameter);
                        parameter.ToString(outbound);
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename PARAMETER, typename SET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<1>&, const string& methodName, REALOBJECT* objectPtr, const SET_METHOD& setMethod)
            {
                std::function<uint32_t(REALOBJECT&, const PARAMETER&)> setter = setMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, setter](const string&, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        parameter.FromString(inbound);
                        code = setter(*objectPtr, parameter);
                    } else {
                        code = Core::ERROR_UNAVAILABLE;
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<1>&, const string& methodName, const GET_METHOD& getMethod, const SET_METHOD& setMethod, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const REALOBJECT&, PARAMETER&)> getter = getMethod;
                std::function<uint32_t(REALOBJECT&, const PARAMETER&)> setter = setMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, getter, setter](const string&, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        parameter.FromString(inbound);
                        code = setter(*objectPtr, parameter);
                    } else {
                        code = getter(*objectPtr, parameter);
                        parameter.ToString(outbound);
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename PARAMETER, typename GET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<2>&, const string& methodName, const GET_METHOD& getMethod, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const REALOBJECT&, const string&, PARAMETER&)> getter = getMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, getter](const string& method, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        code = Core::ERROR_UNAVAILABLE;
                    } else {
                        const string index = Message::Index(method);
                        code = getter(*objectPtr, index, parameter);
                        parameter.ToString(outbound);
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename PARAMETER, typename SET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<2>&, const string& methodName, REALOBJECT* objectPtr, const SET_METHOD& setMethod)
            {
                std::function<uint32_t(REALOBJECT&, const string&, const PARAMETER&)> setter = setMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, setter](const string& method, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        const string index = Message::Index(method);
                        parameter.FromString(inbound);
                        code = setter(*objectPtr, index, parameter);
                    } else {
                        code = Core::ERROR_UNAVAILABLE;
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<2>&, const string& methodName, const GET_METHOD& getMethod, const SET_METHOD& setMethod, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const REALOBJECT&, const string&, PARAMETER&)> getter = getMethod;
                std::function<uint32_t(REALOBJECT&, const string&, const PARAMETER&)> setter = setMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, getter, setter](const string& method, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    const string index = Message::Index(method);
                    if (inbound.empty() == false) {
                        parameter.FromString(inbound);
                        code = setter(*objectPtr, index, parameter);
                    } else {
                        code = getter(*objectPtr, index, parameter);
                        parameter.ToString(outbound);
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                std::function<uint32_t()> actualMethod = method;
                InvokeFunction implementation = [actualMethod](const string&, const string&, string&) -> uint32_t {
                    return (actualMethod());
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                std::function<uint32_t(const INBOUND&)> actualMethod = method;
                InvokeFunction implementation = [actualMethod](const string&, const string& parameters, string&) -> uint32_t {
                    INBOUND inbound;
                    inbound.FromString(parameters);
                    return (actualMethod(inbound));
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                std::function<uint32_t(OUTBOUND&)> actualMethod = method;
                InvokeFunction implementation = [actualMethod](const string&, const string&, string& result) -> uint32_t {
                    OUTBOUND outbound;
                    uint32_t code = actualMethod(outbound);
                    if (code == Core::ERROR_NONE) {
                        outbound.ToString(result);
                    } else {
                        result.clear();
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                std::function<uint32_t(const INBOUND&, OUTBOUND&)> actualMethod = method;
                InvokeFunction implementation = [actualMethod](const string&, const string& parameters, string& result) -> uint32_t {
                    INBOUND inbound;
                    OUTBOUND outbound;
                    inbound.FromString(parameters);
                    uint32_t code = actualMethod(inbound, outbound);
                    if (code == Core::ERROR_NONE) {
                        outbound.ToString(result);
                    } else {
                        result.clear();
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<uint32_t()> actualMethod = std::bind(method, objectPtr);
                InvokeFunction implementation = [actualMethod](const string&, const string&, string&) -> uint32_t {
                    return (actualMethod());
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const INBOUND&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
                InvokeFunction implementation = [actualMethod](const string&, const string& parameters, string&) -> uint32_t {
                    INBOUND inbound;
                    inbound.FromString(parameters);
                    return (actualMethod(inbound));
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(OUTBOUND&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
                InvokeFunction implementation = [actualMethod](const string&, const string&, string& result) -> uint32_t {
                    OUTBOUND outbound;
                    uint32_t code = actualMethod(outbound);
                    if (code == Core::ERROR_NONE) {
                        outbound.ToString(result);
                    } else {
                        result.clear();
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const INBOUND&, OUTBOUND&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2);
                InvokeFunction implementation = [actualMethod](const string&, const string& parameters, string& result) -> uint32_t {
                    INBOUND inbound;
                    OUTBOUND outbound;
                    inbound.FromString(parameters);
                    uint32_t code = actualMethod(inbound, outbound);
                    if (code == Core::ERROR_NONE) {
                        outbound.ToString(result);
                    } else {
                        result.clear();
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD>
            void InternalAnnounce(const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                std::function<void(const Core::JSONRPC::Connection&)> actualMethod = method;
                CallbackFunction implementation = [actualMethod](const Connection& connection, const string&) -> void {
                    actualMethod(connection);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD>
            void InternalAnnounce(const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                std::function<void(const Core::JSONRPC::Connection&, const INBOUND&)> actualMethod = method;
                CallbackFunction implementation = [actualMethod](const Connection& connection, const string& parameters) -> void {
                    INBOUND inbound;
                    inbound.FromString(parameters);
                    actualMethod(connection, inbound);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD, typename REALOBJECT>
            void InternalAnnounce(const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<void(const Core::JSONRPC::Connection&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
                CallbackFunction implementation = [actualMethod](const Connection& connection, const string&) -> void {
                    actualMethod(connection);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD, typename REALOBJECT>
            void InternalAnnounce(const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<void(const Core::JSONRPC::Connection&, const INBOUND&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2);
                CallbackFunction implementation = [actualMethod](const Connection& connection, const string& parameters) -> void {
                    INBOUND inbound;
                    inbound.FromString(parameters);
                    actualMethod(connection, inbound);
                };
                Register(methodName, implementation);
            }
            uint32_t InternalNotify(const string& event, const string& parameters, std::function<bool(const string&)>&& sendifmethod = std::function<bool(const string&)>())
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                _adminLock.Lock();

                ObserverMap::iterator index = _observers.find(event);

                if (index != _observers.end()) {
                    ObserverList& clients = index->second;
                    ObserverList::iterator loop = clients.begin();

                    result = Core::ERROR_NONE;

                    while (loop != clients.end()) {
                        const string& designator(loop->Designator());

                        if (!sendifmethod || sendifmethod(designator)) {

                            _notificationFunction(loop->Id(), (designator.empty() == false ? designator + '.' + event : event), parameters);
                        }

                        loop++;
                    }
                }

                _adminLock.Unlock();

                return (result);
            }

        private:
            Core::CriticalSection _adminLock;
            HandlerMap _handlers;
            ObserverMap _observers;
            NotificationFunction _notificationFunction;
            const std::vector<uint8_t> _versions;
        };

        using Error = Message::Info;
    }
}
} // namespace WPEFramework::Core::JSONRPC
