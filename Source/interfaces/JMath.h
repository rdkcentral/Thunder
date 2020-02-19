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
