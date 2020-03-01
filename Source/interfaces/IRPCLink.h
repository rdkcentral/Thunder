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

#ifndef __INTERFACE_RPCLINK_H
#define __INTERFACE_RPCLINK_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface show the use case of communicating transparently over process boundaries
    struct IRPCLink : virtual public Core::IUnknown {
        enum { ID = ID_RPCLINK };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_RPCLINK_NOTIFICATION };

            virtual ~INotification() {}

            // Some change happened with respect to the test case ..
            virtual void Completed(const uint32_t id, const string& name) = 0;
        };

        virtual ~IRPCLink() {}

        virtual void Register(INotification* notification) = 0;
        virtual void Unregister(INotification* notification) = 0;

        virtual uint32_t Start(const uint32_t id, const string& name) = 0;
        virtual uint32_t Stop() = 0;
        virtual uint32_t ForceCallback() = 0;
    };

} // namespace Exchange
} // namespace WPEFramework

#endif // __INTERFACE_RPCLINK_H
