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

namespace WPEFramework {
namespace Tests {

enum class Purpose {
    MEM,
    SWAP,
    HOST,
    CHIPSET,
    CPULOAD
};

enum class Funcion {
    TOTAL,
    FREE,
    NONE
};

double Round(double value, uint8_t places) {
    uint32_t placeValue = 1;
    for (uint8_t i = 0; i < places; ++i) {
        placeValue *= 10;
    }
    value = (value + 0.005) * placeValue;
    value = (double)((int) value);
    value = value / placeValue;
    return value;
}

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

std::string GetMemory(std::string result, Funcion func)
{
    std::string word;
    int i = 0;
    std::stringstream iss(result);
    while (iss >> result)
    {
        if(i == 1 && func == Funcion::TOTAL){
            word.assign(result);
            break;
        }//retrieves the free memory
        if(i == 3 && (func == Funcion::FREE)) {
            word.assign(result);
            break;
        }
        i++;
    }
    return word;
}

std::string ExecuteCmd(const char* cmd, Purpose purpose, Funcion func) {
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
                if (result.find("Mem:") != std::string::npos) {
                    word = GetMemory(result, func);
                }
            } else if (purpose == Purpose::SWAP) {
                if (result.find("Swap:") != std::string::npos) {
                    word = GetMemory(result, func);
                }
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

TEST(Core_SystemInfo, RawDeviceId)
{
    const uint8_t* rawDeviceId = (WPEFramework::Core::SystemInfo::Instance().RawDeviceId());
    // Simple check added, since the rawId is currently based on MAC address of first active interface
    EXPECT_EQ((rawDeviceId != nullptr), true);
}
TEST(Core_SystemInfo, RawDeviceId_To_ID)
{
    const uint8_t* rawDeviceId = (WPEFramework::Core::SystemInfo::Instance().RawDeviceId());
    string id1 (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, 0xFF));
    uint8_t readPaddedSize = 0x11;
    string id2 (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, readPaddedSize));
    uint8_t readSize = 0xc;
    string id3 (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, readSize));

    EXPECT_GE(id1.size(), 0);
    EXPECT_EQ(id2.size(), readPaddedSize);
    size_t paddingStartPosition = id2.find_first_of('0', 0);
    size_t paddingEndPosition = id2.find_first_not_of('0', paddingStartPosition);
    EXPECT_EQ((id1.size() + (paddingEndPosition - paddingStartPosition)), id2.size());

    EXPECT_EQ((id3.size() == readSize), true);
}
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
    ::unsetenv(env.c_str());
    // Call forced with false after unset/clear environement variable
    Core::SystemInfo::SetEnvironment(env, valueSet, false);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet.c_str());
    ::unsetenv(env.c_str());
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
    ::unsetenv(env.c_str());

    // Call forced with false after unset/clear environement variable
    Core::SystemInfo::SetEnvironment(env, valueSet, false);
    Core::SystemInfo::GetEnvironment(env, valueGet);
    EXPECT_STREQ(valueGet.c_str(), valueSet);
    ::unsetenv(env.c_str());
}
TEST(Core_SystemInfo, HostName)
{
    std::string cmd = "hostname";
    string hostname = ExecuteCmd(cmd.c_str(), Purpose::HOST, Funcion::NONE).c_str();
    hostname.erase(std::remove(hostname.begin(), hostname.end(), '\n'), hostname.end());
    EXPECT_STREQ(Core::SystemInfo::Instance().GetHostName().c_str(), hostname.c_str());
}
TEST(Core_SystemInfo, MemoryInfo)
{
    std::string cmd = "free -b";
    uint32_t pageSize = getpagesize();
    EXPECT_EQ(Core::SystemInfo::Instance().GetPageSize(), getpagesize());

    uint64_t totalRam = stol(ExecuteCmd(cmd.c_str(), Purpose::MEM, Funcion::TOTAL));
    EXPECT_EQ(Core::SystemInfo::Instance().GetTotalRam(), totalRam);
    EXPECT_EQ(Core::SystemInfo::Instance().GetPhysicalPageCount(), static_cast<uint32_t>(totalRam / pageSize));

    EXPECT_EQ(Core::SystemInfo::Instance().GetTotalSwap(), stol(ExecuteCmd(cmd.c_str(), Purpose::SWAP, Funcion::TOTAL)));
    string freeRam = ExecuteCmd(cmd.c_str(), Purpose::SWAP, Funcion::FREE);
    EXPECT_EQ(Core::SystemInfo::Instance().GetFreeSwap(), stol(ExecuteCmd(cmd.c_str(), Purpose::SWAP, Funcion::FREE)));

    // Returns the instant snapshot of the free memory at that moment, hence can't verify it's value.
    WPEFramework::Core::SystemInfo::Instance().GetFreeRam();
}
TEST(Core_SystemInfo, UPTime)
{
    EXPECT_EQ(WPEFramework::Core::SystemInfo::Instance().GetUpTime(), GetUpTime());
}
TEST(Core_SystemInfo, CPUInfo)
{
    // CPULoad
    string cmd = "top -b -n 1 | grep Cpu";
    uint8_t cpuLoad = stoi(ExecuteCmd(cmd.c_str(), Purpose::CPULOAD, Funcion::NONE));
    uint8_t difference = std::abs(cpuLoad - static_cast<uint8_t>(WPEFramework::Core::SystemInfo::Instance().GetCpuLoad()));
    // Checking nearly equal value, since the cpu load calculation is average in the test app code
    EXPECT_EQ((difference <= 2), true);

    // CPULoadAvg
    double loadFromSystem[3];
    // Validate only if we get a valid value for comparison from the system
    if (getloadavg(loadFromSystem, 3) != -1) {
        // Round of values to get nearest equals
        loadFromSystem[0] = Round(loadFromSystem[0], 2);
        loadFromSystem[1] = Round(loadFromSystem[1], 2);
        loadFromSystem[2] = Round(loadFromSystem[2], 2);

        uint64_t* cpuLoadAvg = WPEFramework::Core::SystemInfo::Instance().GetCpuLoadAvg();
	double loadFromThunder[3];
        loadFromThunder[0] = Round((cpuLoadAvg[0] / 65536.0), 2);
        loadFromThunder[1] = Round((cpuLoadAvg[1] / 65536.0), 2);
        loadFromThunder[2] = Round((cpuLoadAvg[2] / 65536.0), 2);
	EXPECT_EQ(loadFromSystem[0], loadFromThunder[0]);
	EXPECT_EQ(loadFromSystem[1], loadFromThunder[1]);
	EXPECT_EQ(loadFromSystem[2], loadFromThunder[2]);
    }
    // Jiffies
}
#if 0
TEST(Core_SystemInfo, Ticks)
{
    WPEFramework::Core::SystemInfo::Instance().Ticks();

    WPEFramework::Core::SystemInfo::Instance().GetCpuLoad();
}

