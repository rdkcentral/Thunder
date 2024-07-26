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

#include "JSON.h"
#include "Module.h"
#include "TypeTraits.h"

#include <cctype>
#include <functional>
#include <vector>

namespace Thunder {

namespace Core {

    namespace JSONRPC {

        class EXTERNAL Message : public Core::JSON::Container {
        public:
            // Arbitrary code base selected in discussion with the team to use 
            // this magical value as a base for Thunder error codes (0..999).
            // These error codes are *not* related to the JSONRPC transport
            // layer but relate to the application layer.
            // Seems the spec is expecting a value > -32767, so with a value
            // range of 0-999 Thunder codes, -31000 should be oke :-)
            static constexpr int32_t ApplicationErrorCodeBase = -31000;

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
                    , Code(copy.Code)
                    , Text(copy.Text)
                    , Data(copy.Data)
                {
                    Add(_T("code"), &Code);
                    Add(_T("message"), &Text);
                    Add(_T("data"), &Data);
                }
                Info(Info&& move) noexcept
                    : Core::JSON::Container()
                    , Code(std::move(move.Code))
                    , Text(std::move(move.Text))
                    , Data(std::move(move.Data))
                {
                    Add(_T("code"), &Code);
                    Add(_T("message"), &Text);
                    Add(_T("data"), &Data);
                }
                ~Info() override = default;

                Info& operator=(const Info& RHS)
                {
                    Code = RHS.Code;
                    Text = RHS.Text;
                    Data = RHS.Data;
                    return (*this);
                }

                Info& operator=(Info&& move) noexcept
                {
                    if (this != &move) {
                        Code = std::move(move.Code);
                        Text = std::move(move.Text);
                        Data = std::move(move.Data);
                    }
                    return (*this);
                }

            public:
                void Clear() override
                {
                    Code.Clear();
                    Text.Clear();
                    Data.Clear();
                }
                void SetError(const uint32_t frameworkError)
                {
                    switch (frameworkError) {
                    case Core::ERROR_INTERNAL_JSONRPC:
                        Code = -32603; // Internal Error
                        Text = _T("Unknown jsonrpc error.");
                        break;
                    case Core::ERROR_INVALID_ENVELOPPE:
                        Text = _T("Invalid Request.");
                        Code = -32600; // Invalid request
                        break;
                    case Core::ERROR_INVALID_PARAMETER:
                        Code = -32602; // Invalid parameters
                        Text = _T("Invalid Parameters.");
                        break;
                    case Core::ERROR_UNKNOWN_METHOD:
                        Text = _T("Unknown method.");
                        Code = -32601; // Method not found
                        break;
                    case Core::ERROR_PRIVILIGED_REQUEST:
                        Code = -32604; // Priviliged
                        Text = _T("method invocation not allowed.");
                        break;
                    case Core::ERROR_PRIVILIGED_DEFERRED:
                        Code = -32604;
                        Text = _T("method invokation is deferred, Currently not allowed.");
                        break;
                    case Core::ERROR_TIMEDOUT:
                        Code = -32000; // Server defined, now mapped to Timed out
                        Text = _T("Call timed out.");
                        break;
                    case Core::ERROR_PARSING_ENVELOPPE:
                        Code = -32700; // Parse error
                        Text = _T("Parsing of the parameters failed");
                        break;
                    case Core::ERROR_INVALID_RANGE:
                        Code = ApplicationErrorCodeBase - Core::ERROR_INVALID_RANGE;
                        Text = _T("Requested version is not supported.");
                        break;
                    case Core::ERROR_INCORRECT_URL:
                        Code = ApplicationErrorCodeBase - Core::ERROR_INCORRECT_URL;
                        Text = _T("Designator is invalid.");
                        break;
                    case Core::ERROR_ILLEGAL_STATE:
                        Code = ApplicationErrorCodeBase - Core::ERROR_ILLEGAL_STATE;
                        Text = _T("The service is in an illegal state!!!.");
                        break;
                    case Core::ERROR_FAILED_REGISTERED:
                        Code = ApplicationErrorCodeBase - Core::ERROR_FAILED_REGISTERED;
                        Text = _T("Registration already done!!!.");
                        break;
                    case Core::ERROR_FAILED_UNREGISTERED:
                        Code = ApplicationErrorCodeBase - Core::ERROR_FAILED_UNREGISTERED;
                        Text = _T("Unregister was already done!!!.");
                        break;
                    case Core::ERROR_HIBERNATED:
                        Code = ApplicationErrorCodeBase - Core::ERROR_HIBERNATED;
                        Text = _T("The service is in an Hibernated state!!!.");
                        break;
                    case Core::ERROR_UNAVAILABLE:
                        Code = ApplicationErrorCodeBase - Core::ERROR_UNAVAILABLE;
                        Text = _T("Requested service is not available.");
                        break;
                    default:
                        Code = ApplicationErrorCodeBase - static_cast<int32_t>(frameworkError);
                        Text = Core::ErrorToString(frameworkError);
                        break;
                    }
                }
                Core::JSON::DecSInt32 Code;
                Core::JSON::String Text;
                Core::JSON::String Data;
            };

