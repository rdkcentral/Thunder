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

#include <gtest/gtest.h>
#include <core/core.h>

using namespace Thunder;

TEST(Core_NodeId, simpleSet)
{
    Core::NodeId nodeId("localhost:80");

    std::cout << "Hostname:           " << nodeId.HostName() << std::endl;
    std::cout << "Type:               " << nodeId.Type() << std::endl;
    std::cout << "PortNumber:         " << nodeId.PortNumber() << std::endl;
    std::cout << "IsValid:            " << nodeId.IsValid() << std::endl;
    std::cout << "Size:               " << nodeId.Size() << std::endl;
    std::cout << "HostAddress:        " << nodeId.HostAddress() << std::endl;
    std::cout << "QualifiedName:      " << nodeId.QualifiedName() << std::endl;
    std::cout << "IsLocalInterface:   " << nodeId.IsLocalInterface() << std::endl;
    std::cout << "IsAnyInterface:     " << nodeId.IsAnyInterface() << std::endl;
    std::cout << "IsMulticast:        " << nodeId.IsMulticast() << std::endl;
    std::cout << "DefaultMask:        " << static_cast<int>(nodeId.DefaultMask()) << std::endl;
}
