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

#pragma once

#include "Module.h"
#include "IIterator.h"
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

    private:
        class Memory {
        public:
            explicit Memory(const process_t pid);
            ~Memory() = default;

            Memory(const Memory&);
            Memory& operator=(const Memory&);

        public:
            void MemoryStats();
            inline uint64_t USS() const
            {
                return _uss;
            }
            inline uint64_t PSS() const
            {
                return _pss;
            }
            inline uint64_t RSS() const
            {
                return _rss;
            }
            inline uint64_t VSS() const
            {
                return _vss;
            }
            inline uint64_t Shared() const
            {
                return _shared;
            }

        private:
            process_t _pid;

            uint64_t _uss;
            uint64_t _pss;
            uint64_t _rss;
            uint64_t _vss;
            uint64_t _shared;
        };

    public:
        class EXTERNAL Iterator {
        public:
            // Get all processes
            Iterator();

            // Get the Child Processes with a name name from a Parent with a certain name
            Iterator(const string& parentname, const string& childname, const bool removepath);

            // Get the Child Processes with a name name from a Parent pid
            Iterator(const process_t parentPID, const string& childname, const bool removepath);

            // Get the Children of the given PID.
            Iterator(const process_t parentPID);

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
            inline void Reset(bool start = true)
            {
                if (start) {
                    _index = 0;
                    _current = _pids.begin();
                } else {
                    _index = _pids.size() + 1;
                    _current = _pids.end();
                }
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
            bool Previous()
            {
                if (_index > 0) {
                    _index--;

                    if (_index > 0) {
                        _current--;
                    }
                }
                return (_index > 0);
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

            TCHAR buffer[128];
            snprintf(buffer, sizeof(buffer), "/proc/%d/oom_adj", _pid);

            FILE* fp = fopen(buffer, _T("r"));
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
            TCHAR buffer[128];
            snprintf(buffer, sizeof(buffer), "/proc/%d/oom_adj", _pid);

            FILE* fp = fopen(buffer, _T("w"));
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

        inline void Kill(const bool hardKill)
        {
            
#ifdef __WINDOWS__
            if (hardKill == true) {
                TerminateProcess(_handle, 1234);
            }
#else
            ::kill(_pid, (hardKill ? SIGKILL : SIGTERM));
#endif
        }
#ifdef __LINUX__
        /**
         * @brief After using this method user is supposed to retrieve memory stats via 
         *        methods below - USS, PSS, RSS, or VSS
         */
        inline void MemoryStats() const
        {
            _memory.MemoryStats();
        }
        inline uint64_t USS() const
        {
            return _memory.USS();
        }
        inline uint64_t PSS() const
        {
            return _memory.PSS();
        }
        inline uint64_t RSS() const
        {
            return _memory.RSS();
        }
        inline uint64_t VSS() const
        {
            return _memory.VSS();
        }
#endif
        uint64_t Allocated() const;
        uint64_t Resident() const;
        uint64_t Shared() const;

        string Name() const;
        void Name(const string& name);
        string Executable() const;
        std::list<string> CommandLine() const;

        static void FindByName(const string& name, const bool exact, std::list<ProcessInfo>& processInfos);

        void Dump()
        {
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
        mutable Memory _memory;
#ifdef __WINDOWS__
        HANDLE _handle;
#endif
    }; // class ProcessInfo

    class EXTERNAL ProcessCurrent : public ProcessInfo {
    public:
        ProcessCurrent(const ProcessInfo&) = delete;
        ProcessCurrent& operator=(const ProcessInfo&) = delete;

        ProcessCurrent()
            : ProcessInfo()
        {
        }
        ~ProcessCurrent() = default;

    public:
        string User() const;
        uint32_t User(const string& userName);
        string Group() const;
        uint32_t Group(const string& groupName);

    private:
        void SupplementryGroups(const string& userName);
    };

    class EXTERNAL ProcessTree {
    public:
        explicit ProcessTree(const ProcessInfo& processInfo);

        bool ContainsProcess(ThreadId pid) const;
        void GetProcessIds(std::list<ThreadId>& processIds) const;
        ThreadId RootId() const;

    private:
        std::list<ProcessInfo> _processes;
    };

} // namespace Core
} // namespace WPEFramework

