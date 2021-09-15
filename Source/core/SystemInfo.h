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
 
#ifndef __SYSTEMINFO_H
#define __SYSTEMINFO_H

#include "Module.h"
#include "Portability.h"
#include "Time.h"

#include <sstream>

#ifdef __LINUX__
#include <time.h>
#endif

namespace WPEFramework {
namespace Core {
    namespace System {

        extern "C" const char* MODULE_NAME;
        extern "C" EXTERNAL uint32_t Reboot();
    }

    class EXTERNAL SystemInfo {
    private:
        SystemInfo(const SystemInfo&) = delete;
        SystemInfo& operator=(const SystemInfo&) = delete;
        SystemInfo();

        static const uint32_t RefreshInterval = 1;

    public:
        string Id(const uint8_t RawDeviceId[], const uint8_t KeyLength);

        // First byte of the RawDeviceId is the length of the DeviceId to follow.
        const uint8_t* RawDeviceId() const;

        static bool GetEnvironment(const string& name, string& value);
        static bool SetEnvironment(const string& name, const TCHAR* value, const bool forced = true);
        static bool SetEnvironment(const string& name, const string& value, const bool forced = true);

#ifdef __WINDOWS__
        static SystemInfo& Instance();
#else
        inline static SystemInfo& Instance()
        {
            return (_systemInfo);
        }
#endif

        ~SystemInfo();

        void SetTime(const Time& time);

        inline const string& GetHostName() const
        {
            return m_HostName;
        }

        inline uint64_t GetTotalRam() const
        {
            return m_totalram;
        }

        inline uint32_t GetPageSize() const
        {
           return m_pageSize;
        }

        inline uint32_t GetPhysicalPageCount() const
        {
           return static_cast<uint32_t>(m_totalram / m_pageSize);
        }

        inline uint32_t GetUpTime()
        {
            UpdateRealtimeInfo();
            return m_uptime;
        }

        inline uint64_t GetFreeRam()
        {
            UpdateRealtimeInfo();
            return m_freeram;
        }
        
        inline uint64_t GetTotalSwap() 
        {
            UpdateRealtimeInfo();
            return m_totalswap;
        }

        inline uint64_t GetFreeSwap() 
        {
            UpdateRealtimeInfo();
            return m_freeswap;
        }        
 
        inline uint64_t GetCpuLoad() const
        {
            UpdateCpuStats();
            return m_cpuload;
        }

        inline uint64_t * GetCpuLoadAvg()
        {
            UpdateRealtimeInfo();
            return m_cpuloadavg;
        }

        inline uint64_t GetJiffies() const
        {
            UpdateCpuStats();
            return m_jiffies;
        }

        class EXTERNAL MemorySnapshot {
        public:
            MemorySnapshot(const MemorySnapshot& copy) = default;
            MemorySnapshot& operator=(const MemorySnapshot& copy) = default;
            ~MemorySnapshot() = default;

        private:
            MemorySnapshot();

            friend class SystemInfo;
        public:
            inline string AsJSON() const {
                std::ostringstream output;
                output << "{\n";
                output << "\"total:\"" << Total() << ",\n";
                output << "\"free:\"" << Free() << ",\n";
                output << "\"avialble:\"" << Available() << ",\n";
                output << "\"cached:\"" << Cached() << ",\n";
                output << "\"swaptotal:\"" << SwapTotal() << ",\n";
                output << "\"swapfree:\"" << SwapFree() << ",\n";
                output << "\"swapcached:\"" << SwapCached() << '\n';
                output << "}\n";
                return output.str();
            }

            inline uint64_t Total() const {
                return _total;
            }

            inline uint64_t Free() const {
                return _free;
            }

            inline uint64_t Available() const {
                return _available;
            }

            inline uint64_t Cached() const {
                return _cached;
            }

            inline uint64_t SwapTotal() const {
                return _swapTotal;
            }

            inline uint64_t SwapFree() const {
                return _swapFree;
            }

            inline uint64_t SwapCached() const {
                return _swapCached;
            }

        private:
            uint64_t _total{0};
            uint64_t _free{0};
            uint64_t _available{0};
            uint64_t _cached{0};
            uint64_t _swapTotal{0};
            uint64_t _swapFree{0};
            uint64_t _swapCached{0};
        };

        inline MemorySnapshot TakeMemorySnapshot() const {
            return MemorySnapshot();
        }

