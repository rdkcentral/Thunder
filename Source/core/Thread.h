// ===========================================================================
//
// Filename:    thread.h
//
// Description: Header file for the Posix thread functions. This class
//              encapsulates all posix thread functionality defined by the
//              system.
//              Note: once the thread is started, it can be stopped only
//                    once. Since there is not a good Posix interface to
//                    suspend a thread and to resume it again, the class
//                    shows this limited, one time start, one time stop
//                    ability. The stop is actually a pthread_cancel call.
//
// History
//
// Author        Reason                                             Date
// ---------------------------------------------------------------------------
// P. Wielders   Initial creation                                   2002/05/24
//
// ===========================================================================
#ifndef __THREAD_H
#define __THREAD_H

#include <sstream>

#include "Module.h"
#include "Sync.h"
#include "StateTrigger.h"
#include "Queue.h"
#include "Time.h"
#include "TextFragment.h"
#include "IAction.h"
#include "Proxy.h"

#ifdef __LINUX__
#define CALLSTACK_SIG SIGUSR2
#endif // __LINUX__

namespace WPEFramework {
namespace Core {
    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType {
        friend class Thread;

    private:
        ThreadLocalStorageType()
        {
            TRACE_L5("Constructor ThreadControlBlockInfo <0x%X>", TRACE_POINTER(this));

#ifdef __WIN32__
            m_Index = TlsAlloc();

            ASSERT(m_Index != -1);
#endif

#ifdef __POSIX__
            if (pthread_key_create(&m_Key, nullptr) != 0) {
                ASSERT(false);
            }
#endif
        }

    public:
        ~ThreadLocalStorageType()
        {
            TRACE_L5("Destructor ThreadControlBlockInfo <0x%X>", TRACE_POINTER(this));

#ifdef __WIN32__
            if (m_Index != -1) {
                TlsFree(m_Index);
            }
#endif

#ifdef __POSIX__
            pthread_key_delete(m_Key);
#endif
        }

        static ThreadLocalStorageType<THREADLOCALSTORAGE>& Instance()
        {
            static ThreadLocalStorageType<THREADLOCALSTORAGE> g_Singleton;

            return (g_Singleton);
        }

        THREADLOCALSTORAGE* Context()
        {
#ifdef __WIN32__
            void* l_Result = TlsGetValue(m_Index);
#endif

#ifdef __POSIX__
            void* l_Result = pthread_getspecific(m_Key);
#endif

            return reinterpret_cast<THREADLOCALSTORAGE*>(l_Result);
        }

        const THREADLOCALSTORAGE* Context() const
        {
#ifdef __WIN32__
            void* l_Result = TlsGetValue(m_Index);
#endif

#ifdef __POSIX__
            void* l_Result = pthread_getspecific(m_Key);
#endif

            return reinterpret_cast<const THREADLOCALSTORAGE*>(l_Result);
        }

    private:
        void Context(THREADLOCALSTORAGE* l_Info)
        {
#ifdef __WIN32__
            if (TlsSetValue(m_Index, &l_Info) == FALSE) {
                ASSERT(false);
            }
#endif

#ifdef __POSIX__
            if (pthread_setspecific(m_Key, &l_Info) != 0) {
                ASSERT(false);
            }
#endif
        }

    private:
#ifdef __WIN32__
        DWORD m_Index;
#endif
#ifdef __UNIX__
        pthread_key_t m_Key;
#endif
    };

    class EXTERNAL Thread {
        // -----------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error and/or link error.
        // -----------------------------------------------------------------------
    private:
        Thread(const Thread& a_Copy) = delete;
        Thread& operator=(const Thread& a_RHS) = delete;

    public:
        typedef enum {
            SUSPENDED = 0x0001,
            RUNNING = 0x0002,
            DEACTIVATE = 0x0004,
            BLOCKED = 0x0008,
            STOPPED = 0x0010,
            INITIALIZED = 0x0020,
            STOPPING = 0x0040

        } thread_state;

        static uint32_t DefaultStackSize () {
            return (_defaultStackSize);
        }

        static void DefaultStackSize (const uint32_t defaultStackSize) {
            _defaultStackSize = defaultStackSize;
        }
    public:
        Thread(const uint32_t stackSize = Thread::DefaultStackSize(), const TCHAR* threadName = nullptr);
        virtual ~Thread();

        void Suspend();
        void Block();
        void Stop();
        void Init();
        void Run();
        thread_state State() const;
        bool Wait(const unsigned int enumState, unsigned int nTime = Core::infinite) const;
        inline bool IsRunning() const
        {
            return (m_enumState == RUNNING);
        }
        inline bool IsBlocked() const
        {
            return (m_enumState == BLOCKED);
        }
        int PriorityMin() const;
        int PriorityMax() const;
        bool Priority(int priority);
        inline ::ThreadId Id() const
        {
#if defined(__WIN32__) || defined(__APPLE__)
            return (reinterpret_cast<::ThreadId>(m_ThreadId));
#else
            return (static_cast<::ThreadId>(m_ThreadId));
#endif
        }
        static ::ThreadId ThreadId();

