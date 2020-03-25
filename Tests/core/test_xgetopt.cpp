#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/XGetopt.h>

int argumentCount = 3;
char* arguments[]= {(char*)"-c", (char*)"-h", (char*)"-b"};

using namespace WPEFramework;
using namespace WPEFramework::Core;

class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions() = delete;

        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Core::Options(argumentCount, arguments, _T("chb"))
        {
            Parse();
        }

        ~ConsoleOptions()
        {
        }

     private:
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
            case 'c':
                EXPECT_EQ(option,'c');
                break;
#ifndef __WIN32__
            case 'b':
                EXPECT_EQ(option,'b');
                break;
#endif
            case 'h':
                EXPECT_EQ(option,'h');
                break;
            default:
                printf("default\n");
                break;
            }
        }
};

TEST(test_xgetopt, simple_xgetopt)
{
    ConsoleOptions consoleOptions(argumentCount,arguments);
}
