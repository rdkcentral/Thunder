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
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

class ThreadClass : public Core::Thread {
public:
    ThreadClass() = delete;
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

    ThreadClass(std::thread::id parentId)
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
        , _threadDone(false)
        , _parentId(parentId)
    {
    }

    virtual ~ThreadClass()
    {
    }

    virtual uint32_t Worker() override
    {
        while (IsRunning() && (!_threadDone)) {
            EXPECT_TRUE(_parentId != std::this_thread::get_id());
            _threadDone = true;
            ::SleepMs(50);
        }
        return (Core::infinite);
    }

private:
    volatile bool _threadDone;
    std::thread::id _parentId;
};

TEST(test_portability, simple_upper)
{
    std::string input = "hello";
    std::string output;
    ToUpper(input,output);
    EXPECT_STREQ(output.c_str(),_T("HELLO"));

    ToUpper(input);
    EXPECT_STREQ(input.c_str(),_T("HELLO"));
}

TEST(test_portability, simple_lower)
{
    std::string input = "HELLO";
    std::string output;
    ToLower(input,output);
    EXPECT_STREQ(output.c_str(),_T("hello"));
    
    ToLower(input);
    EXPECT_STREQ(input.c_str(),_T("hello"));
}

TEST(test_portability, simple_generic)
{
    SleepS(1);
    SleepMs(1);
    EXPECT_EQ(htonl(12345),ntohl(12345));

    std::thread::id parentId;
    ThreadClass object(parentId);
    object.Run();

//#ifdef __DEBUG__
//    DumpCallStack(object.Id(), nullptr);
//#endif

    object.Stop();

    std::string s1 = "Hello";
    uint8_t dest_buffer[6];
    ::memmove((void*)dest_buffer,(void*)s1.c_str(),  static_cast<size_t>(5));
    dest_buffer[5] = '\0';
    EXPECT_STREQ((const char*)(dest_buffer),s1.c_str());
}

TEST(test_error, simple_error)
{
    EXPECT_STREQ(ErrorToString(ERROR_NONE),"ERROR_NONE");
}

TEST(test_void, simple_void)
{
    Void v;
    Void v2 = v;
}
