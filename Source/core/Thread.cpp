#include "Thread.h"
#include "Proxy.h"
#include "Trace.h"
#include "Serialization.h"

#ifdef __WIN32__
#include <process.h>
#endif

//-----------------------------------------------------------------------------------------------
// CLASS: Thread
//-----------------------------------------------------------------------------------------------

namespace WPEFramework {
namespace Core {

    /* static */ uint32_t Thread::_defaultStackSize = 0;

    Thread::Thread(const uint32_t stackSize, const TCHAR* threadName)
        : m_enumState(BLOCKED)
        , m_syncAdmin()
        , m_sigExit(false, true)

#ifdef __WIN32__
        , m_enumSuspendedState(BLOCKED)
        , m_hThreadInstance(nullptr)
        , m_ThreadId(0)
#else
        , m_hThreadInstance()
        , m_ThreadId(0)
#endif
    {
        TRACE_L5("Constructor Thread <0x%X>", TRACE_POINTER(this));

// Create a worker that can do actions in parallel
#ifdef __WIN32__

        m_hThreadInstance = ::CreateThread(nullptr,
            0,
            (LPTHREAD_START_ROUTINE)Thread::StartThread,
            (LPVOID) this,
            0,
            &m_ThreadId);

        // If there is no thread, the "new" thread can also not free the destructor,
        // then it is up to us.
        if (m_hThreadInstance == nullptr)
#endif

#ifdef __POSIX__
        int err;

        pthread_attr_t attr;

        err = pthread_attr_init(&attr);
        ASSERT (err == 0);

        if ((err == 0) && (stackSize != 0)) {
            size_t new_size = (stackSize < PTHREAD_STACK_MIN) ? PTHREAD_STACK_MIN : stackSize;
            err = pthread_attr_setstacksize(&attr, new_size);
            ASSERT (err == 0);
        }

            // If there is no thread, the "new" thread can also not free the destructor,
            // then it is up to us.
            if ((err != 0) || (pthread_create(&m_hThreadInstance, &attr, (void* (*)(void*))Thread::StartThread, this) == -1))
#endif
            {
                // Creation failed, O.K. We will signal the inactive state our selves.
                m_sigExit.SetEvent();
            }

#ifdef __POSIX__
        err = pthread_attr_destroy(&attr);
        ASSERT (err == 0);

        m_ThreadId = (uint32_t)(size_t)m_hThreadInstance;
#endif

        if (threadName != nullptr) {
            std::string convertedName;
            Core::ToString(threadName, convertedName);

            ThreadName(convertedName.c_str());
        }
    }
    Thread::~Thread()
    {
        TRACE_L5("Destructor Thread <0x%X>", TRACE_POINTER(this));

        Terminate();
    }

	::ThreadId Thread::ThreadId()
    {
#ifdef __WIN32__
        return (reinterpret_cast<::ThreadId>(::GetCurrentThreadId()));
#else
        return static_cast<::ThreadId>(pthread_self());
#endif
    }

#ifdef __WIN32__
    void Thread::StartThread(Thread* cClassPointer)
#endif

#ifdef __POSIX__
        void* Thread::StartThread(Thread* cClassPointer)
#endif
    {
#ifdef __POSIX__
        // It is the responsibility of the main app to make sure all threads created are stopped properly.
        // No jumping and bailing out without a proper closure !!!!
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &mask, nullptr);
#endif

        StateTrigger<thread_state>& stateObject = cClassPointer->m_enumState;

        stateObject.WaitState(INITIALIZED | RUNNING | STOPPED | STOPPING, Core::infinite);

        if (((stateObject & (STOPPED | STOPPING)) == 0) && (cClassPointer->Initialize())) {
            CriticalSection& adminLock = cClassPointer->m_syncAdmin;

            // O.K. befor using the state, lock it.
            adminLock.Lock();

            if (stateObject == INITIALIZED) {
                cClassPointer->State(BLOCKED);
            }

            // Do the work that is requested.
            while ((stateObject & (STOPPED | STOPPING)) == 0) {
                unsigned int delayed = Core::infinite;

                if (stateObject == RUNNING) {
                    // O.K. befor using the state, lock it.
                    adminLock.Unlock();

                    delayed = cClassPointer->Worker();

                    // Change the state, we are done with it.
                    adminLock.Lock();

                    if (stateObject == DEACTIVATE) {
                        cClassPointer->State(BLOCKED);
                    }
                }

                // O.K. before we wait for a new state unlock the current stuff.
                adminLock.Unlock();

                // Wait till we reach a runnable state
                stateObject.WaitState(RUNNING | STOPPED | STOPPING, delayed);

                // Change the state, we are done with it.
                adminLock.Lock();

                // Check in which state we reached the criteria for the WaitState !!
                if (stateObject == BLOCKED) {
                    cClassPointer->State(RUNNING);
                }
            }

            // O.K. befor using the state, lock it.
            adminLock.Unlock();
        }

        cClassPointer->State(STOPPED);

        // Report that the worker is done by releasing the Signal sync mechanism.
        cClassPointer->m_sigExit.SetEvent();

#ifdef __POSIX__
        ::pthread_exit(nullptr);
        return (nullptr);
#endif

#ifdef __WIN32__
        ::ExitThread(0);
#endif
    }

    bool Thread::Initialize()
    {
        return (true);
    }