        /*
        * Pentium cycle counter
        */
#if defined(__GNUC__) && defined(__i386__) && !defined(HAVE_TICK_COUNTER)
        inline uint64_t Ticks() const
        {
            uint64_t ret;

            __asm__ __volatile__("rdtsc"
                                 : "=A"(ret));
            /* no input, nothing else clobbered */
            return ret;
        }

#define HAVE_TICK_COUNTER
#endif

/* Visual C++ -- thanks to Morten Nissov for his help with this */
#if _MSC_VER >= 1200 && _M_IX86 >= 500 && !defined(HAVE_TICK_COUNTER)
        inline uint64_t Ticks() const
        {
            LARGE_INTEGER retval;

            __asm {
                __asm __emit 0fh __asm __emit 031h /* hack for VC++ 5.0 */
                mov retval.HighPart, edx
                mov retval.LowPart, eax
            }
            return retval.QuadPart;
        }

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/*
        * X86-64 cycle counter
        */
#if defined(__GNUC__) && defined(__x86_64__) && !defined(HAVE_TICK_COUNTER)
        inline uint64_t Ticks() const
        {
            uint32_t a, d;
            asm volatile("rdtsc"
                         : "=a"(a), "=d"(d));
            return ((uint64_t)a) | (((uint64_t)d) << 32);
        }

#define HAVE_TICK_COUNTER
#endif

/* gcc */
#if defined(__GNUC__) && defined(__ia64__) && !defined(HAVE_TICK_COUNTER)

        inline uint64_t Ticks() const
        {
            ticks ret;

            __asm__ __volatile__("mov %0=ar.itc"
                                 : "=r"(ret));
            return ret;
        }

#define HAVE_TICK_COUNTER
#endif

/* Microsoft Visual C++ */
#if defined(_MSC_VER) && defined(_M_IA64) && !defined(HAVE_TICK_COUNTER)
        typedef unsigned __int64 ticks;

#ifdef __cplusplus
        extern "C"
#endif
            uint64_t
            __getReg(int whichReg);
#pragma intrinsic(__getReg)

        inline uint64_t Ticks() const
        {
            volatile uint64_t temp;
            temp = __getReg(3116);
            return temp;
        }

#define HAVE_TICK_COUNTER
#endif

#if defined(__GNUC__) && !defined(HAVE_TICK_COUNTER)
        inline uint64_t Ticks() const
        {
            struct timespec mytime;

            clock_gettime(CLOCK_MONOTONIC, &mytime);

            return (static_cast<uint64_t>(mytime.tv_nsec) + (static_cast<uint64_t>(mytime.tv_sec) * 1000000000UL));
        }

#define HAVE_TICK_COUNTER
#endif

#if defined(_MSC_VER) && !defined(HAVE_TICK_COUNTER)
        inline uint64_t Ticks() const
        {
            SYSTEMTIME systemTime;
            FILETIME fileTime;

            ::GetSystemTime(&systemTime);

            // Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
            ::SystemTimeToFileTime(&systemTime, &fileTime);
            _ULARGE_INTEGER result;

            result.LowPart = fileTime.dwLowDateTime;
            result.HighPart = fileTime.dwHighDateTime;

            // Return the time in MicroSeconds...
            return (result.QuadPart);
        }

#define HAVE_TICK_COUNTER
#endif

#if !defined(HAVE_TICK_COUNTER)
#error "Come up with an implementation of a High Resolution tick counter for the compiler you are using."
#endif

    public:
        const string Architecture() const;
        const string Chipset() const;
        const string FirmwareVersion() const;

    private:
        const string m_HostName;

        uint64_t m_totalram;
        uint32_t m_pageSize;
        mutable uint32_t m_uptime;
        mutable uint64_t m_freeram;
        mutable uint64_t m_totalswap;
        mutable uint64_t m_freeswap;
        mutable uint64_t m_cpuload;
        mutable uint64_t m_cpuloadavg[3];
        mutable uint64_t m_jiffies;
        mutable time_t m_lastUpdateCpuStats;

        void UpdateCpuStats() const;
        void UpdateRealtimeInfo();

        static SystemInfo _systemInfo;

#ifdef __APPLE__
        mutable uint64_t m_prevCpuSystemTicks;
        mutable uint64_t m_prevCpuUserTicks;
        mutable uint64_t m_prevCpuIdleTicks;
#endif
    }; // class SystemInfo
} // namespace Core
} // namespace WPEFramework

#define SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_1(parameter) #parameter
#define SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(parameter) SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_1(parameter)

#define MODULE_BUILDREF MODULE_NAME##Version

#define MODULE_NAME_DECLARATION(buildref)                                                                              \
    extern "C" {                                                                                                       \
    namespace WPEFramework {                                                                                           \
        namespace Core {                                                                                               \
            namespace System {                                                                                         \
                const char* MODULE_NAME = SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(MODULE_NAME);              \
                const char* ModuleName() { return (MODULE_NAME); }                                            \
                const char* ModuleBuildRef() { return (SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(buildref)); } \
            }                                                                                                          \
        }                                                                                                              \
    }                                                                                                                  \
    } // extern "C" Core::System

#define MODULE_NAME_ARCHIVE_DECLARATION                                                          \
    extern "C" {                                                                                 \
    namespace WPEFramework {                                                                     \
        namespace Core {                                                                         \
            namespace System {                                                                   \
                const char* MODULE_NAME = SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(MODULE_NAME); \
            }                                                                                    \
        }                                                                                        \
    }                                                                                            \
    } // extern "C" Core::System

#endif // __SYSTEMINFO_H
