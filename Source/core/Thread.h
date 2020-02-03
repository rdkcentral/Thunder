#ifndef __THREAD_H
#define __THREAD_H

#include <sstream>

#include "IAction.h"
#include "Module.h"
#include "Proxy.h"
#include "Queue.h"
#include "StateTrigger.h"
#include "Sync.h"
#include "TextFragment.h"
#include "Time.h"

namespace WPEFramework {
namespace Core {
    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType {
    private:
#ifdef __POSIX__
        static void destruct(void* value)
        {
            printf("Destructor ThreadControlBlockInfo <0x%p>\n", value);
            if (value != nullptr) {
                delete reinterpret_cast<THREADLOCALSTORAGE*>(value);
            }
        }
#endif

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
        ~ThreadLocalStorageType()
        {
            TRACE_L5("Destructor ThreadControlBlockInfo <%p>", (this));

#ifdef __WINDOWS__
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
            STOPPING = 0x0040

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
#if defined(__WINDOWS__) || defined(__APPLE__)
#pragma warning(disable : 4312)
            return (reinterpret_cast<const ::ThreadId>(m_ThreadId));
#pragma warning(default : 4312)
#else
            return (static_cast<::ThreadId>(m_ThreadId));
#endif
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
        uint32_t m_ThreadId;
#endif

#ifdef __WINDOWS__
        Event m_sigExit;
        thread_state m_enumSuspendedState;
        HANDLE m_hThreadInstance;
        DWORD m_ThreadId;
#endif
        static uint32_t _defaultStackSize;
    };

    class EXTERNAL ThreadPool {
    public:
        typedef Core::QueueType< Core::ProxyType<IDispatch> > MessageQueue;

        template<typename IMPLEMENTATION>
        class JobType {
        private:
            enum state : uint8_t {
                IDLE,
                SUBMITTED
            };

            class Worker : public Core::IDispatch {
            public:
                Worker() = delete;
                Worker(const Worker&) = delete;
                Worker& operator=(const Worker&) = delete;

                Worker(JobType<IMPLEMENTATION>& parent) : _parent(parent) {
                }
                ~Worker() override {
                }

            private:
                void Dispatch() override {
                    _parent.Dispatch();
                }

            private:
                 JobType<IMPLEMENTATION>& _parent;
            };

        public:
            JobType(const JobType<IMPLEMENTATION>& copy) = delete;
            JobType<IMPLEMENTATION>& operator=(const JobType<IMPLEMENTATION>& RHS) = delete;

            template <typename... Args>
            JobType(Args&&... args)
                : _implementation(args...)
                , _state(IDLE)
                , _job(*this)
            {
                _job.AddRef();
            }
            ~JobType()
            {
                ASSERT (_state == IDLE);
                _job.CompositRelease();
            }

        public:
            Core::ProxyType<Core::IDispatch> Aquire() {

                state expected = IDLE;
                Core::ProxyType<Core::IDispatch> result;

                if (_state.compare_exchange_strong(expected, SUBMITTED) == true) {
                    result = Core::ProxyType<Core::IDispatch>(&_job, &_job);
                }

                return (result);
            }
            Core::ProxyType<Core::IDispatch> Reset() {

                state expected = SUBMITTED;
                _state.compare_exchange_strong(expected, IDLE);

                return (Core::ProxyType<Core::IDispatch>(&_job, &_job));
            }
            operator IMPLEMENTATION& () {
                return (_implementation);
            }
            operator const IMPLEMENTATION& () const {
                return (_implementation);
            }

        private:
            void Dispatch()
            {
                state expected = SUBMITTED;
                if (_state.compare_exchange_strong(expected, IDLE) == true) {
                    _implementation.Dispatch();
                }
            }

        private:
            IMPLEMENTATION _implementation;
            std::atomic<state> _state;
            ProxyObject<Worker> _job;
        };

        class EXTERNAL Minion {
        public:
            Minion(const Minion&) = delete;
            Minion& operator=(const Minion&) = delete;

            Minion(MessageQueue& queue)
                : _queue(queue)
                , _adminLock()
                , _signal(false, false)
                , _interestCount(0)
                , _currentRequest()
                , _runs(0)
            {
            }
            ~Minion()
            {
            }

        public:
            uint32_t Runs() const {
                return (_runs);
            }
            bool IsActive() const {
                return (_currentRequest.IsValid());
            }
            uint32_t Completed (const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime) {
                uint32_t result = Core::ERROR_NONE;

                _adminLock.Lock();
                Core::InterlockedIncrement(_interestCount);
                if (_currentRequest != job) {
                    _adminLock.Unlock();
                }
                else {
                    _adminLock.Unlock();
                    result = _signal.Lock(waitTime);
                }

                Core::InterlockedDecrement(_interestCount);

                return(result);
            }
            void Process()
            {
                while (_queue.Extract(_currentRequest, Core::infinite) == true) {

                    _runs++;

                    _currentRequest->Dispatch();

                    // if someone is observing this run, (WaitForCompletion) make sure that
                    // thread, sees that his object was running and is now completed.
                    _adminLock.Lock();
                    if (_interestCount > 0) {

                        _signal.SetEvent();

                        while (_interestCount > 0) {
                            ::SleepMs(0);
                        }

                        _signal.ResetEvent();
                    }
                    _adminLock.Unlock();
                }
            }

        private:
            MessageQueue& _queue;
            Core::CriticalSection _adminLock;
            Core::Event _signal;
            uint32_t _interestCount;
            Core::ProxyType<Core::IDispatch> _currentRequest;
            uint32_t _runs;
        };

    private:
        class EXTERNAL Executor : public Core::Thread {
        public:
            Executor(const Executor&) = delete;
            Executor& operator=(const Executor&) = delete;

            Executor(MessageQueue* queue, const uint32_t stackSize, const TCHAR* name)
                : Core::Thread(stackSize == 0 ? Core::Thread::DefaultStackSize() : stackSize, name)
                , _minion(*queue)
            {
            }
            ~Executor() override
            {
                Wait(Core::Thread::STOPPED, Core::infinite);
            }

        public:
            uint32_t Runs() const {
                return (_minion.Runs());
            }
            bool IsActive() const {
                return (_minion.IsActive());
            }
            void Run () {
                Core::Thread::Run();
            }
            void Stop () {
                Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
            }
            Minion& Me() {
                return (_minion);
            }

        private:
            uint32_t Worker() override
            {
                _minion.Process();
                Core::Thread::Block();
                return (Core::infinite);
            }

        private:
            Minion _minion;
        };

    public:
        ThreadPool(const ThreadPool& a_Copy) = delete;
        ThreadPool& operator=(const ThreadPool& a_RHS) = delete;

        ThreadPool(const uint8_t count, const uint32_t stackSize, const uint32_t queueSize) 
            : _queue(queueSize)
        {
            const TCHAR* name = _T("WorkerPool::Thread");
            for (uint8_t index = 0; index < count; index++) {
                _units.emplace_back(&_queue, stackSize, name);
            }
        }
        ~ThreadPool() {
            _units.clear();
        }

    public:
        uint8_t Count() const
        {
            return (_units.size());
        }
        uint32_t Pending() const
        {
            return (_queue.Length());
        }
        void Runs(const uint8_t length, uint32_t* counters) const 
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while ((count < length) && (ptr != _units.cend())) { 
                counters[count] = ptr->Runs();
                ptr++; 
                count++; 
            }
        }
        uint8_t Active() const
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while (ptr != _units.cend()) 
            { 
                if (ptr->IsActive() == true) {
                    count++;
                }
                ptr++; 
            }

            return (count);
        }
        ::ThreadId Id(const uint8_t index) const
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while ((index != count) && (ptr != _units.cend())) { ptr++; count++; }

            ASSERT (ptr != _units.cend());

            return (ptr != _units.cend() ? ptr->Id() : 0);
        }
        void Submit(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime)
        {
            _queue.Insert(job, waitTime);
        }
        uint32_t Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime)
        {
            uint32_t result = Core::ERROR_NONE;

            _queue.Remove(job);

            // Check if it is currently being executed and wait till it is done.
            std::list<Executor>::iterator index = _units.begin();

            while (index != _units.end()) {
                uint32_t outcome = index->Me().Completed(job, waitTime);
                if (outcome != Core::ERROR_NONE) {
                    result = outcome;
                }
                index++;
            }

            return (result);
        }
        MessageQueue& Queue() {
            return (_queue);
        }
        void Run()
        {
            _queue.Enable();
            std::list<Executor>::iterator index = _units.begin();
            while (index != _units.end()) {
                index->Run();
                index++;
            }
        }
        void Stop()
        {
            _queue.Disable();
            std::list<Executor>::iterator index = _units.begin();
            while (index != _units.end()) {
                index->Stop();
                index++;
            }
        }

   private:
        MessageQueue _queue;
        std::list<Executor> _units;
    };
}
} // namespace Core

#endif // __THREAD_H
