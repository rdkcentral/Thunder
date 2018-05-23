#ifndef __PROCESSOR_H
#define __PROCESSOR_H

#include "Portability.h"
#include "Queue.h"
#include "Thread.h"

namespace WPEFramework {
namespace Core {

    template <typename CONTEXT>
    class ProcessorType {
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
            ThreadUnitType(const ThreadUnitType<RUNCONTEXT>& a_Copy);
            ThreadUnitType<RUNCONTEXT>& operator=(const ThreadUnitType<RUNCONTEXT>& a_RHS);

        public:
            ThreadUnitType(QueueType<RUNCONTEXT>& queue, const TCHAR* processorName)
                : Thread(false, processorName)
                , m_Queue(queue)
                , m_Executing()
            {
                Run();
            }
            ~ThreadUnitType()
            {
            }

            bool Executing(const ProxyType<RUNCONTEXT>& thisElement) const
            {
                bool result = (m_Executing == thisElement);

                while (m_Executing == thisElement) {
                    // Seems like we are executing it.
                    // No fancy stuff, just give up the slice and try again later.
                    ::SleepMs(0);
                }

                return (result);
            }
            inline void ThreadName(const TCHAR* threadName)
            {
                Thread::ThreadName(threadName);
            }

        private:
            virtual uint32_t Worker()
            {
                if (m_Queue.Extract(m_Executing, Core::infinite) == true) {
                    // Seems like we have work...
                    m_Executing.Process();
                }

                // Do not wait keep on processing !!!
                return (0);
            }

        private:
            QueueType<RUNCONTEXT>& m_Queue;
            RUNCONTEXT m_Executing;
        };

        // -----------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error and/or link error.
        // -----------------------------------------------------------------------
    private:
        ProcessorType(const ProcessorType<CONTEXT>& a_Copy);
        ProcessorType<CONTEXT>& operator=(const ProcessorType<CONTEXT>& a_RHS);

    public:
        ProcessorType(const uint32_t waterMark, const TCHAR* processorName)
            : m_Queue(waterMark)
            , m_Unit(m_Queue, processorName)
        {
        }
        ~ProcessorType()
        {
            // Stop all threads...
            m_Unit.Block();

            m_Queue.Disable();

            // Wait till all threads have reached completion
            m_Unit.Wait(Thread::BLOCKED, Core::infinite);
        }

    public:
        inline void Block()
        {
            m_Queue.Disable();
            m_Unit.Block();
            m_Unit.Wait(Thread::BLOCKED | Thread::STOPPED);
        }
        inline void Run()
        {
            m_Queue.Enable();
            m_Unit.Run();
        }
        inline void Submit(const CONTEXT& data)
        {
            m_Queue.Post(data);
        }
        void Revoke(const ProxyType<CONTEXT>& data)
        {
            if (m_Queue.Remove(data) == false) {
                // Check if it is currently being executed and wait till it is done.
                m_Unit.Executing(data);
            }
        }

    private:
        QueueType<CONTEXT> m_Queue;
        ThreadUnitType<CONTEXT> m_Unit;
    };
}
} // namespace Core

#endif // __PROCESSOR_H
