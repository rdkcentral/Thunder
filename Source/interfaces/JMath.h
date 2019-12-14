#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This is an example to show the workings and how to develope a COMRPC/JSONRPC method/interface
    struct JMath {
    public:
        static void Register(PluginHost::JSONRPC& module, const Exchange::IMath* destination)
        {
            module.Register(_T("Add"), [destination](const string&, const string& parameters, string& result) -> uint32_t {
                class Parameters : public Core::JSON::Container {
                public:
                    Parameters(const Parameters&) = delete;
                    Parameters& operator=(const Parameters&) = delete;
                    Parameters()
                        : Core::JSON::Container()
                    {
                        Add(_T("A"), &A);
                        Add(_T("B"), &B);
                        Add(_T("sum"), &sum);
                    }
                    ~Parameters() = default;

                public:
                    Core::JSON::DecUInt16 A;
                    Core::JSON::DecUInt16 B;
                    Core::JSON::DecUInt16 sum;
                } inbound, outbound; 
               
                inbound.FromString(parameters);
                uint16_t sum;
                uint32_t code = destination->Add(inbound.A.Value(), inbound.B.Value(), sum);
                if (code == Core::ERROR_NONE) {
                    outbound.sum = sum;
                    outbound.ToString(result);
                } else {
                    result.clear();
                }
                return (code);
            });
            module.Register(_T("Sub"), [destination](const string&, const string& parameters, string& result) -> uint32_t {
                class Parameters : public Core::JSON::Container {
                public:
                    Parameters(const Parameters&) = delete;
                    Parameters& operator=(const Parameters&) = delete;
                    Parameters()
                        : Core::JSON::Container()
                    {
                        Add(_T("A"), &A);
                        Add(_T("B"), &B);
                        Add(_T("sum"), &sum);
                    }
                    ~Parameters() = default;

                public:
                    Core::JSON::DecUInt16 A;
                    Core::JSON::DecUInt16 B;
                    Core::JSON::DecUInt16 sum;
                } inbound, outbound; 
               
                inbound.FromString(parameters);
                uint16_t sum;
                uint32_t code = destination->Sub(inbound.A.Value(), inbound.B.Value(), sum);
                if (code == Core::ERROR_NONE) {
                    outbound.sum = sum;
                    outbound.ToString(result);
                } else {
                    result.clear();
                }
                return (code);
            });
        }
        static void Unregister(PluginHost::JSONRPC& module)
        {
            module.Unregister(_T("Sub"));
            module.Unregister(_T("Add"));
        }
    };
}
}
