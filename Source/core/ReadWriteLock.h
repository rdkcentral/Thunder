#ifndef __READWRITELOCK_H
#define __READWRITELOCK_H

#include "Module.h"

#include "Sync.h"
#include "StateTrigger.h"

namespace WPEFramework {
namespace Core {

    class EXTERNAL ReadWriteLock {
    private:
        enum LockState {
            READERS = 0x01,
            WRITER = 0x02,
            IDLE = 0x04
        };

    private:
        ReadWriteLock(const ReadWriteLock& copy);
        ReadWriteLock& operator=(const ReadWriteLock& rhs);

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
            bool aquired = false;

            while ((aquired == false) && (m_State.WaitState(IDLE | READERS, waitTime) == true)) {
                m_State.Lock();

                if (m_State == IDLE) {
                    m_State.SetState(READERS);
                    m_Readers++;
                    aquired = true;
                }
                else if (m_State == READERS) {
                    m_Readers++;
                    aquired = true;
                }

                m_State.Unlock();
            }

            return (aquired);
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
            bool aquired = false;

            while ((aquired == false) && (m_State.WaitState(IDLE, waitTime) == true)) {
                m_State.Lock();

                if (m_State == IDLE) {
                    ASSERT(m_Readers == 0);

                    m_State.SetState(WRITER);
                    aquired = true;
                }

                m_State.Unlock();
            }

            return (aquired);
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
