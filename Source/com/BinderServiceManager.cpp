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

#include "BinderServiceManager.h"
#include "../core/Errors.h"
#include "../core/Trace.h"

#include <linux/android/binder.h>
#include <cstring>

namespace Thunder {
namespace RPC {

// ---------------------------------------------------------------------------
// Binder SVC_MGR protocol codes (from binder.h reference)
// ---------------------------------------------------------------------------
static constexpr uint32_t SVC_MGR_GET_SERVICE   = 1;
static constexpr uint32_t SVC_MGR_CHECK_SERVICE  = 2;
static constexpr uint32_t SVC_MGR_ADD_SERVICE    = 3;
static constexpr uint32_t SVC_MGR_LIST_SERVICES  = 4;

// Interface name as UTF-16LE array (matches svcmgr_id in reference svcmgr)
// "Thunder.IServiceManager"
static const uint16_t k_SvcMgrId[] = {
    'T','h','u','n','d','e','r','.',
    'I','S','e','r','v','i','c','e','M','a','n','a','g','e','r'
};
static constexpr size_t k_SvcMgrIdLen =
    sizeof(k_SvcMgrId) / sizeof(k_SvcMgrId[0]);

// Handle 0 is always the context manager (service manager) on Android binder
static constexpr uint32_t BINDER_SERVICE_MANAGER = 0;

// ============================================================================
// BinderServiceManager (server)
// ============================================================================

BinderServiceManager& BinderServiceManager::Instance()
{
    static BinderServiceManager sInstance;
    return sInstance;
}

BinderServiceManager::BinderServiceManager()
    : BinderEndpoint()
    , _lock()
    , _services()
{
}

BinderServiceManager::~BinderServiceManager()
{
    Stop();
}

uint32_t BinderServiceManager::Start(uint32_t numThreads)
{
    return BinderEndpoint::Open(numThreads, /*contextManager=*/true);
}

uint32_t BinderServiceManager::Stop()
{
    return BinderEndpoint::Close();
}

bool BinderServiceManager::IsRunning() const
{
    return BinderEndpoint::IsOpen();
}

// ---------------------------------------------------------------------------
// IServiceManager
// ---------------------------------------------------------------------------

uint32_t BinderServiceManager::AddService(const string& name,
                                          uint32_t handle,
                                          bool allowIsolated)
{
    if (name.empty() || handle == 0) {
        return Core::ERROR_GENERAL;
    }
    _lock.Lock();
    _services[name] = { handle, allowIsolated };
    _lock.Unlock();

    // Track lifetime so we can remove it on death
    Transport().LinkToDeath(handle, *this);
    Transport().Acquire(handle);

    TRACE_L1("BinderServiceManager::AddService '%s' handle=%u", name.c_str(), handle);
    return Core::ERROR_NONE;
}

uint32_t BinderServiceManager::GetService(const string& name, uint32_t& handle)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;
    _lock.Lock();
    auto it = _services.find(name);
    if (it != _services.end() && it->second.handle != 0) {
        handle = it->second.handle;
        result = Core::ERROR_NONE;
    }
    _lock.Unlock();
    return result;
}

uint32_t BinderServiceManager::ListService(uint32_t index, string& name)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;
    _lock.Lock();
    if (index < static_cast<uint32_t>(_services.size())) {
        auto it = _services.begin();
        std::advance(it, index);
        name   = it->first;
        result = Core::ERROR_NONE;
    }
    _lock.Unlock();
    return result;
}

// ---------------------------------------------------------------------------
// IDeathObserver
// ---------------------------------------------------------------------------

void BinderServiceManager::ServiceDied(uint32_t handle)
{
    _lock.Lock();
    for (auto it = _services.begin(); it != _services.end(); ) {
        if (it->second.handle == handle) {
            TRACE_L1("BinderServiceManager: service '%s' (handle %u) died",
                     it->first.c_str(), handle);
            it = _services.erase(it);
        } else {
            ++it;
        }
    }
    Transport().Release(handle);
    _lock.Unlock();
}

// ---------------------------------------------------------------------------
// HandleTransaction
// ---------------------------------------------------------------------------

