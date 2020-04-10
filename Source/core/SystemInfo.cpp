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

#include "SystemInfo.h"
#include "FileSystem.h"
#include "NetworkInfo.h"
#include "NodeId.h"
#include "Number.h"
#include "Serialization.h"
#include "Sync.h"
#include "Trace.h"

#include <cstdio>
#include <ctime>
#include <fstream>

#ifdef __APPLE__
#import <mach/host_info.h>
#import <mach/mach_host.h>
#include <sys/sysctl.h>
#elif defined(__LINUX__)
#include <cinttypes>
#include <cstdint>
#include <sys/sysinfo.h>
#endif

namespace WPEFramework {
namespace Core {

    /* static */ SystemInfo SystemInfo::_systemInfo;

    static string ConstructUniqueId(
        const TCHAR DeviceId[],
        const uint8_t DeviceIdLength,
        const TCHAR SystemPrefix[],
        const uint8_t SystemPrefixLength,
        const uint8_t KeyLength)
    {
#ifdef __WINDOWS__
        TCHAR* buffer = reinterpret_cast<TCHAR*>(ALLOCA(KeyLength + 1));
#else
        TCHAR buffer[KeyLength + 1];
        buffer[0] = '\0';
#endif

        if (KeyLength == UINT8_MAX) {
            ::memcpy(buffer, SystemPrefix, SystemPrefixLength);

            ::memcpy(&buffer[SystemPrefixLength], DeviceId, DeviceIdLength);

            buffer[DeviceIdLength + SystemPrefixLength] = '\0';
        } else {
            ASSERT(KeyLength > SystemPrefixLength);

            if (KeyLength > SystemPrefixLength) {
                ::memcpy(buffer, SystemPrefix, SystemPrefixLength);

                if (KeyLength < (DeviceIdLength + SystemPrefixLength)) {
                    TRACE_L1("Losing uniqueness because the id is truncated from %d to the first %d chars of your given id!",
                        DeviceIdLength, (KeyLength - SystemPrefixLength))
                    ::memcpy(&buffer[SystemPrefixLength], DeviceId, KeyLength - SystemPrefixLength);
                } else {
                    if (KeyLength > (DeviceIdLength + SystemPrefixLength)) {
                        ::memset(&buffer[SystemPrefixLength], '0', KeyLength - (DeviceIdLength + SystemPrefixLength));
                    }

                    ::memcpy(&buffer[SystemPrefixLength + (KeyLength - (DeviceIdLength + SystemPrefixLength))],
                        DeviceId, DeviceIdLength);
                }

                buffer[KeyLength] = '\0';
            } else {
                buffer[0] = '\0';
            }
        }

        return (buffer);
    }

    static string ConstructHostname()
    {
        TCHAR buffer[128];

        gethostname(buffer, sizeof(buffer) - 1);

        return (buffer);
    }

#ifdef SYSTEM_PREFIX
    static const TCHAR _systemPrefix[] = _T(EXPAND_AND_QUOTE(SYSTEM_PREFIX));
#else
    static const TCHAR _systemPrefix[] = _T("WPE");
#endif

    // Use MAC address and let the framework handle the OTP ID.
    const uint8_t* SystemInfo::RawDeviceId() const
    {
        static uint8_t* MACAddress = nullptr;
        static uint8_t MACAddressBuffer[7];

        if (MACAddress == nullptr) {
            bool valid = false;
            Core::AdapterIterator adapters;

            while ((adapters.Next() == true) && (valid == false)) {
                uint8_t check = 1;
                adapters.MACAddress(&MACAddressBuffer[1], 6);
                while ((check <= 4) && (MACAddressBuffer[check] == 0)) {
                    check++;
                }
                valid = (check <= 4);
            }

            MACAddressBuffer[0] = 6;

            MACAddress = &MACAddressBuffer[0];
        }

        return MACAddress;
    }

    string SystemInfo::Id(const uint8_t RawDeviceId[], const uint8_t KeyLength)
    {
        string id;

        ToString(&RawDeviceId[1], RawDeviceId[0], false, id);

        return (ConstructUniqueId(id.c_str(), static_cast<uint8_t>(id.size()),
            _systemPrefix, sizeof(_systemPrefix) - 1,
            KeyLength));
    }

    SystemInfo::SystemInfo()
        : m_HostName(ConstructHostname())
        , m_lastUpdateCpuStats(0)
    {
#ifdef __LINUX__
#ifdef __APPLE__
        m_uptime = 0;
        m_totalram = 0;
        m_pageSize = 0;
        m_freeram = 0;
        m_prevCpuSystemTicks = 0;
        m_prevCpuUserTicks = 0;
        m_prevCpuIdleTicks = 0;
#else
        struct sysinfo info;
        sysinfo(&info);

        m_uptime = info.uptime;
        m_totalram = info.totalram;
        m_pageSize = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
        m_freeram = info.freeram;
#endif
#endif

        UpdateCpuStats();
    }

    SystemInfo::~SystemInfo()
    {
    }

