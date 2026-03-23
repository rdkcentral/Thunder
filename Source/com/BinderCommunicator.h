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
#include "Administrator.h"
#include "BinderEndpoint.h"
#include "BinderIPCChannel.h"
#include "Messages.h"
#include "../core/BinderTransport.h"

#include <atomic>
#include <unordered_map>

namespace Thunder {
namespace RPC {

    // -----------------------------------------------------------------------
    // BinderCommunicator — COMRPC server over Android Binder
    // -----------------------------------------------------------------------
    /**
     * @brief Hosts a named COMRPC service over the binder transport.
     *
     * The service name is registered with IServiceManager so that clients can
     * discover it.  Each client connects via BINDER_COMRPC_ANNOUNCE, gets a
     * session cookie, and subsequently invokes methods via BINDER_COMRPC_INVOKE.
     *
     * Derived classes must override Acquire() to hand out their interface
     * implementations.  Offer()/Revoke() handle reverse-direction offerings.
     *
     * The handler parameter controls invoke dispatch:
     *   - null (default) → uses InvokeHandler (synchronous, same looper thread)
     *   - custom         → called directly (must be synchronous)
     */
    class EXTERNAL BinderCommunicator : public BinderEndpoint {
    public:
        BinderCommunicator() = delete;
        BinderCommunicator(BinderCommunicator&&) = delete;
        BinderCommunicator(const BinderCommunicator&) = delete;
        BinderCommunicator& operator=(BinderCommunicator&&) = delete;
        BinderCommunicator& operator=(const BinderCommunicator&) = delete;

        BinderCommunicator(
            const string& serviceName,
            const string& proxyStubPath,
            const Core::ProxyType<Core::IIPCServer>& handler,
            const TCHAR* sourceName = nullptr);

        virtual ~BinderCommunicator();

        /**
         * @brief Open the service, start looper threads, and register with svcmgr.
         * @param numThreads  Number of binder looper threads (default 4).
         */
        uint32_t Open(uint32_t numThreads = 4);

        /** @brief Un-register from svcmgr and close all sessions. */
        uint32_t Close();

        bool IsOpen() const;

    protected:
        // Override these to implement the service
        virtual void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t versionId) { return nullptr; }
        virtual void  Offer(Core::IUnknown* remote, const uint32_t interfaceId) {}
        virtual void  Revoke(const Core::IUnknown* remote, const uint32_t interfaceId) {}
        virtual void  Dangling(const Core::IUnknown* remote, const uint32_t interfaceId) {}

        // BinderEndpoint
        int HandleTransaction(binder_transaction_data* txn,
                              Core::BinderBuffer& msg,
                              Core::BinderBuffer& reply) override;

    private:
        int HandleAnnounce(Core::BinderBuffer& msg, Core::BinderBuffer& reply);
        int HandleInvoke(Core::BinderBuffer& msg, Core::BinderBuffer& reply);

        Core::ProxyType<BinderIPCChannel> FindSession(uint32_t sessionId);

        static uint32_t SerialiseData(const Core::IMessage& msg, uint8_t* buf, uint32_t bufSize);

        string                                              _serviceName;
        string                                              _proxyStubPath;
        Core::ProxyType<Core::IIPCServer>                   _handler;
        std::atomic<uint32_t>                               _nextSessionId;
        Core::CriticalSection                               _sessionsLock;
        std::unordered_map<uint32_t,
            Core::ProxyType<BinderIPCChannel>>              _sessions;
        uint32_t                                            _channelSeq;  // monotonic channel id
    };

    // -----------------------------------------------------------------------
    // BinderCommunicatorClient — COMRPC client over Android Binder
    // -----------------------------------------------------------------------
    /**
     * @brief Client side of the binder COMRPC transport.
     *
     * Steps:
     *  1. Open() — opens /dev/binder, discovers the service via IServiceManager,
     *              sends BINDER_COMRPC_ANNOUNCE, establishes a session.
     *  2. Acquire<INTERFACE>() — returns a proxy for the requested interface,
     *              registering it with the global Administrator.
     *  3. Close() — releases the session and closes the transport.
     */
    class EXTERNAL BinderCommunicatorClient {
    public:
        BinderCommunicatorClient() = delete;
        BinderCommunicatorClient(BinderCommunicatorClient&&) = delete;
        BinderCommunicatorClient(const BinderCommunicatorClient&) = delete;
        BinderCommunicatorClient& operator=(BinderCommunicatorClient&&) = delete;
        BinderCommunicatorClient& operator=(const BinderCommunicatorClient&) = delete;

        explicit BinderCommunicatorClient(const string& serviceName);
        ~BinderCommunicatorClient();

        uint32_t Open(uint32_t waitTimeMs = CommunicationTimeOut);
        uint32_t Close();
        bool IsOpen() const;

        /**
         * @brief Acquire a proxy for @a INTERFACE from the remote service.
         *
         * Sends BINDER_COMRPC_ANNOUNCE with className/interfaceId/versionId,
         * then creates an Administrator proxy via ProxyInstance.
         *
         * @return Interface proxy (caller must Release), or nullptr on failure.
         */
        template <typename INTERFACE>
        INTERFACE* Acquire(uint32_t waitTimeMs,
                           const string& className,
                           const uint32_t versionId)
        {
            if (!IsOpen()) return nullptr;

            INTERFACE* result = nullptr;
            void* iface       = nullptr;

            Core::instance_id impl = AcquireImpl(
                className, INTERFACE::ID, versionId, waitTimeMs);

            if (impl != 0 && _channel.IsValid()) {
                Core::ProxyType<Core::IPCChannel> baseChannel(_channel);
                Administrator::Instance().ProxyInstance(
                    baseChannel, impl, /*outbound=*/true,
                    INTERFACE::ID, iface);
                result = reinterpret_cast<INTERFACE*>(iface);
            }

            return result;
        }

        /**
         * @brief Returns the session ID established by the last successful Acquire<>() call.
         * A non-zero value means the ANNOUNCE transaction succeeded end-to-end.
         */
        uint32_t SessionId() const { return _sessionId; }

    private:
        // Send BINDER_COMRPC_ANNOUNCE and return the server-side impl pointer.
        // On success also fills _sessionId, _proxyStubPath, _channel.
        Core::instance_id AcquireImpl(const string& className,
                                       uint32_t interfaceId,
                                       uint32_t versionId,
                                       uint32_t waitTimeMs);

        static uint32_t SerialiseData(const Core::IMessage& msg, uint8_t* buf, uint32_t bufSize);

        string                           _serviceName;
        Core::BinderTransport            _transport;
        uint32_t                         _serviceHandle;
        uint32_t                         _sessionId;
        string                           _proxyStubPath;
        Core::ProxyType<BinderIPCChannel> _channel;
        uint32_t                         _channelId;
    };

} // namespace RPC
} // namespace Thunder