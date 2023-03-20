/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifndef __CYCLICBUFFER_H
#define __CYCLICBUFFER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "DataElementFile.h"

#ifndef __WINDOWS__
#include <semaphore.h>
#endif

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

namespace WPEFramework {

namespace Core {
    // Rationale:
    // This class allows to share data over process boundaries. Private access can be arranged by taking a lock.
    // The lock is also Process Wide.
    // Whoever holds the lock, can privately read or write from the buffer.
    class EXTERNAL CyclicBuffer {
    public:
        CyclicBuffer() = delete;
        CyclicBuffer(const CyclicBuffer&) = delete;
        CyclicBuffer& operator=(const CyclicBuffer&) = delete;

        CyclicBuffer(const string& fileName, const uint32_t mode, const uint32_t bufferSize, const bool overwrite);
        CyclicBuffer(Core::DataElementFile& buffer, const bool initiator, const uint32_t offset, const uint32_t bufferSize, const bool overwrite);
        virtual ~CyclicBuffer();

    protected:
        class Cursor {
        public:
            Cursor(CyclicBuffer& parent, uint32_t tail, uint32_t requiredSize)
                : _Parent(parent)
                , _Tail(tail)
                , _Size(requiredSize)
                , _Offset(0)
            {
            }

            template <typename ArgType>
            void Peek(ArgType& buffer) const
            {
                uint32_t startIndex = _Tail & _Parent._administration->_tailIndexMask;
                startIndex += _Offset;
                startIndex %= _Parent._administration->_size;

                uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&buffer);

                for (uint32_t i = 0; i < sizeof(buffer); i++) {
                    uint32_t index = (startIndex + i) % _Parent._administration->_size;
                    bytePtr[i] = _Parent._realBuffer[index];
                }
            }

            uint32_t Size() const
            {
                return _Size;
            }

            uint32_t Offset() const
            {
                return _Offset;
            }

            void Forward(uint32_t byteCount)
            {
                _Offset += byteCount;
            }

            uint32_t GetCompleteTail(uint32_t offset) const
            {
                uint32_t oldTail = _Tail;
                uint32_t roundCount = oldTail / (1 + _Parent._administration->_tailIndexMask);
                oldTail &= _Parent._administration->_tailIndexMask;

                uint32_t completeTail = oldTail + offset;
                completeTail %= _Parent._administration->_size;
                if (completeTail < oldTail) {
                    // Add one round, but prevent overflow.
                    roundCount = (roundCount + 1) % _Parent._administration->_roundCountModulo;
                }

                completeTail |= roundCount * (1 + _Parent._administration->_tailIndexMask);
                return completeTail;
            }

        private:
            uint32_t GetCurrentTail() const
            {
                return GetCompleteTail(_Offset);
            }

            uint32_t GetRemainingRequired() const
            {
                return (_Size - _Offset);
            }

            CyclicBuffer& _Parent;
            uint32_t _Tail;

            uint32_t _Size;
            uint32_t _Offset;
        };

        inline uint32_t Used(uint32_t head, uint32_t tail) const
        {
            uint32_t output = (head >= tail ? head - tail : _administration->_size - (tail - head));
            return output;
        }

        inline uint32_t Free(uint32_t head, uint32_t tail) const
        {
            uint32_t result = (head >= tail ? _administration->_size - (head - tail) : tail - head);
            return result;
        }

    public:
        inline void Flush()
        {
            std::atomic_store_explicit(&(_administration->_tail), (std::atomic_load(&(_administration->_head))), std::memory_order_relaxed);
        }
        inline bool Overwritten() const
        {
            bool overwritten((std::atomic_load(&(_administration->_state)) & OVERWRITTEN) == OVERWRITTEN);

            // Now clear the flag.
            std::atomic_fetch_and(&(_administration->_state), static_cast<uint16_t>(~OVERWRITTEN));

            return (overwritten);
        }
        inline uint32_t ErrorCode() const
        {
            return (_buffer.ErrorCode());
        }
        inline const string& Name() const
        {
            return (_buffer.Storage().Name());
        }
        inline uint32_t User(const string& userName) const
        {
            return (_buffer.User(userName));
        }
        inline uint32_t Group(const string& groupName) const
        {
            return (_buffer.Group(groupName));
        }
        inline uint32_t Permission(uint32_t mode) const
        {
            return (_buffer.Permission(mode));
        }
        inline bool IsLocked() const
        {
            return ((std::atomic_load(&(_administration->_state)) & LOCKED) == LOCKED);
        }
        inline uint32_t LockPid() const
        {
            return (_administration->_lockPID);
        }
        inline bool IsOverwrite() const
        {
            return ((std::atomic_load(&(_administration->_state)) & OVERWRITE) == OVERWRITE);
        }
        inline bool IsValid() const
        {
            return (_administration != nullptr);
        }
        inline const File& Storage() const
        {
            return (_buffer.Storage());
        }
        inline uint32_t Used() const
        {
            uint32_t head(_administration->_head);
            uint32_t tail(_administration->_tail & _administration->_tailIndexMask);

            return Used(head, tail);
        }
        inline uint32_t Free() const
        {
            uint32_t head(_administration->_head);
            uint32_t tail(_administration->_tail & _administration->_tailIndexMask);

            return Free(head, tail);
        }
        inline uint32_t Size() const
        {
            return (_administration->_size);
        }        
        bool Validate();
      
