#ifndef __PROCESSINFO_H
#define __PROCESSINFO_H

#include <list>

#include "Module.h"
#include "Portability.h"
#include "IIterator.h"

namespace WPEFramework {
namespace Core {
    class EXTERNAL ProcessInfo {
    public:
#ifdef __WIN32__
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
            // Get the Processes with this name.
            Iterator(const string& name, const bool exact);

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
                return (_pids.size());
            }

        private:
            std::list<uint32_t> _pids;
            std::list<uint32_t>::iterator _current;
            uint32_t _index;
        };

    public:
        // Current Process Information
        ProcessInfo();

        // Specifice Process Info
        ProcessInfo(const uint32_t id);

        ProcessInfo(const ProcessInfo&);
        ProcessInfo& operator=(const ProcessInfo&);

        ~ProcessInfo();

    public:
        inline uint32_t Id() const
        {
            return (_pid);
        }

        inline Iterator Children()
        {
            return (Iterator(_pid));
        }

		inline int8_t Priority() const {
#ifdef __WIN32__
			return (0);
#else
			errno = 0;
			int result = getpriority(PRIO_PROCESS, _pid);

			return (errno != 0 ? 0 : static_cast<int8_t>(result));
#endif
		}
		inline void Priority(const int8_t priority) {
#ifndef __WIN32__
			if (setpriority(PRIO_PROCESS, _pid, priority) == -1) {
				TRACE_L1("Failed to set priority. Error: %d", errno);
			}
#endif
		}
		inline scheduler Policy() const {
#ifdef __WIN32__
			return (OTHER);
#else
			errno = 0;
			int result = getpriority(PRIO_PROCESS, _pid);

			return (errno != 0 ? OTHER : static_cast<scheduler>(result));
#endif
		}
		inline void Policy(const scheduler priority) {
#ifndef __WIN32__
			if (setpriority(PRIO_PROCESS, _pid, priority) == -1) {
				TRACE_L1("Failed to set priority. Error: %d", errno);
			}
#endif
		}
		inline int8_t OOMAdjust() const {
#ifdef __WIN32__
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
		inline void OOMAdjust(const int8_t adjust) {
#ifndef __WIN32__
			FILE* fp = fopen(_T("/proc/self/oom_adj"), _T("w"));

			if (fp) {
				fprintf(fp, "%d", adjust);
				fclose(fp);
			}
#endif
		}

        inline bool IsActive() const
        {
#ifdef __WIN32__
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
        string Name() const;
        string Executable() const;

    private:
        uint32_t _pid;
#ifdef __WIN32__
        HANDLE _handle;
#endif
    }; // class ProcessInfo
} // namespace Core
} // namespace WPEFramework

#endif // __PROCESSINFO_H
