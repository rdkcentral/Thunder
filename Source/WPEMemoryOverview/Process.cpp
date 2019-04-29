// process.cpp : Defines the entry point for the console application.
//

#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Process {

    class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
 //           : Core::Options(argumentCount, arguments, _T("h:l:c:r:p:s:d:a:m:i:u:g:t:e:"))
            : Core::Options(argumentCount, arguments, _T("h"))
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
            case 'h':
            default:
                RequestUsage(true);
                break;
            }
        }
    };

}
} // Process

using namespace WPEFramework;

void CloseDown()
{
    TRACE_L1("Entering @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());

    // Now clear all singeltons we created.
    Core::Singleton::Dispose();

    TRACE_L1("Leaving @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());
}

#ifdef __WIN32__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{

    if (atexit(CloseDown) != 0) {
        TRACE_L1("Could not register @exit handler. Argc %d.", argc);
        CloseDown();
        exit(EXIT_FAILURE);
    }

    Process::ConsoleOptions options(argc, argv);

    if ( (options.RequestUsage() == true) ) {
        printf("WPEMemoryOverview [-h] \n");
        printf("This application provides a MemoryOverview of the system and selected processes\n");

    } else {

        MemoryOverview::MemoryOverview::ClearOSCaches();


        printf("Meminfo: %s\n", MemoryOverview::MemoryOverview::GetMeminfo().c_str());

        printf("Childs: %s\n", MemoryOverview::MemoryOverview::GetDependencies().c_str());

    }

    return 0;
}
