/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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
#include <string>

namespace Thunder {
namespace RPC {

    /**
     * @brief Pure-virtual interface for querying / registering named binder services.
     *
     * The server-side implementation is BinderServiceManager (singleton context manager).
     * The client-side implementation is BinderServiceManagerProxy (outbound calls to
     * handle 0, the context manager).
     */
    struct EXTERNAL IServiceManager {
        virtual ~IServiceManager() = default;

        /**
         * @brief Register a service under @a name.
         *
         * @param name          UTF-8 service name.
         * @param handle        Outgoing binder handle for the service object.
         * @param allowIsolated True if isolated (sandboxed) clients may access this service.
         * @return Core::ERROR_NONE on success.
         */
        virtual uint32_t AddService(const string& name,
                                    uint32_t handle,
                                    bool allowIsolated) = 0;

        /**
         * @brief Look up a service by name.
         *
         * @param name   UTF-8 service name to look up.
         * @param handle Receives the binder handle on success.
         * @return Core::ERROR_NONE on success, Core::ERROR_UNAVAILABLE if not found.
         */
        virtual uint32_t GetService(const string& name, uint32_t& handle) = 0;

        /**
         * @brief Enumerate registered services.
         *
         * @param index Zero-based index.
         * @param name  Receives the service name at @a index.
         * @return Core::ERROR_NONE while more entries exist,
         *         Core::ERROR_UNAVAILABLE when the index is out of range.
         */
        virtual uint32_t ListService(uint32_t index, string& name) = 0;
    };

    /**
     * @brief Returns the IServiceManager for this process.
     *
     * If BinderServiceManager::Instance() is running (i.e. we ARE the context manager)
     * it returns that instance directly.  Otherwise it returns a BinderServiceManagerProxy
     * that makes outbound calls to handle 0.
     *
     * The returned reference is live for the process lifetime; do not delete it.
     */
    EXTERNAL IServiceManager& ServiceManager();

} // namespace RPC
} // namespace Thunder
