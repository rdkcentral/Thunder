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

namespace WPEFramework {

    namespace PluginHost {

        struct EXTERNAL ILocalDispatcher;

        struct EXTERNAL IDispatcher : public virtual Core::IUnknown {
            virtual ~IDispatcher() override = default;

            enum { ID = RPC::ID_DISPATCHER };

            struct EXTERNAL ICallback : public virtual Core::IUnknown {
                virtual ~ICallback() override = default;

                enum { ID = RPC::ID_DISPATCHER_CALLBACK };

                virtual Core::hresult Event(const string& event, const string& parameters /* @restrict:(4M-1) */) = 0;
                virtual Core::hresult Error(const uint32_t channel, const uint32_t id, const uint32_t code, const string& message) = 0;
                virtual Core::hresult Response(const uint32_t channel, const uint32_t id, const string& response /* @restrict:(4M-1) */) = 0;

                virtual Core::hresult Subscribe(const uint32_t channel, const string& event, const string& designator) = 0;
                virtual Core::hresult Unsubscribe(const uint32_t channel, const string& event, const string& designator) = 0;
            };

            virtual Core::hresult Validate(const string& token, const string& method, const string& paramaters /* @restrict:(4M-1) */) const = 0;
            virtual Core::hresult Invoke(ICallback* callback, const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters /* @restrict:(4M-1) */, string& response /* @restrict:(4M-1) @out */) = 0;
            virtual Core::hresult Revoke(ICallback* callback) = 0;

            // If we need to activate this locally, we can get access to the base..
            /* @stubgen:stub */
            virtual ILocalDispatcher* Local() = 0;
        };
    }

}

#endif
