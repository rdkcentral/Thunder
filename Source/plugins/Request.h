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

#ifndef __PLUGIN_FRAMEWORK_REQUEST_H
#define __PLUGIN_FRAMEWORK_REQUEST_H

#include "Module.h"

namespace WPEFramework {
namespace PluginHost {

    // Forward declaration. We only have a smart pointer to a service.
    class Service;

    // Request related classes
    // These classes realize the allocation and supply of requests. Whenever
    // a reuest is coming in, all information, gathered during the loading
    // of the request, is stored in the request. The pool, recycles requests
    // used but are not actively used.
    class EXTERNAL Request : public Web::Request {
    public:
        enum enumState : uint8_t {
            INCOMPLETE = 0x01,
            OBLIVIOUS,
            MISSING_CALLSIGN,
            INVALID_VERSION,
            COMPLETE,
            UNAUTHORIZED,
            SERVICE_CALL = 0x80	
        };

    private:
        Request(const Request&) = delete;
        Request& operator=(const Request&) = delete;

    public:
        Request();
        virtual ~Request();

    public:
		// This method tells us if this call was received over the 
		// Service prefix path or (if it is false) over the JSONRPC
		// prefix path.
        inline bool ServiceCall() const
        {
            return ((_state & SERVICE_CALL) != 0);
        }
        inline enumState State() const
        {
            return (static_cast<enumState>(_state & 0x7F));
        }
        inline Core::ProxyType<PluginHost::Service>& Service()
        {
            return (_service);
        }
		inline void Unauthorized() {
            _state = ((_state & 0x80) | UNAUTHORIZED);
		}

        void Clear();
        void Service(const uint32_t errorCode, const Core::ProxyType<PluginHost::Service>& service, const bool serviceCall);

    private:
        uint8_t _state;
        Core::ProxyType<PluginHost::Service> _service;
    };

    typedef Core::ProxyPoolType<PluginHost::Request> RequestPool;
}
}

#endif // __PLUGIN_FRAMEWORK_REQUEST_
