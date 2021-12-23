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

        extern "C" EXTERNAL_EXPORT const char* ModuleName();
        extern "C" EXTERNAL_EXPORT const char* ModuleBuildRef();
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
        void SetTimeZone(const string& tz);

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

        inline uint64_t Ticks() const
        {
            return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        }

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

#define MODULE_NAME_DECLARATION(buildref)                                                                     \
    extern "C" {                                                                                              \
    namespace WPEFramework {                                                                                  \
        namespace Core {                                                                                      \
            namespace System {                                                                                \
                const char* MODULE_NAME = SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(MODULE_NAME);              \
                const char* ModuleName() { return (MODULE_NAME); }                                            \
                const char* ModuleBuildRef() { return (SOLUTIONS_GENERICS_SYSTEM_PREPROCESSOR_2(buildref)); } \
            }                                                                                                 \
        }                                                                                                     \
    }                                                                                                         \
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
