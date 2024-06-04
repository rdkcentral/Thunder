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

#ifndef __IDISPATCHER_H__
#define __IDISPATCHER_H__

#include <com/ICOM.h>

namespace Thunder {

    namespace PluginHost {

        struct EXTERNAL ILocalDispatcher;

        struct EXTERNAL IDispatcher : public virtual Core::IUnknown {
            ~IDispatcher() override = default;

            enum { ID = RPC::ID_DISPATCHER };

            struct EXTERNAL ICallback : public virtual Core::IUnknown {
                ~ICallback() override = default;

                enum { ID = RPC::ID_DISPATCHER_CALLBACK };

                virtual Core::hresult Event(const string& event, const string& designator, const string& parameters /* @restrict:(4M-1) */) = 0;
            };

            virtual uint32_t Invoke(const uint32_t channelid, const uint32_t id, const string& token, const string& method, const string& parameters, string& response /* @out */) = 0;

            virtual Core::hresult Subscribe(ICallback* callback, const string& event, const string& designator) = 0;
            virtual Core::hresult Unsubscribe(ICallback* callback, const string& event, const string& designator) = 0;

            // If this is a local instance of this interface, we get access to the IShell
            // of this service which in turn allows access to the channels and thus the 
            // possibility to return responses on the right JSONRPC channels.
            /* @stubgen:stub */
            virtual ILocalDispatcher* Local() = 0;
        };
    }

}

#endif
