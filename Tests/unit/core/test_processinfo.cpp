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

using namespace WPEFramework;

TEST(Core_ProcessInfo, simpleSet)
{
    pid_t pid = getpid();
    Core::ProcessInfo processInfo(pid);
    std::cout << "Name           :" << processInfo.Name() << std::endl;
    std::cout << "Id             :" << processInfo.Id() << std::endl;
    std::cout << "Priority       :" << static_cast<int>(processInfo.Priority()) << std::endl;
    std::cout << "IsActive       :" << (processInfo.IsActive() ? "Yes" : "No") << std::endl;
    std::cout << "Executable     :" << processInfo.Executable() << std::endl;
    //std::cout << "Group          :" << processInfo.Group() << std::endl;
    //std::cout << "User           :" << processInfo.User() << std::endl;
    std::cout << "Size Allocated :" << (processInfo.Allocated() >> 10) << " KB" << std::endl;
    std::cout << "Size Resident  :" << (processInfo.Resident() >> 10) << " KB" << std::endl;
    std::cout << "Size Shared    :" << (processInfo.Shared() >> 10) << " KB" << std::endl;

    Core::ProcessInfo::Iterator childIterator = processInfo.Children();

    std::cout << "Children (" << childIterator.Count() << ") " << std::endl;
    while (childIterator.Next()) {
        Core::ProcessInfo childProcessInfo = childIterator.Current();
        std::cout << "\tName        : " << childProcessInfo.Name() << " (" << childProcessInfo.Id() << "): " << childProcessInfo.Resident() << std::endl;
    }
}
