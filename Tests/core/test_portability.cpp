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
    bool _threadDone;
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
   EXPECT_EQ(htonll(12345),ntohll(12345));
   DumpCallStack();

   std::thread::id parentId;
   ThreadClass object(parentId);
   object.Run();

   DumpCallStack(object.Id());  
   object.Stop();
   
   std::string s1 = "Hello";
   uint8_t dest_buffer[5];
   memrcpy((void*)dest_buffer,(void*)s1.c_str(),  static_cast<size_t>(5));
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
 
