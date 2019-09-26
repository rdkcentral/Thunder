#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>


using namespace WPEFramework;
using namespace WPEFramework::Core;

static std::thread::id g_parentId;
static bool g_threadDone = false;


class ThreadClass : public Core::Thread {
private:
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

public:
    ThreadClass()
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
    {
    }

    virtual ~ThreadClass()
    {
    }

    virtual uint32_t Worker() override
    {
        while (IsRunning() && (!g_threadDone)) {
            EXPECT_TRUE(g_parentId != std::this_thread::get_id());
            g_threadDone = true;
            ::SleepMs(50);
        }
        return (Core::infinite);
    }
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
   uint64_t value = 12345;
   EXPECT_EQ(htonll(value),4120793659044003840);
   EXPECT_EQ(ntohll(value),4120793659044003840);
   DumpCallStack();

   ThreadClass object;
   object.Run();
   DumpCallStack(object.Id());  
   ::SleepMs(50);
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
 
