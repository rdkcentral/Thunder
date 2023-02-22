#ifndef MODULE_NAME
#define MODULE_NAME LocalTraceTest
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

constexpr char module[] = "LocalTraceTest";

namespace {
class SleepObject {
public:
    SleepObject(const SleepObject&) = delete;
    SleepObject& operator=(const SleepObject&) = delete;

    SleepObject()
    {
        TRACE(Trace::Error, ("Constructing %s.... ", __FUNCTION__));
    }

    ~SleepObject()
    {
        TRACE(Trace::Error, ("Destructing %s.... ", __FUNCTION__));
    }

    void Sleep(uint8_t time)
    {
        TRACE(Trace::Information, ("Sleep for %ds ", time));
        sleep(time);
        TRACE(Trace::Information, ("Hello again!"));
    }
};
}
int main(int /*argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));
    {
        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);
        tracer.EnableMessage(module, "Error", true);
        tracer.EnableMessage(module, "Information", true);

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        {
            SleepObject object;
            object.Sleep(5);
        }

        TRACE_GLOBAL(Trace::Error, ("Exiting.... "));
    }

    tracer.Close();

    WPEFramework::Core::Singleton::Dispose();

    return 0;
}