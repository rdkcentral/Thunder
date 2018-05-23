// ===========================================================================
//
// Filename:    sync.cpp
//
// Description: Implementation file for for the CriticalSection,
//              CBinarySemahore, CountingSemaphore and the Event
//              synchronisation classes.
//
// History
//
// Author        Reason                                             Date
// ---------------------------------------------------------------------------
// P. Wielders   Initial creation                                   2002/05/24
//
// ===========================================================================

#include "Sync.h"
#include "Trace.h"
#include "ProcessInfo.h"

#ifdef CRITICAL_SECTION_LOCK_LOG
#include "Thread.h"
#endif

#if defined(__LINUX__) && !defined(__APPLE__)
#include <time.h>
#include <asm/errno.h>
#include <unistd.h>
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// GLOBAL INTERLOCKED METHODS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace WPEFramework {
namespace Core {

#ifdef __WIN32__
    uint32_t
    InterlockedIncrement(
        volatile uint32_t& a_Number)
    {
        return (::InterlockedIncrement(&a_Number));
    }

    uint32_t
    InterlockedDecrement(
        volatile uint32_t& a_Number)
    {
        return (::InterlockedDecrement(&a_Number));
    }

    uint32_t
    InterlockedIncrement(
        volatile int& a_Number)
    {
        return (::InterlockedIncrement(reinterpret_cast<volatile unsigned int*>(&a_Number)));
    }

    uint32_t
    InterlockedDecrement(
        volatile int& a_Number)
    {
        return (::InterlockedDecrement(reinterpret_cast<volatile unsigned int*>(&a_Number)));
    }

#else

    uint32_t
    InterlockedIncrement(
        volatile uint32_t& a_Number)
    {
        return (__sync_fetch_and_add(&a_Number, 1) + 1);
    }

    uint32_t
    InterlockedDecrement(
        volatile uint32_t& a_Number)
    {
        return (__sync_fetch_and_sub(&a_Number, 1) - 1);
    }

    uint32_t
    InterlockedIncrement(
        volatile int& a_Number)
    {
        return (__sync_fetch_and_add(&a_Number, 1) + 1);
    }

    uint32_t
    InterlockedDecrement(
        volatile int& a_Number)
    {
        return (__sync_fetch_and_sub(&a_Number, 1) - 1);
    }

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CriticalSection class
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CONSTRUCTOR & DESTRUCTOR
//----------------------------------------------------------------------------
#ifdef __WIN32__
    CriticalSection::CriticalSection()
    {
        TRACE_L5("Constructor CriticalSection <0x%X>", TRACE_POINTER(this));

        ::InitializeCriticalSection(&m_syncMutex);
    }
#endif

#ifdef __POSIX__
    CriticalSection::CriticalSection()
#ifdef CRITICAL_SECTION_LOCK_LOG
        : _UsedStackEntries(0)
        , _LockingThread(0)
#endif // CRITICAL_SECTION_LOCK_LOG
    {
        TRACE_L5("Constructor CriticalSection <0x%X>", TRACE_POINTER(this));

        pthread_mutexattr_t structAttributes;

        // Create a recursive mutex for this process (no named version, use semaphore)
        if (pthread_mutexattr_init(&structAttributes) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }
        else if (pthread_mutexattr_settype(&structAttributes, PTHREAD_MUTEX_RECURSIVE) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }
        else if (pthread_mutex_init(&m_syncMutex, &structAttributes) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }
    }

#ifdef CRITICAL_SECTION_LOCK_LOG
    void CriticalSection::TryLock()
    {
        // Wait time in seconds.
        const int nTimeSecs = 5;
        timespec structTime;

        clock_gettime(CLOCK_REALTIME, &structTime);
        structTime.tv_sec += nTimeSecs;

        int result = pthread_mutex_timedlock(&m_syncMutex, &structTime);
        if (result != 0) {
            void* addresses[_AllocatedStackEntries];

            int addressCount = backtrace(addresses, _AllocatedStackEntries);

            // Remove top two frames because we are not interested in Lock+TryLock.
            addressCount = StripStackTop(addresses, addressCount, 2);

            // Lock mutex guarding stderr so no other critical section can dump its deadlock info
            _StdErrDumpMutex.Lock();

            TRACE_L1("Issue on process: <%d>", Core::ProcessInfo().Id());
            TRACE_L1("Probably creating a deadlock situation. <%d>", result);

            fprintf(stderr, "Failing lock:\n");
            backtrace_symbols_fd(addresses, addressCount, fileno(stderr));

            fprintf(stderr, "\nLocked location:\n");
            backtrace_symbols_fd(_LockingStack, _UsedStackEntries, fileno(stderr));

            fprintf(stderr, "\nCurrent stack of locking thread:\n");
            addressCount = ::GetCallStack(_LockingThread, addresses, _AllocatedStackEntries);
            backtrace_symbols_fd(addresses, _AllocatedStackEntries, fileno(stderr));

            _StdErrDumpMutex.Unlock();

            if (result == ETIMEDOUT && ((result = pthread_mutex_lock(&m_syncMutex)) != 0)) {
                TRACE_L1("After detection, continued to wait. Wait failed with error: <%d>", result);
            }
        }
        else {
            _UsedStackEntries = backtrace(_LockingStack, _AllocatedStackEntries);

            // Remove top two frames because we are not interested in Lock+TryLock.
            _UsedStackEntries = StripStackTop(_LockingStack, _UsedStackEntries, 2);

            _LockingThread = pthread_self();
        }
    }