        template <typename STORAGETYPE>
        static STORAGETYPE* GetContext()
        {
            Core::ThreadLocalStorageType<STORAGETYPE>& block = Core::ThreadLocalStorageType<STORAGETYPE>::Instance();

            return (block.GetContext());
        }
#ifdef __LINUX__
        inline void Signal(const int signal) const
        {
            ::pthread_kill(m_hThreadInstance, signal);
        }
#endif

#ifdef __DEBUG__
		int GetCallstack(void** buffer, int size);
#endif

    protected:
        virtual bool Initialize();
        virtual uint32_t Worker() = 0;
        void Terminate();
        bool State(thread_state enumState);
        void ThreadName(const char* threadName);

        inline void SignalTermination()
        {
            m_sigExit.Unlock();
        }
        inline void Lock() const
        {
            m_syncAdmin.Lock();
        }
        inline void Unlock() const
        {
            m_syncAdmin.Unlock();
        }

    private:
#ifdef __WIN32__
        static void StartThread(Thread* pObject);
#endif

#ifdef __POSIX__
        static void* StartThread(Thread* pObject);
#endif

    private:
        StateTrigger<thread_state> m_enumState;

        mutable CriticalSection m_syncAdmin;

#ifdef __POSIX__
        Event m_sigExit;
        pthread_t m_hThreadInstance;
        uint32_t m_ThreadId;
#endif

#ifdef __WIN32__
        Event m_sigExit;
        thread_state m_enumSuspendedState;
        HANDLE m_hThreadInstance;
        DWORD m_ThreadId;
#endif
        static uint32_t _defaultStackSize;
    };

    //
    // Description: Helper class to use pointers or proxies (if lifetime management needs to be automated)
    //              as a carrier to be executed by the threadpooltype.
    //
    template <typename CONTEXT>
    class ThreadContextType {
    public:
        inline ThreadContextType()
            : m_Content()
        {
        }
        inline ThreadContextType(const CONTEXT& content)
            : m_Content(content)
        {
        }
        inline ThreadContextType(const ThreadContextType<CONTEXT>& copy)
            : m_Content(copy.m_Content)
        {
        }
        inline ~ThreadContextType()
        {
        }
        inline ThreadContextType<CONTEXT>& operator=(const ThreadContextType<CONTEXT>& RHS)
        {
            m_Content = RHS.m_Content;

            return (*this);
        }
        inline void Process()
        {
            m_Content->Process();
        }
        CONTEXT& Context()
        {
            return (m_Content);
        }
        inline bool operator==(const ThreadContextType<CONTEXT>& RHS) const
        {
            return (m_Content == RHS.m_Content);
        }
        inline bool operator!=(const ThreadContextType<CONTEXT>& RHS) const
        {
            return !(operator==(RHS));
        }

    private:
        CONTEXT m_Content;
    };

    class Job {
    public:
        Job()
            : _job()
        {
        }
        Job(const ProxyType<IDispatch>& job)
            : _job(job)
        {

            ASSERT(_job.IsValid() == true);
        }
        Job(const Job& copy)
            : _job(copy._job)
        {
        }
        ~Job()
        {
            if (_job.IsValid() == true) {
		_job.Release();
	    }
        }

        Job& operator=(const Job& rhs)
        {
            _job = rhs._job;

            return (*this);
        }

    public:
        inline bool operator==(const Job& rhs) const
        {
            return (_job == rhs._job);
        }
        inline bool operator!=(const Job& rhs) const
        {
            return (!operator==(rhs));
        }
        void Dispatch()
        {

            ASSERT(_job.IsValid() == true);

            _job->Dispatch();

            _job.Release();
        }

    private:
        ProxyType<IDispatch> _job;
    };

 
    template <typename CONTEXT, const uint16_t THREADCOUNT, const uint32_t QUEUESIZE = 0x7FFFFFFF>
    class ThreadPoolType {
    private:
        template <typename RUNCONTEXT>
        class ThreadUnitType : public Thread {
            // -----------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error and/or link error.
            // -----------------------------------------------------------------------
        private:
            ThreadUnitType(const ThreadUnitType<RUNCONTEXT>& a_Copy) = delete;
            ThreadUnitType<RUNCONTEXT>& operator=(const ThreadUnitType<RUNCONTEXT>& a_RHS) = delete;

        public:
            ThreadUnitType(ThreadUnitType&& move)
                : _executing()
                , _queue(move._queue)
                , _run(0)
                , _active(false)
                , _signal(false, false)
            {
            }
            ThreadUnitType(QueueType<RUNCONTEXT>& queue, const TCHAR* threadName)
                : Thread(Thread::DefaultStackSize(), threadName)
                , _executing()
                , _queue(queue)
                , _run(0)
                , _active(false)
                , _signal(false, false)
            {
                Run();
            }
            ThreadUnitType(QueueType<RUNCONTEXT>& queue, const uint32_t stackSize, const TCHAR* threadName)
                : Thread(stackSize, threadName)
                , _executing()
                , _queue(queue)
                , _run(0)
                , _active(false)
                , _signal(false, false)
            {
                Run();
            }
 