    SystemInfo::MemorySnapshot::MemorySnapshot() {
        std::ifstream file("/proc/meminfo", std::ifstream::in);
        std::string line;
        while( std::getline(file, line).eof() == false && file.good() == true ) {
            std::stringstream stream(line);
            std::string key;
            stream >> key;
            if( key == "MemTotal:" ) {
                stream >> _total; 
            } else if( key == "MemFree:" ) {
                stream >> _free; 
            } else if( key == "MemAvailable:" ) {
                stream >> _available; 
            } else if( key == "Cached:" ) {
                stream >> _cached; 
            } else if( key == "SwapTotal:" ) {
                stream >> _swapTotal; 
            } else if( key == "SwapFree:" ) {
                stream >> _swapFree; 
            } else if( key == "SwapCached:" ) {
                stream >> _swapCached; 
            } 
        }
        file.close();
    }

    void SystemInfo::UpdateCpuStats() const
    {
#ifdef __APPLE__
        processor_cpu_load_info_t cpuLoad;
        mach_msg_type_number_t processorMsgCount;
        natural_t processorCount;

        uint64_t totalSystemTime = 0, totalUserTime = 0, totalIdleTime = 0;
        uint64_t tempTotalSystemTime = 0, tempTotalUserTime = 0, temoTotalIdleTime = 0;

        kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount, (processor_info_array_t*)&cpuLoad, &processorMsgCount);
        if (err == KERN_SUCCESS) {
            for (natural_t i = 0; i < processorCount; i++) {
                tempTotalSystemTime += cpuLoad[i].cpu_ticks[CPU_STATE_SYSTEM];
                tempTotalUserTime += cpuLoad[i].cpu_ticks[CPU_STATE_USER] + cpuLoad[i].cpu_ticks[CPU_STATE_NICE];
                temoTotalIdleTime += cpuLoad[i].cpu_ticks[CPU_STATE_IDLE];
            }
        } else {
            TRACE_L1("host_processor_info call failed [%d]", __LINE__);
        }
        totalSystemTime = tempTotalSystemTime - m_prevCpuSystemTicks;
        totalUserTime = tempTotalUserTime - m_prevCpuUserTicks;
        totalIdleTime = temoTotalIdleTime - m_prevCpuIdleTicks;

        m_prevCpuSystemTicks = tempTotalSystemTime;
        m_prevCpuUserTicks = tempTotalUserTime;
        m_prevCpuIdleTicks = temoTotalIdleTime;

        m_cpuload = ((totalSystemTime + totalUserTime) * 100) / (totalSystemTime + totalUserTime + totalIdleTime);
#elif defined(__LINUX__)

        static uint64_t previousTickCount, previousIdleTime;

        // Update once a second to limit file system reads.
        if (difftime(time(nullptr), m_lastUpdateCpuStats) >= RefreshInterval) {
            FILE* input = fopen(_T("/proc/stat"), _T("r"));

            ASSERT(input && "ERROR: Unable to open /proc/stat");

            // First line of /proc/stat contains the overall CPU information
            uint64_t CpuFields[4];

            int numFields = fscanf(input, _T("cpu %llu %llu %llu %llu"),
                &CpuFields[0], &CpuFields[1],
                &CpuFields[2], &CpuFields[3]);

            fclose(input);

            ASSERT((numFields >= 4) && "ERROR: Read invalid CPU information.");

            uint64_t CurrentIdleTime = CpuFields[3]; // 3 is index of idle ticks time
            uint64_t CurrentTickCount = 0L;

            for (int i = 0; i < numFields && i < 10; ++i) {
                CurrentTickCount += CpuFields[i];
            }
            uint64_t DeltaTickCount = CurrentTickCount - previousTickCount;
            uint64_t DeltaIdleTime = CurrentIdleTime - previousIdleTime;

            if (DeltaTickCount == 0)
                SystemInfo::m_cpuload = 0;
            else
                SystemInfo::m_cpuload = ((DeltaTickCount - DeltaIdleTime) * 100) / DeltaTickCount;

            SystemInfo::m_jiffies = CurrentTickCount;

            // Store current tick statistics for next cycle
            previousTickCount = CurrentTickCount;
            previousIdleTime = CurrentIdleTime;

            // Store last update time value
            time(&m_lastUpdateCpuStats);
        }

#endif
    }

#if defined(__LINUX__) && !defined(__APPLE__)


    // Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
    // Licensed under the BSD-3 License
 