int BinderServiceManager::HandleTransaction(binder_transaction_data* txn,
                                            Core::BinderBuffer& msg,
                                            Core::BinderBuffer& reply)
{
    if (txn->target.ptr != BINDER_SERVICE_MANAGER) {
        return -1;
    }

    // ---- Decode protocol header: strict_policy (uint32) + interface name (UTF-16) ----
    uint32_t strict_policy = msg.GetUInt32(); (void)strict_policy;

    size_t ifNameLen = 0;
    const uint16_t* ifName = msg.GetString16(ifNameLen);

    if (ifName == nullptr || ifNameLen != k_SvcMgrIdLen ||
        ::memcmp(ifName, k_SvcMgrId, k_SvcMgrIdLen * sizeof(uint16_t)) != 0) {
        TRACE_L1("BinderServiceManager: invalid interface name");
        return -1;
    }

    // ---- Dispatch ----
    switch (txn->code) {

    case SVC_MGR_GET_SERVICE:
    case SVC_MGR_CHECK_SERVICE: {
        size_t nameLen = 0;
        const uint16_t* s16 = msg.GetString16(nameLen);
        if (s16 == nullptr) {
            return -1;
        }
        // Convert UTF-16LE to UTF-8 for the map key
        string nameUtf8;
        nameUtf8.reserve(nameLen);
        for (size_t i = 0; i < nameLen; ++i) {
            nameUtf8.push_back(static_cast<char>(s16[i] & 0xFF));
        }
        uint32_t handle = 0;
        if (GetService(nameUtf8, handle) != Core::ERROR_NONE || handle == 0) {
            // Not found — fall through to put_uint32(0)
            break;
        }
        reply.PutRef(handle);
        return 0;
    }

    case SVC_MGR_ADD_SERVICE: {
        size_t nameLen = 0;
        const uint16_t* s16 = msg.GetString16(nameLen);
        if (s16 == nullptr || nameLen == 0 || nameLen > 127) {
            return -1;
        }
        string nameUtf8;
        nameUtf8.reserve(nameLen);
        for (size_t i = 0; i < nameLen; ++i) {
            nameUtf8.push_back(static_cast<char>(s16[i] & 0xFF));
        }
        const uint32_t handle        = msg.GetRef();
        const bool     allowIsolated = (msg.GetUInt32() != 0);

        if (AddService(nameUtf8, handle, allowIsolated) != Core::ERROR_NONE) {
            return -1;
        }
        break; // write uint32(0) reply
    }

    case SVC_MGR_LIST_SERVICES: {
        const uint32_t idx = msg.GetUInt32();
        string name;
        if (ListService(idx, name) != Core::ERROR_NONE) {
            return -1;
        }
        // Reply with UTF-16LE string
        reply.PutString16(name.c_str());
        return 0;
    }

    default:
        TRACE_L1("BinderServiceManager: unknown code %u", txn->code);
        return -1;
    }

    // Generic success reply
    reply.PutUInt32(0);
    return 0;
}

// ============================================================================
// BinderServiceManagerProxy (client)
// ============================================================================

// ---------------------------------------------------------------------------
// ServiceManager() factory
// ---------------------------------------------------------------------------

IServiceManager& ServiceManager()
{
    if (BinderServiceManager::Instance().IsRunning()) {
        return BinderServiceManager::Instance();
    }
    static BinderServiceManagerProxy sProxy;
    // Lazily open; ignore errors here — individual calls will fail gracefully
    if (!sProxy.IsOpen()) {
        sProxy.Open();
    }
    return sProxy;
}

// ---------------------------------------------------------------------------
// Proxy ctor / dtor
// ---------------------------------------------------------------------------

BinderServiceManagerProxy::BinderServiceManagerProxy()
    : _transport()
{
}

BinderServiceManagerProxy::~BinderServiceManagerProxy()
{
    _transport.Close();
}

uint32_t BinderServiceManagerProxy::Open(size_t mapSize)
{
    return _transport.Open(mapSize);
}

uint32_t BinderServiceManagerProxy::Close()
{
    return _transport.Close();
}

bool BinderServiceManagerProxy::IsOpen() const
{
    return _transport.IsOpen();
}

// ---------------------------------------------------------------------------
// WriteHeader — strict_policy(0) + "Thunder.IServiceManager" in UTF-16
// ---------------------------------------------------------------------------

void BinderServiceManagerProxy::WriteHeader(Core::BinderBuffer& msg)
{
    msg.PutUInt32(0); // strict_policy = 0
    msg.PutString16(string("Thunder.IServiceManager").c_str());
}

// ---------------------------------------------------------------------------
// RegisterNode — send addService with BINDER_TYPE_BINDER using caller's fd
// ---------------------------------------------------------------------------

