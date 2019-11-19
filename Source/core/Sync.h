#ifndef __SYNC_H
#define __SYNC_H

#include "Module.h"
#include "Trace.h"

#include <list>

#ifdef __LINUX__
#include <pthread.h>
#include <semaphore.h>
#endif

namespace WPEFramework {
namespace Core {
    class EXTERNAL SyncHandle {
    public:
        SyncHandle()
            : m_SyncHandle(reinterpret_cast<SYSTEM_SYNC_HANDLE>(~0))
        {
        }
        SyncHandle(SYSTEM_SYNC_HANDLE a_Handle)
            : m_SyncHandle(a_Handle)
        {
        }
        SyncHandle(const SyncHandle& a_Copy)
            : m_SyncHandle(a_Copy.m_SyncHandle)
        {
        }
        ~SyncHandle()
        {
        }

        inline SyncHandle& operator=(const SyncHandle& a_RHS)
        {
            // Copy my own members
            return (operator=(a_RHS.m_SyncHandle));
        }

        SyncHandle& operator=(SYSTEM_SYNC_HANDLE a_SyncHandle)
        {
            //  This does not change the base, so...
            m_SyncHandle = a_SyncHandle;

            return (*this);
        }

    public:
        operator SYSTEM_SYNC_HANDLE() const
        {
            return (m_SyncHandle);
        }

    private:
        SYSTEM_SYNC_HANDLE m_SyncHandle;
    };

    // ===========================================================================
    // class CriticalSection
    // ===========================================================================

    class EXTERNAL CriticalSection {
    private:
        CriticalSection(const CriticalSection&) = delete;
        CriticalSection& operator=(const CriticalSection&) = delete;

    public: // Methods
        CriticalSection();
        ~CriticalSection();

        inline void Lock()
        {
#ifdef __LINUX__
#if defined(CRITICAL_SECTION_LOCK_LOG)
            TryLock();
#else
            if (pthread_mutex_lock(&m_syncMutex) != 0) {
                TRACE_L1("Probably creating a deadlock situation. <%d>", 0);
            }
#endif
#endif

#ifdef __WIN32__
            ::EnterCriticalSection(&m_syncMutex);
#endif
        }

        inline void Unlock()
        {
#ifdef __POSIX__
            if (pthread_mutex_unlock(&m_syncMutex) != 0) {
                TRACE_L1("Probably does the calling thread not own this CCriticalSection. <%d>", 0);
            }
#endif

#ifdef __WIN32__
            ::LeaveCriticalSection(&m_syncMutex);
#endif
        }

    protected: // Members
#ifdef __POSIX__
        pthread_mutex_t m_syncMutex;
#endif
#ifdef __WIN32__
        CRITICAL_SECTION m_syncMutex;
#endif

    private:
#ifdef __LINUX__
#if defined(CRITICAL_SECTION_LOCK_LOG)
        void TryLock();

        static const int _AllocatedStackEntries = 20;
        int _UsedStackEntries;
        void* _LockingStack[_AllocatedStackEntries];

        pthread_t _LockingThread;
        static int StripStackTop(void** stack, int stackEntries, int stripped);

        static CriticalSection _StdErrDumpMutex;
#endif // CRITICAL_SECTION_LOCK_LOG
#endif
    };

    // ===========================================================================
    // class BinairySemaphore
    // ===========================================================================

    class EXTERNAL BinairySemaphore {
    private:
        BinairySemaphore() = delete;
        BinairySemaphore(const BinairySemaphore&) = delete;
        BinairySemaphore& operator=(const BinairySemaphore&) = delete;

    public: // Methods
        BinairySemaphore(unsigned int nInitialCount, unsigned int nMaxCount);
        BinairySemaphore(bool blLocked);
        ~BinairySemaphore();

        uint32_t Lock();

        // Time in milliseconds!
        uint32_t Lock(unsigned int nSeconds);
        void Unlock();
        bool Locked() const;

#ifdef __WIN32__
        inline operator SyncHandle()
        {
            return (SyncHandle(m_syncMutex));
        }
#endif

    protected: // Members
#ifdef __POSIX__
        pthread_mutex_t m_syncAdminLock;
        pthread_cond_t m_syncCondition;
        volatile bool m_blLocked;
#endif

#ifdef __WIN32__
        HANDLE m_syncMutex;
#endif
    };

    // ===========================================================================
    // class CountingSemaphore
    // ===========================================================================

    class EXTERNAL CountingSemaphore {
    private:
        CountingSemaphore() = delete;
        CountingSemaphore(const CountingSemaphore&) = delete;
        CountingSemaphore& operator=(const CountingSemaphore&) = delete;

    public: // Methods
        CountingSemaphore(unsigned int nInitialCount, unsigned int nMaxCount);
        ~CountingSemaphore();

        uint32_t Lock();

        // Time in milliseconds!
        uint32_t Lock(unsigned int nSeconds);
        uint32_t Unlock(unsigned int nCount = 1);
        uint32_t TryUnlock(unsigned int nSeconds);

#ifdef __WIN32__
        inline operator SyncHandle()
        {
            return (SyncHandle(m_syncSemaphore));
        }
#endif

    protected: // Members
#ifdef __POSIX__
        pthread_mutex_t m_syncAdminLock;
        BinairySemaphore m_syncMinLimit;
        BinairySemaphore m_syncMaxLimit;
        unsigned int m_nCounter;
        unsigned int m_nMaxCount;
#endif

#ifdef __WIN32__
        HANDLE m_syncSemaphore;
#endif
    };

    // For Windows platform compatibility
    typedef CountingSemaphore Semaphore;

    // ===========================================================================
    // class Event
    // ===========================================================================

    class EXTERNAL Event {
    private:
        Event() = delete;
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;

    public: // Methods
        Event(bool blSet, bool blManualReset);
        ~Event();

        uint32_t Lock();
        uint32_t Unlock();

        // Time in milliseconds!
        uint32_t Lock(unsigned int nTime);
        void ResetEvent();
        void SetEvent();
        void PulseEvent();
        bool IsSet() const;

#ifdef __WIN32__
        inline operator SyncHandle()
        {
            return (SyncHandle(m_syncEvent));
        }
#endif

    protected: // Members
        bool m_blManualReset;

#ifdef __POSIX__
        volatile bool m_blCondition;
        pthread_mutex_t m_syncAdminLock;
        pthread_cond_t m_syncCondition;
#endif

#ifdef __WIN32__
        HANDLE m_syncEvent;
#endif
    };

    template <typename SYNCOBJECT>
    class SafeSyncType {
    private:
        SafeSyncType() = delete;
        SafeSyncType(const SafeSyncType<SYNCOBJECT>&) = delete;
        SafeSyncType<SYNCOBJECT>& operator=(const SafeSyncType<SYNCOBJECT>&) = delete;

    public:
        explicit SafeSyncType(SYNCOBJECT& _cs)
            : m_Lock(_cs)
        {
            m_Lock.Lock();
        }

        ~SafeSyncType()
        {
            m_Lock.Unlock();
        }

    private:
        SYNCOBJECT& m_Lock;
    };

    EXTERNAL uint32_t InterlockedIncrement(volatile uint32_t& a_Number);
    EXTERNAL uint32_t InterlockedDecrement(volatile uint32_t& a_Number);
    EXTERNAL uint32_t InterlockedIncrement(volatile int& a_Number);
    EXTERNAL uint32_t InterlockedDecrement(volatile int& a_Number);
}
} // namespace Core

#endif // __SYNC_H
