#include "Number.h"
#include "SystemInfo.h"
#include "Trace.h"
#include "Serialization.h"
#include "NodeId.h"
#include "FileSystem.h"
#include "NetworkInfo.h"
#include "Sync.h"

#include <cstdio>
#include <ctime>


#ifdef __APPLE__
#include <sys/sysctl.h>
#import <mach/host_info.h>
#import <mach/mach_host.h>
#elif defined(__LINUX__)
#include <sys/sysinfo.h>
#include <cstdint>
#include <cinttypes>
#endif

#if defined(PLATFORM_RPI)
#include <bcm_host.h>
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
#ifdef __WIN32__
    TCHAR* buffer = reinterpret_cast<TCHAR*>(ALLOCA(KeyLength + 1));
#else
    TCHAR buffer[KeyLength + 1];
    buffer[0] = '\0';
#endif

    if (KeyLength == UINT8_MAX) {
        ::memcpy(buffer, SystemPrefix, SystemPrefixLength);

        ::memcpy(&buffer[SystemPrefixLength], DeviceId, DeviceIdLength);

        buffer[DeviceIdLength + SystemPrefixLength] = '\0';
    }
    else {
        ASSERT(KeyLength > SystemPrefixLength);

        if (KeyLength > SystemPrefixLength) {
            ::memcpy(buffer, SystemPrefix, SystemPrefixLength);

            if (KeyLength < (DeviceIdLength + SystemPrefixLength)) {
                TRACE_L1("Losing uniqueness because the id is truncated from %d to the first %d chars of your given id!",
                        DeviceIdLength, (KeyLength - SystemPrefixLength))
                ::memcpy(&buffer[SystemPrefixLength], DeviceId, KeyLength - SystemPrefixLength);
            }
            else {
                if (KeyLength > (DeviceIdLength + SystemPrefixLength)) {
                    ::memset(&buffer[SystemPrefixLength], '0', KeyLength - (DeviceIdLength + SystemPrefixLength));
                }

                ::memcpy(&buffer[SystemPrefixLength + (KeyLength - (DeviceIdLength + SystemPrefixLength))],
                DeviceId, DeviceIdLength);
            }

            buffer[KeyLength] = '\0';
        }
        else {
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

        while ( (adapters.Next() == true) && (valid == false) ) {
            uint8_t check = 1;
            adapters.MACAddress(&MACAddressBuffer[1], 6);
            while ( (check <= 4) && (MACAddressBuffer[check] == 0) ) { check++; } 
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
  
#if defined(PLATFORM_RPI)
    static std::string vcgencmd_request(const char request[])
    {
        // Most VC API calls are guarded but we want to be sure anyway
        static Core::CriticalSection mutualExclusion;
        mutualExclusion.Lock();

        char buffer[512];

        // Reset the string
        buffer[0] = '\0';

        int VARIABLE_IS_NOT_USED status = vc_gencmd(buffer, sizeof(buffer), &request[0]);
        assert((status == 0) && "Error: vc_gencmd failed.\n");

        // Make sure it is null-terminated
        buffer[sizeof(buffer) - 1] = '\0';

        // Create string from buffer.
        std::string response(buffer);

        mutualExclusion.Unlock();

        return response;
    }
#endif

    SystemInfo::SystemInfo()
        : m_HostName(ConstructHostname())
        , m_lastUpdateCpuStats(0)
    {
#ifdef __LINUX__
#ifdef __APPLE__
        m_uptime = 0;
        m_totalram = 0;
        m_freeram = 0;
        m_prevCpuSystemTicks = 0;
        m_prevCpuUserTicks = 0;
        m_prevCpuIdleTicks = 0;
#else
        struct sysinfo info;
        sysinfo(&info);

        m_uptime = info.uptime;
        m_totalram = info.totalram;
        m_freeram = info.freeram;
#endif
#endif

#if defined(PLATFORM_RPI)
        // Init GPU resources.
        // We can call it always. If we are not the first it does not do anything (harmful).
        // Moreover, it takes care of all underlying necessary API init's
        bcm_host_init();
#endif
        UpdateCpuStats();
    }

    SystemInfo::~SystemInfo()
    {
    }

    void SystemInfo::UpdateCpuStats() const
    {
#ifdef __APPLE__
        processor_cpu_load_info_t cpuLoad;
        mach_msg_type_number_t processorMsgCount;
        natural_t processorCount;

        uint64_t totalSystemTime = 0, totalUserTime = 0, totalIdleTime = 0;
        uint64_t tempTotalSystemTime = 0, tempTotalUserTime = 0, temoTotalIdleTime = 0;

        kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount, (processor_info_array_t *)&cpuLoad, &processorMsgCount);
        if(err == KERN_SUCCESS) {
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

        m_cpuload =  ((totalSystemTime + totalUserTime) * 100)/ (totalSystemTime + totalUserTime + totalIdleTime);
#elif defined(__LINUX__)

        static double previousTickCount, previousIdleTime;

        // Update once a second to limit file system reads.
        if (difftime(time(nullptr), m_lastUpdateCpuStats) >= RefreshInterval) {
            FILE* input = fopen(_T("/proc/stat"), _T("r"));

            ASSERT(input && "ERROR: Unable to open /proc/stat");

            // First line of /proc/stat contains the overall CPU information
            unsigned long long CpuFields[4];

            int numFields = fscanf(input, _T("cpu %llu %llu %llu %llu"),
                &CpuFields[0], &CpuFields[1],
                &CpuFields[2], &CpuFields[3]);

            fclose(input);

            ASSERT((numFields >= 4) && "ERROR: Read invalid CPU information.");

            unsigned long long CurrentIdleTime = CpuFields[3]; // 3 is index of idle ticks time
            unsigned long long CurrentTickCount = 0L;

            for (int i = 0; i < numFields && i < 10; ++i) {
                CurrentTickCount += CpuFields[i];
            }

            double DeltaTickCount = CurrentTickCount - previousTickCount;
            double DeltaIdleTime = CurrentIdleTime - previousIdleTime;

            SystemInfo::m_cpuload = ((DeltaTickCount - DeltaIdleTime) / DeltaTickCount) * 100;

            // Store current tick statistics for next cycle
            previousTickCount = CurrentTickCount;
            previousIdleTime = CurrentIdleTime;

            // Store last update time value
            time(&m_lastUpdateCpuStats);
        }

#endif
    }

    void SystemInfo::UpdateTotalGpuRam()
    {
        // Save result in gpu field.
        SystemInfo::m_totalgpuram = 0;
#if defined(PLATFORM_RPI)
        if (SystemInfo::m_totalgpuram != 0) {
            return;
        }

        std::string response = vcgencmd_request("get_mem reloc_total ");

        // Erase prefix in order to get value.
        std::size_t prefix = response.find("=");
        response.erase(0, prefix + 1);

        // Find unit of response and omit from response.
        std::string unit;
        std::size_t postfix = response.find_first_not_of("0123456789,. ");
        if (postfix != std::string::npos) {
            unit = response.substr(postfix, response.length());
            response.erase(postfix, response.length());
        }

        // Convert string to integer.
        SystemInfo::m_totalgpuram = NumberType<uint64_t>(TextFragment(response)).Value();

        // Convert into bytes, if necessary.
        if (unit.compare("M") == 0) {
            // Multiply with MB = 1024*1024.
            SystemInfo::m_totalgpuram *= 1048576;
        }
        else if (unit.compare("K") == 0) {
            // Multiply with KB = 1024.
            SystemInfo::m_totalgpuram *= 1024;
        }
#endif
    }

    void SystemInfo::UpdateFreeGpuRam()
    {
        // Save result in gpu field.
        SystemInfo::m_freegpuram = 0;

#if defined(PLATFORM_RPI)
        static Core::CriticalSection mutualExclusion;

        if (SystemInfo::m_totalgpuram == 0) {
            SystemInfo::UpdateTotalGpuRam();
        }

        std::string response = vcgencmd_request("get_mem reloc ");

        // Erase prefix in order to get value.
        std::size_t prefix = response.find("=");
        response.erase(0, prefix + 1);

        // Find unit of response and omit from response.
        std::string unit;
        std::size_t postfix = response.find_first_not_of("0123456789,. ");
        if (postfix != std::string::npos) {
            unit = response.substr(postfix, response.length());
            response.erase(postfix, response.length());
        }

        // Convert string to integer.
        SystemInfo::m_freegpuram = NumberType<uint64_t>(TextFragment(response)).Value();

        // Convert into bytes, if necessary.
        if (unit.compare("M") == 0) {
            // Multiply with MB = 1024*1024.
            SystemInfo::m_freegpuram *= 1048576;
        }
        else if (unit.compare("K") == 0) {
            // Multiply with KB = 1024.
            SystemInfo::m_freegpuram *= 1024;
        }
#endif

    }

#if defined(__LINUX__) && !defined(__APPLE__)

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
        return (y2/4 - y1/4) - (y2/100 - y1/100) + (y2/400 - y1/400);
    }

    static time_t mktimegm(const struct tm *tm)
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

        m_uptime =  (Core::Time::Now().Ticks() - Core::Time(time).Ticks()) / (Core::Time::TicksPerMillisecond * 1000);

        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        if (host_statistics (mach_host_self (), HOST_VM_INFO, (host_info_t) &vmstat, &count) != KERN_SUCCESS)
        {
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
#ifdef __WIN32__
        ::SetSystemTime(&(time.Handle()));

#elif defined(__APPLE__)
    ASSERT("Time not set");
#else

        struct tm setTime;

        ::memcpy(&setTime, &(time.Handle()), sizeof (setTime));

        time_t value = mktimegm(&setTime);

        if(stime(&value) != 0 ){
            TRACE_L1("Failed to set system time [%d]", errno);
        }
        else {
            TRACE_L1("System time updated [%d]", errno);
        }
#endif
    }

    /* static */ bool SystemInfo::GetEnvironment(const string& name, string& value)
    {
#ifdef __LINUX__
        TCHAR* text = ::getenv(name.c_str());

        if(text != nullptr) {
            value = text;
        }
        else {
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
            }
            else {
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

#ifdef __WIN32__
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

#ifdef __WIN32__

            // What to do on windows ????
            ASSERT(false);

#else

                pid_t pid;

                if ((pid = fork()) == 0) {
                    execlp("/sbin/reboot", "/sbin/reboot", nullptr);
                    exit(1); // This should never be reached.
                }
                else if (pid != -1) {
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
                    }
                    else {
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