/*static*/
uint32_t BinderServiceManagerProxy::RegisterNode(Core::BinderTransport& tx,
                                                  const string& name,
                                                  uintptr_t binderPtr,
                                                  bool allowIsolated)
{
    if (!tx.IsOpen()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    uint8_t dataBuf[512];
    Core::BinderBuffer msg(dataBuf, sizeof(dataBuf), 4);
    msg.PutUInt32(0); // strict_policy
    msg.PutString16("Thunder.IServiceManager");
    msg.PutString16(name.c_str());
    msg.PutObject(reinterpret_cast<void*>(binderPtr)); // BINDER_TYPE_BINDER
    msg.PutUInt32(allowIsolated ? 1 : 0);

    uint8_t replyBuf[64];
    Core::BinderBuffer reply(replyBuf, sizeof(replyBuf), 0);

    if (tx.Call(msg, reply, BINDER_SERVICE_MANAGER, SVC_MGR_ADD_SERVICE) != 0) {
        return Core::ERROR_GENERAL;
    }
    const uint32_t status = reply.GetUInt32();
    return (status == 0) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
}

// ---------------------------------------------------------------------------
// LookupHandle — send getService via caller's fd to the context manager
// ---------------------------------------------------------------------------

/*static*/
uint32_t BinderServiceManagerProxy::LookupHandle(Core::BinderTransport& tx,
                                                  const string& name,
                                                  uint32_t& handle)
{
    if (!tx.IsOpen()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    uint8_t dataBuf[512];
    Core::BinderBuffer msg(dataBuf, sizeof(dataBuf), 0);
    msg.PutUInt32(0); // strict_policy
    msg.PutString16("Thunder.IServiceManager");
    msg.PutString16(name.c_str());

    uint8_t replyBuf[64];
    Core::BinderBuffer reply(replyBuf, sizeof(replyBuf), 1);

    if (tx.Call(msg, reply, BINDER_SERVICE_MANAGER, SVC_MGR_GET_SERVICE) != 0) {
        return Core::ERROR_GENERAL;
    }
    handle = reply.GetRef();
    return (handle != 0) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE;
}

// ---------------------------------------------------------------------------
// Proxy: IServiceManager
// ---------------------------------------------------------------------------

uint32_t BinderServiceManagerProxy::AddService(const string& name,
                                               uint32_t handle,
                                               bool allowIsolated)
{
    if (!_transport.IsOpen()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    uint8_t dataBuf[512];
    Core::BinderBuffer msg(dataBuf, sizeof(dataBuf), 4);
    WriteHeader(msg);
    msg.PutString16(name.c_str());
    msg.PutRef(handle);
    msg.PutUInt32(allowIsolated ? 1 : 0);

    uint8_t replyBuf[64];
    Core::BinderBuffer reply(replyBuf, sizeof(replyBuf), 0);

    if (_transport.Call(msg, reply, BINDER_SERVICE_MANAGER, SVC_MGR_ADD_SERVICE) != 0) {
        return Core::ERROR_GENERAL;
    }
    const uint32_t status = reply.GetUInt32();
    return (status == 0) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
}

uint32_t BinderServiceManagerProxy::GetService(const string& name, uint32_t& handle)
{
    if (!_transport.IsOpen()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    uint8_t dataBuf[512];
    Core::BinderBuffer msg(dataBuf, sizeof(dataBuf), 1);
    WriteHeader(msg);
    msg.PutString16(name.c_str());

    uint8_t replyBuf[64];
    Core::BinderBuffer reply(replyBuf, sizeof(replyBuf), 1);

    if (_transport.Call(msg, reply, BINDER_SERVICE_MANAGER, SVC_MGR_GET_SERVICE) != 0) {
        return Core::ERROR_GENERAL;
    }

    handle = reply.GetRef();
    return (handle != 0) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE;
}

uint32_t BinderServiceManagerProxy::ListService(uint32_t index, string& name)
{
    if (!_transport.IsOpen()) {
        return Core::ERROR_ILLEGAL_STATE;
    }

    uint8_t dataBuf[512];
    Core::BinderBuffer msg(dataBuf, sizeof(dataBuf), 0);
    WriteHeader(msg);
    msg.PutUInt32(index);

    uint8_t replyBuf[256];
    Core::BinderBuffer reply(replyBuf, sizeof(replyBuf), 0);

    if (_transport.Call(msg, reply, BINDER_SERVICE_MANAGER, SVC_MGR_LIST_SERVICES) != 0) {
        return Core::ERROR_UNAVAILABLE;
    }

    size_t nameLen = 0;
    const uint16_t* s16 = reply.GetString16(nameLen);
    if (s16 == nullptr || nameLen == 0) {
        return Core::ERROR_UNAVAILABLE;
    }

    name.clear();
    name.reserve(nameLen);
    for (size_t i = 0; i < nameLen; ++i) {
        name.push_back(static_cast<char>(s16[i] & 0xFF));
    }
    return Core::ERROR_NONE;
}

} // namespace RPC
} // namespace Thunder
