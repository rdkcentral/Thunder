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

#include "../IPTestAdministrator.h"

#include <fstream>

#include <gtest/gtest.h>
#include <core/core.h>
#include <sys/utsname.h>
#include <inttypes.h>

namespace WPEFramework {
namespace Tests {

enum class Purpose {
    MEM,
    SWAP,
    HOST,
    CHIPSET,
    CPULOAD,
};

enum class Function {
    TOTAL,
    FREE,
    AVAILABLE,
    CACHED,
    NONE
};

inline uint32_t PlaceValue(uint8_t places) {
    uint32_t placeValue = 1;
    for (uint8_t i = 0; i < places; ++i) {
        placeValue *= 10;
    }
    return placeValue;
}

double Round(double value, uint8_t places) {
    uint32_t placeValue  = PlaceValue(places);
    value = (value + 0.005) * placeValue;
    value = (double)((int) value);
    value = value / placeValue;
    return value;
}

#ifdef __POSIX__
std::string Architecture()
{
    std::string architecture;
    struct utsname buf;
    if (uname(&buf) == 0) {
        architecture = buf.machine;
    }
    return architecture;
}

std::string FirmwareVersion()
{
    std::string release;
    struct utsname buf;
    if (uname(&buf) == 0) {
        release = buf.release;
    }
    return release;
}

std::string GetChipset(std::string result)
{
    std::string chipset;
    std::stringstream iss(result);

    while (getline(iss, result, '\n'))
    {
        if (result.find("model name") != std::string::npos) {
            std::size_t position = result.find(':');
            if (position != std::string::npos) {
                chipset.assign(result.substr(result.find_first_not_of(" ", position + 1)));
                break;
           }
        }
    }
    return chipset;
}
std::string GetCPUJiffies(std::string result) 
{
    int i = 0;
    std::stringstream iss(result);

    uint64_t jiffies = 0;
    size_t* endPtr = nullptr;
    iss >> result; // Skip first word
    while (iss >> result)
    {
        if (result.empty() != true) {
            uint64_t value = stol(result, endPtr);
            if (endPtr == nullptr) {
                jiffies += value; // FIXME: cross check do we need to use all fields to get jiffies or not
                // https://titanwolf.org/Network/Articles/Article?AID=3d8450d1-470b-4533-bb5a-c46ded0215bb
            }
        }
        i++;
    }

    return std::to_string(jiffies);

}
std::string GetCPULoad(std::string result)
{
    int i = 0;
    std::stringstream iss(result);

    std::vector<float> usageData;
    size_t* endPtr = nullptr;

    while (iss >> result)
    {
        if ((i % 2) == 1) {
            float value = stof(result, endPtr);
            if (endPtr == nullptr) {
                usageData.push_back(value);
            }
        }
        i++;
    }

    float nonIdle = 0, idle = 0;
    for (uint8_t index = 0; index < usageData.size(); ++index) {
        if ((index == 3) || (index == 4)) {
            idle += usageData[index];
        } else {
            nonIdle += usageData[index];
        }
    }
    // Send non decimal part
    return std::to_string(static_cast<uint8_t>(nonIdle));
}

inline std::string GetMemoryValue(std::string buffer, bool found) {

    int i = 0;
    string result;
    if (found == true) {
        std::stringstream iss(buffer);

        while (iss >> result) {
            if (i == 1) {
                break;
            }

            i++;
        }
    }
    return result;
}

std::string GetMemory(std::string result, Function func)
{
    std::string word;

    bool found = false;
    if ((func == Function::TOTAL) && (result.find("MemTotal:") != std::string::npos)) {
        found = true;
    } else if ((func == Function::FREE) && (result.find("MemFree:") != std::string::npos)) {
        found = true;
    } else if ((func == Function::CACHED) && (result.find("Cached:") != std::string::npos)) {
        found = true;
    } else if ((func == Function::AVAILABLE) && (result.find("MemAvailable") != std::string::npos)) {
        found = true;
    }

    return GetMemoryValue(result, found);
}

std::string GetSwapMemory(std::string result, Function func)
{
    std::string word;

    bool found = false;
    if ((func == Function::TOTAL) && (result.find("SwapTotal:") != std::string::npos)) {
        found = true;
    } else if ((func == Function::FREE) && (result.find("SwapFree:") != std::string::npos)) {
        found = true;
    } else if ((func == Function::CACHED) && (result.find("SwapCached:") != std::string::npos)) {
        found = true;
    }

    return GetMemoryValue(result, found);
}

std::string ExecuteCmd(const char* cmd, Purpose purpose, Function func) {
    char buffer[128];
    std::string result = "";
    std::string word = "";
    FILE* pipe = popen(cmd, "r");

    EXPECT_TRUE(pipe != nullptr);
#ifdef __CORE_EXCEPTION_CATCHING__
    try {
#endif
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result = buffer;
            if (purpose == Purpose::MEM) {
                word = GetMemory(result, func);
                break;
            } else if (purpose == Purpose::SWAP) {
                word = GetSwapMemory(result, func);
                break;
            } else if (purpose == Purpose::HOST) {
                word.assign(result);
                break;
            } else if (purpose == Purpose::CHIPSET) {
                if (result.find("model name") != std::string::npos) {
                    word = GetChipset(result);
                }
                break;
            } else if (purpose == Purpose::CPULOAD) {
                if (result.find("%Cpu(s):") != std::string::npos) {
                    word = GetCPULoad(result);
                }
                break;
            }
        }
#ifdef __CORE_EXCEPTION_CATCHING__
    } catch (...) {
        pclose(pipe);
        throw;
    }
#endif
    pclose(pipe);

