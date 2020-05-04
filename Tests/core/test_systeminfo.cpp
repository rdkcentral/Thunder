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

#include <gtest/gtest.h>
#include <core/core.h>

TEST(Core_SystemInfo, simpleSet)
{
    const uint8_t* rawDeviceId (WPEFramework::Core::SystemInfo::Instance().RawDeviceId());

    string id (WPEFramework::Core::SystemInfo::Instance().Id(rawDeviceId, 0xFF));

    std::cout << "SystemInfo Test:" << std::endl;

    std::cout << "Test GetId: " << id << std::endl;

    // std::cout << "Test GetPrefix: " << WPEFramework::Core::SystemInfo::Instance().GetPrefix() << std::endl;

    std::cout << "Test GetHostname: " << WPEFramework::Core::SystemInfo::Instance().GetHostName() << std::endl;

    std::cout << "Test GetUpTime: " << WPEFramework::Core::SystemInfo::Instance().GetUpTime() << " s" << std::endl;

    std::cout << "Test GetTotalRam: " << WPEFramework::Core::SystemInfo::Instance().GetTotalRam() << " bytes" << std::endl;

    std::cout << "Test GetFreeRam: " << WPEFramework::Core::SystemInfo::Instance().GetFreeRam() << " bytes" << std::endl;

    //std::cout << "Test GetTotalGpuRam: " << WPEFramework::Core::SystemInfo::Instance().GetTotalGpuRam() << " bytes" << std::endl;

    //std::cout << "Test GetFreeGpuRam: " << WPEFramework::Core::SystemInfo::Instance().GetFreeGpuRam() << " bytes" << std::endl;

    std::cout << "Test Ticks(Time stamp counter): " << WPEFramework::Core::SystemInfo::Instance().Ticks() << std::endl;

    for (int a = 0; a < 3; a++) {
        std::cout << a << ". Test GetCpuLoad: " << WPEFramework::Core::SystemInfo::Instance().GetCpuLoad() << " %" << std::endl;
    }
    std::string variable = "TEST_VARIABLE";
    std::string variable_value = "test value";
    bool forced = true;
    std::cout << "Test SetEnvironment: " << std::endl;
    WPEFramework::Core::SystemInfo::Instance().SetEnvironment(variable, variable_value, forced);
    variable_value.clear();
    std::cout << "\t" << variable << " : " << (WPEFramework::Core::SystemInfo::Instance().GetEnvironment(variable, variable_value) ? variable_value : "--")  << std::endl;
}