            ~ThreadUnitType()
            {
            }

        public:
            // For debugging purpose only !!!!!
            inline bool IsActive() const
            {
                return (_active);
            }

            // For debugging purpose only !!!!!
            inline uint32_t Runs() const
            {
                return (_run);
            }
 
            uint32_t Executing(const RUNCONTEXT& thisElement, const uint32_t waitTime) const
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (thisElement == _executing) {

		    TRACE_L1("Revoking object is currently running [%d].",  _run);

                    // You can not wait on yourself to actually remove yourself. This will deadlock !!!
                    ASSERT (Thread::Id() != Thread::ThreadId());

                    result = _signal.Lock(waitTime);
                }

                return (result);
            }

       private:
            virtual uint32_t Worker()
            {
                _signal.PulseEvent();

                if (_queue.Extract(_executing, Core::infinite) == true) {

                    // Seems like we have work...
                    _executing.Dispatch();

                    // Clear it out, we processed it.
                    _executing = RUNCONTEXT();

                    _active = false;
                    _run++;

                    // Yield the processor, just to make sure that the gap, between the comparison 
                    // of the Executing(.....) ended up in the lock, before we pulse it :-)
                    ::SleepMs(0); 

                    // Do not wait keep on processing !!!
                    return (0);
                }

                // Oops queue disabled, wait for queue to start us again..
                return (Core::infinite);
            }

        private:
            RUNCONTEXT _executing;
            QueueType<RUNCONTEXT>& _queue;
            uint32_t _run;
            bool _active;
            mutable Core::Event _signal;
        };

    public:
       // -----------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error and/or link error.
        // -----------------------------------------------------------------------
    private:
        ThreadPoolType(const ThreadPoolType& a_Copy) = delete;
        ThreadPoolType& operator=(const ThreadPoolType& a_RHS) = delete;

    public:
        ThreadPoolType(const uint32_t stackSize = 0, const TCHAR* poolName = nullptr)
            : _queue(QUEUESIZE)
        {
            _units.reserve(THREADCOUNT);

            for (uint32_t teller = 0; teller < THREADCOUNT; teller++) {

                _units.emplace_back (_queue, stackSize, poolName);
            }
        }

        ~ThreadPoolType()
        {
            // Stop all threads...
            Block();

            _queue.Flush();

            // Wait till all threads have reached completion
            Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
        }

    public:
        inline uint8_t Count() const
        {
            return (THREADCOUNT);
        }
        inline uint32_t Pending() const
        {
            return (_queue.Length());
        }
        inline uint32_t Active() const
        {
            uint32_t result = 0;

            // Make all threads active again !!
            for (uint16_t teller = THREADCOUNT; teller > 0; --teller) {
                if (_units[teller - 1].IsActive() == true) {
                    result++;
                }
            }
            return (result);
        }
        inline void Submit(const CONTEXT& data, const uint32_t waitTime)
        {
            if (QUEUESIZE == ~0) {
                _queue.Post(data);
            }
            else {
                _queue.Insert(data, waitTime);
            }
        }
        uint32_t Revoke(const CONTEXT& data, const uint32_t waitTime = Core::infinite)
        {
            uint32_t result = Core::ERROR_NONE;

            if (_queue.Remove(data) == false) {
                uint16_t count = THREADCOUNT;

                // Check if it is currently being executed and wait till it is done.
                while ((count > 0) && ((result = _units[count - 1].Executing(data, waitTime)) == Core::ERROR_UNAVAILABLE)) {
                    --count;
                }
            }
            else {
	        TRACE_L1("Found the revoking object in the queue: %d", waitTime);
            }

            return (result);
        }

        bool Wait(const unsigned int enumState, unsigned int nTime = Core::infinite) const
        {
            uint16_t teller = THREADCOUNT;

            // Block all threads!!
            while ((teller > 0) && (_units[teller - 1].Wait(enumState, nTime) == true)) {
                teller--;
            }

            return (teller == 0);
        }

        void Block()
        {
            // Block all threads!!
            for (uint16_t teller = THREADCOUNT; teller > 0; --teller) {
                _units[teller - 1].Block();
            }

            _queue.Disable();
        }

        void Run()
        {
            // Make all threads active again !!
            for (uint16_t teller = THREADCOUNT; teller > 0; --teller) {
                _units[teller - 1].Run();
            }
        }
        const ThreadUnitType<CONTEXT>& operator[](const uint32_t index) const
        {
            return (_units[index]);
        }

    private:
        QueueType<CONTEXT> _queue;
        std::vector< ThreadUnitType<CONTEXT> > _units;
    };

 
    // template <typename CONTEXT, const uint16_t THREADCOUNT, const uint32_t QUEUESIZE>
    // CONTEXT typename ThreadPoolType<CONTEXT,THREADCOUNT,QUEUESIZE>::s_EmptyContext;
}
} // namespace Core

#endif // __THREAD_H
