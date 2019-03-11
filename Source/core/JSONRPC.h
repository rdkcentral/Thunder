#pragma once

#include "JSON.h"
#include "Module.h"

#include <cctype>
#include <functional>
#include <vector>

namespace WPEFramework {

namespace Core {

    namespace JSONRPC {

        class Message : public Core::JSON::Container {
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
            }
            ~Message()
            {
            }

        public:
            static string Callsign(const string& designator)
            {
                size_t pos = designator.find_last_of('.');
                return (pos == string::npos ? _T("") : designator.substr(0, pos));
            }
            static string Method(const string& designator)
            {
                size_t pos = designator.find_last_of('.');
                return (pos == string::npos ? designator : designator.substr(pos + 1));
            }
            static uint8_t Version(const string& designator)
            {
                uint8_t result = ~0;
                string number(Callsign(designator));
                size_t pos = number.find_last_of('.');

                if (pos != string::npos) {
                    number = number.substr(pos + 1);

                    if ((number.length() > 0) && (std::all_of(number.begin(), number.end(), [](TCHAR c) { return std::isdigit(c); }))) {
                        result = static_cast<uint8_t>(atoi(number.c_str()));
                    }
                }
                return (result);
            }
            void Clear()
            {
                JSONRPC.Clear();
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
            string Method() const
            {
                return (Method(Designator.Value()));
            }
            uint8_t Version() const
            {
                return (Version(Designator.Value()));
            }
            Core::JSON::String JSONRPC;
            Core::JSON::DecUInt32 Id;
            Core::JSON::String Designator;
            Core::JSON::String Parameters;
            Core::JSON::String Result;
            Info Error;
        };

        class EXTERNAL Handler {
        private:
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

            typedef std::function<uint32_t(const string& parameters, string& result)> InvokeFunction;
            typedef std::map<const string, InvokeFunction> HandlerMap;

        public:
            Handler()
                : _handlers()
            {
            }
            virtual ~Handler()
            {
            }

        public:
            // For now the version is not used for exist determination, but who knows what will happen in the future.
            // The interface is prepared.
            inline uint32_t Exists(const string& methodName, const uint8_t version) const
            {
                return (_handlers.find(methodName) != _handlers.end() ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY);
            }
            uint32_t Validate(const Message& message) const
            {
                const string callsign(message.Callsign());
                uint32_t result = (callsign.empty() ? Core::ERROR_NONE : Core::ERROR_INVALID_DESIGNATOR);
                if (result != Core::ERROR_NONE) {
                    uint32_t length = static_cast<uint32_t>(_callsign.length());
                    if (callsign.compare(0, length, _callsign) == 0) {
                        result = Core::ERROR_INVALID_SIGNATURE;

                        if ((callsign.length() == length) || ((callsign[length] == '.') && (HasVersionSupport(callsign.substr(length + 1))))) {
                            result = Core::ERROR_NONE;
                        }
                    }
                }
                return (result);
            }
            template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
            void Property(const string& methodName, const GET_METHOD& getMethod, const SET_METHOD& setMethod, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const REALOBJECT&, PARAMETER&)> getter = getMethod;
                std::function<uint32_t(REALOBJECT&, const PARAMETER&)> setter = setMethod;
                ASSERT(objectPtr != nullptr);
                InvokeFunction implementation = [objectPtr, getter, setter](const string& inbound, string& outbound) -> uint32_t {
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
            template <typename INBOUND, typename OUTBOUND, typename METHOD>
            void Register(const string& methodName, const METHOD& method)
            {
                std::function<uint32_t(const INBOUND&, OUTBOUND&)> actualMethod = method;
                InvokeFunction implementation = [actualMethod](const string& parameters, string& result) -> uint32_t {
                    INBOUND inbound;
                    OUTBOUND outbound;
                    inbound = parameters;
                    uint32_t code = actualMethod(inbound, outbound);
                    result = outbound;
                    return (code);
                };
                Register(methodName, implementation);
            }
            template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
            void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
            {
                std::function<uint32_t(const INBOUND&, OUTBOUND&)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2);
                InvokeFunction implementation = [actualMethod](const string& parameters, string& result) -> uint32_t {
                    INBOUND inbound;
                    OUTBOUND outbound;
                    inbound.FromString(parameters);
                    uint32_t code = actualMethod(inbound, outbound);
                    outbound.ToString(result);
                    return (code);
                };
                Register(methodName, implementation);
            }
            void Register(const string& methodName, const InvokeFunction& lambda)
            {
                ASSERT(_handlers.find(methodName) == _handlers.end());
                _handlers[methodName] = lambda;
            }
            void Unregister(const string& methodName)
            {
                HandlerMap::iterator index = _handlers.find(methodName);

                // Only unregister if you registered.
                ASSERT(index != _handlers.end());

                if (index != _handlers.end()) {
                    _handlers.erase(index);
                }
            }
            uint32_t Invoke(const string& method, const string& parameters, string& response)
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                response.clear();

                HandlerMap::iterator index = _handlers.find(method);
                if (index != _handlers.end()) {
                    result = index->second(parameters, response);
                }
                return (result);
            }
            void Designator(const string& callsign, const std::vector<uint8_t>& versions)
            {
                _callsign = callsign;
                _versions = versions;
                _designator = callsign + '.' + Core::NumberType<uint8_t>(versions.back()).Text();
            }
            const string& Callsign() const
            {
                return (_designator);
            }

        private:
            bool HasVersionSupport(const string& number) const
            {
                return (number.length() > 0) && (std::all_of(number.begin(), number.end(), [](TCHAR c) { return std::isdigit(c); })) && (std::find(_versions.begin(), _versions.end(), static_cast<uint8_t>(atoi(number.c_str()))) != _versions.end());
            }

        private:
            HandlerMap _handlers;
            string _callsign;
            string _designator;
            std::vector<uint8_t> _versions;
        };
    }
}
} // namespace WPEFramework::Core::JSONRPC
