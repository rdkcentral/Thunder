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

#include <cstring>
#include <cstdint>
#include <linux/android/binder.h>

namespace Thunder {
namespace Core {

    /**
     * @brief C++ RAII wrapper around the kernel binder_io buffer.
     *
     * Replaces the bio_* C functions from the AOSP binder_io reference.
     * Callers supply a stack or heap backing buffer; BinderBuffer manages
     * the read/write cursor and object-offset table within it.
     *
     * Max 1 GB of data or 0x7FFFFFFF (MAX_BIO_SIZE — same limit as AOSP).
     */
    class EXTERNAL BinderBuffer {
    public:
        static constexpr uint32_t MaxBioSize = (1u << 30);

        BinderBuffer(const BinderBuffer&) = delete;
        BinderBuffer& operator=(const BinderBuffer&) = delete;

        /**
         * Initialise a write buffer from a caller-supplied backing store.
         * @param backingBuf  Raw memory block. First (maxObjects * sizeof(binder_size_t))
         *                    bytes are reserved for the offset table;
         *                    the rest is the data region.
         * @param maxData     Total size of backingBuf in bytes.
         * @param maxObjects  Maximum number of binder objects embedded in this buffer.
         */
        BinderBuffer(void* backingBuf, size_t maxData, size_t maxObjects)
        {
            size_t offsetTableSize = maxObjects * sizeof(binder_size_t);
            if (offsetTableSize > maxData) {
                _flags      = BIO_F_OVERFLOW;
                _data_avail = 0;
                _offs_avail = 0;
                _data       = _data0 = nullptr;
                _offs       = _offs0 = nullptr;
                return;
            }
            _data  = _data0 = static_cast<char*>(backingBuf) + offsetTableSize;
            _offs  = _offs0 = static_cast<binder_size_t*>(backingBuf);
            _data_avail = maxData - offsetTableSize;
            _offs_avail = maxObjects;
            _flags      = 0;
        }

        /**
         * Initialise a read buffer from a received binder_transaction_data.
         * The buffer is shared (BIO_F_SHARED) — it must be freed with
         * BinderTransport::FreeBuffer() after use.
         */
        explicit BinderBuffer(const struct binder_transaction_data* txn)
        {
            _data  = _data0 = reinterpret_cast<char*>(static_cast<uintptr_t>(txn->data.ptr.buffer));
            _offs  = _offs0 = reinterpret_cast<binder_size_t*>(static_cast<uintptr_t>(txn->data.ptr.offsets));
            _data_avail = txn->data_size;
            _offs_avail = txn->offsets_size / sizeof(binder_size_t);
            _flags      = BIO_F_SHARED;
        }

        ~BinderBuffer() = default;

        // ----------------------------------------------------------------
        // Write helpers
        // ----------------------------------------------------------------

        void PutUInt32(uint32_t value)
        {
            uint32_t* ptr = static_cast<uint32_t*>(Alloc(sizeof(uint32_t)));
            if (ptr != nullptr) {
                *ptr = value;
            }
        }

        /**
         * Write a byte array prefixed by a uint32 length.
         * Writes 0xFFFFFFFF if data is null or length exceeds MaxBioSize.
         */
        void PutByteArray(const uint8_t* data, uint32_t length)
        {
            if ((data == nullptr) || (length >= MaxBioSize)) {
                PutUInt32(0xFFFFFFFFu);
                return;
            }
            PutUInt32(length);
            uint8_t* dst = static_cast<uint8_t*>(Alloc(length));
            if (dst != nullptr) {
                ::memcpy(dst, data, length);
            }
        }

        /**
         * Write a UTF-8 string as a UTF-16LE string16 (compatible with
         * the svcmgr wire format). Prefixed by uint32 character count
         * (not byte count), NUL-terminated.
         */
        void PutString16(const char* utf8Str)
        {
            if (utf8Str == nullptr) {
                PutUInt32(0xFFFFFFFFu);
                return;
            }
            size_t len = ::strlen(utf8Str);
            if (len >= (MaxBioSize / sizeof(uint16_t))) {
                PutUInt32(0xFFFFFFFFu);
                return;
            }
            PutUInt32(static_cast<uint32_t>(len));
            uint16_t* dst = static_cast<uint16_t*>(Alloc((len + 1) * sizeof(uint16_t)));
            if (dst != nullptr) {
                const unsigned char* src = reinterpret_cast<const unsigned char*>(utf8Str);
                for (size_t i = 0; i < len; ++i) {
                    dst[i] = static_cast<uint16_t>(src[i]);
                }
                dst[len] = 0;
            }
        }

        /**
         * Write a local binder object (BINDER_TYPE_BINDER).
         * Used by a server to include its own object reference.
         */
        void PutObject(void* ptr)
        {
            struct flat_binder_object* obj = AllocObject();
            if (obj != nullptr) {
                obj->flags      = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
                obj->hdr.type   = BINDER_TYPE_BINDER;
                obj->binder     = reinterpret_cast<uintptr_t>(ptr);
                obj->cookie     = 0;
            }
        }

        /**
         * Write a remote handle reference (BINDER_TYPE_HANDLE).
         */
        void PutRef(uint32_t handle)
        {
            struct flat_binder_object* obj;
            if (handle != 0) {
                obj = AllocObject();
            } else {
                obj = static_cast<struct flat_binder_object*>(Alloc(sizeof(*obj)));
            }
            if (obj != nullptr) {
                obj->flags      = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
                obj->hdr.type   = BINDER_TYPE_HANDLE;
                obj->handle     = handle;
                obj->cookie     = 0;
            }
        }