        // THREAD SAFE
        // If there are threads blocked in the Lock, they can be relinquised by
        // calling this method. This method, will un-block, all blocking calls
        // on the lock.
        void Alert();

        // THREAD SAFE
        // Give system wide, so over process boundaries, access to the cyclic buffer.
        // This is a blocking call. The call is blocked untill  no other requestor has
        // the lock, or there is DataPresent (if dataPresent parameter is set to true).
        uint32_t Lock(bool dataPresent = false, const uint32_t waitTime = Core::infinite);
        uint32_t Unlock();

        // Extract data from the cyclic buffer. Peek, is nondestructive. The cyclic
        // tail pointer is not progressed.
        uint32_t Peek(uint8_t buffer[], const uint32_t length) const;
        // Extract data from the cyclic buffer. Read, is destructive. The cyclic tail
        // pointer is progressed by the amount of data being inserted.
        uint32_t Read(uint8_t buffer[], const uint32_t length, bool partialRead = false);

        // Insert data into the cyclic buffer. By definition the head pointer is
        // progressed after the write.
        uint32_t Write(const uint8_t buffer[], const uint32_t length);

        // Move the tail out of the way so at least we can write "length" bytes.
        // The head will only be moved once all data is written.
        // This allows for writes of partial buffers without worrying about
        //    readers seeing incomplete data.
        uint32_t Reserve(const uint32_t length);

        virtual void DataAvailable();

    protected:
        inline bool Destroy()
        {
            _administration = nullptr;
            return (_buffer.Destroy());
        }
 
    private:
        // If the write occures, this method is called to determine the amount of spaces
        // that should be cleared out. The returned number of bytes must be equal to, or
        // larger than the minimumBytesToOverwrite. This method allows for skipping frames
        // if they are prefixed by a size, for example.
        virtual uint32_t GetOverwriteSize(Cursor& cursor);

        // If the read occures, this method is called to determine the amount of data
        // that should really be read. The returned number of bytes must be equal to, or
        // less than the CursorSize. This method allows for creating user length data in
        // a generic cyclic buffer file.
        virtual uint32_t GetReadSize(Cursor& cursor);

        // Makes sure "required" is available. If not, tail is moved in a smart way.
        void AssureFreeSpace(const uint32_t required);

        void AdminLock();
        void AdminUnlock();
        void Reevaluate();
        uint32_t SignalLock(const uint32_t waitTime);

    private:
        enum state {
            UNLOCKED = 0x00,
            LOCKED = 0x01,
            OVERWRITE = 0x02,
            OVERWRITTEN = 0x04
        };

        Core::DataElementFile _buffer;
        uint8_t* _realBuffer;
        bool _alert;

// Synchronisation over Process boundaries
#ifdef __WINDOWS__
        HANDLE _mutex;
        HANDLE _signal;
        HANDLE _event;
#endif

    public:
        // Shared data over the processes...
        struct control {
#ifndef __WINDOWS__
            pthread_mutex_t _mutex;
            pthread_cond_t _signal;

#endif

            std::atomic<uint32_t> _head;
            std::atomic<uint32_t> _tail;
            uint32_t _tailIndexMask; // Bitmask of index in buffer, rest is round count.
            uint32_t _roundCountModulo; // Value with which to mod round count to prevent overflow.
            std::atomic<uint32_t> _agents;
            std::atomic<uint16_t> _state;
            uint32_t _size;
            uint32_t _lockPID;

            // Keeps track of how much has been reserved for writing and by whom.
            uint32_t _reserved; // How much reserved in total.
            uint32_t _reservedWritten; // How much has already been written.
#ifndef __WINDOWS__
            std::atomic<pid_t> _reservedPID; // What process made the reservation.
#else
            std::atomic<DWORD> _reservedPID; // What process made the reservation.
#endif

        } * _administration;
    };
}
} // Core

#endif // __CYCLICBUFFER_H