TEST(Core_SystemInfo, memorySnapShot)
{
    WPEFramework::Core::SystemInfo::MemorySnapshot snapshot = WPEFramework::Core::SystemInfo::Instance().TakeMemorySnapshot();
    snapshot.AsJSON();
    std::string cmd = "free -k";
    EXPECT_EQ(snapshot.Total(), stoi(ExecuteCmd(cmd.c_str(), Purpose::MEM, Funcion::TOTAL)));
    snapshot.Free(); //Returns the instant snapshot of the free memory at that moment, hence can't verify it's value.
    snapshot.Available();//Returns the instant snapshot of the available memory at that moment, hence can't verify it's value.
    snapshot.Cached();

    EXPECT_EQ(snapshot.SwapTotal(),stoi(ExecuteCmd(cmd.c_str(), Purpose::SWAP, Funcion::TOTAL)));
    snapshot.SwapFree();
    snapshot.SwapCached();
}
#endif
TEST(Core_SystemInfo, HardwareInfo)
{
    EXPECT_STREQ(Core::SystemInfo::Instance().Architecture().c_str(), Architecture().c_str());
    std::string cmd = "cat /proc/cpuinfo | grep model | grep name";
    EXPECT_STREQ(Core::SystemInfo::Instance().Chipset().c_str(), ExecuteCmd(cmd.c_str(), Purpose::CHIPSET, Funcion::NONE).c_str());
}
TEST(Core_SystemInfo, FirmwareInfo)
{
    EXPECT_STREQ(Core::SystemInfo::Instance().FirmwareVersion().c_str(), FirmwareVersion().c_str()); 
}
} // Tests

ENUM_CONVERSION_BEGIN(Tests::Purpose)
    { WPEFramework::Tests::Purpose::MEM, _TXT("mem") },
    { WPEFramework::Tests::Purpose::SWAP, _TXT("swap") },
    { WPEFramework::Tests::Purpose::HOST, _TXT("host") },
    { WPEFramework::Tests::Purpose::CHIPSET, _TXT("chipset") },
    { WPEFramework::Tests::Purpose::CPULOAD, _TXT("cpuload") },
ENUM_CONVERSION_END(Tests::Purpose)

ENUM_CONVERSION_BEGIN(Tests::Funcion)
    { WPEFramework::Tests::Funcion::TOTAL, _TXT("total") },
    { WPEFramework::Tests::Funcion::FREE, _TXT("free") },
ENUM_CONVERSION_END(Tests::Funcion)

} // WPEFramework