        // ----------------------------------------------------------------
        // Read helpers
        // ----------------------------------------------------------------

        uint32_t GetUInt32()
        {
            const uint32_t* ptr = static_cast<uint32_t*>(Get(sizeof(uint32_t)));
            return (ptr != nullptr) ? *ptr : 0;
        }

        /**
         * Read a byte array written by PutByteArray().
         * @param outSize Set to the number of bytes in the returned pointer.
         * @return Pointer into the shared buffer (valid until FreeBuffer is called),
         *         or nullptr on error.
         */
        uint8_t* GetByteArray(size_t& outSize)
        {
            size_t len = static_cast<size_t>(GetUInt32());
            if (len == static_cast<size_t>(0xFFFFFFFFu)) {
                outSize = 0;
                return nullptr;
            }
            outSize = len;
            return static_cast<uint8_t*>(Get(len));
        }

        /**
         * Read a UTF-16LE string16 written by PutString16().
         * @param outLen Set to the character count (not bytes).
         * @return Pointer into buffer, or nullptr on error.
         */
        uint16_t* GetString16(size_t& outLen)
        {
            size_t len = static_cast<size_t>(GetUInt32());
            if (len == static_cast<size_t>(0xFFFFFFFFu)) {
                outLen = 0;
                return nullptr;
            }
            outLen = len;
            return static_cast<uint16_t*>(Get((len + 1) * sizeof(uint16_t)));
        }

        /**
         * Read a binder handle (BINDER_TYPE_HANDLE) from the offset table.
         * @return handle value, or 0 if not found / wrong type.
         */
        uint32_t GetRef()
        {
            const struct flat_binder_object* obj = GetObject();
            if (obj == nullptr) {
                return 0;
            }
            return (obj->hdr.type == BINDER_TYPE_HANDLE) ? obj->handle : 0;
        }

        // ----------------------------------------------------------------
        // Status / metrics
        // ----------------------------------------------------------------

        bool HasOverflow() const  { return (_flags & BIO_F_OVERFLOW) != 0; }
        bool IsShared() const     { return (_flags & BIO_F_SHARED)   != 0; }

        /** Total data bytes consumed so far (from start of data region). */
        size_t DataUsed()    const { return (_data0 != nullptr) ? static_cast<size_t>(_data - _data0) : 0; }
        /** Total offset bytes consumed so far. */
        size_t OffsetUsed()  const { return (_offs0 != nullptr) ? static_cast<size_t>(
                reinterpret_cast<const char*>(_offs) -
                reinterpret_cast<const char*>(_offs0)) : 0; }

        /** Start of data region — used by BinderTransport to build binder_transaction_data. */
        const char*          Data0() const { return _data0; }
        /** Start of offset table — used by BinderTransport to build binder_transaction_data. */
        const binder_size_t* Offs0() const { return _offs0; }

    private:
        static constexpr uint32_t BIO_F_SHARED   = 0x01u;
        static constexpr uint32_t BIO_F_OVERFLOW  = 0x02u;
        static constexpr uint32_t BIO_F_MALLOCED  = 0x08u;

        void* Alloc(size_t size)
        {
            // Round up to 4-byte alignment
            size = (size + 3u) & ~3u;
            if (size > _data_avail) {
                _flags |= BIO_F_OVERFLOW;
                return nullptr;
            }
            void* ptr = _data;
            _data       += size;
            _data_avail -= size;
            return ptr;
        }

        void* Get(size_t size)
        {
            size = (size + 3u) & ~3u;
            if (_data_avail < size) {
                _data_avail = 0;
                _flags |= BIO_F_OVERFLOW;
                return nullptr;
            }
            void* ptr = _data;
            _data       += size;
            _data_avail -= size;
            return ptr;
        }

        struct flat_binder_object* AllocObject()
        {
            struct flat_binder_object* obj =
                static_cast<struct flat_binder_object*>(Alloc(sizeof(*obj)));
            if ((obj != nullptr) && (_offs_avail > 0)) {
                --_offs_avail;
                *_offs++ = static_cast<binder_size_t>(
                    reinterpret_cast<char*>(obj) - _data0);
                return obj;
            }
            _flags |= BIO_F_OVERFLOW;
            return nullptr;
        }

        const struct flat_binder_object* GetObject()
        {
            if (_data0 == nullptr || _offs0 == nullptr) {
                _flags |= BIO_F_OVERFLOW;
                return nullptr;
            }
            size_t currentOffset = static_cast<size_t>(_data - _data0);
            size_t count = static_cast<size_t>(_offs_avail);
            const binder_size_t* offsets = _offs0;
            for (size_t i = 0; i < count; ++i) {
                if (offsets[i] == static_cast<binder_size_t>(currentOffset)) {
                    return static_cast<const struct flat_binder_object*>(
                        Get(sizeof(struct flat_binder_object)));
                }
            }
            _data_avail = 0;
            _flags |= BIO_F_OVERFLOW;
            return nullptr;
        }

        char*          _data        { nullptr };
        binder_size_t* _offs        { nullptr };
        size_t         _data_avail  { 0 };
        size_t         _offs_avail  { 0 };
        char*          _data0       { nullptr };
        binder_size_t* _offs0       { nullptr };
        uint32_t       _flags       { 0 };
    };

} // namespace Core
} // namespace Thunder
