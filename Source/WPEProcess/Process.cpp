// process.cpp : Defines the entry point for the console application.
//

#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {

    static std::list<Core::Library> _proxyStubs;
	static Core::ProxyType<RPC::CommunicatorClient> _server;

    class ExitHandler : public Core::Thread {
    private:
        ExitHandler(const ExitHandler&) = delete;
        ExitHandler& operator=(const ExitHandler&) = delete;

    public:
        ExitHandler()
            : Core::Thread(Core::Thread::DefaultStackSize(), nullptr)
        {
        }
        virtual ~ExitHandler()
        {
            Stop();
            Wait(Core::Thread::STOPPED, Core::infinite);
        }

        static void Construct()
        {
            _adminLock.Lock();
            if (_instance == nullptr) {
                _instance = new ExitHandler();
                _instance->Run();
            }
            _adminLock.Unlock();
        }
        static void Destruct()
        {
            _adminLock.Lock();
            if (_instance != nullptr) {
                delete _instance; //It will wait till the worker execution completed
                _instance = nullptr;
            } else {
                CloseDown();
            }
            _adminLock.Unlock();
        }

    private:
        virtual uint32_t Worker() override
        {
            CloseDown();
            Block();
            return (Core::infinite);
        }
        static void CloseDown()
        {
            TRACE_L1("Entering @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());

            if (_server.IsValid() == true) {

                // We are done, close the channel and unregister all shit we added...
                _server->Close(2 * RPC::CommunicationTimeOut);

                _proxyStubs.clear();

                _server.Release();
            }

            Core::Singleton::Dispose();
            TRACE_L1("Leaving @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());
        }

    private:
        static ExitHandler* _instance;
        static Core::CriticalSection _adminLock;
    };

    ExitHandler* ExitHandler::_instance = nullptr;
    Core::CriticalSection ExitHandler::_adminLock;

namespace Process {

    class WorkerPoolImplementation : public Core::IIPCServer, public Core::WorkerPool {
    public:
        WorkerPoolImplementation() = delete;
        WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
        WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

        WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize)
            : WorkerPool(threads, reinterpret_cast<uint32_t*>(::malloc(sizeof(uint32_t) * threads)))
            , _minions()
            , _announceHandler(nullptr)
            , _administration(Core::ServiceAdministrator::Instance())
        {
            for (uint8_t index = 1; index < threads; index++) {
                _minions.emplace_back();
            }

            if (threads > 1) {
                SYSLOG(Logging::Notification, ("Spawned: %d additional minions.", threads - 1));
            }
        }
        ~WorkerPoolImplementation()
        {
            // Diable the queue so the minions can stop, even if they are processing and waiting for work..
            Stop();
            _minions.clear();
            delete Snapshot().Slot;
        }
        void Announcements(Core::IIPCServer* announces)
        {
            ASSERT((announces != nullptr) ^ (_announceHandler != nullptr));
            _announceHandler = announces;
        }
        void Run() {
 
            Core::WorkerPool::Run();
            Core::WorkerPool::Join();
        }
        void Stop() {
            Core::WorkerPool::Stop();
	}

    protected:
        virtual Core::WorkerPool::Minion& Index(const uint8_t index) override {
            uint8_t count = index;
            std::list<Core::WorkerPool::Minion>::iterator element (_minions.begin());

            while ((element != _minions.end()) && (count > 1)) {
                count--;
                element++;
            }

            ASSERT (element != _minions.end());

            return (*element);
        }
        virtual bool Running() override {
            // This call might have killed the last living object in our process, if so, commit HaraKiri :-)
            _administration.FlushLibraries();

            return (_administration.Instances() > 0);
	}
        virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
        {
            Core::ProxyType<RPC::Job> job(RPC::Job::Instance());

            job->Set(channel, data, _announceHandler);

            WorkerPool::Submit(Core::ProxyType<Core::IDispatch>(job));
        }
 
    private:
        std::list<Core::WorkerPool::Minion> _minions;
        Core::IIPCServer* _announceHandler;
        Core::ServiceAdministrator& _administration;
    };

    class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Core::Options(argumentCount, arguments, _T("C:h:l:c:r:p:s:d:a:m:i:u:g:t:e:x:V:v:"))
            , Callsign(nullptr)
            , Locator(nullptr)
            , ClassName(nullptr)
            , RemoteChannel(nullptr)
            , InterfaceId(Core::IUnknown::ID)
            , Version(~0)
            , Exchange(0)
            , PersistentPath(nullptr)
            , SystemPath(nullptr)
            , DataPath(nullptr)
            , VolatilePath(nullptr)
            , AppPath(nullptr)
            , ProxyStubPath(nullptr)
            , User(nullptr)
            , Group(nullptr)
            , Threads(1)
            , EnabledLoggings(0)
        {
            Parse();
        }
        ~ConsoleOptions()
        {
        }

    public:
        const TCHAR* Callsign;
        const TCHAR* Locator;
        const TCHAR* ClassName;
        const TCHAR* RemoteChannel;
        uint32_t InterfaceId;
        uint32_t Version;
        uint32_t Exchange;
        const TCHAR* PersistentPath;
        const TCHAR* SystemPath;
        const TCHAR* DataPath;
        const TCHAR* VolatilePath;
        const TCHAR* AppPath;
        const TCHAR* ProxyStubPath;
        const TCHAR* User;
        const TCHAR* Group;
        uint8_t Threads;
        uint32_t EnabledLoggings;

    private:
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
            case 'C':
                Callsign = argument;
                break;
            case 'l':
                Locator = argument;
                break;
            case 'c':
                ClassName = argument;
                break;
            case 'r':
                RemoteChannel = argument;
                break;
            case 'p':
                PersistentPath = argument;
                break;
            case 's':
                SystemPath = argument;
                break;
            case 'd':
                DataPath = argument;
                break;
            case 'v':
                VolatilePath = argument;
                break;
            case 'a':
                AppPath = argument;
                break;
            case 'm':
                ProxyStubPath = argument;
                break;
            case 'u':
                User = argument;
                break;
            case 'g':
                Group = argument;
                break;
            case 'i':
                InterfaceId = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
                break;
            case 'e':
                EnabledLoggings = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
                break;
            case 'V':
                Version = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
                break;
            case 'x':
                Exchange = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
                break;
            case 't':
                Threads = Core::NumberType<uint8_t>(Core::TextFragment(argument)).Value();
                break;
            case 'h':
            default:
                RequestUsage(true);
                break;
            }
        }
    };

    static void* CheckInstance(const TCHAR* path, const TCHAR locator[], const TCHAR className[], const uint32_t ID, const uint32_t version)
    {
        void* result = nullptr;

        if (path != nullptr) {
            Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());

            string libraryPath = locator;
            if (libraryPath.empty() || (libraryPath[0] != '/')) {
                // Relative path, prefix with path name.
                string pathName(Core::Directory::Normalize(string(path)));
                libraryPath = pathName + locator;
            }

            Core::Library library(libraryPath.c_str());

            if (library.IsLoaded() == true) {
                // Instantiate the object
                result = admin.Instantiate(library, className, version, ID);
            }
        }

        return (result);
    }

    static void* AquireInterfaces(ConsoleOptions& options)
    {
        void* result = nullptr;

        if ((options.Locator != nullptr) && (options.ClassName != nullptr)) {
            result = CheckInstance(options.PersistentPath, options.Locator, options.ClassName, options.InterfaceId, options.Version);

            if (result == nullptr) {
                result = CheckInstance(options.SystemPath, options.Locator, options.ClassName, options.InterfaceId, options.Version);

                if (result == nullptr) {
                    result = CheckInstance(options.DataPath, options.Locator, options.ClassName, options.InterfaceId, options.Version);

                    if (result == nullptr) {
                        string searchPath(options.AppPath != nullptr ? Core::Directory::Normalize(string(options.AppPath)) : string());

                        result = CheckInstance((searchPath + _T("Plugins/")).c_str(), options.Locator, options.ClassName, options.InterfaceId, options.Version);
                    }
                }
            }
        }

        return (result);
    }

}
} // Process

