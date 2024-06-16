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
using namespace Thunder::Core;

TEST(test_messageException, simple_messageException)
{
    std::string msg = "Testing the message exception.";
    MessageException exception(msg.c_str(),false);
    EXPECT_STREQ(exception.Message(),msg.c_str());
    
    MessageException exception1(msg.c_str(),true);
    char buffer[50];
    string status = ": File exists";
    snprintf(buffer, msg.size()+status.size()+1, "%s%s",msg.c_str(),status.c_str());
#ifdef BUILD_ARM
    if (strcmp(exception1.Message(), buffer) != 0) {
#else
    if (strcmp(exception1.Message(), buffer) != 0) {
#endif
        memset(buffer, 0, sizeof buffer);
        status = ": No such file or directory";
        snprintf(buffer, msg.size()+status.size()+1, "%s%s",msg.c_str(),status.c_str());
    }
    EXPECT_STREQ(exception1.Message(),buffer);
}
