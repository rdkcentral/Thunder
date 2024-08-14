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
 
#ifndef __READWRITELOCK_H
#define __READWRITELOCK_H

#include "Module.h"

#include "StateTrigger.h"
#include "Sync.h"

namespace Thunder {
namespace Core {

    class EXTERNAL ReadWriteLock {
    private:
        enum LockState {
            READERS = 0x01,
            WRITER = 0x02,
            IDLE = 0x04
        };

    public:
        ReadWriteLock(const ReadWriteLock& copy) = delete;
        ReadWriteLock(ReadWriteLock&& move) = delete;
        ReadWriteLock& operator=(const ReadWriteLock& rhs) = delete;
        ReadWriteLock& operator=(ReadWriteLock&& move) = delete;

    public:
        ReadWriteLock()
            : m_State(IDLE)
            , m_Readers(0)
        {
        }
        ~ReadWriteLock()
        {
            ASSERT((m_Readers == 0) && (m_State == IDLE));
        }

    public:
        bool ReadLock(const uint32_t waitTime = Core::infinite)
        {
            bool acquired = false;

            while ((acquired == false) && (m_State.WaitState(IDLE | READERS, waitTime) == true)) {
                m_State.Lock();

                if (m_State == IDLE) {
                    m_State.SetState(READERS);
                    m_Readers++;
                    acquired = true;
                } else if (m_State == READERS) {
                    m_Readers++;
                    acquired = true;
                }

                m_State.Unlock();
            }

            return (acquired);
        }

        void ReadUnlock()
        {
            // If we want to unlock, we must have locked it and thus we must be in the READERS state
            ASSERT((m_State == READERS) && (m_Readers > 0));

            m_State.Lock();

            m_Readers--;

            if (m_Readers == 0) {
                m_State.SetState(IDLE);
            }

            m_State.Unlock();
        }

        bool WriteLock(const uint32_t waitTime = Core::infinite)
        {
            // Only 1 allowed, so we need to be in INDLE state
            bool acquired = false;

            while ((acquired == false) && (m_State.WaitState(IDLE, waitTime) == true)) {
                m_State.Lock();

                if (m_State == IDLE) {
                    ASSERT(m_Readers == 0);

                    m_State.SetState(WRITER);
                    acquired = true;
                }

                m_State.Unlock();
            }

            return (acquired);
        }
        void WriteUnlock()
        {
            // If we want to unlock, we must have locked it and thus we must be in the WRITER state
            ASSERT((m_State == WRITER) && (m_Readers == 0));

            m_State.SetState(IDLE);
        }

    private:
        StateTrigger<LockState> m_State;
        uint32_t m_Readers;
    };
}
} // namespace Core

#endif // __READWRITELOCK_H
