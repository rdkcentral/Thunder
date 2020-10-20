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
    // @json
    struct EXTERNAL IAmazonPrime : virtual public Core::IUnknown {

        enum { ID = ID_AMAZONPRIME };

        /* @event */
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_AMAZONPRIME_NOTIFICATION };

            // @brief Receive a message from the generic message bus
            // @param messsage the message that was received.
            virtual void Receive(const string& message) = 0;
        };

        virtual void Register(IAmazonPrime::INotification* ignition) = 0;
        virtual void Unregister(IAmazonPrime::INotification* ignition) = 0;

        // @brief Send a message over the message bus to ignition
        // @param messsage the message to send according the amazon spec found
        //        in the parter portal.
        virtual uint32_t Send(const string& message) = 0;
};
}
}
