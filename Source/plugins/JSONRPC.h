#pragma once

#include "Module.h"

#include <functional>

namespace WPEFramework {

namespace PluginHost {

    struct IDispatcher : public virtual Core::IUnknown {
        virtual ~IDispatcher() {}

        virtual uint32_t Invoke (const uint32_t id, const string& methodName, const string& parameters, string& result) = 0;
    };

    class EXTERNAL JSONRPC : public IDispatcher  {
    private:
        JSONRPC(const JSONRPC&) = delete;
        JSONRPC& operator= (const JSONRPC&) = delete;

    public:
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
            Message()
                : Core::JSON::Container()
                , Version()
                , Id(~0)
                , Designator()
                , Parameters()
                , Result()
                , Error()
            {
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

   private:
        typedef std::function<uint32_t(const uint32_t id, const string& parameters, string& result)> InvokeFunction;
        typedef std::map<const string, InvokeFunction > HandlerMap;

    public:
        JSONRPC() 
            : _handlers()
            , _service(nullptr) {
        }
        virtual ~JSONRPC() {
        }

    public:
        uint32_t Validate (const Message& message) const {
            const string callsign (message.Callsign());
            uint32_t result = (callsign.empty() ? Core::ERROR_NONE : Core::ERROR_INVALID_DESIGNATOR);
            if ( (result != Core::ERROR_NONE) && (_service != nullptr) ) {
                const string source (_service->Callsign());
                if (source.compare(0, source.length(), callsign) == 0) {
                    result = Core::ERROR_INVALID_SIGNATURE;
                    uint32_t length = source.length();

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
            std::function<uint32_t(const uint32_t id, const INBOUND& parameters, OUTBOUND& result)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            InvokeFunction implementation = [actualMethod](const uint32_t id, const string& parameters, string& result) -> uint32_t {
                                               INBOUND inbound; 
                                               OUTBOUND outbound;
                                               inbound.FromString(parameters);
                                               uint32_t code = actualMethod(id, inbound, outbound);
                                               if (id != static_cast<uint32_t>(~0)) { 
                                                   string response; 
                                                   Message message;
                                                   message.Version = _T("2.0");
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
        virtual uint32_t Invoke (const uint32_t id, const string& method, const string& parameters, string& response) override {
            uint32_t result = Core::ERROR_UNKNOWN_KEY_PASSED;

            response.clear();

            HandlerMap::iterator index = _handlers.find(method);
            if (index != _handlers.end()) {
                result = index->second(id, parameters, response);
            }
            else if (id != static_cast<uint32_t>(~0)) {
                Message message;
                message.Version = _T("2.0");
                message.Error.SetError(result);
                message.Error.Text = "Method not found";
                message.Id = id;
                message.ToString(response);
            }
            return (result);
        }
        uint32_t Invoke (const Message& inbound, string& response) {
            uint32_t result = Validate(inbound);
            
            if (result == Core::ERROR_NONE) {
               result = Invoke(inbound.Id.Value(), inbound.Method(), inbound.Parameters.Value(), response);
            }
            else if (inbound.Id.Value() != static_cast<uint32_t>(~0)) {
                Message message;
                message.Version = _T("2.0");
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
        void SetMetadata(const IShell* service) {
            ASSERT ((service != nullptr) ^ (_service != nullptr));
            _service = service;
        }

    private:
        bool HasVersionSupport(const string& number) const {
            return (number.length() > 0) &&
                   (std::all_of(number.begin(), number.end(), [](TCHAR c) { return std::isdigit(c); })) &&
                   (_service->IsSupported(static_cast<uint8_t>(atoi(number.c_str()))));
        }

    private:
        HandlerMap _handlers;
        const IShell* _service;
    };

} } // Namespace WPEFramework::PluginHost

