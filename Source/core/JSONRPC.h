#pragma once

#include "Module.h"
#include "JSON.h"

#include <vector>
#include <functional>

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
                , Data() {
                Add(_T("code"), &Code);
                Add(_T("message"), &Text);
                Add(_T("data"), &Data);
            }
            Info(const Info& copy) 
                : Core::JSON::Container()
                , Code(0)
                , Text()
                , Data() {
                Add(_T("code"), &Code);
                Add(_T("message"), &Text);
                Add(_T("data"), &Data);
                Code = copy.Code;
                Text = copy.Text;
                Data = copy.Data;
            }
            virtual ~Info() {
            }

            Info& operator= (const Info& RHS) {
                Code = RHS.Code;
                Text = RHS.Text;
                Data = RHS.Data;
                return (*this);
            }

        public:
            void SetError(const uint32_t frameworkError) {
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
                case Core::ERROR_UNKNOWN_KEY_PASSED:
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
            , Version(DefaultVersion)
            , Id(~0)
            , Designator()
            , Parameters()
            , Result()
            , Error() {
            Add(_T("jsonrpc"), &Version);
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
        string Callsign() const {
            size_t pos = Designator.Value().find_last_of('.');
            return (pos == string::npos ? _T("") : Designator.Value().substr(0, pos));
        }
        string Method() const {
            size_t pos = Designator.Value().find_last_of('.');
            return (pos == string::npos ? Designator.Value() : Designator.Value().substr(pos));
        }
        Core::JSON::String Version;
        Core::JSON::DecUInt32 Id;
        Core::JSON::String Designator;
        Core::JSON::String Parameters;
        Core::JSON::String Result;
        Info Error;
    };

    class EXTERNAL Handler {
    private:
        Handler(const Handler&) = delete;
        Handler& operator= (const Handler&) = delete;

        typedef std::function<uint32_t(const uint32_t id, const string& parameters, string& result)> InvokeFunction;
        typedef std::map<const string, InvokeFunction > HandlerMap;

    public:
        Handler() 
            : _handlers() {
        }
        virtual ~Handler() {
        }

    public:
        uint32_t Validate (const Message& message) const {
            const string callsign (message.Callsign());
            uint32_t result = (callsign.empty() ? Core::ERROR_NONE : Core::ERROR_INVALID_DESIGNATOR);
            if (result != Core::ERROR_NONE) {
                uint32_t length = _callsign.length();
                if (_callsign.compare(0, length, callsign) == 0) {
                    result = Core::ERROR_INVALID_SIGNATURE;

                    if ((callsign.length() == length) ||
                        ((callsign[length] == '.') && (HasVersionSupport(callsign.substr(length+1))))) {
                        result = Core::ERROR_NONE;
                    }
                }
            }
            return (result);
        }
        template<typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
        void Register (const string& methodName, const METHOD& method, REALOBJECT* objectPtr) {
            std::function<uint32_t(const INBOUND& parameters, OUTBOUND& result)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2);
            InvokeFunction implementation = [actualMethod](const uint32_t id, const string& parameters, string& result) -> uint32_t {
                                               INBOUND inbound; 
                                               OUTBOUND outbound;
                                               inbound.FromString(parameters);
                                               uint32_t code = actualMethod(inbound, outbound);
                                               if (id != static_cast<uint32_t>(~0)) { 
                                                   string response; 
                                                   Message message;
                                                   message.Version = Message::DefaultVersion;
                                                   message.Id = id;
                                                   outbound.ToString(response);
                                                   if (code == Core::ERROR_NONE) {
                                                       message.Result = response;
                                                   }
                                                   else {
                                                       message.Error.SetError(code);
                                                       message.Error.Text = response;
                                                   }
                                                   message.ToString(result);
                                               }
                                               return (code);
                                            };
            Register(methodName, implementation);
        }
        template<typename METHOD, typename REALOBJECT>
        void Register (const string& methodName, const METHOD& method, REALOBJECT objectPtr ) {
            std::function<uint32_t(const string& parameters, string& result)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2);
            InvokeFunction implementation = [actualMethod](const uint32_t id, const string& parameters, string& result) -> uint32_t {
                                               string response; 
                                               uint32_t code = actualMethod(parameters, response);
                                               if (id != static_cast<uint32_t>(~0)) { 
                                                   Message message;
                                                   message.Version = Message::DefaultVersion;
                                                   message.Id = id;
                                                   if (code == Core::ERROR_NONE) {
                                                       message.Result = response;
                                                   }
                                                   else {
                                                       message.Error.SetError(code);
                                                       message.Error.Text = response;
                                                   }
                                                   message.ToString(result);
                                               }
                                               return (code);
                                            };
            Register(methodName, implementation);
        }
        void Register (const string& methodName, const InvokeFunction& lambda) {
            ASSERT (_handlers.find(methodName) == _handlers.end());
            _handlers[methodName] = lambda;
        }
        void Unregister (const string& methodName) {
            HandlerMap::iterator index = _handlers.find(methodName);

            if (index == _handlers.end()) {
                _handlers.erase(index);
            }
        }
        uint32_t Invoke (const Message& inbound, string& response) {
            uint32_t result = Validate(inbound);

            if (result == Core::ERROR_NONE) {
               result = Invoke(inbound.Id.Value(), inbound.Method(), inbound.Parameters.Value(), response);
            }
            else if (inbound.Id.Value() != static_cast<uint32_t>(~0)) {
                Message message;
                message.Version = Message::DefaultVersion;
                message.Error.SetError(result);
                message.Error.Text = "Invalid JSONRPC Request";
                message.Id = inbound.Id.Value();
                message.ToString(response);
            }
            else {
                response.clear();
            }
 
            return (result);
        }
        uint32_t Invoke (const uint32_t id, const string& method, const string& parameters, string& response) {
            uint32_t result = Core::ERROR_UNKNOWN_KEY_PASSED;

            response.clear();

            HandlerMap::iterator index = _handlers.find(method);
            if (index != _handlers.end()) {
                result = index->second(id, parameters, response);
            }
            else if (id != static_cast<uint32_t>(~0)) {
                Message message;
                message.Version = Message::DefaultVersion;
                message.Error.SetError(result);
                message.Error.Text = "Method not found";
                message.Id = id;
                message.ToString(response);
            }
            return (result);
        }
        void Designator(const string& callsign, const std::vector<uint8_t>& versions) {
            _callsign = callsign;
            _versions = versions;
            _designator = callsign + '.' + Core::NumberType<uint8_t>(versions.back()).Text();
        }
        const string& Designator() const {
            return (_designator);
        }

    private:
        bool HasVersionSupport(const string& number) const {
            return (number.length() > 0) &&
                   (std::all_of(number.begin(), number.end(), [](TCHAR c) { return std::isdigit(c); })) &&
                   (std::find(_versions.begin(), _versions.end(), static_cast<uint8_t>(atoi(number.c_str()))) != _versions.end());
        }

    private:
        HandlerMap _handlers;
        string _callsign;
        string _designator;
        std::vector<uint8_t> _versions;
    };

} } } // namespace WPEFramework::Core::JSONRPC