    int CriticalSection::StripStackTop(void** stack, int stackEntries, int stripped)
    {
        int newEntryCount = stackEntries - stripped;
        if (newEntryCount < 0) {
            newEntryCount = 0;
            stripped = stackEntries;
        }

        memmove(stack, stack + stripped, sizeof(void*) * newEntryCount);

        // Set rest of buffer to zeroes.
        memset(stack + newEntryCount, 0, sizeof(void*) * stripped);

        return newEntryCount;
    }
#endif // CRITICAL_SECTION_LOCK_LOG

#endif

    CriticalSection::~CriticalSection()
    {
        TRACE_L5("Destructor CriticalSection <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__
        if (pthread_mutex_destroy(&m_syncMutex) != 0) {
            TRACE_L1("Probably trying to delete a used CriticalSection <%d>.", 0);
        }
#endif
#ifdef __WIN32__
        ::DeleteCriticalSection(&m_syncMutex);
#endif
    }

    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------
    // BinairySemaphore class
    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // CONSTRUCTOR & DESTRUCTOR
    //----------------------------------------------------------------------------

    // This constructor is to be compatible with the WIN32 CSemaphore class which
    // sets the inital count an the maximum count. This way, on platform changes,
    // only the declaration/definition of the synchronisation object has to be defined
    // as being Binairy, not the coding.
    BinairySemaphore::BinairySemaphore(unsigned int nInitialCount, unsigned int nMaxCount)
    {
        DEBUG_VARIABLE(nMaxCount);
        ASSERT((nInitialCount == 0) || (nInitialCount == 1));

        TRACE_L5("Constructor BinairySemaphore (int, int)  <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__
        m_blLocked = (nInitialCount == 0);

        pthread_condattr_t attr;

        if (0 != pthread_condattr_init(&attr)) {
            ASSERT(false);
        }

#ifndef __APPLE__
        if (0 != pthread_condattr_setclock(&attr, CLOCK_REALTIME)) {
            ASSERT(false);
        }
#endif

        if (pthread_mutex_init(&m_syncAdminLock, nullptr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }

        if (pthread_cond_init(&m_syncCondition, &attr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }
#endif

#ifdef __WIN32__
        m_syncMutex = ::CreateMutex(nullptr, (nInitialCount == 0), nullptr);

        ASSERT(m_syncMutex != nullptr);
#endif
    }

    BinairySemaphore::BinairySemaphore(bool blLocked)
#ifdef __POSIX__
        : m_blLocked(blLocked)
#endif
    {
        TRACE_L5("Constructor BinairySemaphore <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__
        pthread_condattr_t attr;

        if (0 != pthread_condattr_init(&attr)) {
            ASSERT(false);
        }

#ifndef __APPLE__
        if (0 != pthread_condattr_setclock(&attr, CLOCK_REALTIME)) {
            ASSERT(false);
        }
#endif
        if (pthread_mutex_init(&m_syncAdminLock, nullptr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }

        if (pthread_cond_init(&m_syncCondition, &attr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }
#endif

#ifdef __WIN32__
        m_syncMutex = ::CreateMutex(nullptr, blLocked, nullptr);

        ASSERT(m_syncMutex != nullptr);
#endif
    }

    BinairySemaphore::~BinairySemaphore()
    {
        TRACE_L5("Destructor BinairySemaphore <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__
        // If we really create it, we really have to destroy it.
        pthread_mutex_destroy(&m_syncAdminLock);
        pthread_cond_destroy(&m_syncCondition);
#endif

#ifdef __WIN32__
        ::CloseHandle(m_syncMutex);
#endif
    }

    //----------------------------------------------------------------------------
    // PUBLIC METHODS
    //----------------------------------------------------------------------------

    uint32_t
    BinairySemaphore::Lock()
    {

#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncMutex, Core::infinite, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_GENERAL);
#else
		int nResult = Core::ERROR_NONE;

		// See if we can check the state.
        pthread_mutex_lock(&m_syncAdminLock);

        // We are not busy Setting the flag, so we can check it.
        if (m_blLocked != false) {
            do {
                // Oops it seems that we are not allowed to pass.
                nResult = pthread_cond_wait(&m_syncCondition, &m_syncAdminLock);

                if (nResult != 0) {
                    // Something went wrong, so assume...
                    TRACE_L5("Error waiting for event <%d>.", nResult);
                    nResult = Core::ERROR_GENERAL;
                }

                // For some reason the documentation says that we have to double check on
                // the condition variable to see if we are allowed to fall through, so we
                // do (Guide to DEC threads, March 1996 ,page pthread-56, paragraph 4)
            } while ((m_blLocked == true) && (nResult == Core::ERROR_NONE));
        }

        if (nResult == Core::ERROR_NONE) {
            // Seems like we have the token, So the object is locked now.
            m_blLocked = true;
        }

        // Done with the internals of the binairy semphore, everyone can access it again.
        pthread_mutex_unlock(&m_syncAdminLock);

        // Wait forever so...
        return (nResult);
#endif
    }

    uint32_t
    BinairySemaphore::Lock(unsigned int nTime)
    {
#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncMutex, nTime, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#else
        uint32_t nResult = Core::ERROR_NONE;
        if (nTime == Core::infinite) {
            return (Lock());
        }
        else {

            // See if we can check the state.
            pthread_mutex_lock(&m_syncAdminLock);

            // We are not busy Setting the flag, so we can check it.
            if (m_blLocked == true) {
                struct timespec structTime;

#ifdef __LINUX__
                clock_gettime(CLOCK_REALTIME, &structTime);
                structTime.tv_nsec += ((nTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
                structTime.tv_sec += (nTime / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
                structTime.tv_nsec = structTime.tv_nsec % 1000000000;
#endif

                do {
                    // Oops it seems that we are not allowed to pass.
                    nResult = pthread_cond_timedwait(&m_syncCondition, &m_syncAdminLock, &structTime);

                    if (nResult == ETIMEDOUT) {
                        // Som/ething went wrong, so assume...
                        TRACE_L5("Timed out waiting for event <%d>.", nTime);
                        nResult = Core::ERROR_TIMEDOUT;
                    }
                    else if (nResult != 0) {
                        // Something went wrong, so assume...
                        TRACE_L5("Waiting on semaphore failed. Error code <%d>", nResult);
                        nResult = Core::ERROR_GENERAL;
                    }

                    // For some reason the documentation says that we have to double check on
                    // the condition variable to see if we are allowed to fall through, so we
                    // do (Guide to DEC threads, March 1996 ,page pthread-56, paragraph 4)
                } while ((m_blLocked == true) && (nResult == Core::ERROR_NONE));
            }

            if (nResult == Core::ERROR_NONE) {
                // Seems like we have the token, So the object is locked now.
                m_blLocked = true;
            }

            // Done with the internals of the binairy semphore, everyone can access it again.
            pthread_mutex_unlock(&m_syncAdminLock);
        }

        // Timed out or did we get the token ?
        return (nResult);
#endif
    }

    void
    BinairySemaphore::Unlock()
    {

#ifdef __POSIX__
        // See if we can get access to the data members of this object.
        pthread_mutex_lock(&m_syncAdminLock);

        // Yep, that's it we are no longer locked. Signal the change.
        m_blLocked = false;

        // O.K. that is arranged, Now we should at least signal the first
        // waiting process that is waiting for this condition to occur.
        pthread_cond_signal(&m_syncCondition);

        // Now that we are done with the variablegive other threads access
        // to the object again.
        pthread_mutex_unlock(&m_syncAdminLock);
#endif

#ifdef __WIN32__
        ::ReleaseMutex(m_syncMutex);
#endif
    }

    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------
    // CountingSemaphore class
    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // CONSTRUCTOR & DESTRUCTOR
    //----------------------------------------------------------------------------

    CountingSemaphore::CountingSemaphore(
        unsigned int nInitialCount,
        unsigned int nMaxCount)
#ifdef __POSIX__
        : m_syncMinLimit(false)
        , m_syncMaxLimit(false)
        , m_nCounter(nInitialCount)
        , m_nMaxCount(nMaxCount)
#endif
    {
        TRACE_L5("Constructor CountingSemaphore <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__

        if (pthread_mutex_init(&m_syncAdminLock, nullptr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }

        // Well that is it, see if one of the Limit locks should be taken ?
        if (m_nCounter == 0) {
            // This should be possible since we created them Not Locked.
            m_syncMinLimit.Lock();
        }

        // Or maybe we are at the upper limit ?
        if (m_nCounter == m_nMaxCount) {
            // This should be possible since we created them Not Locked.
            m_syncMaxLimit.Lock();
        }
#endif

#ifdef __WIN32__
        m_syncSemaphore = ::CreateSemaphore(nullptr, nInitialCount, nMaxCount, nullptr);

        ASSERT(m_syncSemaphore != nullptr);
#endif
    }

    CountingSemaphore::~CountingSemaphore()
    {
        TRACE_L5("Destructor CountingSemaphore <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__
        // O.K. Destroy all the semaphores used by this class.
        pthread_mutex_destroy(&m_syncAdminLock);
#endif

#ifdef __WIN32__
        ::CloseHandle(m_syncSemaphore);
#endif
    }

    //----------------------------------------------------------------------------
    // PUBLIC METHODS
    //----------------------------------------------------------------------------

    uint32_t
    CountingSemaphore::Lock()
    {
#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncSemaphore, Core::infinite, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_GENERAL);
#else
        // First see if we could still decrease the count..
        uint32_t nResult = m_syncMinLimit.Lock();

        // See that we were able to get the semaphore.
        if (nResult == Core::ERROR_NONE) {
            // If we have this semaphore, no Other lock can take place
            // now make sure that the counter is handled atomic. Get the
            // administration lock (unlock can still access it).
            pthread_mutex_lock(&m_syncAdminLock);

            // Now we are in the clear, Lock cannot access this (blocked
            // on MinLimit) and unlock cannot access the counter (blocked on
            // m_syncAdminLock). Work the Semaphore counter. It's safe.

            // If we leave the absolute max position, make sure we release
            // the MaxLimit synchronisation.
            if (m_nCounter == m_nMaxCount) {
                m_syncMaxLimit.Unlock();
            }

            // Now update the counter.
            m_nCounter--;

            // See if the counter can still be decreased.
            if (m_nCounter != 0) {
                m_syncMinLimit.Unlock();
            }

            // Now we are completely done with the counter and it's logic. Free all
            // waiting threads for this resource.
            pthread_mutex_unlock(&m_syncAdminLock);
        }

        return (nResult);
#endif
    }

    uint32_t
    CountingSemaphore::Lock(unsigned int nMilliSeconds)
    {
#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncSemaphore, nMilliSeconds, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#else
        // First see if we could still decrease the count..
        uint32_t nResult = m_syncMinLimit.Lock(nMilliSeconds);

        // See that we were able to get the semaphore.
        if (nResult == Core::ERROR_NONE) {
            // If we have this semaphore, no Other lock can take place
            // now make sure that the counter is handled atomic. Get the
            // administration lock (unlock can still access it).
            pthread_mutex_lock(&m_syncAdminLock);

            // Now we are in the clear, Lock cannot access this (blocked
            // on MinLimit) and unlock cannot access the counter (blocked on
            // m_syncAdminLock). Work the Semaphore counter. It's safe.

            // If we leave the absolute max position, make sure we release
            // the MaxLimit synchronisation.
            if (m_nCounter == m_nMaxCount) {
                m_syncMaxLimit.Unlock();
            }

            // Now update the counter.
            m_nCounter--;

            // See if the counter can still be decreased.
            if (m_nCounter != 0) {
                m_syncMinLimit.Unlock();
            }

            // Now we are completely done with the counter and it's logic. Free all
            // waiting threads for this resource.
            pthread_mutex_unlock(&m_syncAdminLock);
        }

        return (nResult);
#endif
    }

    uint32_t
    CountingSemaphore::Unlock(unsigned int nCount)
    {
        ASSERT(nCount != 0);

#ifdef __POSIX__
        // First see if we could still increase the count..
        uint32_t nResult = m_syncMaxLimit.Lock(Core::infinite);

        // See that we were able to get the semaphore.
        if (nResult == Core::ERROR_NONE) {
            // If we have this semaphore, no Other lock can take place
            // now make sure that the counter is handled atomic. Get the
            // administration lock (unlock can still access it).
            pthread_mutex_lock(&m_syncAdminLock);

            // Now we are in the clear, Unlock cannot access this (blocked on
            // MaxLimit) and Lock cannot access the counter (blocked on
            // m_syncAdminLock). Work the Semaphore counter. It's safe.

            // If we leave the absolute min position (0), make sure we signal
            // the Lock proCess, give the MinLimit synchronisation free.
            if (m_nCounter == 0) {
                m_syncMinLimit.Unlock();
            }

            // See if the given count
            m_nCounter += nCount;

            // See if we reached or overshot the max ?
            if (m_nCounter > m_nMaxCount) {
                // Release the Admin Semephore so the Lock on the max limit
                //  can proceed.
                pthread_mutex_unlock(&m_syncAdminLock);

                // Seems like we added more than allowed, so wait till the Max
                // mutex get's unlocked by the Lock process.
                m_syncMaxLimit.Lock(Core::infinite);

                // Before we continue processing, Get the administrative lock
                // again.
                pthread_mutex_lock(&m_syncAdminLock);
            }

            // See if we are still allowed to increase the counter.
            if (m_nCounter != m_nMaxCount) {
                m_syncMaxLimit.Unlock();
            }

            // Now we are completely done with the counter and it's logic. Free all
            // waiting threads for this resource.
            pthread_mutex_unlock(&m_syncAdminLock);
        }
#endif

#ifdef __WIN32__
        uint32_t nResult = Core::ERROR_NONE;

        if (::ReleaseSemaphore(m_syncSemaphore, nCount, nullptr) == FALSE) {
            // Could not give all tokens.
            ASSERT(false);
        }
#endif

        return (nResult);
    }

#ifdef __POSIX__
    uint32_t CountingSemaphore::TryUnlock(unsigned int nMilliSeconds)
#else
    uint32_t CountingSemaphore::TryUnlock(unsigned int /* nMilliSeconds */)
#endif
    {
#ifdef __POSIX__
        // First see if we could still increase the count..
        uint32_t nResult = m_syncMaxLimit.Lock(nMilliSeconds);

        // See that we were able to get the semaphore.
        if (nResult == Core::ERROR_NONE) {
            // If we have this semaphore, no Other lock can take place
            // now make sure that the counter is handled atomic. Get the
            // administration lock (unlock can still access it).
            pthread_mutex_lock(&m_syncAdminLock);

            // Now we are in the clear, Unlock cannot access this (blocked on
            // MaxLimit) and Lock cannot access the counter (blocked on
            // m_syncAdminLock). Work the Semaphore counter. It's safe.

            // If we leave the absolute min position (0), make sure we signal
            // the Lock process, give the MinLimit synchronisation free.
            if (m_nCounter == 0) {
                m_syncMinLimit.Unlock();
            }

            // Now update the counter.
            m_nCounter++;

            // See if we are still allowed to increase the counter.
            if (m_nCounter != m_nMaxCount) {
                m_syncMaxLimit.Unlock();
            }

            // Now we are completely done with the counter and it's logic. Free all
            // waiting threads for this resource.
            pthread_mutex_unlock(&m_syncAdminLock);
        }
#endif

#ifdef __WIN32__
        uint32_t nResult = Core::ERROR_NONE;

        if (::ReleaseSemaphore(m_syncSemaphore, 1, nullptr) == FALSE) {
            // Wait for the given time to see if we can "give" the lock.
            // To be Coded.
            ASSERT(false);
        }

#endif

        return (nResult);
    }

    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------
    // Event class (AVAILABLE WITHIN PROCESS SPACE)
    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // CONSTRUCTOR & DESTRUCTOR
    //----------------------------------------------------------------------------

    Event::Event(bool blSet, bool blManualReset)
        : m_blManualReset(blManualReset)
#ifdef __POSIX__
        , m_blCondition(blSet)
#endif
    {
        TRACE_L5("Constructor Event <0x%X>", TRACE_POINTER(this));

#ifdef __POSIX__
        pthread_condattr_t attr;

        if (0 != pthread_condattr_init(&attr)) {
            ASSERT(false);
        }
#ifndef __APPLE__
        if (0 != pthread_condattr_setclock(&attr, CLOCK_REALTIME)) {
            ASSERT(false);
        }
#endif

        if (pthread_mutex_init(&m_syncAdminLock, nullptr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }

        if (pthread_cond_init(&m_syncCondition, &attr) != 0) {
            // That will be the day, if this fails...
            ASSERT(false);
        }
#endif

#ifdef __WIN32__
        m_syncEvent = ::CreateEvent(nullptr, blManualReset, blSet, nullptr);

        ASSERT(m_syncEvent != nullptr);
#endif
    }

    Event::~Event()
    {

#ifdef __POSIX__
        TRACE_L5("Destructor Event <0x%X>", TRACE_POINTER(this));

        // If we really create it, we really have to destroy it.
        pthread_mutex_destroy(&m_syncAdminLock);
        pthread_cond_destroy(&m_syncCondition);
#endif

#ifdef __WIN32__
        ::CloseHandle(m_syncEvent);
#endif
    }

    //----------------------------------------------------------------------------
    // PUBLIC METHODS
    //----------------------------------------------------------------------------

    uint32_t
    Event::Lock()
    {
#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncEvent, Core::infinite, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_GENERAL);
#else
        int nResult = Core::ERROR_NONE;
        // See if we can check the state.
        pthread_mutex_lock(&m_syncAdminLock);

        // We are not busy Setting the flag, so we can check it.
        if (m_blCondition == false) {
            do {
                // Oops it seems that we are not allowed to pass.
                nResult = (pthread_cond_wait(&m_syncCondition, &m_syncAdminLock) == 0 ? Core::ERROR_NONE : Core::ERROR_GENERAL);

                // For some reason the documentation says that we have to double check on
                // the condition variable to see if we are allowed to fall through, so we
                // do (Guide to DEC threads, March 1996 ,page pthread-56, paragraph 4)
            } while ((m_blCondition == false) && (nResult == Core::ERROR_NONE));

            if (nResult != 0) {
                // Something went wrong, so assume...
                TRACE_L5("Error waiting for event <%d>.", nResult);
            }
        }

        // Seems that the event is triggered, lets continue. but
        // do not forget to give back the flag..
        pthread_mutex_unlock(&m_syncAdminLock);

        // Wait forever so...
        return (nResult);
#endif
    }

    bool
    Event::IsSet() const
    {
#ifdef __POSIX__
        return (m_blCondition);
#endif

#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncEvent, 0, FALSE) == WAIT_OBJECT_0);
#endif
    }

    uint32_t
    Event::Lock(unsigned int nTime)
    {
#ifdef __WIN32__
        return (::WaitForSingleObjectEx(m_syncEvent, nTime, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#else
        if (nTime == Core::infinite) {
            return (Lock());
        }
        else {
            int nResult = Core::ERROR_NONE;

            // See if we can check the state.
            pthread_mutex_lock(&m_syncAdminLock);

            // We are not busy Setting the flag, so we can check it.
            if (m_blCondition == false) {
                struct timespec structTime;

                clock_gettime(CLOCK_REALTIME, &structTime);
                structTime.tv_nsec += ((nTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
                structTime.tv_sec += (nTime / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
                structTime.tv_nsec = structTime.tv_nsec % 1000000000;

                do {
                    // Oops it seems that we are not allowed to pass.
                    nResult = (pthread_cond_timedwait(&m_syncCondition, &m_syncAdminLock, &structTime) != 0 ? Core::ERROR_TIMEDOUT : Core::ERROR_NONE);

                    // For some reason the documentation says that we have to double check on
                    // the condition variable to see if we are allowed to fall through, so we
                    // do (Guide to DEC threads, March 1996 ,page pthread-56, paragraph 4)
                } while ((m_blCondition == false) && (nResult == Core::ERROR_NONE));

                if (nResult != 0) {
                    // Something went wrong, so assume...
                    TRACE_L5("Timed out waiting for event <%d>!", nResult);
                }
            }

            // Seems that the event is triggered, lets continue. but
            // do not forget to give back the flag..
            pthread_mutex_unlock(&m_syncAdminLock);

            return (nResult);
        }
#endif
    }

    uint32_t
    Event::Unlock()
    {
        uint32_t nResult = Core::ERROR_NONE;

#ifdef __POSIX__
        // See if we can get access to the data members of this object.
        pthread_mutex_lock(&m_syncAdminLock);

        // Yep, that's it we are no longer locked. Signal the change.
        m_blCondition = true;

        // O.K. that is arranged, Now we should at least signal the first
        // waiting process that is waiting for this condition to occur.
        pthread_cond_signal(&m_syncCondition);

        // Now that we are done with the variablegive other threads access
        // to the object again.
        pthread_mutex_unlock(&m_syncAdminLock);
#endif

#ifdef __WIN32__
        if (m_blManualReset) {
            ::SetEvent(m_syncEvent);
        }
        else {
            ::PulseEvent(m_syncEvent);
        }
#endif

        return (nResult);
    }

    void
    Event::ResetEvent()
    {
#ifdef __POSIX__
        // See if we can check the state.
        pthread_mutex_lock(&m_syncAdminLock);

        // We are the onlyones who can access the data, time to update it.
        m_blCondition = false;

        // Done changing the data, free other threads so the can use this
        // object again
        pthread_mutex_unlock(&m_syncAdminLock);
#endif

#ifdef __WIN32__
        ::ResetEvent(m_syncEvent);
#endif
    }

    void
    Event::SetEvent()
    {
#ifdef __POSIX__
        // See if we can get access to the data members of this object.
        pthread_mutex_lock(&m_syncAdminLock);

        // Yep, that's it we are signalled, Broadcast the change.
        m_blCondition = true;

        // O.K. that is arranged, Now we should at least signal waiting
        // process that the event has occured.
        pthread_cond_broadcast(&m_syncCondition);

        // All waiting threads are now in the running mode again. See
        // if the event should be cleared manually again.
        if (m_blManualReset == false) {
            // Make sure all threads are in running mode, place our request
            // for sync at the end of the FIFO-queue for syncConditionMutex.
            pthread_mutex_unlock(&m_syncAdminLock);
            ::SleepMs(0);
            pthread_mutex_lock(&m_syncAdminLock);

            // They all had a change to continue so, now it is over, we can
            // not wait forever......
            m_blCondition = false;
        }

        // Now that we are done with the variablegive other threads access
        // to the object again.
        pthread_mutex_unlock(&m_syncAdminLock);
#endif

#ifdef __WIN32__
        ::SetEvent(m_syncEvent);
#endif
    }

    void
    Event::PulseEvent()
    {
#ifdef __POSIX__
        // See if we can get access to the data members of this object.
        pthread_mutex_lock(&m_syncAdminLock);

        // Yep, that's it we are signalled, Broadcast the change.
        m_blCondition = true;

        // O.K. that is arranged, Now we should at least signal waiting
        // process that the event has occured.
        pthread_cond_broadcast(&m_syncCondition);

        // Make sure all threads are in running mode, place our request
        // for sync at the end of the FIFO-queue for syncConditionMutex.
        pthread_mutex_unlock(&m_syncAdminLock);
        ::SleepMs(0);
        pthread_mutex_lock(&m_syncAdminLock);

        // They all had a change to continue so, now it is over, we can
        // not wait forever......
        m_blCondition = false;

        // Now that we are done with the variablegive other threads access
        // to the object again.
        pthread_mutex_unlock(&m_syncAdminLock);
#endif

#ifdef __WIN32__
        ::PulseEvent(m_syncEvent);
#endif
    }
#ifndef __WIN32__
#if defined(CRITICAL_SECTION_LOCK_LOG)
    CriticalSection CriticalSection::_StdErrDumpMutex;
#endif
#endif

		DoorBell::DoorBell(const TCHAR sourceName[]) {
#ifdef __WIN32__
			_doorBell = ::CreateEvent(nullptr, TRUE, FALSE, sourceName);
#else
			_doorBell = sem_open(sourceName, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0); /* Initial value is 0. */

			if (_doorBell == nullptr) {
				TRACE_L1("Failed to create DoorBell, error %d", errno);

				ASSERT(false);
			}
#endif
		}
		DoorBell::~DoorBell() {
#ifdef __WIN32__
			if (_doorBell != nullptr)
			{
				::CloseHandle(_doorBell);
			}
#else
			if (_doorBell != nullptr)
			{
				sem_close(_doorBell);
			}
#endif
		}

		void DoorBell::Ring() {
#ifdef __WIN32__
			if (_doorBell != nullptr)
			{
				BOOL result = ::SetEvent(_doorBell);

				ASSERT(result != FALSE);
			}
#else
			if (_doorBell != nullptr)
			{
				VARIABLE_IS_NOT_USED int result = sem_post(_doorBell);

				ASSERT((result == 0) || (errno == EOVERFLOW));
			}
#endif
		}

		uint32_t DoorBell::Wait(const uint32_t waitTime) const {

			uint32_t result = Core::ERROR_GENERAL;
            if (_doorBell != nullptr) {
#ifdef __WIN32__
                return (::WaitForSingleObjectEx(_doorBell, waitTime, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
#elif defined(__APPLE__)

                uint32_t timeLeft = waitTime;
                int semResult;
                while(((semResult = sem_trywait(_doorBell)) != 0) && timeLeft > 0) {
                    ::SleepMs(100);
                    if (timeLeft != Core::infinite) {
                        timeLeft -= (timeLeft > 100 ? 100 : timeLeft);
                    }
                }

                if (semResult == 0) {

                    // We have seen it, signal, cause there might be other  interested in this lock.
		    VARIABLE_IS_NOT_USED int result = sem_post(_doorBell);

		    ASSERT((result == 0) || (errno == EOVERFLOW));
                }
                result = semResult == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT;
#else

                struct timespec structTime;

                clock_gettime(CLOCK_REALTIME, &structTime);
                structTime.tv_nsec += ((waitTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
                structTime.tv_sec += (waitTime / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
                structTime.tv_nsec = structTime.tv_nsec % 1000000000;

                if (sem_timedwait(_doorBell, &structTime) == 0) {
                    // We have seen it, signal, cause there might be other  interested in this lock.
                    sem_post(_doorBell);
                    result = Core::ERROR_NONE;
                }
                else if ( (errno == EINTR) || (errno == ETIMEDOUT) ) {
                    result = Core::ERROR_TIMEDOUT;
                }
                else {
                    ASSERT(false);
                }
#endif
            }
			return (result);
		}
		void DoorBell::Acknowledge() {
#ifdef __WIN32__
			if (_doorBell != nullptr)
			{
				BOOL result = ::ResetEvent(_doorBell);

				ASSERT(result != FALSE);
			}
#else
			if (_doorBell != nullptr)
			{
				while (sem_trywait(_doorBell) == 0) /* intentionally left empty */;
			}
#endif
		}

}
} // namespace Solution::Core