    return word;
}

float GetUpTime() {
    std::string uptime;
    std::ifstream file("/proc/uptime");
    while(file >> uptime)
        return (ceil(stof(uptime)));
    return 0;
}
#endif
const uint8_t* RawDeviceId()
{
    static uint8_t* MACAddress = nullptr;
    static uint8_t MACAddressBuffer[Core::AdapterIterator::MacSize + 1];

    if (MACAddress == nullptr) {
        memset(MACAddressBuffer, 0, Core::AdapterIterator::MacSize + 1);

        Core::AdapterIterator adapters;
        while ((adapters.Next() == true)) {
            if (adapters.HasMAC() == true) {
                adapters.MACAddress(&MACAddressBuffer[1], Core::AdapterIterator::MacSize);
                break;
            }
        }
        MACAddressBuffer[0] = Core::AdapterIterator::MacSize;
        MACAddress = &MACAddressBuffer[0];
    }

    return MACAddress;
}

TEST(Core_SystemInfo, RawDeviceId)
{
    const uint8_t* rawDeviceId = RawDeviceId();
    // Simple check added, since the rawId is currently based on MAC address of first active interface
    EXPECT_EQ((rawDeviceId != nullptr), true);

    // Call to dispose Network adapter instance created with RawDeviceId sequence
    Core::Singleton::Dispose();
}
TEST(Core_SystemInfo, RawDeviceId_To_ID)
{
    const uint8_t* rawDeviceId = RawDeviceId();
    string id1 (Core::SystemInfo::Instance().Id(rawDeviceId, 0xFF));
    uint8_t readPaddedSize = 0x11;
    string id2 (Core::SystemInfo::Instance().Id(rawDeviceId, readPaddedSize));
    uint8_t readSize = 0xc;
    string id3 (Core::SystemInfo::Instance().Id(rawDeviceId, readSize));

    EXPECT_GE(id1.size(), static_cast<size_t>(0));
    EXPECT_EQ(id2.size(), readPaddedSize);
    size_t paddingStartPosition = id2.find_first_of('0', 0);
    size_t paddingEndPosition = id2.find_first_not_of('0', paddingStartPosition);
    EXPECT_EQ((id1.size() + (paddingEndPosition - paddingStartPosition)), id2.size());

    EXPECT_EQ((id3.size() == readSize), true);

    // Call to dispose Network adapter instance created with RawDeviceId sequence
    Core::Singleton::Dispose();
}

#ifdef __POSIX__
TEST(Core_SystemInfo, GetEnvironment_SetUsing_setenv)
{
    string env = "TEST_GETENVIRONMENT_WITH_SETENV";
    string valueSet = "HelloTest";
    ::setenv(env.c_str(), valueSet.c_str(), 1);
    string valueGet;
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet.c_str());
    ::unsetenv(env.c_str());
}
#endif
TEST(Core_SystemInfo, GetEnvironment_SetUsing_SetEnvironment_With_ValueTypeAsString)
{
    string env = "TEST_GETENVIRONMENT_WITH_SETENVIRONMENT";
    string valueSet = "HaiTest";
    // Call forced with default value
    Core::SystemInfo::SetEnvironment(env, valueSet);
    string valueGet;
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet.c_str());

    valueSet = "ForcedTest";
    // Call forced with true
    Core::SystemInfo::SetEnvironment(env, valueSet, true);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet.c_str());

    valueSet = "NotForcedTest";
    // Call forced with false
    Core::SystemInfo::SetEnvironment(env, valueSet, false);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), "ForcedTest");

