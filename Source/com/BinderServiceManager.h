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
#include "BinderEndpoint.h"
#include "IServiceManager.h"

#include <unordered_map>
#include <string>

namespace Thunder {
namespace RPC {

    // -----------------------------------------------------------------------
    // BinderServiceManager — the in-process context manager server
    // -----------------------------------------------------------------------
    /**
     * @brief Singleton binder context manager that runs inside the Thunder process.
     *
     * It becomes handle 0 on the binder kernel and answers SVC_MGR_* protocol
     * codes from any client in the system.
     *
     * Usage:
     *   BinderServiceManager::Instance().Start(4);  // 4 looper threads
     *   ...
     *   BinderServiceManager::Instance().Stop();
     */
    class EXTERNAL BinderServiceManager
        : public BinderEndpoint
        , public IServiceManager
        , public Core::BinderTransport::IDeathObserver
    {
    public:
        BinderServiceManager(const BinderServiceManager&) = delete;
        BinderServiceManager& operator=(const BinderServiceManager&) = delete;

        static BinderServiceManager& Instance();

        /**
         * @brief Start the context manager with @a numThreads looper threads.
         * @return Core::ERROR_NONE on success.
         */
        uint32_t Start(uint32_t numThreads = 4);

        /**
         * @brief Stop the context manager and all looper threads.
         */
        uint32_t Stop();

        bool IsRunning() const;

        // IServiceManager
        uint32_t AddService(const string& name, uint32_t handle, bool allowIsolated) override;
        uint32_t GetService(const string& name, uint32_t& handle) override;
        uint32_t ListService(uint32_t index, string& name) override;

        // IDeathObserver
        void ServiceDied(uint32_t handle) override;

    protected:
        // BinderEndpoint
        int HandleTransaction(binder_transaction_data* txn,
                              Core::BinderBuffer& msg,
                              Core::BinderBuffer& reply) override;

    private:
        BinderServiceManager();
        ~BinderServiceManager() override;

        struct SvcEntry {
            uint32_t handle;
            bool     allowIsolated;
        };

        Core::CriticalSection                    _lock;
        std::unordered_map<string, SvcEntry>     _services;
    };

    // -----------------------------------------------------------------------
    // BinderServiceManagerProxy — client-side proxy for handle 0
    // -----------------------------------------------------------------------
    /**
     * @brief Client proxy that sends SVC_MGR_* transactions to binder handle 0.
     *
     * Does NOT run a looper thread — every IServiceManager call is a blocking
     * synchronous binder transaction.
     */
    class EXTERNAL BinderServiceManagerProxy : public IServiceManager {
    public:
        BinderServiceManagerProxy(const BinderServiceManagerProxy&) = delete;
        BinderServiceManagerProxy& operator=(const BinderServiceManagerProxy&) = delete;

        BinderServiceManagerProxy();
        ~BinderServiceManagerProxy() override;

        uint32_t Open(size_t mapSize = Core::BinderTransport::DefaultMapSize);
        uint32_t Close();
        bool IsOpen() const;

        // IServiceManager
        uint32_t AddService(const string& name, uint32_t handle, bool allowIsolated) override;
        uint32_t GetService(const string& name, uint32_t& handle) override;
        uint32_t ListService(uint32_t index, string& name) override;

        /**
         * @brief Register a binder node with the context manager using @a tx.
         *
         * Uses @a tx to send SVC_MGR_ADD_SERVICE with a BINDER_TYPE_BINDER object
         * so the kernel associates the node with @a tx's fd and routes future
         * transactions to that fd's looper threads.
         *
         * @param tx          The transport whose looper should receive transactions.
         * @param name        Service name to register.
         * @param binderPtr   User-space pointer to the server object (any stable address).
         * @param allowIsolated  Whether isolated processes may connect.
         */
        static uint32_t RegisterNode(Core::BinderTransport& tx,
                                     const string& name,
                                     uintptr_t binderPtr,
                                     bool allowIsolated = false);

        /**
         * @brief Look up a service handle via @a tx by sending to the context manager.
         *
         * @param tx    Transport to use for the binder IPC call.
         * @param name  Service name to look up.
         * @param handle  Set to the kernel-assigned handle on success.
         */
        static uint32_t LookupHandle(Core::BinderTransport& tx,
                                     const string& name,
                                     uint32_t& handle);

    private:
        /**
         * @brief Prepare a message header required by all SVC_MGR calls.
         *
         * Writes strict_policy (uint32 = 0) followed by the UTF-16 interface name
         * "Thunder.IServiceManager" into @a msg.
         */
        static void WriteHeader(Core::BinderBuffer& msg);

        Core::BinderTransport _transport;
    };

} // namespace RPC
} // namespace Thunder