    // GMT specific version of mktime(), taken from BSD source code
    /* Number of days per month (except for February in leap years). */
    static const int monoff[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    static int is_leap_year(int year)
    {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    }

    static int leap_days(int y1, int y2)
    {
        --y1;
        --y2;
        return (y2 / 4 - y1 / 4) - (y2 / 100 - y1 / 100) + (y2 / 400 - y1 / 400);
    }
   /*
    * Code adapted from Python 2.4.1 sources (Lib/calendar.py).
    */
    static time_t mktimegm(const struct tm* tm)
    {
        int year;
        time_t days;
        time_t hours;
        time_t minutes;
        time_t seconds;

        year = 1900 + tm->tm_year;
        days = 365 * (year - 1970) + leap_days(1970, year);
        days += monoff[tm->tm_mon];

        if (tm->tm_mon > 1 && is_leap_year(year))
            ++days;
        days += tm->tm_mday - 1;

        hours = days * 24 + tm->tm_hour;
        minutes = hours * 60 + tm->tm_min;
        seconds = minutes * 60 + tm->tm_sec;

        return seconds;
    }

#endif

    void SystemInfo::UpdateRealtimeInfo()
    {

#if defined(__APPLE__)
        vm_statistics_data_t vmstat;
        struct timeval time;
        int mib[2];
        size_t length;
        length = sizeof(m_totalram);

        mib[0] = CTL_HW;
        mib[1] = HW_MEMSIZE;
        sysctl(mib, 2, &m_totalram, &length, nullptr, 0);

        mib[0] = CTL_KERN;
        mib[1] = KERN_BOOTTIME;
        length = sizeof(time);
        sysctl(mib, 2, &time, &length, nullptr, 0);

        m_uptime = (Core::Time::Now().Ticks() - Core::Time(time).Ticks()) / (Core::Time::TicksPerMillisecond * 1000);

        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count) != KERN_SUCCESS) {
            TRACE_L1("vm_stat call failed [%d]", __LINE__);
        }
        m_freeram = vmstat.free_count * 4096;
#elif defined(__LINUX__)
        struct sysinfo info;
        sysinfo(&info);

        m_uptime = info.uptime;
        m_freeram = info.freeram;
        m_totalram = info.totalram;
#endif
    }

    void SystemInfo::SetTime(const Time& time)
    {
#ifdef __WINDOWS__
        ::SetSystemTime(&(time.Handle()));

#elif defined(__APPLE__)
        ASSERT("Time not set");
#else

        struct tm setTime;

        ::memcpy(&setTime, &(time.Handle()), sizeof(setTime));

        time_t value = mktimegm(&setTime);

#if defined(__GNU_LIBRARY__)
  #if (__GLIBC__ >= 2) && (__GLIBC_MINOR__ > 30)
        timespec ts = {};
        ts.tv_sec = value;
        if (clock_settime(CLOCK_REALTIME, &ts) != 0){
  #else
        if (stime(&value) != 0) {
  #endif
#else
        if (stime(&value) != 0) {
#endif
            TRACE_L1("Failed to set system time [%d]", errno);
        } else {
            TRACE_L1("System time updated [%d]", errno);
        }
#endif
    }

    /* static */ bool SystemInfo::GetEnvironment(const string& name, string& value)
    {
#ifdef __LINUX__
        TCHAR* text = ::getenv(name.c_str());

        if (text != nullptr) {
            value = text;
        } else {
            value.clear();
        }

        return (text != nullptr);
#else
        TCHAR buffer[1024];
        DWORD bytes = GetEnvironmentVariable(name.c_str(), buffer, sizeof(buffer));

        value = string(buffer, bytes);

        return (bytes > 0);
#endif
    }

    /* static */ bool SystemInfo::SetEnvironment(const string& name, const TCHAR* value, const bool forced)
    {
#ifdef __LINUX__
        if ((forced == true) || (::getenv(name.c_str()) == nullptr)) {
            if (value != nullptr) {
                return (::setenv(name.c_str(), value, 1) == 0);
            } else {
                return (::unsetenv(name.c_str()));
            }
        }
#else
        if ((forced == true) || (GetEnvironmentVariable(name.c_str(), nullptr, 0) == 0)) {
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms686206(v=vs.85).aspx
            SetEnvironmentVariable(name.c_str(), value);
            return (true);
        }
#endif
        return (false);
    }

    /* static */ bool SystemInfo::SetEnvironment(const string& name, const string& value, const bool forced)
    {
        return SetEnvironment(name, value.c_str(), forced);
    }

#ifdef __WINDOWS__
    /* static */ SystemInfo& SystemInfo::Instance()
    {
        return (_systemInfo);
    }
#endif

    namespace System {

        extern "C" {

        uint32_t Reboot()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

#ifdef __WINDOWS__

            // What to do on windows ????
            ASSERT(false);

#else

            pid_t pid;

            if ((pid = fork()) == 0) {
                execlp("/sbin/reboot", "/sbin/reboot", nullptr);
                exit(1); // This should never be reached.
            } else if (pid != -1) {
                int32_t status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) == false) {
                    // reboot process did not exit sanely
                    result = Core::ERROR_UNAVAILABLE;
                }
                if (WEXITSTATUS(status) != 0) {
                    // reboot process exited with error;
                    // most likely the user lacks the required privileges
                    result = Core::ERROR_PRIVILIGED_REQUEST;
                } else {
                    // The init system is now shutting down the system. It will signals all
                    // programs to terminate by sending SIGTERM, followed by SIGKILL to
                    // programs that didn't terminate gracefully.
                    result = Core::ERROR_NONE;
                }
            }
#endif

            return (result);
        }
        }
    } // Extern "C":: System

} // namespace Core
} // namespace WPEFramework
