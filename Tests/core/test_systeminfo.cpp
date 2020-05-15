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

#include "../IPTestAdministrator.h"

#include <fstream>
#include <gtest/gtest.h>
#include <core/core.h>

std::string getMem(std::string result, string func)
{
    std::string word = "";
    int i = 1;
    std::stringstream iss(result);
    while (iss >> result)
    {
        if(i == 2 && func == "total"){
            word.assign(result);
            break;
        }/* //retrieves the free memory
        if(i == 4 && (func == "free")){
            word.assign(result);
            break;
        }*/
        i++;
    }
    return word;
}

std::string exec(const char* cmd, string purpose, string func) {
    char buffer[128];
    std::string result = "";
    std::string word = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result = buffer;
            if (purpose == "mem") {
                if (result.find("Mem:") != std::string::npos) {
                    word = getMem(result, func);
                }
            } else if (purpose == "swap") {
                if (result.find("Swap:") != std::string::npos) {
                    word = getMem(result, func);
                }
            } else if (purpose == "host") {
                word.assign(result);
                break;
            }
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return word;
}

float getUptime() {
    std::string uptime;
    std::ifstream file("/proc/uptime");
    while(file >> uptime)
        return (ceil(stof(uptime)));
    return 0;
}

TEST(Core_SystemInfo, systemInfo)
{
    const uint8_t* rawDeviceId (WPEFramework::Core::SystemInfo::Instance().RawDeviceId());

    string id (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, 0xFF));
    string id1 (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, 0x11));
    string id2 (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, 0x7));

    string hostname = exec("hostname", "host", "").c_str();
    hostname.erase(std::remove(hostname.begin(), hostname.end(), '\n'), hostname.end());
    EXPECT_STREQ(WPEFramework::Core::SystemInfo::Instance().GetHostName().c_str(), hostname.c_str());

    EXPECT_EQ(WPEFramework::Core::SystemInfo::Instance().GetPageSize(),getpagesize());
    EXPECT_EQ(WPEFramework::Core::SystemInfo::Instance().GetTotalRam(), stoi(exec("free -b", "mem", "total")));

    //EXPECT_EQ(WPEFramework::Core::SystemInfo::Instance().GetFreeRam(),stoi(exec("free -b","free")));
    WPEFramework::Core::SystemInfo::Instance().GetFreeRam(); //Returns the instant snapshot of the free memory at that moment, hence can't verify it's value.

    EXPECT_EQ(WPEFramework::Core::SystemInfo::Instance().GetUpTime(),getUptime());

    WPEFramework::Core::SystemInfo::Instance().Ticks();

    WPEFramework::Core::SystemInfo::Instance().GetCpuLoad();

    std::string variable = "TEST_VARIABLE";
    std::string value = "test value";
    std::string getValue;
    bool forced = true;
    WPEFramework::Core::SystemInfo::Instance().SetEnvironment(variable, value, forced);
    WPEFramework::Core::SystemInfo::Instance().GetEnvironment(variable, getValue);

    EXPECT_STREQ(getValue.c_str(),value.c_str());
}

TEST(Core_SystemInfo, memorySnapShot)
{
    WPEFramework::Core::SystemInfo::MemorySnapshot snapshot = WPEFramework::Core::SystemInfo::Instance().TakeMemorySnapshot();
    snapshot.AsJSON();
    EXPECT_EQ(snapshot.Total(), stoi(exec("free -k", "mem", "total")));
    snapshot.Free(); //Returns the instant snapshot of the free memory at that moment, hence can't verify it's value.
    snapshot.Available();//Returns the instant snapshot of the available memory at that moment, hence can't verify it's value.
    snapshot.Cached();

    EXPECT_EQ(snapshot.SwapTotal(),stoi(exec("free -k", "swap", "total")));
    snapshot.SwapFree();
    snapshot.SwapCached();
}
