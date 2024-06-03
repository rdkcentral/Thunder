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
 
#ifndef __THREAD_H
#define __THREAD_H

#include <sstream>

#include "IAction.h"
#include "Module.h"
#include "Portability.h"
#include "Proxy.h"
#include "Queue.h"
#include "StateTrigger.h"
#include "Sync.h"
#include "TextFragment.h"
#include "Time.h"

namespace Thunder {
namespace Core {
    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType {
    private:
#ifdef __POSIX__
        static void destruct(void* value)
        {
            if (value != nullptr) {
                delete reinterpret_cast<THREADLOCALSTORAGE*>(value);
            }
        }
#endif
        void Delete() { 
#ifdef __WINDOWS__
            if (m_Index != -1) {
                void* l_Result = TlsGetValue(m_Index);
                delete reinterpret_cast<THREADLOCALSTORAGE*>(l_Result);
                TlsFree(m_Index);
            }
#endif

#ifdef __POSIX__
          pthread_key_delete(m_Key);
#endif

        }

        ThreadLocalStorageType()
        {
            TRACE_L5("Constructor ThreadControlBlockInfo <%p>", (this));

#ifdef __WINDOWS__
            m_Index = TlsAlloc();

            ASSERT(m_Index != -1);
#endif

#ifdef __POSIX__
            if (pthread_key_create(&m_Key, &destruct) != 0) {
                ASSERT(false);
            }
#endif
        }

    public:

        // not very convenient if these can be copied, it will delete the key
        ThreadLocalStorageType(const ThreadLocalStorageType&) = delete;
        ThreadLocalStorageType& operator=(const ThreadLocalStorageType&) = delete;

        ~ThreadLocalStorageType()
        {
            TRACE_L5("Destructor ThreadControlBlockInfo <%p>", (this));
            Delete();
        }

        static ThreadLocalStorageType<THREADLOCALSTORAGE>& Instance()
        {
            static ThreadLocalStorageType<THREADLOCALSTORAGE> g_Singleton;

            return (g_Singleton);
        }

        THREADLOCALSTORAGE& Context()
        {
#ifdef __WINDOWS__
            void* l_Result = TlsGetValue(m_Index);

            if (l_Result == nullptr) {
                l_Result = new THREADLOCALSTORAGE;
                TlsSetValue(m_Index, l_Result);
            }
#endif

#ifdef __POSIX__
            void* l_Result = pthread_getspecific(m_Key);

            if (l_Result == nullptr) {
                l_Result = new THREADLOCALSTORAGE;
                pthread_setspecific(m_Key, l_Result);
            }
#endif

            return *(reinterpret_cast<THREADLOCALSTORAGE*>(l_Result));
        }

        bool IsSet() const {
            bool isset = false;
#ifdef __WINDOWS__
            void* l_Result = TlsGetValue(m_Index);
            isset = (l_Result != nullptr);
#endif

#ifdef __POSIX__
            void* l_Result = pthread_getspecific(m_Key);
            isset = (l_Result != nullptr);
#endif
            return isset;
        }

    private:
#ifdef __WINDOWS__
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
            STOPPING = 0x0040,
            FAILED = 0x0080

        } thread_state;

        static uint32_t DefaultStackSize()
        {
            return (_defaultStackSize);
        }

        static void DefaultStackSize(const uint32_t defaultStackSize)
        {
            _defaultStackSize = defaultStackSize;
        }

    public:

        #ifdef __CORE_EXCEPTION_CATCHING__
        struct IExceptionCallback
        {
            virtual ~IExceptionCallback() = default;
            virtual void Exception(const string& message) = 0;
        };

        static void ExceptionCallback(IExceptionCallback* callback)
        {
            ASSERT(( callback != nullptr ) ^ ( _exceptionHandler != nullptr ));
            _exceptionHandler = callback;
        }
        #endif
                
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
        inline bool IsFailed() const
        {
            return (m_enumState == FAILED);
        }
        int PriorityMin() const;
        int PriorityMax() const;
        bool Priority(int priority);
        inline ::ThreadId Id() const
        {
            return (m_ThreadId);
        }
        static ::ThreadId ThreadId();

        template <typename STORAGETYPE>
        static STORAGETYPE& GetContext()
        {
            Core::ThreadLocalStorageType<STORAGETYPE>& block = Core::ThreadLocalStorageType<STORAGETYPE>::Instance();

            return (block.Context());
        }
        void Signal(const int signal) const;

#ifdef __DEBUG__
        int GetCallstack(void** buffer, int size);
#endif

    protected:
        virtual uint32_t Initialize();
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
#ifdef __WINDOWS__
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
#endif

#ifdef __WINDOWS__
        Event m_sigExit;
        thread_state m_enumSuspendedState;
        HANDLE m_hThreadInstance;
#endif

        ::ThreadId m_ThreadId;
        static uint32_t _defaultStackSize;

#ifdef __CORE_EXCEPTION_CATCHING__
        static IExceptionCallback* _exceptionHandler;
#endif

    };

}
} // namespace Core

#endif // __THREAD_H
