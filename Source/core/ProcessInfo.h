/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#ifndef __PROCESSINFO_H
#define __PROCESSINFO_H

#include <list>

#include "IIterator.h"
#include "Module.h"
#include "Portability.h"

namespace WPEFramework {
namespace Core {

    // On 64 bits deployments, this is probably a uint64_t, lets prepare for it :-)
    using process_t = uint32_t;

    class EXTERNAL ProcessInfo {
    public:
#ifdef __WINDOWS__
        enum scheduler {
            BATCH,
            IDLE,
            FIFO,
            ROUNDROBIN,
            OTHER
        };
#else
// There are platforms/kernels being used that do not support all
// scheduling types, like Horizon. Define a dummy value here..
#ifndef SCHED_IDLE
#define SCHED_IDLE 0x80000001
#endif

        enum scheduler {
            BATCH = SCHED_BATCH,
            FIFO = SCHED_FIFO,
            ROUNDROBIN = SCHED_RR,
            OTHER = SCHED_OTHER,
            IDLE = SCHED_IDLE
        };
#endif

        class EXTERNAL Iterator {
        public:
            // Get all processes
            Iterator();

            // Get the Child Processes with a name name from a Parent with a certain name
            Iterator(const string& parentname, const string& childname, const bool removepath);

            // Get the Child Processes with a name name from a Parent pid
            Iterator(const uint32_t parentPID, const string& childname, const bool removepath);

            // Get the Children of the given PID.
            Iterator(const uint32_t parentPID);

            Iterator(const Iterator& copy)
                : _pids(copy._pids)
                , _current(copy._current)
                , _index(copy._index)
            {
            }
            ~Iterator()
            {
            }

            Iterator& operator=(const Iterator& RHS)
            {
                _pids = RHS._pids;
                _current = RHS._current;
                _index = RHS._index;

                return (*this);
            }

        public:
            inline bool IsValid() const
            {
                return ((_index != 0) && (_index <= _pids.size()));
            }
            inline void Reset()
            {
                _index = 0;
                _current = _pids.begin();
            }
            bool Next()
            {
                if (_index <= _pids.size()) {
                    _index++;

                    if (_index != 1) {
                        _current++;
                    }
                }
                return (_index <= _pids.size());
            }
            inline ProcessInfo Current() const
            {
                ASSERT(IsValid() == true);

                return (ProcessInfo(*_current));
            }
            inline uint32_t Count() const
            {
                return (static_cast<uint32_t>(_pids.size()));
            }

        private:
            std::list<process_t> _pids;
            std::list<process_t>::iterator _current;
            uint32_t _index;
        };

    public:
        // Current Process Information
        ProcessInfo();

        // Specifice Process Info
        ProcessInfo(const process_t id);

        ProcessInfo(const ProcessInfo&);
        ProcessInfo& operator=(const ProcessInfo&);

        ~ProcessInfo();

    public:
        inline process_t Id() const
        {
            return (_pid);
        }

        inline Iterator Children() const
        {
            return (Iterator(_pid));
        }

        inline int8_t Priority() const
        {
#ifdef __WINDOWS__
            return (0);
#else
            errno = 0;
            int result = getpriority(PRIO_PROCESS, _pid);

            return (errno != 0 ? 0 : static_cast<int8_t>(result));
#endif
        }
        inline void Priority(const int8_t priority)
        {
#ifndef __WINDOWS__
            if (setpriority(PRIO_PROCESS, _pid, priority) == -1) {
                TRACE_L1("Failed to set priority. Error: %d", errno);
            }
#endif
        }
        inline scheduler Policy() const
        {
#ifdef __WINDOWS__
            return (OTHER);
#else
            errno = 0;
            int result = getpriority(PRIO_PROCESS, _pid);

            return (errno != 0 ? OTHER : static_cast<scheduler>(result));
#endif
        }
        inline void Policy(const scheduler priority)
        {
#ifndef __WINDOWS__
            if (setpriority(PRIO_PROCESS, _pid, priority) == -1) {
                TRACE_L1("Failed to set priority. Error: %d", errno);
            }
#endif
        }
        inline int8_t OOMAdjust() const
        {
#ifdef __WINDOWS__
            return (0);
#else
            int8_t result = 0;
            FILE* fp = fopen(_T("/proc/self/oom_adj"), _T("r"));

            if (fp) {
                int number;
                fscanf(fp, "%d", &number);
                fclose(fp);
                result = static_cast<uint8_t>(number);
            }
            return (result);
#endif
        }
        inline void OOMAdjust(const int8_t adjust)
        {
#ifndef __WINDOWS__
            FILE* fp = fopen(_T("/proc/self/oom_adj"), _T("w"));

            if (fp) {
                fprintf(fp, "%d", adjust);
                fclose(fp);
            }
#endif
        }

        inline bool IsActive() const
        {
#ifdef __WINDOWS__
            DWORD exitCode = 0;
            if ((_handle != 0) && (GetExitCodeProcess(_handle, &exitCode) != 0) && (exitCode == STILL_ACTIVE)) {
                return (true);
            }
            return (false);
#else
            return ((_pid == 0) || (::kill(_pid, 0) == 0) ? true : false);
#endif
        }

        uint64_t Allocated() const;
        uint64_t Resident() const;
        uint64_t Shared() const;
        uint64_t Jiffies() const;
        string Name() const;
        void Name(const string& name);
        string Executable() const;
        std::list<string> CommandLine() const;
        uint32_t Group(const string& groupName);
        string Group() const;
        void MarkOccupiedPages(uint32_t bitSet[], const uint32_t size) const;

        // Setting, or getting, the user can onl be done for the
        // current process, hence why they are static.
        static uint32_t User(const string& userName);
        static string User();
        static void FindByName(const string& name, const bool exact, std::list<ProcessInfo>& processInfos);

        void Dump() {
            // The initial customer deploying this functionality sends a Floating Point Exception signal to
            // this process to indicate that it detected a hang and it requires a Dump for PostMortem analyses.
            // Agree, if there is a real Floating Point Exception causing the PostMortem, it can not
            // be distinguished from this (The logged deactivation code later on might help to diffrentiate
            // the two use cases, but you need to check more for that).
            // Bottomline such a functionality was not yet available in Thunder. hence why we start, if we
            // mimic this functionality, with how this customer is using it.
            // Send the Process a SIG_FPE, to start the generation of a PostMortem dump!

#ifdef __WINDOWS__
#else
            ::kill(_pid, SIGFPE);
#endif
        }

    private:
        process_t _pid;
#ifdef __WINDOWS__
        HANDLE _handle;
#endif
    }; // class ProcessInfo

   class ProcessTree
   {
      public:
         explicit ProcessTree(const ProcessInfo& processInfo);

         void MarkOccupiedPages(uint32_t bitSet[], const uint32_t size) const;
         bool ContainsProcess(ThreadId pid) const;
         void GetProcessIds(std::list<ThreadId>& processIds) const;
         ThreadId RootId() const;
         uint64_t Jiffies() const;

      private:
         std::list<ProcessInfo> _processes;
   };

} // namespace Core
} // namespace WPEFramework

#endif // __PROCESSINFO_H
