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
 
#ifndef __SYNC_H
#define __SYNC_H

#include "Module.h"
#include "Trace.h"
#include "WarningReportingControl.h"
#include "WarningReportingCategories.h"

#include <list>

#ifdef __LINUX__
#include <pthread.h>
#include <semaphore.h>
#endif

namespace Thunder {
namespace Core {
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
#if defined(__CORE_CRITICAL_SECTION_LOG__)
            TryLock();
#else

            int result  = 0;

            REPORT_DURATION_WARNING( { result = pthread_mutex_lock(&m_syncMutex); }, WarningReporting::TooLongWaitingForLock);
            if (result != 0) {
                TRACE_L1("Probably creating a deadlock situation or lock on already destroyed mutex. <%d>", result);
            }

#endif
#endif

#ifdef __WINDOWS__
            ::EnterCriticalSection(&m_syncMutex);
#endif
        }

        inline void Unlock()
        {
#ifdef __POSIX__
            int result = pthread_mutex_unlock(&m_syncMutex);
            if (result != 0) {
                TRACE_L1("Probably does the calling thread not own this CriticalSection or unlock on already destroyed mutex. <%d>", result);
            }
#endif

#ifdef __WINDOWS__
            ::LeaveCriticalSection(&m_syncMutex);
#endif
        }

    protected: // Members
#ifdef __POSIX__
        pthread_mutex_t m_syncMutex;
#endif
#ifdef __WINDOWS__
        CRITICAL_SECTION m_syncMutex;
#endif

    private:
#ifdef __LINUX__
#if defined(__CORE_CRITICAL_SECTION_LOG__)
        void TryLock();

        static const int _AllocatedStackEntries = 20;
        static const int _AllocatedStacks = 20;
        int _UsedStackEntries[_AllocatedStacks];
        void* _LockingStack[_AllocatedStacks][_AllocatedStackEntries];

        pthread_t _LockingThread;
        static int StripStackTop(void** stack, int stackEntries, int stripped);

        static CriticalSection _StdErrDumpMutex;
#endif // __CORE_CRITICAL_SECTION_LOG__
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

    protected: // Members
#ifdef __POSIX__
        pthread_mutex_t m_syncAdminLock;
        pthread_cond_t m_syncCondition;
        volatile bool m_blLocked;
#endif

#ifdef __WINDOWS__
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

    protected: // Members
#ifdef __POSIX__
        pthread_mutex_t m_syncAdminLock;
        BinairySemaphore m_syncMinLimit;
        BinairySemaphore m_syncMaxLimit;
        unsigned int m_nCounter;
        unsigned int m_nMaxCount;
#endif

#ifdef __WINDOWS__
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
        bool IsSet() const;

    protected: // Members
        bool m_blManualReset;

#ifdef __POSIX__
        volatile bool m_blCondition;
        pthread_mutex_t m_syncAdminLock;
        pthread_cond_t m_syncCondition;
#endif

#ifdef __WINDOWS__
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
