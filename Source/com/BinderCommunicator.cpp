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

#include "BinderCommunicator.h"
#include "BinderServiceManager.h"
#include "Communicator.h"
#include "../core/Trace.h"

#include <cstring>

namespace Thunder {
namespace RPC {

// ---------------------------------------------------------------------------
// BinderCommunicator
// ---------------------------------------------------------------------------

BinderCommunicator::BinderCommunicator(
    const string& serviceName,
    const string& proxyStubPath,
    const Core::ProxyType<Core::IIPCServer>& handler,
    const TCHAR* /*sourceName*/)
    : BinderEndpoint()
    , _serviceName(serviceName)
    , _proxyStubPath(proxyStubPath)
    , _handler(handler.IsValid() ? handler
                                 : Core::ProxyType<Core::IIPCServer>(
                                       Core::ProxyType<InvokeHandler>::Create()))
    , _nextSessionId(1)
    , _sessionsLock()
    , _sessions()
    , _channelSeq(0)
{
}

BinderCommunicator::~BinderCommunicator()
{
    Close();
}

uint32_t BinderCommunicator::Open(uint32_t numThreads)
{
    uint32_t result = BinderEndpoint::Open(numThreads);
    if (result != Core::ERROR_NONE) {
        return result;
    }

    // Register our binder node with the context manager using our own transport.
    // Passing our own transport is critical: the kernel associates the binder node
    // with this fd, so future transactions from clients are delivered to our looper
    // threads rather than to some other fd with no looper.
    result = BinderServiceManagerProxy::RegisterNode(
        Transport(), _serviceName, reinterpret_cast<uintptr_t>(this));
    if (result != Core::ERROR_NONE) {
        TRACE_L1("BinderCommunicator::Open '%s' svcmgr registration failed", _serviceName.c_str());
        BinderEndpoint::Close();
        return result;
    }

    TRACE_L1("BinderCommunicator::Open '%s' registered with %u threads", _serviceName.c_str(), numThreads);
    return Core::ERROR_NONE;
}

uint32_t BinderCommunicator::Close()
{
    // Drop all sessions
    _sessionsLock.Lock();
    _sessions.clear();
    _sessionsLock.Unlock();

    return BinderEndpoint::Close();
}

bool BinderCommunicator::IsOpen() const
{
    return BinderEndpoint::IsOpen();
}

// ---------------------------------------------------------------------------
// HandleTransaction — dispatched by BinderEndpoint looper threads
// ---------------------------------------------------------------------------

int BinderCommunicator::HandleTransaction(binder_transaction_data* txn,
                                          Core::BinderBuffer& msg,
                                          Core::BinderBuffer& reply)
{
    switch (txn->code) {
    case BINDER_COMRPC_ANNOUNCE:
        return HandleAnnounce(msg, reply);
    case BINDER_COMRPC_INVOKE:
        return HandleInvoke(msg, reply);
    default:
        TRACE_L1("BinderCommunicator: unknown transaction code 0x%08x", txn->code);
        return -1;
    }
}

// ---------------------------------------------------------------------------
// HandleAnnounce
// ---------------------------------------------------------------------------

int BinderCommunicator::HandleAnnounce(Core::BinderBuffer& msg,
                                       Core::BinderBuffer& reply)
{
    // Wire format: [uint32 initLen][init bytes]
    size_t initLen = 0;
    const uint8_t* initData = msg.GetByteArray(initLen);
    if (initData == nullptr || initLen == 0) {
        TRACE_L1("BinderCommunicator::HandleAnnounce: empty init blob");
        return -1;
    }

    Data::Init init;
    uint32_t offset = 0;
    while (offset < static_cast<uint32_t>(initLen)) {
        uint16_t consumed = init.Deserialize(
            initData + offset,
            static_cast<uint16_t>(initLen - offset),
            offset);
        if (consumed == 0) break;
        offset += consumed;
    }

    if (!init.IsValid()) {
        TRACE_L1("BinderCommunicator::HandleAnnounce: invalid init");
        return -1;
    }

    // Acquire the implementation
    void* impl = nullptr;

    if (init.IsAcquire()) {
        impl = Acquire(init.ClassName(), init.InterfaceId(), init.VersionId());
    } else if (init.IsOffer()) {
        // Client is offering us an interface — create a proxy and call Offer()
        Core::instance_id remoteImpl = init.Implementation();
        // Session must already exist from a previous ACQUIRE
        _sessionsLock.Lock();
        Core::ProxyType<BinderIPCChannel> existing;
        if (!_sessions.empty()) {
            existing = _sessions.begin()->second; // best-effort: associate with first session
        }
        _sessionsLock.Unlock();

        if (existing.IsValid()) {
            Core::ProxyType<Core::IPCChannel> baseChannel(existing);
            void* realIF = nullptr;
            ProxyStub::UnknownProxy* base = Administrator::Instance().ProxyInstance(
                baseChannel, remoteImpl, /*outbound=*/false, init.InterfaceId(), realIF);
            if (base != nullptr) {
                Core::IUnknown* realIFbase = base->Parent();
                if (realIFbase != nullptr) {
                    Offer(realIFbase, init.InterfaceId());
                }
            }
        }
        reply.PutUInt32(0); // session_id = 0 for Offer/Revoke replies
        reply.PutByteArray(nullptr, 0);
        return 0;
    }

    if (impl == nullptr && init.IsAcquire()) {
        TRACE_L1("BinderCommunicator::HandleAnnounce: Acquire returned null");
        return -1;
    }

    // Generate a session
    const uint32_t sessionId  = _nextSessionId.fetch_add(1, std::memory_order_relaxed);
    const uint32_t channelId  = ++_channelSeq;

    auto channel = Core::ProxyType<BinderIPCChannel>::Create(
        Transport(), sessionId, channelId);

    // Register the implementation with the Administrator so that stubs can find it
    {
        Core::ProxyType<Core::IPCChannel> baseChannel(channel);
        Administrator::Instance().RegisterInterface(baseChannel, impl, init.InterfaceId());
    }

    _sessionsLock.Lock();
    _sessions[sessionId] = channel;
    _sessionsLock.Unlock();

    // Build reply: [session_id][setup blob]
    Data::Setup setup;
    setup.Set(instance_cast<void*>(impl), sessionId, _proxyStubPath, "", "");

    uint8_t setupBuf[Data::IPC_BLOCK_SIZE];
    uint32_t written = 0;
    uint32_t off     = 0;
    while (written < setup.Length()) {
        uint16_t chunk = setup.Serialize(
            setupBuf + written,
            static_cast<uint16_t>(sizeof(setupBuf) - written),
            off);
        if (chunk == 0) break;
        written += chunk;
        off += chunk;
    }

    reply.PutUInt32(sessionId);
    reply.PutByteArray(setupBuf, written);

    TRACE_L1("BinderCommunicator: ANNOUNCE session %u impl=%p", sessionId, impl);
    return 0;
}

// ---------------------------------------------------------------------------
// HandleInvoke
// ---------------------------------------------------------------------------

int BinderCommunicator::HandleInvoke(Core::BinderBuffer& msg,
                                     Core::BinderBuffer& reply)
{
    // Wire format: [uint32 session_id][uint32 paramsLen][params bytes]
    const uint32_t sessionId = msg.GetUInt32();

    Core::ProxyType<BinderIPCChannel> channel = FindSession(sessionId);
    if (!channel.IsValid()) {
        TRACE_L1("BinderCommunicator: unknown session %u", sessionId);
        return -1;
    }

    size_t paramsLen = 0;
    const uint8_t* paramsData = msg.GetByteArray(paramsLen);
    if (paramsData == nullptr || paramsLen == 0) {
        TRACE_L1("BinderCommunicator: empty invoke params");
        return -1;
    }

    // Get an InvokeMessage from the pool and fill its parameters
    Core::ProxyType<InvokeMessage> invokeMsg = Administrator::Instance().Message();
    {
        uint32_t offset = 0;
        while (offset < static_cast<uint32_t>(paramsLen)) {
            uint16_t consumed = invokeMsg->Parameters().Deserialize(
                paramsData + offset,
                static_cast<uint16_t>(paramsLen - offset),
                offset);
            if (consumed == 0) break;
            offset += consumed;
        }
    }

    if (!invokeMsg->Parameters().IsValid()) {
        TRACE_L1("BinderCommunicator: invalid InvokeMessage params");
        return -1;
    }

    // Set the reply buffer on the channel so ReportResponse() can fill it
    channel->SetReplyBuffer(reply);

    // Dispatch: handler calls Job::Invoke → Administrator::Invoke → stub::Handle
    // After returning, Job::Invoke has called channel->ReportResponse(ipcData)
    // which has written the response into `reply`.
    Core::ProxyType<Core::IIPC> ipcData(invokeMsg);
    _handler->Procedure(*channel, ipcData);

    return 0;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

Core::ProxyType<BinderIPCChannel>
BinderCommunicator::FindSession(uint32_t sessionId)
{
    Core::ProxyType<BinderIPCChannel> result;
    _sessionsLock.Lock();
    auto it = _sessions.find(sessionId);
    if (it != _sessions.end()) {
        result = it->second;
    }
    _sessionsLock.Unlock();
    return result;
}

/*static*/ uint32_t BinderCommunicator::SerialiseData(const Core::IMessage& msg,
                                                       uint8_t* buf,
                                                       uint32_t bufSize)
{
    const uint32_t totalLen = msg.Length();
    if (totalLen == 0 || totalLen > bufSize) return 0;

    uint32_t written = 0;
    uint32_t offset  = 0;
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

// ============================================================================
// BinderCommunicatorClient
// ============================================================================

BinderCommunicatorClient::BinderCommunicatorClient(const string& serviceName)
    : _serviceName(serviceName)
    , _transport()
    , _serviceHandle(0)
    , _sessionId(0)
    , _proxyStubPath()
    , _channel()
    , _channelId(0)
{
}

BinderCommunicatorClient::~BinderCommunicatorClient()
{
    Close();
}

uint32_t BinderCommunicatorClient::Open(uint32_t waitTimeMs)
{
    (void)waitTimeMs;

    uint32_t result = _transport.Open();
    if (result != Core::ERROR_NONE) {
        return result;
    }

    // Look up the service handle via binder IPC to the context manager.
    // We always use the binder protocol (not the in-process shortcut from ServiceManager())
    // so that the kernel correctly wires up handle table entries — only then can we route
    // transactions to the server's looper fd.
    result = BinderServiceManagerProxy::LookupHandle(_transport, _serviceName, _serviceHandle);
    if (result != Core::ERROR_NONE) {
        TRACE_L1("BinderCommunicatorClient: service '%s' not found", _serviceName.c_str());
        _transport.Close();
        return result;
    }

    TRACE_L1("BinderCommunicatorClient: opened service '%s' handle=%u",
             _serviceName.c_str(), _serviceHandle);
    return Core::ERROR_NONE;
}

uint32_t BinderCommunicatorClient::Close()
{
    if (_channel.IsValid()) {
        _channel.Release();
    }
    _sessionId = 0;
    return _transport.Close();
}

bool BinderCommunicatorClient::IsOpen() const
{
    return _transport.IsOpen() && _serviceHandle != 0;
}

// ---------------------------------------------------------------------------
// AcquireImpl — send BINDER_COMRPC_ANNOUNCE and get impl pointer + session
// ---------------------------------------------------------------------------

Core::instance_id BinderCommunicatorClient::AcquireImpl(
    const string& className,
    uint32_t      interfaceId,
    uint32_t      versionId,
    uint32_t      waitTimeMs)
{
    (void)waitTimeMs;

    // Build Data::Init announce
    Data::Init init;
    init.Set(Core::ProcessInfo().Id(), className, interfaceId, versionId);

    uint8_t initBuf[Data::IPC_BLOCK_SIZE];
    uint32_t written = 0;
    uint32_t off     = 0;
    while (written < init.Length()) {
        uint16_t chunk = init.Serialize(
            initBuf + written,
            static_cast<uint16_t>(sizeof(initBuf) - written),
            off);
        if (chunk == 0) break;
        written += chunk;
        off += chunk;
    }

    // Binder request: [initLen][init bytes]
    uint8_t msgData[Data::IPC_BLOCK_SIZE + 4];
    Core::BinderBuffer binderMsg(msgData, sizeof(msgData), 0);
    binderMsg.PutByteArray(initBuf, written);

    uint8_t replyData[Data::IPC_BLOCK_SIZE + 8];
    Core::BinderBuffer binderReply(replyData, sizeof(replyData), 0);

    if (_transport.Call(binderMsg, binderReply, _serviceHandle,
                        BINDER_COMRPC_ANNOUNCE) != 0) {
        TRACE_L1("BinderCommunicatorClient: ANNOUNCE call failed");
        return 0;
    }

    // Reply: [session_id uint32][setup bytes len][setup bytes]
    _sessionId = binderReply.GetUInt32();
    size_t setupLen = 0;
    const uint8_t* setupData = binderReply.GetByteArray(setupLen);

    if (setupData == nullptr || setupLen == 0 || _sessionId == 0) {
        TRACE_L1("BinderCommunicatorClient: invalid ANNOUNCE reply");
        return 0;
    }

    Data::Setup setup;
    {
        uint32_t offset = 0;
        while (offset < static_cast<uint32_t>(setupLen)) {
            uint16_t consumed = setup.Deserialize(
                setupData + offset,
                static_cast<uint16_t>(setupLen - offset),
                offset);
            if (consumed == 0) break;
            offset += consumed;
        }
    }

    if (!setup.IsSet()) {
        TRACE_L1("BinderCommunicatorClient: invalid Setup blob");
        return 0;
    }

    _proxyStubPath = setup.ProxyStubPath();

    // Create the client-side BinderIPCChannel
    _channelId = _sessionId; // reuse session as channel id
    _channel = Core::ProxyType<BinderIPCChannel>::Create(
        _transport, _serviceHandle, _sessionId, _channelId);

    TRACE_L1("BinderCommunicatorClient: session %u impl=0x%" PRIxPTR,
             _sessionId, static_cast<uintptr_t>(setup.Implementation()));

    return setup.Implementation();
}

/*static*/ uint32_t BinderCommunicatorClient::SerialiseData(
    const Core::IMessage& msg, uint8_t* buf, uint32_t bufSize)
{
    const uint32_t totalLen = msg.Length();
    if (totalLen == 0 || totalLen > bufSize) return 0;

    uint32_t written = 0;
    uint32_t offset  = 0;
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
