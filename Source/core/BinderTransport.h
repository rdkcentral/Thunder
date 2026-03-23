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
#include "BinderBuffer.h"
#include "Sync.h"

#include <functional>
#include <unordered_map>
#include <linux/android/binder.h>

namespace Thunder {
namespace Core {

    /**
     * @brief Low-level C++ abstraction over the Linux Android Binder kernel driver.
     *
     * Analogous to SocketPort in the Thunder transport stack.
     * Manages a single /dev/binder file descriptor, the mmap'd buffer window, and
     * the ioctl(BINDER_WRITE_READ) read/write loop.
     *
     * Reference: Binder/binder_io/binder.c in the ThunderBinder workspace.
     */
    class EXTERNAL BinderTransport {
    public:
        static constexpr size_t DefaultMapSize = 128u * 1024u;

        /**
         * C++ death notification observer. Implement this to receive
         * service-died callbacks instead of the raw binder_death C struct.
         */
        struct IDeathObserver {
            virtual ~IDeathObserver() = default;
            virtual void ServiceDied(uint32_t handle) = 0;
        };

        /**
         * Transaction handler type. Called from Loop() for every
         * incoming BR_TRANSACTION.
         *
         * @return 0 on success, non-zero to reply with a status code.
         */
        using Handler = std::function<int(
            struct binder_transaction_data*  txn,
            BinderBuffer&                    msg,
            BinderBuffer&                    reply)>;

    public:
        BinderTransport();
        ~BinderTransport();

        BinderTransport(const BinderTransport&) = delete;
        BinderTransport& operator=(const BinderTransport&) = delete;

        /**
         * Open /dev/binder, check protocol version and mmap the buffer window.
         * @return Core::ERROR_NONE on success, Core::ERROR_OPENING_FAILED otherwise.
         */
        uint32_t Open(size_t mapSize = DefaultMapSize);

        /**
         * Close the fd and unmap the buffer. Any thread blocked in Loop() will
         * return naturally when the fd is closed (ioctl returns EBADF).
         */
        uint32_t Close();

        bool IsOpen() const { return (_fd >= 0); }

        /**
         * Claim handle 0 as binder context manager (service manager).
         * Must be called after Open() and before Loop().
         * @return Core::ERROR_NONE on success, Core::ERROR_ILLEGAL_STATE if
         *         another process/fd already holds the context manager role.
         */
        uint32_t BecomeContextManager();

        /**
         * Blocking outbound binder call (BC_TRANSACTION), waits for BR_REPLY.
         * Thread-safe: internally serialised via a per-call CriticalSection.
         *
         * @param msg          Pre-filled outbound BinderBuffer.
         * @param reply        Empty BinderBuffer to receive the reply.
         * @param targetHandle Remote service handle.
         * @param code         Transaction code.
         * @return 0 on success, -1 on failure.
         */
        int Call(BinderBuffer& msg, BinderBuffer& reply,
                 uint32_t targetHandle, uint32_t code);

        /**
         * Enter the binder event loop (blocking). Sends BC_ENTER_LOOPER then
         * continuously calls ioctl(BINDER_WRITE_READ) and dispatches BR_TRANSACTION
         * to handler. Returns when the fd is closed or an I/O error occurs.
         */
        void Loop(Handler handler);

        /**
         * Increment refcount on a remote handle (BC_ACQUIRE).
         */
        void Acquire(uint32_t handle);

        /**
         * Decrement refcount on a remote handle (BC_RELEASE).
         */
        void Release(uint32_t handle);

        /**
         * Register a death notification for handle. The observer's ServiceDied()
         * is called (from the Loop() thread) when the remote process dies.
         * The handle must have been Acquire()'d first.
         */
        void LinkToDeath(uint32_t handle, IDeathObserver& observer);

        /**
         * Free a kernel buffer previously mapped for an incoming transaction.
         * Must be called after processing a BR_TRANSACTION whose bio has
         * BIO_F_SHARED set (i.e. after BinderBuffer constructed from txn).
         */
        void FreeBuffer(binder_uintptr_t bufferPtr);

        int Fd() const { return _fd; }

    private:
        // Raw write of a pre-built command buffer to the driver.
        int Write(void* data, size_t len);

        // Parse a read buffer from BINDER_WRITE_READ; dispatch transactions.
        // @param replyBio  If non-null, filled when BR_REPLY arrives (used by Call()).
        int Parse(BinderBuffer* replyBio, uintptr_t ptr, size_t size,
                  Handler& handler);

        // Build and send BC_REPLY (or BC_REPLY with error status) after a handler returns.
        void SendReply(BinderBuffer& reply, binder_uintptr_t bufferToFree, int status);

        int     _fd     { -1 };
        void*   _mapped { nullptr };
        size_t  _mapSize { 0 };

        // Death observers: handle → observer pointer
        struct DeathEntry {
            IDeathObserver* observer { nullptr };
        };
        CriticalSection _deathLock;
        std::unordered_map<uint32_t, DeathEntry> _deathMap;
    };

} // namespace Core
} // namespace Thunder
