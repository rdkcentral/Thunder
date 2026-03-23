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
#include "../core/BinderTransport.h"

#include <thread>
#include <vector>
#include <functional>

namespace Thunder {
namespace RPC {

    /**
     * @brief Base class for any binder server endpoint (service manager or COMRPC server).
     *
     * Manages a pool of worker threads each driving a Core::BinderTransport loop.
     * Derived classes override HandleTransaction() to decode and handle incoming
     * binder transactions on their own BinderTransport instance.
     */
    class EXTERNAL BinderEndpoint {
    public:
        BinderEndpoint(const BinderEndpoint&) = delete;
        BinderEndpoint& operator=(const BinderEndpoint&) = delete;

        BinderEndpoint();
        virtual ~BinderEndpoint();

        /**
         * @brief Open the binder transport and start @a numThreads worker threads.
         *
         * @param numThreads        Number of looper threads to spawn.
         * @param contextManager    If true, call BecomeContextManager() on the transport
         *                          (used by BinderServiceManager).
         * @param mapSize           mmap size for the transport (default 128 KiB).
         * @return Core::ERROR_NONE on success.
         */
        uint32_t Open(uint32_t numThreads,
                      bool contextManager = false,
                      size_t mapSize = Core::BinderTransport::DefaultMapSize);

        /**
         * @brief Stop all looper threads and close the transport.
         */
        uint32_t Close();

        /**
         * @brief Returns true when the transport is open and threads are running.
         */
        bool IsOpen() const;

        /**
         * @brief Direct access to the underlying transport (for derived classes that
         *        need to issue outbound calls, Acquire/Release, etc.).
         */
        Core::BinderTransport& Transport();

    protected:
        /**
         * @brief Called on a looper thread for every incoming BR_TRANSACTION.
         *
         * @param txn   Raw kernel transaction data (target, cookie, code, data).
         * @param msg   Decoded read-buffer wrapping the kernel data pointer.
         * @param reply Write-buffer whose contents will be sent as BC_REPLY.
         * @return 0 on success, non-zero status encoded in BC_REPLY as TF_STATUS_CODE.
         */
        virtual int HandleTransaction(binder_transaction_data* txn,
                                      Core::BinderBuffer& msg,
                                      Core::BinderBuffer& reply) = 0;

    private:
        Core::BinderTransport           _transport;
        std::vector<std::thread>        _threads;
    };

} // namespace RPC
} // namespace Thunder