using namespace WPEFramework;

#ifndef __WINDOWS__
void ExitDaemonHandler(int signo)
{
    TRACE_L1("Signal received %d.", signo);
    syslog(LOG_NOTICE, "Signal received %d.", signo);

    if ((signo == SIGTERM) || (signo == SIGQUIT)) {
        ExitHandler::Construct();
    } else if (signo == SIGSEGV) {
        DumpCallStack();
        // now invoke the default segfault handler
        signal(signo, SIG_DFL);
        kill(getpid(), signo);
    }
}
#endif

#ifdef __WINDOWS__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
    // Give the debugger time to attach to this process..
    // Sleep(20000);

    if (atexit(ExitHandler::Destruct) != 0) {
        TRACE_L1("Could not register @exit handler. Argc %d.", argc);
        ExitHandler::Destruct();
        exit(EXIT_FAILURE);
    } else {
        TRACE_L1("Spawning a new process: %d.", Core::ProcessInfo().Id());
#ifndef __WINDOWS__
            struct sigaction sa;
            memset(&sa, 0, sizeof(struct sigaction));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = ExitDaemonHandler;
            sa.sa_flags = 0; // not SA_RESTART!;

            sigaction(SIGINT, &sa, nullptr);
            sigaction(SIGTERM, &sa, nullptr);
#ifdef __DEBUG__
            sigaction(SIGSEGV, &sa, nullptr);
#endif
            sigaction(SIGQUIT, &sa, nullptr);
#endif
    }

    Process::ConsoleOptions options(argc, argv);

    if ((options.RequestUsage() == true) || (options.Locator == nullptr) || (options.ClassName == nullptr) || (options.RemoteChannel == nullptr) || (options.Exchange == 0)) {
        printf("Process [-h] \n");
        printf("         -C <callsign>\n");
        printf("         -l <locator>\n");
        printf("         -c <classname>\n");
        printf("         -r <communication channel>\n");
        printf("         -x <eXchange identifier>\n");
        printf("        [-i <interface ID>]\n");
        printf("        [-t <thread count>\n");
        printf("        [-V <version>]\n");
        printf("        [-u <user>]\n");
        printf("        [-g <group>]\n");
        printf("        [-p <persistent path>]\n");
        printf("        [-s <system path>]\n");
        printf("        [-d <data path>]\n");
        printf("        [-v <volatile path>]\n");
        printf("        [-a <app path>]\n");
        printf("        [-m <proxy stub library path>]\n");
        printf("        [-e <enabled SYSLOG categories>]\n\n");
        printf("This application spawns a seperate process space for a plugin. The plugins");
        printf("are searched in the same order as they are done in process. Starting from:\n");
        printf(" 1) <persistent path>/<locator>\n");
        printf(" 2) <system path>/<locator>\n");
        printf(" 3) <data path>/<locator>\n");
        printf(" 4) <app path>/Plugins/<locator>\n\n");
        printf("Within the DSO, the system looks for an object with <classname>, this object must implement ");
        printf("the interface, indicated byt the Id <interfaceId>, and if passed, the object should be of ");
        printf("version <version>. All these conditions must met for an object to be instantiated and thus run.\n\n");

        for (uint8_t teller = 0; teller < argc; teller++) {
            printf("Argument [%02d]: %s\n", teller, argv[teller]);
        }
    } else {
        Core::NodeId remoteNode(options.RemoteChannel);

        // Any remote connection that will be spawned from here, will have this ExchangeId as its parent ID.
        Core::SystemInfo::SetEnvironment(_T("COM_PARENT_EXCHANGE_ID"), Core::NumberType<uint32_t>(options.Exchange).Text());

        TRACE_L1("Opening a trace file on %s : [%d].", options.VolatilePath, options.Exchange);

        // Due to the LXC container support all ID's get mapped. For the TraceBuffer, use the host given ID.
        Trace::TraceUnit::Instance().Open(options.VolatilePath, options.Exchange);

        // Time to open up the LOG tracings as specified by the caller.
        Logging::LoggingType<Logging::Startup>::Enable((options.EnabledLoggings & 0x00000001) != 0);
        Logging::LoggingType<Logging::Shutdown>::Enable((options.EnabledLoggings & 0x00000002) != 0);
        Logging::LoggingType<Logging::Notification>::Enable((options.EnabledLoggings & 0x00000004) != 0);

        if (remoteNode.IsValid()) {
            void* base = nullptr;

            TRACE_L1("Spawning a new plugin %s.", options.ClassName);

            // Firts make sure we apply the correct rights to our selves..
            if (options.User != nullptr) {
                Core::ProcessInfo::User(string(options.User));
            }

            if (options.Group != nullptr) {
                Core::ProcessInfo().Group(string(options.Group));
            }

            // Seems like we have enough information, open up the Process communcication Channel.
            Core::ProxyType<Process::WorkerPoolImplementation> invokeServer = Core::ProxyType<Process::WorkerPoolImplementation>::Create(options.Threads, Core::Thread::DefaultStackSize());
            _server = (Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, Core::ProxyType<Core::IIPCServer>(invokeServer)));
            invokeServer->Announcements(_server->Announcement());

            // Register an interface to handle incoming requests for interfaces.
            if ((base = Process::AquireInterfaces(options)) != nullptr) {
                TRACE_L1("Loading ProxyStubs from %s", (options.ProxyStubPath != nullptr ? options.ProxyStubPath : _T("<< No Proxy Stubs Loaded >>")));

                if ((options.ProxyStubPath != nullptr) && (*(options.ProxyStubPath) != '\0')) {
                    Core::Directory index(options.ProxyStubPath, _T("*.so"));

                    while (index.Next() == true) {
                        Core::Library library(index.Current().c_str());

                        if (library.IsLoaded() == true) {
                            _proxyStubs.push_back(library);
                        }
                    }
                }
                TRACE_L1("Interface Aquired. %p.", base);

                uint32_t result;

                // We have something to report back, do so...
                if ((result = _server->Open((RPC::CommunicationTimeOut != Core::infinite ? 2 * RPC::CommunicationTimeOut : RPC::CommunicationTimeOut), options.InterfaceId, base, options.Exchange)) == Core::ERROR_NONE) {
                    TRACE_L1("Process up and running: %d.", Core::ProcessInfo().Id());
                    invokeServer->Run();
                    _server->Close(Core::infinite);
                } else {
                    TRACE_L1("Could not open the connection, error (%d)", result);
                }
            }
        }
    }

    ExitHandler::Destruct();
    return 0;
}
