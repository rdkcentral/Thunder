/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

// This file only serves as documentation source for the built-in JSON-RPC methods!

#pragma once
#include <core/core.h>

namespace Thunder {
namespace Exchange {
namespace JSONRPC {

    // @json 1.0.0
    struct IServiceBuiltIns {

        struct Interface {
            string name /* @brief Name of the interface (e.g. "JMyInterface") */;
            uint8_t major /* @brief Major part of version number (e.g. 1) */;
            uint8_t minor /* @brief Minor part of version number */;
            uint8_t patch /* @brief Patch part of version version number */;
        };

        // @brief Retrieves a list of JSON-RPC interfaces offered by this service
        // @description %%VERSIONS%%
        // @param versions A list ofsinterfaces with their version numbers
        virtual Core::hresult Versions(std::vector<Interface>& versions /* @out @restrict:255 */) = 0;
    };

    // @json 1.0.0
    struct IInterfaceBuiltIns  {

        // @brief Checks if a JSON-RPC method or property exists
        // @details This method will return *True* for the following methods/properties: *$METHODSANDPROPERTIES*.
        // @param method Name of the method or property to look up (e.g. $METHODSANDPROPERTIES[0])
        // @param exists Denotes if the method exists or not
        virtual Core::hresult Exists(const string& method, bool& exists /* @out */) = 0;
    };

    // @json 1.0.0
    struct IInterfaceWithEventsBuiltIns {

        // @brief Registers for an asynchronous JSON-RPC notification
        // @details This method supports the following event names: *$EVENTSWITHLINKS*.
        // @param event Name of the notification to register for (e.g. $EVENTS[0])
        // @param id Client identifier (e.g. "myapp")
        // @retval ERROR_FAILED_REGISTERED Failed to register for the notification (e.g. already registered)
        virtual Core::hresult Register(const string& event, const string& id) = 0;

        // @brief Unregisters from an asynchronous JSON-RPC notification
        // @details This method supports the following event names: *$EVENTSWITHLINKS*.
        // @param event Name of the notification to register for  (e.g. $EVENTS[0])
        // @param id Client identifier (e.g. "myapp")
        // @retval ERROR_FAILED_UNREGISTERED Failed to unregister from the notification (e.g. not yet registered)
        virtual Core::hresult Unregister(const string& event, const string& id) = 0;
    };

} // namespace JSONRPCs
} // namespace Exchange 
}

