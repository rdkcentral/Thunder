#ifndef __SYSTEMINFO_H
#define __SYSTEMINFO_H

#include "Module.h"
#include "Portability.h"
#include "Time.h"

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
        const uint8_t * RawDeviceId() const;

        static bool GetEnvironment(const string& name, string& value);
        static bool SetEnvironment(const string& name, const TCHAR * value, const bool forced = true);
        static bool SetEnvironment(const string& name, const string& value, const bool forced = true);

#ifdef __WIN32__
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

        inline double GetCpuLoad() const
        {
            UpdateCpuStats();
            return m_cpuload;
        }

        inline uint64_t GetTotalGpuRam()
        {
            UpdateTotalGpuRam();
            return m_totalgpuram;
        }

        inline uint64_t GetFreeGpuRam()
        {
            UpdateFreeGpuRam();
            return m_freegpuram;
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

/* Visual C++, courtesy of Dirk Michaelis */
#if _MSC_VER >= 1400 && (defined(_M_AMD64) || defined(_M_X64)) && !defined(HAVE_TICK_COUNTER)

#include <intrin.h>
#pragma intrinsic(__rdtsc)
        inline uint64_t Ticks() const
        {
            return (__rdtsc);
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

#if defined(_MSC_VER) && defined(_M_IA64) && !defined(HAVE_TICK_COUNTER)
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

    private:
        const string m_HostName;

        uint64_t m_totalram;
        mutable uint32_t m_uptime;
        mutable uint64_t m_freeram;
        mutable double m_cpuload;
        mutable time_t m_lastUpdateCpuStats;
        mutable uint64_t m_totalgpuram;
        mutable uint64_t m_freegpuram;

        void UpdateCpuStats() const;
        void UpdateRealtimeInfo();

        void UpdateTotalGpuRam();
        void UpdateFreeGpuRam();

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

#define MODULE_NAME_DECLARATION(buildref)                                                         \
    extern "C" {                                                                                  \
    namespace WPEFramework {                                                                         \
        namespace Core {                                                                      \
            namespace System {                                                                    \
                const char* MODULE_NAME = SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(MODULE_NAME);  \
                const char* ModuleName() { return (MODULE_NAME); }                                \
                const char* ModuleBuildRef() { return (SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(buildref)); }                        \
            }                                                                                     \
        }                                                                                         \
    }                                                                                             \
    } // extern "C" Core::System

#endif // __SYSTEMINFO_H
