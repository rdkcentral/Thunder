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

#include "ThreadPool.h"

#ifdef THUNDER_CRASH_HANDLER
#include <list>
WPEFramework::Core::ThreadPool::CrashMonitor::WorkerThreadInfo* WPEFramework::Core::ThreadPool::CrashMonitor::_workerThreadInfo = NULL;
uint8_t WPEFramework::Core::ThreadPool::CrashMonitor::_numberOfWorkerThread = 0;
WPEFramework::Core::ThreadPool::CrashMonitor::SigAction WPEFramework::Core::ThreadPool::CrashMonitor::_orig_act={0};
#endif /* THUNDER_CRASH_HANDLER */

namespace WPEFramework
{
    namespace Core
    {
#ifdef THUNDER_CRASH_HANDLER
        ThreadPool::CrashMonitor::CrashMonitor(std::list<Executor>& _units)
             : _threadlist(_units)
        {
            syslog(LOG_NOTICE, "CrashMonitor constructor.");
        }

        ThreadPool::CrashMonitor::~CrashMonitor()
        {
            DeleteCrashMonitor();
        }

        void ThreadPool::CrashMonitor::SetupCrashHandling()
        {
            _adminLock.Lock();
            SigAction current_act;

            sigaction(SIGSEGV, nullptr, &current_act);

            if (current_act.sa_sigaction != signal_segv)
            {
                SigAction sa;

                memset(&sa, 0, sizeof(SigAction));
                sigemptyset(&sa.sa_mask);
                sa.sa_sigaction = signal_segv;
                sa.sa_flags = SA_SIGINFO;
                _orig_act = current_act;
                sigaction(SIGSEGV,&sa,nullptr);
            }
            _adminLock.Unlock();
        }

        void ThreadPool::CrashMonitor::CreateCrashMonitor()
        {
            if (_threadlist.size())
            {
                std::list<WPEFramework::Core::ThreadPool::Executor>::iterator ptr = _threadlist.begin();
                uint8_t count = 0;

                syslog(LOG_NOTICE, "Updating the thread ids in threadlist array.");

                _numberOfWorkerThread = _threadlist.size();

                _workerThreadInfo = new WPEFramework::Core::ThreadPool::CrashMonitor::WorkerThreadInfo[_numberOfWorkerThread];
                ASSERT( NULL != _workerThreadInfo);

                for (ptr = _threadlist.begin(), count = 0; ptr!=_threadlist.end(); ptr++, count++)
                {
                    _workerThreadInfo[count].threadId = ptr->Id();
                    _workerThreadInfo[count].jsonRPCString = "";
                }
            }
        }

        void ThreadPool::CrashMonitor::StoreRequestString(::ThreadId threadId, const string& jsonString)
        {
            ASSERT( NULL != _workerThreadInfo);

            for (uint8_t  index = 0; index < _numberOfWorkerThread; ++index)
            {
                if (threadId == _workerThreadInfo[index].threadId)
                {
                  _workerThreadInfo[index].jsonRPCString = jsonString;
                }
            }
        }

        void ThreadPool::CrashMonitor::DeleteCrashMonitor()
        {
            if (NULL != _workerThreadInfo)
            {
                delete[] _workerThreadInfo;
                _workerThreadInfo = NULL;
            }
        }

