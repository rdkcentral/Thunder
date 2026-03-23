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
#include "Messages.h"
#include "../core/BinderTransport.h"
#include "../core/IPCConnector.h"

#include <atomic>

namespace Thunder {
namespace RPC {

    // -----------------------------------------------------------------------
    // Binder COMRPC transaction codes
    // BINDER_COMRPC_ANNOUNCE: client announces itself and requests a session
    // BINDER_COMRPC_INVOKE:   client invokes a method on a session
    // -----------------------------------------------------------------------
    static constexpr uint32_t BINDER_COMRPC_ANNOUNCE = 0x4E415243u; // B_PACK_CHARS('C','R','A','N')
    static constexpr uint32_t BINDER_COMRPC_INVOKE    = 0x56495243u; // B_PACK_CHARS('C','R','I','V')

    // -----------------------------------------------------------------------
    // BinderIPCChannel
    //
    // Implements Core::IPCChannel over the Android Binder transport.
    //
    // Two usage modes:
    //
    //  SERVER SIDE (created by BinderCommunicator when an ANNOUNCE arrives):
    //    - _targetHandle == 0 (no outbound capability)
    //    - _sessionId    == server-assigned session cookie
    //    - Before dispatching an INVOKE, BinderCommunicator calls
    //      SetReplyBuffer() to point this channel at the binder reply buffer.
    //    - ReportResponse() serialises inbound->IResponse() into that buffer.
    //
    //  CLIENT SIDE (created by BinderCommunicatorClient after ANNOUNCE):
    //    - _targetHandle == binder handle of the remote service
    //    - _sessionId    == session cookie returned by the server
    //    - Execute(cmd, timeout) serialises cmd params, calls binder, and
    //      deserialises the reply into cmd->IResponse().
    // -----------------------------------------------------------------------
    class EXTERNAL BinderIPCChannel : public Core::IPCChannel {
    public:
        BinderIPCChannel()                                    = delete;
        BinderIPCChannel(const BinderIPCChannel&)             = delete;
        BinderIPCChannel& operator=(const BinderIPCChannel&)  = delete;

        /**
         * @brief Server-side constructor.
         *
         * @param transport   Reference to the BinderTransport owned by
         *                    BinderCommunicator (BinderEndpoint).
         * @param sessionId   Unique 32-bit session cookie assigned by the server.
         * @param channelId   Unique channel identifier (returned by Id()).
         */
        BinderIPCChannel(Core::BinderTransport& transport,
                         uint32_t sessionId,
                         uint32_t channelId);

        /**
         * @brief Client-side constructor.
         *
         * @param transport     Reference to the BinderTransport owned by the client.
         * @param targetHandle  Binder handle of the remote service.
         * @param sessionId     Session cookie returned by the server in the
         *                      ANNOUNCE reply.
         * @param channelId     Unique channel identifier (returned by Id()).
         */
        BinderIPCChannel(Core::BinderTransport& transport,
                         uint32_t targetHandle,
                         uint32_t sessionId,
                         uint32_t channelId);

        ~BinderIPCChannel() override;

        /**
         * @brief Server side only: set the binder reply buffer for the
         *        currently-dispatched INVOKE.  Must be called before
         *        BinderCommunicator dispatches the IIPCServer handler.
         */
        void SetReplyBuffer(Core::BinderBuffer& replyBuf);

        // Core::IPCChannel overrides
        uint32_t Id()     const override;
        string   Origin() const override;
        bool     IsOpen() const override;
        bool     IsClosed() const override;

        uint32_t ReportResponse(Core::ProxyType<Core::IIPC>& inbound) override;

    private:
        // Serialise the full contents of an IMessage into a heap buffer.
        // Returns the number of bytes written.
        static uint32_t SerialiseMessage(const Core::IMessage& msg,
                                         uint8_t* buf,
                                         uint32_t bufSize);

        uint32_t Execute(const Core::ProxyType<Core::IIPC>& command,
                         Core::IDispatchType<Core::IIPC>* completed) override;

        uint32_t Execute(const Core::ProxyType<Core::IIPC>& command,
                         const uint32_t waitTime) override;

        Core::BinderTransport& _transport;
        const uint32_t         _targetHandle; // 0 on server side
        const uint32_t         _sessionId;
        const uint32_t         _channelId;
        Core::BinderBuffer*    _replyBuf;     // non-null only during server-side invoke dispatch
        mutable bool           _open;
    };

} // namespace RPC
} // namespace Thunder
