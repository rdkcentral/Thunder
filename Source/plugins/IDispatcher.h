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
 
#include <com/ICOM.h>
#include "IShell.h"

 // @stubgen:include <plugins/IShell.h>

namespace WPEFramework {

    namespace PluginHost {

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

            // Lifetime managment of the IDispatcher.
            // Attach is to be called prior to receiving JSONRPC requests!
            // Detach is to be called if the service is nolonger required!
            virtual Core::hresult Attach(IShell::IConnectionServer::INotification*& sink /* @out */, IShell* service) = 0;
            virtual Core::hresult Detach(IShell::IConnectionServer::INotification*& sink /* @out */) = 0;

            // If a callback is unexpectedly dropepd (non-happy day scenarios) it is reported through this
            // method that all subscribtions for a certain callback can be dropped..
            virtual void Dropped(const ICallback* callback) = 0;
        };
    }
}