        void ThreadPool::CrashMonitor::DumpBacktrace()
        {
            void* callstack[32];
            uint32_t entries = 0;

            entries = backtrace(callstack, (sizeof(callstack) / sizeof(callstack[0])));
            char** symbols = backtrace_symbols(callstack, entries);

            for (uint32_t index = 0; index < entries; index++)
            {
                char  buffer[1024];
                Dl_info info;

                if (dladdr(callstack[index], &info) && info.dli_sname)
                {
                    char* demangled = NULL;
                    int status = -1;

                    if (info.dli_sname[0] == '_')
                    {
                        demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
                    }
                    snprintf(buffer, sizeof(buffer), "%-3d %*p %s + %zd\n", index, int(2 + sizeof(void*) * 2), callstack[index],
                        status == 0 ? demangled : info.dli_sname == 0 ? symbols[index] : info.dli_sname,
                        (char*)callstack[index] - (char*)info.dli_saddr);
                    free(demangled);
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), "%-3d %*p %s\n", index, int(2 + sizeof(void*) * 2), callstack[index], symbols[index]);
                }
                syslog(LOG_ERR, "%s", buffer);
            }
            free(symbols);
        }

        void ThreadPool::CrashMonitor::signal_segv(int signum, siginfo_t* info, void*ptr)
        {
            static bool handling_crash = false;

            if (handling_crash == true)
            {
                /* Crashed while in crash handler - proceed with crash */
                sigaction(SIGSEGV, &_orig_act, nullptr);
            }
            else
            {
                handling_crash = true;

                ucontext_t *p_uc = (ucontext_t *)ptr;
                pthread_t thread = pthread_self();

                syslog(LOG_ERR, "###### Crash Monitor Signal Handler ######");

                syslog(LOG_ERR, "Signal received %d. in process [%d] thread id[%u]", signum, getpid(), (uint32_t)thread);

                ASSERT( NULL != _workerThreadInfo);

                for (uint8_t  index = 0; index < _numberOfWorkerThread; ++index)
                {
                    if (thread == _workerThreadInfo[index].threadId)
                    {
                        syslog(LOG_ERR, "Crashed in worker thread");
                        syslog(LOG_ERR, "###### Dumping the Json RPC Request ######");

                        if (!(_workerThreadInfo[index].jsonRPCString.empty()))
                        {
                            syslog(LOG_ERR, "Json RPC Request: %s", _workerThreadInfo[index].jsonRPCString.c_str());
                        }
                        else
                        {
                            syslog(LOG_ERR, "Json RPC Request not present");
                        }

                        break;
                    }
                }

                syslog(LOG_ERR, "###### Dumping the Back Trace ######");
                DumpBacktrace();

                syslog(LOG_ERR, "###### Dumping the RegisterInfo ######");
                if(p_uc)
                {
                    show_context(p_uc);
                }
                else
                {
                    syslog(LOG_ERR, ("ucontext_t is NULL"));
                }

                sigaction(SIGSEGV, &_orig_act, nullptr);
                syslog(LOG_ERR, "Restored original handler.");
                handling_crash = false;

                syslog(LOG_ERR, "##########################################");
            }

            return;
        }
        void ThreadPool::CrashMonitor::show_context(const ucontext_t *p_uc)
        {
#if defined (__arm__)
            if(p_uc)
            {
                syslog(LOG_ERR, "r0 : %08x\tr1 : %08x", p_uc->uc_mcontext.arm_r0, p_uc->uc_mcontext.arm_r1);
                syslog(LOG_ERR, "r2 : %08x\tr3 : %08x", p_uc->uc_mcontext.arm_r2, p_uc->uc_mcontext.arm_r3);
                syslog(LOG_ERR, "r4 : %08x\tr5 : %08x", p_uc->uc_mcontext.arm_r4, p_uc->uc_mcontext.arm_r5);
                syslog(LOG_ERR, "r6 : %08x\tr7 : %08x", p_uc->uc_mcontext.arm_r6, p_uc->uc_mcontext.arm_r7);
                syslog(LOG_ERR, "r8 : %08x\tr9 : %08x", p_uc->uc_mcontext.arm_r8, p_uc->uc_mcontext.arm_r9);
                syslog(LOG_ERR, "r10: %08x\tfp : %08x", p_uc->uc_mcontext.arm_r10, p_uc->uc_mcontext.arm_fp);
                syslog(LOG_ERR, "ip : %08x\tsp : %08x", p_uc->uc_mcontext.arm_ip, p_uc->uc_mcontext.arm_sp);
                syslog(LOG_ERR, "lr : %08x\tpc : %08x", p_uc->uc_mcontext.arm_lr, p_uc->uc_mcontext.arm_pc);
            }
            else
            {
                syslog(LOG_ERR, "p_uc is NULL");
            }
#else
            syslog(LOG_ERR, "uc_mcontext decoding not supported on this architecture");
#endif /* __arm__ */
        }
#endif /* THUNDER_CRASH_HANDLER */
    }
}
