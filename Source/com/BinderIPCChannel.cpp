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

#include "BinderIPCChannel.h"

#include <cstring>

namespace Thunder {
namespace RPC {

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

BinderIPCChannel::BinderIPCChannel(Core::BinderTransport& transport,
                                   uint32_t sessionId,
                                   uint32_t channelId)
    : Core::IPCChannel()
    , _transport(transport)
    , _targetHandle(0)
    , _sessionId(sessionId)
    , _channelId(channelId)
    , _replyBuf(nullptr)
    , _open(true)
{
}

BinderIPCChannel::BinderIPCChannel(Core::BinderTransport& transport,
                                   uint32_t targetHandle,
                                   uint32_t sessionId,
                                   uint32_t channelId)
    : Core::IPCChannel()
    , _transport(transport)
    , _targetHandle(targetHandle)
    , _sessionId(sessionId)
    , _channelId(channelId)
    , _replyBuf(nullptr)
    , _open(true)
{
}

BinderIPCChannel::~BinderIPCChannel()
{
    _open = false;
}

// ---------------------------------------------------------------------------
// SetReplyBuffer (server-side only)
// ---------------------------------------------------------------------------

void BinderIPCChannel::SetReplyBuffer(Core::BinderBuffer& replyBuf)
{
    _replyBuf = &replyBuf;
}

// ---------------------------------------------------------------------------
// Core::IPCChannel overrides
// ---------------------------------------------------------------------------

uint32_t BinderIPCChannel::Id() const
{
    return _channelId;
}

string BinderIPCChannel::Origin() const
{
    return string("Binder:") + std::to_string(_targetHandle);
}

bool BinderIPCChannel::IsOpen() const
{
    return _open && _transport.IsOpen();
}

bool BinderIPCChannel::IsClosed() const
{
    return !IsOpen();
}

// ---------------------------------------------------------------------------
// ReportResponse (server-side: write response into the binder reply buffer)
// ---------------------------------------------------------------------------

uint32_t BinderIPCChannel::ReportResponse(Core::ProxyType<Core::IIPC>& inbound)
{
    if (_replyBuf == nullptr) {
        TRACE_L1("BinderIPCChannel::ReportResponse called without a reply buffer");
        return Core::ERROR_ILLEGAL_STATE;
    }

    Core::ProxyType<Core::IMessage> resp = inbound->IResponse();
    if (!resp.IsValid()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    const uint32_t respLen = resp->Length();
    uint8_t tmpBuf[RPC::CommunicationBufferSize];

    if (respLen > sizeof(tmpBuf)) {
        TRACE_L1("BinderIPCChannel::ReportResponse: response too large (%u bytes)", respLen);
        return Core::ERROR_GENERAL;
    }

    uint32_t written = SerialiseMessage(*resp, tmpBuf, sizeof(tmpBuf));
    _replyBuf->PutByteArray(tmpBuf, written);
    _replyBuf = nullptr; // consume

    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Execute (client-side: synchronous binder invoke)
// ---------------------------------------------------------------------------

uint32_t BinderIPCChannel::Execute(const Core::ProxyType<Core::IIPC>& command,
                                   const uint32_t waitTime)
{
    if (!_transport.IsOpen() || _targetHandle == 0) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    // --- Serialise parameters ---
    Core::ProxyType<Core::IMessage> params = command->IParameters();
    if (!params.IsValid()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    uint8_t paramBuf[RPC::CommunicationBufferSize];
    const uint32_t paramLen = SerialiseMessage(*params, paramBuf, sizeof(paramBuf));
    if (paramLen == 0) {
        return Core::ERROR_GENERAL;
    }

    // --- Build binder request: [session_id uint32][paramLen uint32][param bytes] ---
    uint8_t msgData[RPC::CommunicationBufferSize + 8];
    Core::BinderBuffer binderMsg(msgData, sizeof(msgData), 0);
    binderMsg.PutUInt32(_sessionId);
    binderMsg.PutByteArray(paramBuf, paramLen);

    uint8_t replyData[RPC::CommunicationBufferSize + 4];
    Core::BinderBuffer binderReply(replyData, sizeof(replyData), 0);

    if (binderMsg.HasOverflow()) {
        TRACE_L1("BinderIPCChannel::Execute: outgoing message overflow");
        return Core::ERROR_GENERAL;
    }

    // --- Issue the binder call ---
    if (_transport.Call(binderMsg, binderReply, _targetHandle, BINDER_COMRPC_INVOKE) != 0) {
        TRACE_L1("BinderIPCChannel::Execute: binder Call failed");
        return Core::ERROR_GENERAL;
    }

    // --- Deserialise response ---
    size_t respLen = 0;
    const uint8_t* respData = binderReply.GetByteArray(respLen);
    if (respData != nullptr && respLen > 0) {
        Core::ProxyType<Core::IMessage> resp = command->IResponse();
        if (resp.IsValid()) {
            uint32_t offset = 0;
            while (offset < static_cast<uint32_t>(respLen)) {
                uint16_t consumed = resp->Deserialize(
                    respData + offset,
                    static_cast<uint16_t>(respLen - offset),
                    offset);
                if (consumed == 0) break;
                offset += consumed;
            }
        }
    }

    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Execute — async variant (wraps sync using CoreEvent / callback)
// ---------------------------------------------------------------------------

namespace {
    class AsyncTrigger : public Core::IDispatchType<Core::IIPC> {
    public:
        AsyncTrigger(Core::IDispatchType<Core::IIPC>* callback)
            : _callback(callback)
            , _signal(false, true)
        {
        }
        void Dispatch(Core::IIPC& element) override
        {
            if (_callback != nullptr) {
                _callback->Dispatch(element);
            }
            _signal.SetEvent();
        }
        uint32_t Wait(uint32_t ms)
        {
            return _signal.Lock(ms);
        }

    private:
        Core::IDispatchType<Core::IIPC>* _callback;
        Core::Event                      _signal;
    };
}

uint32_t BinderIPCChannel::Execute(const Core::ProxyType<Core::IIPC>& command,
                                   Core::IDispatchType<Core::IIPC>* completed)
{
    // For binder the synchronous path is used; call Execute(sync) and then
    // fire the callback.
    const uint32_t result = Execute(command, RPC::CommunicationTimeOut);

    if (completed != nullptr) {
        completed->Dispatch(*command);
    }

    return result;
}

// ---------------------------------------------------------------------------
// SerialiseMessage (static helper)
// ---------------------------------------------------------------------------

/*static*/ uint32_t BinderIPCChannel::SerialiseMessage(const Core::IMessage& msg,
                                                        uint8_t* buf,
                                                        uint32_t bufSize)
{
    const uint32_t totalLen = msg.Length();
    if (totalLen > bufSize || totalLen == 0) {
        return 0;
    }

    uint32_t offset  = 0;
    uint32_t written = 0;
    while (written < totalLen) {
        uint16_t chunk = msg.Serialize(
            buf + written,
            static_cast<uint16_t>(bufSize - written),
            offset);
        if (chunk == 0) break;
        written += chunk;
        offset  += chunk;
    }
    return written;
}

} // namespace RPC
} // namespace Thunder