#ifdef __POSIX__
    ::unsetenv(env.c_str());
    // Call forced with false after unset/clear environement variable
    Core::SystemInfo::SetEnvironment(env, valueSet, false);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet.c_str());
    ::unsetenv(env.c_str());
#endif
}
TEST(Core_SystemInfo, GetEnvironment_SetUsing_SetEnvironment_WithValueTypeAsTCharPointer)
{
    string env = "TEST_GETENVIRONMENT_WITH_SETENVIRONMENT";
    TCHAR valueSet[25] = "HaiTest";
    // Call forced with default value
    Core::SystemInfo::SetEnvironment(env, valueSet);
    string valueGet;
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet);

    strcpy(valueSet, "ForcedTest");
    // Call forced with true
    Core::SystemInfo::SetEnvironment(env, valueSet, true);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet);

    strcpy(valueSet, "NotForcedTest");
    // Call forced with false
    Core::SystemInfo::SetEnvironment(env, valueSet, false);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), "ForcedTest");

#ifdef __POSIX__
    ::unsetenv(env.c_str());

    // Call forced with false after unset/clear environement variable
    Core::SystemInfo::SetEnvironment(env, valueSet, false);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet);
    ::unsetenv(env.c_str());
#endif
}
#ifdef __POSIX__
TEST(Core_SystemInfo, HostName)
{
    std::string cmd = "hostname";
    string hostname = ExecuteCmd(cmd.c_str(), Purpose::HOST, Function::NONE).c_str();
    hostname.erase(std::remove(hostname.begin(), hostname.end(), '\n'), hostname.end());
    EXPECT_STREQ(Core::SystemInfo::Instance().GetHostName().c_str(), hostname.c_str());
}
TEST(Core_SystemInfo, HardwareInfo)
{
    EXPECT_STREQ(Core::SystemInfo::Instance().Architecture().c_str(), Architecture().c_str());
    std::string cmd = "cat /proc/cpuinfo | grep model | grep name";
    EXPECT_STREQ(Core::SystemInfo::Instance().Chipset().c_str(), ExecuteCmd(cmd.c_str(), Purpose::CHIPSET, Function::NONE).c_str());
}
TEST(Core_SystemInfo, FirmwareInfo)
{
    EXPECT_STREQ(Core::SystemInfo::Instance().FirmwareVersion().c_str(), FirmwareVersion().c_str()); 
}
static constexpr uint32_t MegaBytesPerBytes = 1024 * 1024;
static constexpr uint32_t MegaBytesPerKBytes = 1024;
TEST(Core_SystemInfo, MemoryInfo)
{
    uint32_t pageSize = getpagesize();
    EXPECT_EQ(Core::SystemInfo::Instance().GetPageSize(), static_cast<uint32_t>(getpagesize()));

    std::string cmd = "cat /proc/meminfo | grep MemTotal";
    string result = ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::TOTAL);
    if (result.empty() != true) {
        uint64_t totalRam = stol(result) * 1024;
        EXPECT_EQ(Core::SystemInfo::Instance().GetTotalRam(), totalRam);
        EXPECT_EQ(Core::SystemInfo::Instance().GetPhysicalPageCount(), static_cast<uint32_t>(totalRam / pageSize));
    }
    cmd = "cat /proc/meminfo | grep SwapTotal:";
    result = ExecuteCmd(cmd.c_str(), Purpose::SWAP, Function::TOTAL);
    if (result.empty() != true) {
        EXPECT_EQ(Core::SystemInfo::Instance().GetTotalSwap(), static_cast<uint32_t>(stol(result) * 1024));
    }