        public:
            static constexpr TCHAR DefaultVersion[] = _T("2.0");

            Message& operator=(const Message&) = delete;

            Message()
                : Core::JSON::Container()
                , JSONRPC(DefaultVersion)
                , Id(~0)
                , Designator()
                , Parameters(false)
                , Result(false)
                , Error()
                , _implicitCallsign()
            {
                Add(_T("jsonrpc"), &JSONRPC);
                Add(_T("id"), &Id);
                Add(_T("method"), &Designator);
                Add(_T("params"), &Parameters);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);

                Clear();
            }
            Message(const Message& copy)
                : Core::JSON::Container()
                , JSONRPC(copy.JSONRPC)
                , Id(copy.Id)
                , Designator(copy.Designator)
                , Parameters(copy.Parameters)
                , Result(copy.Result)
                , Error(copy.Error)
                , _implicitCallsign(copy._implicitCallsign)
            {
                Add(_T("jsonrpc"), &JSONRPC);
                Add(_T("id"), &Id);
                Add(_T("method"), &Designator);
                Add(_T("params"), &Parameters);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);
            }

            Message(Message&& move) noexcept
                : Core::JSON::Container()
                , JSONRPC(std::move(move.JSONRPC))
                , Id(std::move(move.Id))
                , Designator(std::move(move.Designator))
                , Parameters(std::move(move.Parameters))
                , Result(std::move(move.Result))
                , Error(std::move(move.Error))
                , _implicitCallsign(std::move(move._implicitCallsign))
            {
                Add(_T("jsonrpc"), &JSONRPC);
                Add(_T("id"), &Id);
                Add(_T("method"), &Designator);
                Add(_T("params"), &Parameters);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);
            }

            ~Message() override = default;

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
            static string Method(const string& designator)
            {
                size_t end = designator.find_last_of('@');
                size_t begin = designator.find_last_of('.', end);
                size_t lookup = designator.find_first_of('#', begin+1);

                if (lookup != string::npos) {
                    size_t ns = designator.find_first_of(':', lookup + 1);
                    return (designator.substr((begin == string::npos) ? 0 : begin + 1, lookup - begin - 1) + (ns != string::npos? designator.substr(ns, end - ns) : string{}));
                 }
                else {
                    return (designator.substr((begin == string::npos) ? 0 : begin + 1, (end == string::npos ? string::npos : (begin == string::npos) ? end : end - begin - 1)));
                }
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
                uint16_t base = 1;
                uint16_t result = 0;

                // Optimization, no need to parse behind index, all behind the @ is index, before is the method!
                size_t length = designator.find_last_of('@');

                // Whether we found the indexer or not, looking from the back (or @) for the first dot we find is 
                // the beginning of the method marker. Before that dot *must* be the version (if applicable).
                length = designator.find_last_of('.', length);

                // If we did not find a dot, in the previous run, the length available for the version == 0.
                length = (length == string::npos ? 0 : length);

                // Now by definition we dropped off the method name. We got a "clean" designator without the method
                // name (before length) and the last character can be found @length. So time to check version..
                while ((length > 0) && (isdigit(designator[--length])) && (base <= 100)) {
                    result += ((designator[length] - '0') * base);
                    base *= 10;
                }

                // Now do the math, check if the version we calculated is valid..
                return ( (base > 1) && (result < 0xFF) && ((length == 0) || (designator[length] == '.')) ? static_cast<uint8_t>(result) : ~0);
            }
            static string VersionAsString(const string& designator)
            {
                string textResult;
                uint8_t count = 0;
                uint16_t base = 1;
                uint16_t result = 0;

                // Optimization, no need to parse behind index, all behind the @ is index, before is the method!
                size_t length = designator.find_last_of('@');

                // Whether we found the indexer or not, looking from the back (or @) for the first dot we find is 
                // the beginning of the method marker. Before that dot *must* be the version (if applicable).
                length = designator.find_last_of('.', length);

                // If we did not find a dot, in the previous run, the length available for the version == 0.
                length = (length == string::npos ? 0 : length);

                // Now by definition we dropped off the method name. We got a "clean" designator without the method
                // name (before length) and the last character can be found @length. So time to check version..
                while ((length > 0) && (isdigit(designator[--length])) && (base <= 100)) {
                    result += ((designator[length] - '0') * base);
                    base *= 10;
                    count++;
                }

                // Now do the math, check if the version we calculated is valid..
                if ((base > 1) && (result < 0xFF)) {
                    if (length == 0) {
                        textResult = designator.substr(0, count);
                    }
                    else if (designator[length] == '.') {
                        textResult = designator.substr(length + 1, count);
                    }
                }

                return (textResult);
            }
            static string Index(const string& designator)
            {
                size_t end = designator.find_last_of('@');

                return (end == string::npos ? EMPTY_STRING : designator.substr(end + 1, string::npos));
            }
            static uint32_t InstanceId(const string& designator)
            {
                const size_t pos = designator.find_first_of('#');
                uint32_t val = 0;

                if (pos != string::npos) {
                    Core::FromString(designator.substr(pos + 1, (designator.find_first_of(":@", pos) - (pos + 1))), val);
                }

                return (val);
            }
            void Clear() override
            {
                JSONRPC = DefaultVersion;
                Id.Clear();
                Designator.Clear();
                Parameters.Clear();
                Result.Clear();
                Error.Clear();
                _implicitCallsign.erase();
            }
            string Callsign() const
            {
                string result(Callsign(Designator.Value()));
                return (result.empty() == true ? _implicitCallsign : result);
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
            string VersionAsString() const
            {
                return (VersionAsString(Designator.Value()));
            }
            string Index() const
            {
                return (Index(Designator.Value()));
            }
            void ImplicitCallsign(const string& implicitCallsign) 
            {
                _implicitCallsign = implicitCallsign;
            }

            Core::JSON::String JSONRPC;
            Core::JSON::DecUInt32 Id;
            Core::JSON::String Designator;
            Core::JSON::String Parameters;
            Core::JSON::String Result;
            Info Error;

        private:
            string _implicitCallsign;
        };

        class EXTERNAL Context {
        public:
            Context& operator=(const Context& rhs) = delete;

            Context() 
                : _channelId(~0)
                , _sequence(~0)
                , _token() {
            }
            Context(const Context& copy) 
                : _channelId(copy._channelId)
                , _sequence(copy._sequence)
                , _token(copy._token) {
            }
            Context(Context&& move) noexcept
                : _channelId(move._channelId)
                , _sequence(move._sequence)
                , _token(std::move(move._token)) {
            }
            Context(const uint32_t channelId, const uint32_t sequence, const string& token)
                : _channelId(channelId)
                , _sequence(sequence)
                , _token(token) {
            }
            ~Context() = default;

        public:
            uint32_t ChannelId() const
            {
                return (_channelId);
            }
            uint32_t Sequence() const
            {
                return (_sequence);
            }
            const string& Token() const {
                return (_token);
            }

        private:
            uint32_t _channelId;
            uint32_t _sequence;
            string _token;
        };

        typedef std::function<void(const Context& context, const string& parameters, Core::OptionalType<Core::JSON::Error>&)> CallbackFunction;
        typedef std::function<uint32_t(const Context& context, const string& method, const string& parameters, string& result)> InvokeFunction;

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
                    Functions(Functions&& function, const bool async)
                    {
                        if (async == true) {
                            new (&_callback) auto(std::move(function._callback));
                        } else {
                            new (&_invoke) auto(std::move(function._invoke));
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
                Entry(Entry&& move) noexcept
                    : _asynchronous(move._asynchronous)
                    , _info(std::move(move._info), move._asynchronous)
                {
                    move._asynchronous = false;
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
                uint32_t Invoke(const Context& context, const string& method, const string& parameters, string& response)
                {
                    uint32_t result;

                    if (_asynchronous == true) {
                        Core::OptionalType<Core::JSON::Error> report;
                        _info._callback(context, parameters, report);
                        if (report.IsSet() == false) {
                            result = ~0;
                        }
                        else {
                            result = Core::ERROR_PARSING_ENVELOPPE;
                            response = report.Value().Message();
                        }
                    } else {
                        result = _info._invoke(context, method, parameters, response);
                    }
                    return (result);
                }

            private:
                bool _asynchronous;
                Functions _info;
            };

            using HandlerMap = std::unordered_map<string, Entry>;

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
                EventIterator(EventIterator&& move) noexcept
                    : _container(move._container)
                    , _index(std::move(move._index))
                    , _position(move._position)
                {
                    move._container = nullptr;
                    move._position = ~0;
                }
                ~EventIterator() = default;

                EventIterator& operator=(const EventIterator& rhs)
                {
                    _container = rhs._container;
                    _index = rhs._index;
                    _position = rhs._position;

                    return (*this);
                }
                EventIterator& operator=(EventIterator&& move) noexcept
                {
                    if (this != &move) {
                        _container = move._container;
                        _index = std::move(move._index);
                        _position = move._position;

                        move._container = nullptr;
                        move._position = ~0;
                    }
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

            Handler(const std::vector<uint8_t>& versions)
                : _adminLock()
                , _handlers()
                , _versions(versions)
            {
            }
            Handler(const std::vector<uint8_t>& versions, const Handler& copy)
                : _adminLock()
                , _handlers(copy._handlers)
                , _versions(versions)
            {
            }
            ~Handler() = default;

        public:
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
                return ((_handlers.find(methodName) != _handlers.end()) ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_METHOD);
            }
            bool HasVersionSupport(const uint8_t number) const
            {
                return (std::find(_versions.begin(), _versions.end(), number) != _versions.end());
            }

        public:
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            typename std::enable_if<(std::is_same<std::nullptr_t, typename std::remove_cv<GET_METHOD>::type>::value && !std::is_same<std::nullptr_t, typename std::remove_cv<SET_METHOD>::type>::value), void>::type
            Property(const string& methodName, GET_METHOD, SET_METHOD setMethod, REALOBJECT* objectPtr)
            {
                using COUNT = Core::TypeTraits::func_traits<SET_METHOD>;

                static_assert((COUNT::Arguments == 1) || (COUNT::Arguments == 2), "We need 1 (value to set) or 2 (index and value to set) arguments!!!");

                InternalProperty<PARAMETER, SET_METHOD, REALOBJECT>(::TemplateIntToType<COUNT::Arguments>(), methodName, objectPtr, setMethod);
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            typename std::enable_if<(!std::is_same<std::nullptr_t, typename std::remove_cv<GET_METHOD>::type>::value && std::is_same<std::nullptr_t, typename std::remove_cv<SET_METHOD>::type>::value), void>::type
            Property(const string& methodName, GET_METHOD getMethod, SET_METHOD, REALOBJECT* objectPtr)
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

        public:
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void Register(const string& methodName, const METHOD& method)
            {
                using ARG0 = typename std::decay<typename TypeTraits::func_traits<METHOD>::template argument<0>::type>::type;
                using ARG1 = typename std::decay<typename TypeTraits::func_traits<METHOD>::template argument<1>::type>::type;

                InternalRegister<INBOUND, OUTBOUND, METHOD>(
                    ::TemplateIntToType<std::is_same<ARG0, Context>::value | (std::is_same<ARG0, string>::value << 1) | (std::is_same<ARG1, string>::value << 1) | (std::is_same<ARG0, uint32_t>::value << 2)>(),
                    ::TemplateIntToType<std::is_same<INBOUND, void>::value>(),
                    ::TemplateIntToType<std::is_same<OUTBOUND, void>::value>(),
                    methodName,
                    method);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                using ARG0 = typename std::decay<typename TypeTraits::func_traits<METHOD>::template argument<0>::type>::type;
                using ARG1 = typename std::decay<typename TypeTraits::func_traits<METHOD>::template argument<1>::type>::type;

                InternalRegister<INBOUND, OUTBOUND, METHOD, REALOBJECT>(
                    ::TemplateIntToType<std::is_same<ARG0, Context>::value | (std::is_same<ARG0, string>::value << 1) | (std::is_same<ARG1, string>::value << 1) | (std::is_same<ARG0, uint32_t>::value << 2)>(),
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
            void Register(const string& methodName, const string& primaryName)
            {
                ASSERT(methodName.empty() == false);
                ASSERT(primaryName.empty() == false);

                auto retval = _handlers.find(primaryName);
                ASSERT(retval != _handlers.end());

                auto retvalAlias = _handlers.find(methodName);
                ASSERT(retvalAlias == _handlers.end());

                // Register the handler under an alternative name.

                if ((retval != _handlers.end()) && (retvalAlias == _handlers.end()))  {
                    _handlers.emplace(std::piecewise_construct,
                        std::make_tuple(methodName),
                        std::make_tuple(retval->second));
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
            uint32_t Invoke(const Context& context, const string& method, const string& parameters, string& response)
            {
                uint32_t result = Core::ERROR_UNKNOWN_METHOD;

                response.clear();

                HandlerMap::iterator index = _handlers.find(Message::Method(method));
                if (index != _handlers.end()) {
                    result = index->second.Invoke(context, method, parameters, response);
                }
                return (result);
            }

        private:
            template <typename PARAMETER, typename GET_METHOD, typename REALOBJECT>
            void InternalProperty(const ::TemplateIntToType<1>&, const string& methodName, const GET_METHOD& getMethod, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const REALOBJECT&, PARAMETER&)> getter = getMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, getter](const Context&, const string&, const string& inbound, string& outbound) -> uint32_t {
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
                InvokeFunction implementation = [objectPtr, setter](const Core::JSONRPC::Context&, const string&, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        Core::OptionalType<Core::JSON::Error> report;
                        parameter.FromString(inbound, report);
                        if (report.IsSet() == false) {
                            code = setter(*objectPtr, parameter);
                        }
                        else {
                            outbound = report.Value().Message();
                            code = Core::ERROR_PARSE_FAILURE;
                        }
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
                InvokeFunction implementation = [objectPtr, getter, setter](const Context&, const string&, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        Core::OptionalType<Core::JSON::Error> report;
                        parameter.FromString(inbound, report);
                        if (report.IsSet() == false) {
                            code = setter(*objectPtr, parameter);
                        }
                        else {
                            code = Core::ERROR_PARSE_FAILURE;
                            outbound = report.Value().Message();
                        }
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
                InvokeFunction implementation = [objectPtr, getter](const Context&, const string& method, const string& inbound, string& outbound) -> uint32_t {
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
                InvokeFunction implementation = [objectPtr, setter](const Core::JSONRPC::Context&, const string& method, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    if (inbound.empty() == false) {
                        const string index = Message::Index(method);
                        Core::OptionalType<Core::JSON::Error> report;
                        parameter.FromString(inbound, report);
                        if (report.IsSet() == false) {
                            code = setter(*objectPtr, index, parameter);
                        }
                        else {
                            outbound = report.Value().Message();
                            code = Core::ERROR_PARSE_FAILURE;
                        }
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
                InvokeFunction implementation = [objectPtr, getter, setter](const Context&, const string& method, const string& inbound, string& outbound) -> uint32_t {
                    PARAMETER parameter;
                    uint32_t code;
                    const string index = Message::Index(method);
                    if (inbound.empty() == false) {
                        Core::OptionalType<Core::JSON::Error> report;
                        parameter.FromString(inbound, report);
                        if (report.IsSet() == false) {
                            code = setter(*objectPtr, index, parameter);
                        }
                        else {
                            outbound = report.Value().Message();
                            code = Core::ERROR_PARSE_FAILURE;
                        }
                    } else {
                        code = getter(*objectPtr, index, parameter);
                        parameter.ToString(outbound);
                    }
                    return (code);
                };
                Register(methodName, implementation);
            }

         private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
                    return (InternalRegisterImpl<METHOD>(method));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // inbound
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND, METHOD>(parameters, result, method));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // outbound
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND, METHOD>(result, method));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // inbound+outbound
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD>(parameters, result, method));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // context
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<METHOD>(method, context));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // context+inbound
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND, METHOD>(parameters, result, method, context));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // context+outbound
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string&, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND, METHOD>(result, method, context));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // context+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD>(parameters, result, method, context));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // index+inbound
            void InternalRegister(const ::TemplateIntToType<2>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND, METHOD>(parameters, result, method, Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // index+outbound
            void InternalRegister(const ::TemplateIntToType<2>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND, METHOD>(result, method, Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> /// index+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<2>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD>(parameters, result, method, Message::Index(methodName)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // context+index+inbound
            void InternalRegister(const ::TemplateIntToType<3>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND, METHOD>(parameters, result, method, context, Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // context+index+outbound
            void InternalRegister(const ::TemplateIntToType<3>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND, METHOD>(result, method, context, Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> /// context+index+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<3>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context& context, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD>(parameters, result, method, context, Message::Index(methodName)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<METHOD>(method, Message::InstanceId(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id+inbound
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND, METHOD>(parameters, result, method, Message::InstanceId(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id+outbound
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND, METHOD>(result, method, Message::InstanceId(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD>(parameters, result, method, Message::InstanceId(methodName)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id+index+inbound
            void InternalRegister(const ::TemplateIntToType<6>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND, METHOD>(parameters, result, method, Message::InstanceId(methodName), Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id+index+outbound
            void InternalRegister(const ::TemplateIntToType<6>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND, METHOD>(result, method, Message::InstanceId(methodName), Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD> // id+index+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<6>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                Register(methodName, [method](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND, METHOD>(parameters, result, method, Message::InstanceId(methodName), Message::Index(methodName)));
                });
            }

         private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
                    return (InternalRegisterImpl(std::bind(method, objectPtr)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // inbound
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // outbound
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND>(result, std::bind(method, objectPtr, std::placeholders::_1)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // inbound+outbound
            void InternalRegister(const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // context
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl(std::bind(method, objectPtr, std::placeholders::_1), context));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // context+inbound
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2), context));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // context+outbound
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string&, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND>(result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2), context));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // context+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string&, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), context));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // index+inbound
            void InternalRegister(const ::TemplateIntToType<2>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2), Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // index+outbound
            void InternalRegister(const ::TemplateIntToType<2>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND>(result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2), Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> /// index+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<2>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), Message::Index(methodName)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // context+index+inbound
            void InternalRegister(const ::TemplateIntToType<3>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), context, Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // context+index+outbound
            void InternalRegister(const ::TemplateIntToType<3>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND>(result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), context, Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> /// context+index+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<3>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context& context, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), context, Message::Index(methodName)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl(std::bind(method, objectPtr, std::placeholders::_1), Message::InstanceId(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id+inbound
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2), Message::InstanceId(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id+outbound
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND>(result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2), Message::InstanceId(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<4>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), Message::InstanceId(methodName)));
                });
            }

        private:
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id+index+inbound
            void InternalRegister(const ::TemplateIntToType<6>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImpl<INBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), Message::InstanceId(methodName), Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id+index+outbound
            void InternalRegister(const ::TemplateIntToType<6>&, const ::TemplateIntToType<1>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string&, string& result) -> uint32_t {
                    return (InternalRegisterImpl<OUTBOUND>(result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), Message::InstanceId(methodName), Message::Index(methodName)));
                });
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT> // id+index+inbound+outbound
            void InternalRegister(const ::TemplateIntToType<6>&, const ::TemplateIntToType<0>&, const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                Register(methodName, [method, objectPtr](const Context&, const string& methodName, const string& parameters, string& result) -> uint32_t {
                    return (InternalRegisterImplIO<INBOUND, OUTBOUND>(parameters, result, std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), Message::InstanceId(methodName), Message::Index(methodName)));
                });
            }

        private:
            template <typename METHOD, typename... Args>
            static uint32_t InternalRegisterImpl(const METHOD& method, Args&&... args)
            {
                return (method(std::forward<Args>(args)...));
            }
            template <typename INBOUND, typename METHOD, typename... Args>
            static uint32_t InternalRegisterImpl(const string& parameters, string& result, const METHOD& method, Args&&... args)
            {
                uint32_t code;
                INBOUND inbound;
                Core::OptionalType<Core::JSON::Error> report;

                inbound.FromString(parameters, report);

                if (report.IsSet() == false) {
                    code = method(std::forward<Args>(args)..., inbound);
                }
                else {
                    code = Core::ERROR_PARSE_FAILURE;
                    result = report.Value().Message();
                }

                return(code);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename... Args>
            static uint32_t InternalRegisterImplIO(const string& parameters, string& result, const METHOD& method, Args&&... args)
            {
                uint32_t code;
                INBOUND inbound;
                OUTBOUND outbound;
                Core::OptionalType<Core::JSON::Error> report;

                inbound.FromString(parameters, report);

                if (report.IsSet() == false) {
                    code = method(std::forward<Args>(args)..., inbound, outbound);
                    if (code == Core::ERROR_NONE) {
                        outbound.ToString(result);
                    }
                    else {
                        result.clear();
                    }
                }
                else {
                    result = report.Value().Message();
                    code = Core::ERROR_PARSE_FAILURE;
                }

                return (code);
            }
            template <typename OUTBOUND, typename METHOD, typename... Args>
            static uint32_t InternalRegisterImpl(string& result, const METHOD& method, Args&&... args)
            {
                OUTBOUND outbound;

                uint32_t code = method(std::forward<Args>(args)..., outbound);

                if (code == Core::ERROR_NONE) {
                    outbound.ToString(result);
                }
                else {
                    result.clear();
                }

                return (code);
            }

        private:
            template <typename INBOUND, typename METHOD>
            void InternalAnnounce(const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method)
            {
                std::function<void(const Core::JSONRPC::Context&)> actualMethod = method;
                CallbackFunction implementation = [actualMethod](const Context& connection, const string&, Core::OptionalType<Core::JSON::Error>&) -> void {
                    actualMethod(connection);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD>
            void InternalAnnounce(const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method)
            {
                std::function<void(const Core::JSONRPC::Context&, const INBOUND&)> actualMethod = method;
                CallbackFunction implementation = [actualMethod](const Context& connection, const string& parameters, Core::OptionalType<Core::JSON::Error>& report) -> void {
                    INBOUND inbound;
                    inbound.FromString(parameters, report);
                    if (report.IsSet() == false) {
                        actualMethod(connection, inbound);
                    }
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD, typename REALOBJECT>
            void InternalAnnounce(const ::TemplateIntToType<1>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<void(const Core::JSONRPC::Context&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
                CallbackFunction implementation = [actualMethod](const Context& connection, const string&, Core::OptionalType<Core::JSON::Error>&) -> void {
                    actualMethod(connection);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename METHOD, typename REALOBJECT>
            void InternalAnnounce(const ::TemplateIntToType<0>&, const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<void(const Core::JSONRPC::Context&, const INBOUND&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2);
                CallbackFunction implementation = [actualMethod](const Context& connection, const string& parameters, Core::OptionalType<Core::JSON::Error>& report) -> void {
                    INBOUND inbound;
                    inbound.FromString(parameters, report);
                    if (report.IsSet() == false) {
                        actualMethod(connection, inbound);
                    }
                };
                Register(methodName, implementation);
            }

        private:
            mutable Core::CriticalSection _adminLock;
            HandlerMap _handlers;
            const std::vector<uint8_t> _versions;
        };

        using Error = Message::Info;
    }
}
}
