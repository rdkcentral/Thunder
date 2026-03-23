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

#include "BinderTransport.h"
#include "Errors.h"
#include "Trace.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace Thunder {
namespace Core {

// ---------------------------------------------------------------------------
// Internal: read buffer size for the binder loop
// ---------------------------------------------------------------------------
static constexpr size_t ReadBufWords = 32;

// ---------------------------------------------------------------------------
// BinderTransport ctor / dtor
// ---------------------------------------------------------------------------

BinderTransport::BinderTransport()
    : _fd(-1)
    , _mapped(nullptr)
    , _mapSize(0)
    , _deathLock()
    , _deathMap()
{
}

BinderTransport::~BinderTransport()
{
    Close();
}

// ---------------------------------------------------------------------------
// Open / Close
// ---------------------------------------------------------------------------

uint32_t BinderTransport::Open(size_t mapSize)
{
    if (_fd >= 0) {
        return Core::ERROR_INPROGRESS;
    }

    _fd = ::open("/dev/binder", O_RDWR | O_CLOEXEC);
    if (_fd < 0) {
        TRACE_L1("BinderTransport: cannot open /dev/binder (%s)", ::strerror(errno));
        return Core::ERROR_OPENING_FAILED;
    }

    struct binder_version vers {};
    if ((::ioctl(_fd, BINDER_VERSION, &vers) == -1) ||
        (vers.protocol_version != BINDER_CURRENT_PROTOCOL_VERSION)) {
        TRACE_L1("BinderTransport: kernel binder version mismatch (kernel=%d user=%d)",
                 vers.protocol_version, BINDER_CURRENT_PROTOCOL_VERSION);
        ::close(_fd);
        _fd = -1;
        return Core::ERROR_OPENING_FAILED;
    }

    _mapSize = mapSize;
    _mapped  = ::mmap(nullptr, mapSize, PROT_READ, MAP_PRIVATE, _fd, 0);
    if (_mapped == MAP_FAILED) {
        TRACE_L1("BinderTransport: mmap failed (%s)", ::strerror(errno));
        ::close(_fd);
        _fd     = -1;
        _mapped = nullptr;
        return Core::ERROR_OPENING_FAILED;
    }

    return Core::ERROR_NONE;
}

uint32_t BinderTransport::Close()
{
    if (_mapped != nullptr && _mapped != MAP_FAILED) {
        ::munmap(_mapped, _mapSize);
        _mapped = nullptr;
    }
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// BecomeContextManager
// ---------------------------------------------------------------------------

uint32_t BinderTransport::BecomeContextManager()
{
    if (_fd < 0) {
        return Core::ERROR_ILLEGAL_STATE;
    }
    if (::ioctl(_fd, BINDER_SET_CONTEXT_MGR, 0) != 0) {
        TRACE_L1("BinderTransport: BINDER_SET_CONTEXT_MGR failed (%s)", ::strerror(errno));
        return Core::ERROR_ILLEGAL_STATE;
    }
    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Write (internal)
// ---------------------------------------------------------------------------

int BinderTransport::Write(void* data, size_t len)
{
    struct binder_write_read bwr{};
    bwr.write_size     = static_cast<binder_size_t>(len);
    bwr.write_consumed = 0;
    bwr.write_buffer   = reinterpret_cast<uintptr_t>(data);
    bwr.read_size      = 0;
    bwr.read_consumed  = 0;
    bwr.read_buffer    = 0;

    int res = ::ioctl(_fd, BINDER_WRITE_READ, &bwr);
    if (res < 0) {
        TRACE_L1("BinderTransport::Write: ioctl failed (%s)", ::strerror(errno));
    }
    return res;
}

// ---------------------------------------------------------------------------
// FreeBuffer
// ---------------------------------------------------------------------------

void BinderTransport::FreeBuffer(binder_uintptr_t bufferPtr)
{
    struct {
        uint32_t           cmd;
        binder_uintptr_t   buffer;
    } __attribute__((packed)) data;

    data.cmd    = BC_FREE_BUFFER;
    data.buffer = bufferPtr;
    Write(&data, sizeof(data));
}

// ---------------------------------------------------------------------------
// SendReply (internal)
// ---------------------------------------------------------------------------

void BinderTransport::SendReply(BinderBuffer& reply,
                                binder_uintptr_t bufferToFree,
                                int status)
{
    struct {
        uint32_t                  cmd_free;
        binder_uintptr_t          buffer;
        uint32_t                  cmd_reply;
        struct binder_transaction_data txn;
    } __attribute__((packed)) data;

    data.cmd_free = BC_FREE_BUFFER;
    data.buffer   = bufferToFree;
    data.cmd_reply = BC_REPLY;
    data.txn.target.ptr    = 0;
    data.txn.cookie        = 0;
    data.txn.code          = 0;

    if (status != 0) {
        data.txn.flags        = TF_STATUS_CODE;
        data.txn.data_size    = sizeof(int);
        data.txn.offsets_size = 0;
        data.txn.data.ptr.buffer  = reinterpret_cast<uintptr_t>(&status);
        data.txn.data.ptr.offsets = 0;
    } else {
        data.txn.flags        = 0;
        data.txn.data_size    = static_cast<binder_size_t>(reply.DataUsed());
        data.txn.offsets_size = static_cast<binder_size_t>(reply.OffsetUsed());
        data.txn.data.ptr.buffer  = reinterpret_cast<uintptr_t>(reply.Data0());
        data.txn.data.ptr.offsets = reinterpret_cast<uintptr_t>(reply.Offs0());
    }

    Write(&data, sizeof(data));
}

// ---------------------------------------------------------------------------
// Acquire / Release / LinkToDeath
// ---------------------------------------------------------------------------

void BinderTransport::Acquire(uint32_t handle)
{
    uint32_t cmd[2] = { BC_ACQUIRE, handle };
    Write(cmd, sizeof(cmd));
}

void BinderTransport::Release(uint32_t handle)
{
    uint32_t cmd[2] = { BC_RELEASE, handle };
    Write(cmd, sizeof(cmd));
}

void BinderTransport::LinkToDeath(uint32_t handle, IDeathObserver& observer)
{
    _deathLock.Lock();
    DeathEntry& entry = _deathMap[handle];
    entry.observer         = &observer;
    _deathLock.Unlock();

    struct {
        uint32_t                    cmd;
        struct binder_handle_cookie payload;
    } __attribute__((packed)) data;

    data.cmd            = BC_REQUEST_DEATH_NOTIFICATION;
    data.payload.handle = handle;
    // Use the handle itself as the cookie so we can recover it in BR_DEAD_BINDER
    data.payload.cookie = static_cast<binder_uintptr_t>(handle);
    Write(&data, sizeof(data));
}

// ---------------------------------------------------------------------------
// Parse
// ---------------------------------------------------------------------------

int BinderTransport::Parse(BinderBuffer* replyBio,
                           uintptr_t ptr,
                           size_t size,
                           Handler& handler)
{
    int result = 1; // >0 means keep looping
    const uintptr_t end = ptr + static_cast<uintptr_t>(size);

    while (ptr < end) {
        const uint32_t cmd = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += sizeof(uint32_t);

        switch (cmd) {
        case BR_NOOP:
            break;

        case BR_TRANSACTION_COMPLETE:
            break;

        case BR_INCREFS:
        case BR_ACQUIRE:
        case BR_RELEASE:
        case BR_DECREFS:
            ptr += sizeof(struct binder_ptr_cookie);
            break;

        case BR_TRANSACTION: {
            if ((end - ptr) < sizeof(struct binder_transaction_data)) {
                TRACE_L1("BinderTransport::Parse: BR_TRANSACTION too small");
                return -1;
            }
            struct binder_transaction_data* txn =
                reinterpret_cast<struct binder_transaction_data*>(ptr);
            ptr += sizeof(struct binder_transaction_data);

            // Build a read-buffer on the shared kernel buffer
            BinderBuffer msgBuf(txn);

            // Build a stack write-buffer for the reply
            uint8_t replyData[256];
            BinderBuffer replyBuf(replyData, sizeof(replyData), 4);

            int status = 0;
            if (handler) {
                status = handler(txn, msgBuf, replyBuf);
            }
            SendReply(replyBuf, txn->data.ptr.buffer, status);
            break;
        }

        case BR_REPLY: {
            if ((end - ptr) < sizeof(struct binder_transaction_data)) {
                TRACE_L1("BinderTransport::Parse: BR_REPLY too small");
                return -1;
            }
            struct binder_transaction_data* txn =
                reinterpret_cast<struct binder_transaction_data*>(ptr);
            ptr += sizeof(struct binder_transaction_data);

            if (replyBio != nullptr) {
                // Populate the caller's reply buffer from the shared kernel buffer
                replyBio->~BinderBuffer();
                new (replyBio) BinderBuffer(txn);
                replyBio = nullptr; // only fill once
            }
            result = 0; // outbound call complete
            break;
        }

        case BR_DEAD_BINDER: {
            // The cookie we registered in LinkToDeath is the handle itself
            const binder_uintptr_t cookie = *reinterpret_cast<const binder_uintptr_t*>(ptr);
            ptr += sizeof(binder_uintptr_t);

            uint32_t handle = static_cast<uint32_t>(cookie);
            _deathLock.Lock();
            auto it = _deathMap.find(handle);
            if (it != _deathMap.end() && it->second.observer != nullptr) {
                it->second.observer->ServiceDied(handle);
                it->second.observer = nullptr;
                _deathMap.erase(it);
            }
            _deathLock.Unlock();
            break;
        }

        case BR_FAILED_REPLY:
        case BR_DEAD_REPLY:
            TRACE_L1("BinderTransport::Parse: transaction failed (cmd=0x%x)", cmd);
            result = -1;
            break;

        default:
            TRACE_L1("BinderTransport::Parse: unhandled cmd 0x%x", cmd);
            return -1;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Call (blocking outbound transaction)
// ---------------------------------------------------------------------------

int BinderTransport::Call(BinderBuffer& msg, BinderBuffer& reply,
                          uint32_t targetHandle, uint32_t code)
{
    if (_fd < 0) {
        return -1;
    }

    if (msg.HasOverflow()) {
        TRACE_L1("BinderTransport::Call: message buffer overflow");
        return -1;
    }

    struct {
        uint32_t                  cmd;
        struct binder_transaction_data txn;
    } __attribute__((packed)) writebuf;

    writebuf.cmd                     = BC_TRANSACTION;
    writebuf.txn.target.handle       = targetHandle;
    writebuf.txn.code                = code;
    writebuf.txn.flags               = 0;
    writebuf.txn.data_size           = static_cast<binder_size_t>(msg.DataUsed());
    writebuf.txn.offsets_size        = static_cast<binder_size_t>(msg.OffsetUsed());
    writebuf.txn.data.ptr.buffer     = reinterpret_cast<uintptr_t>(msg.Data0());
    writebuf.txn.data.ptr.offsets    = reinterpret_cast<uintptr_t>(msg.Offs0());

    struct binder_write_read bwr{};
    bwr.write_size   = sizeof(writebuf);
    bwr.write_buffer = reinterpret_cast<uintptr_t>(&writebuf);

    uint32_t readbuf[ReadBufWords];

    for (;;) {
        bwr.read_size     = sizeof(readbuf);
        bwr.read_consumed = 0;
        bwr.read_buffer   = reinterpret_cast<uintptr_t>(readbuf);

        int res = ::ioctl(_fd, BINDER_WRITE_READ, &bwr);
        if (res < 0) {
            TRACE_L1("BinderTransport::Call: ioctl failed (%s)", ::strerror(errno));
            return -1;
        }

        // After the first write the driver consumes it; zero out for next iter
        bwr.write_size   = 0;
        bwr.write_buffer = 0;

        Handler noHandler; // null — we are the caller, not a server
        res = Parse(&reply, reinterpret_cast<uintptr_t>(readbuf),
                    static_cast<size_t>(bwr.read_consumed), noHandler);
        if (res == 0) {
            return 0;
        }
        if (res < 0) {
            return -1;
        }
    }
}

// ---------------------------------------------------------------------------
// Loop (blocking event loop for server threads)
// ---------------------------------------------------------------------------

void BinderTransport::Loop(Handler handler)
{
    if (_fd < 0) {
        return;
    }

    uint32_t enterCmd = BC_ENTER_LOOPER;
    Write(&enterCmd, sizeof(enterCmd));

    struct binder_write_read bwr{};
    uint32_t readbuf[ReadBufWords];

    for (;;) {
        bwr.write_size     = 0;
        bwr.write_consumed = 0;
        bwr.write_buffer   = 0;
        bwr.read_size      = sizeof(readbuf);
        bwr.read_consumed  = 0;
        bwr.read_buffer    = reinterpret_cast<uintptr_t>(readbuf);

        int res = ::ioctl(_fd, BINDER_WRITE_READ, &bwr);
        if (res < 0) {
            if (errno != EBADF) {
                TRACE_L1("BinderTransport::Loop: ioctl failed (%s)", ::strerror(errno));
            }
            break; // fd closed or error — exit cleanly
        }

        res = Parse(nullptr, reinterpret_cast<uintptr_t>(readbuf),
                    static_cast<size_t>(bwr.read_consumed), handler);
        if (res == 0) {
            TRACE_L1("BinderTransport::Loop: unexpected reply?");
            break;
        }
        if (res < 0) {
            TRACE_L1("BinderTransport::Loop: io error %d", res);
            break;
        }
    }
}

} // namespace Core
} // namespace Thunder