#if 0 // Disabling this since this can be varied time to time and endup different value
      // even the APIs functioning properly
    // Returns the instant snapshot of the free memory/swap at that moment,
    // hence it can be different values due to any change happens in between the calls.
    // And onvert to KB to get nearly equal value
    cmd = "cat /proc/meminfo | grep MemFree:";
    result = ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::FREE);
    if (result.empty() != true) {
        // Ignore last digit to ignore difference
        EXPECT_EQ((Core::SystemInfo::Instance().GetFreeRam() / (MegaBytesPerBytes * 10)), (stol(result) / (MegaBytesPerKBytes * 10)));
    }
    cmd = "cat /proc/meminfo | grep SwapFree:";
    result = ExecuteCmd(cmd.c_str(), Purpose::SWAP, Function::FREE);
    if (result.empty() != true) {
        EXPECT_EQ(Core::SystemInfo::Instance().GetFreeSwap() / 10, (stol(result) * 1024) / 10);
    }
#endif
}
TEST(Core_SystemInfo, memorySnapShot)
{
    Core::SystemInfo::MemorySnapshot snapshot = Core::SystemInfo::Instance().TakeMemorySnapshot();
    snapshot.AsJSON();
    std::string cmd = "cat /proc/meminfo | grep MemTotal:";
    string result = ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::TOTAL);
    if (result.empty() != true) {
        EXPECT_EQ(snapshot.Total(), static_cast<uint32_t>(stol(ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::TOTAL))));
    }

#if 0 // Disabling this since this can be varied time to time and endup different value
      // even the APIs functioning properly
    // Returns the instant snapshot of the free/available/cached memory at that moment,
    // hence it can be different values due to any change happens in between the calls.
    // And use convert to KB/10 to get nearly equal value
    cmd = "cat /proc/meminfo | grep MemFree:";
    result = ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::FREE);
    if (result.empty() != true) {
        EXPECT_EQ((snapshot.Free() / (MegaBytesPerKBytes * 10)), (stol(result) / (MegaBytesPerKBytes * 10)));
    }
    cmd = "cat /proc/meminfo | grep MemAvailable:";
    result = ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::AVAILABLE);
    if (result.empty() != true) {
        EXPECT_EQ((snapshot.Available() / (MegaBytesPerKBytes * 10)), (stol(result) / (MegaBytesPerKBytes  * 10)));
    }
    cmd = "cat /proc/meminfo | grep Cached:";
    result = ExecuteCmd(cmd.c_str(), Purpose::MEM, Function::CACHED);
    if (result.empty() != true) {
        EXPECT_EQ((snapshot.Cached() / MegaBytesPerKBytes), (stol(result) / MegaBytesPerKBytes));
    }
#endif
    cmd = "cat /proc/meminfo | grep SwapTotal:";
    result = ExecuteCmd(cmd.c_str(), Purpose::SWAP, Function::TOTAL);
    if (result.empty() != true) {
        EXPECT_EQ((snapshot.SwapTotal() / MegaBytesPerKBytes), static_cast<uint32_t>(stol(result) / MegaBytesPerKBytes));
    }

#if 0 // Disabling this since this can be varied time to time and endup different value
      // even the APIs functioning properly
    // Returns the instant snapshot of the free/cached swap at that moment,
    // hence it can be different values due to any change happens in between the calls.
    // And use convert to KB to get nearly equal value
    cmd = "cat /proc/meminfo | grep SwapFree:";
    result = ExecuteCmd(cmd.c_str(), Purpose::SWAP, Function::FREE);
    if (result.empty() != true) {
        EXPECT_EQ((snapshot.SwapFree() / (MegaBytesPerKBytes * 10)), (stol(result) / (MegaBytesPerKBytes * 10)));
    }
    cmd = "cat /proc/meminfo | grep SwapCached:";
    result = ExecuteCmd(cmd.c_str(), Purpose::SWAP, Function::CACHED);
    if (result.empty() != true) {
        EXPECT_EQ((snapshot.SwapCached() / (MegaBytesPerKBytes * 10)), (stol(result) / (MegaBytesPerKBytes * 10)));
    }