    void Thread::Terminate()
    {
        // Make sure the thread knows that it should stop.
        m_syncAdmin.Lock();

        State(STOPPING);

        m_syncAdmin.Unlock();

        // We have to wait till the thread is completely stopped. If we
        // would continue here, the memory allocated for this instance of
        // this class would be freed. If our worker thread would still
        // be running, it will still reference to the data stored in the
        // allocated memory for the instance of this class. In the mean
        // time this data might have been taken by another thread/procces
        // and thay might have changed the data with all consequences, so
        // let's wait until we are triggered that the thread is dead.
        // We do not want to be busy waiting so do it via a synchronisation
        // mechanism, e.g. the semaphore created during construction
        // time.

        m_sigExit.Lock(Core::infinite);

#ifdef __POSIX__
        void* l_Dummy;
        ::pthread_join(m_hThreadInstance, &l_Dummy);
#endif
    }

    void Thread::Suspend()
    {
#ifdef __POSIX__
        Block();
#else
        State(SUSPENDED);
#endif
    }

    void Thread::Init()
    {
        m_syncAdmin.Lock();

        State(INITIALIZED);

        m_syncAdmin.Unlock();
    }

    void Thread::Stop()
    {
        m_syncAdmin.Lock();

        State(STOPPING);

        m_syncAdmin.Unlock();
    }

    void Thread::Block()
    {
        m_syncAdmin.Lock();

        State(DEACTIVATE);

        m_syncAdmin.Unlock();
    }

    void Thread::Run()
    {
        m_syncAdmin.Lock();

        State(RUNNING);

        m_syncAdmin.Unlock();
    }

    int Thread::PriorityMin() const
    {
#ifdef __POSIX__
        return (sched_get_priority_min(SCHED_OTHER));
#else
        return (0);
#endif
    }

    int Thread::PriorityMax() const
    {
#ifdef __POSIX__
        return (sched_get_priority_max(SCHED_OTHER));
#else
        return (255);
#endif
    }

    bool Thread::Priority(int priority)
    {
        bool result = false;

#ifdef __POSIX__
        struct sched_param p;
        p.sched_priority = priority;
        pthread_setschedparam(m_hThreadInstance, SCHED_OTHER, &p);
        result = (0 == pthread_setschedparam(m_hThreadInstance, SCHED_OTHER, &p));
#endif

        return (result);
    }

    bool Thread::Wait(const unsigned int enumState, unsigned int nTime) const
    {
        return (m_enumState.WaitState(enumState, nTime));
    }

    Thread::thread_state Thread::State() const
    {
        // Return the current status of the worker thread.
        return (m_enumState);
    }

    bool
    Thread::State(Thread::thread_state enumNewState)
    {
        bool blOK = false;

        switch (m_enumState) {
        case INITIALIZED:
            blOK = ((enumNewState == RUNNING) || (enumNewState == BLOCKED) || (enumNewState == STOPPED) || (enumNewState == STOPPING));
            break;
        case SUSPENDED:
            blOK = ((enumNewState == RUNNING) || (enumNewState == STOPPED) || (enumNewState == STOPPING));
            break;
        case RUNNING:
            blOK = ((enumNewState == SUSPENDED) || (enumNewState == INITIALIZED) || (enumNewState == BLOCKED) || (enumNewState == DEACTIVATE) || (enumNewState == STOPPING) || (enumNewState == STOPPED));
            break;
        case DEACTIVATE:
            blOK = ((enumNewState == SUSPENDED) || (enumNewState == BLOCKED) || (enumNewState == RUNNING) || (enumNewState == STOPPING) || (enumNewState == STOPPED));
            break;
        case BLOCKED:
            blOK = ((enumNewState == SUSPENDED) || (enumNewState == INITIALIZED) || (enumNewState == RUNNING) || (enumNewState == STOPPING) || (enumNewState == STOPPED));
            break;
        case STOPPING:
            blOK = ((enumNewState == STOPPING) || (enumNewState == STOPPED));
            break;
        case STOPPED: // The STOPPED state is the end,
            // No changes possible anymore.
            blOK = (enumNewState == STOPPED);
            break;
        }

        if (blOK) {
#ifndef __POSIX__
            if (enumNewState == SUSPENDED) {
                m_enumSuspendedState = m_enumState;
                m_enumState.SetState(SUSPENDED);

                // O.K. Suspend this thread.
                ::SuspendThread(m_hThreadInstance);
            }
            else if (m_enumState == SUSPENDED) {
                m_enumState.SetState(m_enumSuspendedState);

                // Done in the suspended state, resume.
                ::ResumeThread(m_hThreadInstance);
            }
            else
#endif
            {
                m_enumState.SetState(enumNewState);
            }
        }

        return (blOK);
    }

    void Thread::ThreadName(const char* threadName VARIABLE_IS_NOT_USED)
    {
#ifdef __WIN32__
#ifdef __DEBUG__

#pragma pack(push, 8)

        struct tagTHREADNAME_INFO {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } info;

#pragma pack(pop)

        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = m_ThreadId;
        info.dwFlags = 0;

        __try {
            RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
#endif // __DEBUG__
#endif // __WIN32__
    }


#ifdef __DEBUG__
    int Thread::GetCallstack(void** buffer, int size)
    {
        return GetCallStack(m_hThreadInstance, buffer, size);
    }
#endif // __DEBUG

}
} // namespace Core