#endif
}
TEST(Core_SystemInfo, DISABLED_UPTime)
{
    EXPECT_EQ(Core::SystemInfo::Instance().GetUpTime(), GetUpTime());
}
TEST(Core_SystemInfo, DISABLED_CPUInfo)
{
    // CPULoad
    string cmd = "top -b -n 1 | grep Cpu";
    string result = ExecuteCmd(cmd.c_str(), Purpose::CPULOAD, Function::NONE);
    if (result.empty() != true) {
        /* CPULoad values showing big differences sometimes
	 * Load can be vary in between the time, hence this test will be disabling
        uint8_t cpuLoad = stoi(result);
        uint8_t difference = std::abs(cpuLoad - static_cast<uint8_t>(Core::SystemInfo::Instance().GetCpuLoad()));
        // Checking nearly equal value, since the cpu load calculation is average in the test app code
        EXPECT_EQ((difference <= 2), true);
        */
    }

    // CPULoadAvg
    double loadFromSystem[3];
    // Validate only if we get a valid value for comparison from the system
    if (getloadavg(loadFromSystem, 3) != -1) {
        // Round of values to get nearest equals
        loadFromSystem[0] = Round(loadFromSystem[0], 2);
        loadFromSystem[1] = Round(loadFromSystem[1], 2);
        loadFromSystem[2] = Round(loadFromSystem[2], 2);

        uint64_t* cpuLoadAvg = Core::SystemInfo::Instance().GetCpuLoadAvg();
        double loadFromThunder[3];
        loadFromThunder[0] = Round((cpuLoadAvg[0] / 65536.0), 2);
        loadFromThunder[1] = Round((cpuLoadAvg[1] / 65536.0), 2);
        loadFromThunder[2] = Round((cpuLoadAvg[2] / 65536.0), 2);
        EXPECT_EQ(loadFromSystem[0], loadFromThunder[0]);
        EXPECT_EQ(loadFromSystem[1], loadFromThunder[1]);
        EXPECT_EQ(loadFromSystem[2], loadFromThunder[2]);
    }
}
#endif
TEST(Core_SystemInfo, Ticks_withoutDelay)
{
    uint64_t tick1 = Core::SystemInfo::Instance().Ticks();
    uint64_t tick2 = Core::SystemInfo::Instance().Ticks();
    EXPECT_EQ(tick1 / Core::Time::MicroSecondsPerSecond, tick2 / Core::Time::MicroSecondsPerSecond);
}
TEST(Core_SystemInfo, Ticks_withDelay)
{
    uint8_t seconds = 1;
    uint64_t tick1 = Core::SystemInfo::Instance().Ticks();
    sleep(seconds);
    uint64_t tick2 = Core::SystemInfo::Instance().Ticks();
    EXPECT_EQ((tick1 / Core::Time::MicroSecondsPerSecond) + seconds, tick2 / Core::Time::MicroSecondsPerSecond);
}

} // Tests

ENUM_CONVERSION_BEGIN(Tests::Purpose)
    { WPEFramework::Tests::Purpose::MEM, _TXT("mem") },
    { WPEFramework::Tests::Purpose::SWAP, _TXT("swap") },
    { WPEFramework::Tests::Purpose::HOST, _TXT("host") },
    { WPEFramework::Tests::Purpose::CHIPSET, _TXT("chipset") },
    { WPEFramework::Tests::Purpose::CPULOAD, _TXT("cpuload") },
ENUM_CONVERSION_END(Tests::Purpose)

ENUM_CONVERSION_BEGIN(Tests::Function)
    { WPEFramework::Tests::Function::TOTAL, _TXT("total") },
    { WPEFramework::Tests::Function::FREE, _TXT("free") },
    { WPEFramework::Tests::Function::AVAILABLE, _TXT("available") },
    { WPEFramework::Tests::Function::CACHED, _TXT("cached") },
ENUM_CONVERSION_END(Tests::Function)

} // WPEFramework
