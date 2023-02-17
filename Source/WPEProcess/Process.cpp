// process.cpp : Defines the entry point for the console application.
//

#include "Module.h"

#ifdef USE_BREAKPAD
#include <client/linux/handler/exception_handler.h>
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {

namespace Process {

    class WorkerPoolImplementation : public Core::IIPCServer, public Core::WorkerPool, public Core::ThreadPool::ICallback {
    private:
        class Dispatcher : public Core::ThreadPool::IDispatcher {
        public:
            Dispatcher(const Dispatcher&) = delete;
            Dispatcher& operator=(const Dispatcher&) = delete;

            Dispatcher(const string& identifier) 
                : _identifier(identifier) {
            }
            ~Dispatcher() override = default;

        private:
            void Initialize() override {
                WARNING_REPORTING_THREAD_SETCALLSIGN(_identifier.c_str());
            }
            void Deinitialize() override {
                WARNING_REPORTING_THREAD_SETCALLSIGN(nullptr);
            }
            void Dispatch(Core::IDispatch* job) override {
            #ifdef __CORE_EXCEPTION_CATCHING__
            try {
                job->Dispatch();
            }
            catch (const std::exception& type) {
                Logging::DumpException(type.what());
            }
            catch (...) {
                Logging::DumpException(_T("Unknown"));
            }
            #else
            job->Dispatch();
            #endif
            }

        private:
            string _identifier;
        };

        class Sink : public Core::ServiceAdministrator::ICallback {
        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            Sink(WorkerPoolImplementation& parent) 
                : _parent(parent)
                , _job(*this) 
            {
            }
POP_WARNING()
            ~Sink() override
            {
            }

        public:
            void Dispatch() {

                uint32_t instances = Core::ServiceAdministrator::Instance().Instances();

                if (instances != 0) {
                    TRACE_L1("We still have living object [%d].", instances);
                }
                else {
                    
                    TRACE_L1("All living objects are killed. Time for HaraKiri!!.");

                    // Seems there is no more live here, time to signal the
                    // WorkerPool to quit running and close down...
                    _parent.Stop();
                }
            }
            void Destructed() override {
                Core::ProxyType<Core::IDispatch> job(_job.Submit());

                if (job.IsValid() == true) {
                    _parent.Submit(job);
                }
            }

        private:
            WorkerPoolImplementation& _parent;
            Core::ThreadPool::JobType<Sink&> _job;
        };

    public:
        WorkerPoolImplementation() = delete;
        WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
        WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize, const string& callsign)
            : WorkerPool(threads - 1, stackSize, queueSize, &_dispatcher, this)
            , _dispatcher(callsign)
            , _announceHandler(nullptr)
            , _sink(*this)
        {
            Core::ServiceAdministrator::Instance().Callback(&_sink);

            if (threads > 1) {
                SYSLOG(Logging::Notification, ("Spawned: %d additional minions.", threads - 1));
            }
        }
POP_WARNING()

        ~WorkerPoolImplementation()
        {
            Core::ServiceAdministrator::Instance().Callback(nullptr);

            // Disable the queue so the minions can stop, even if they are processing and waiting for work..
            Core::WorkerPool::Stop();
        }
        void Announcements(Core::IIPCServer* announces)
        {
            ASSERT((announces != nullptr) ^ (_announceHandler != nullptr));
            _announceHandler = announces;
        }
        void Run()
        {

            Core::WorkerPool::Run();
            Core::WorkerPool::Join();
        }
        void Stop()
        {
            Core::WorkerPool::Stop();
        }
        void Idle() {
            // If we handled all pending requests, it is safe to "unload"
            Core::ServiceAdministrator::Instance().FlushLibraries();
        }

    protected:
        void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
        {
            Core::ProxyType<RPC::Job> job(RPC::Job::Instance());

            job->Set(channel, data, _announceHandler);

            WorkerPool::Submit(Core::ProxyType<Core::IDispatch>(job));
        }
    private:
        Dispatcher _dispatcher;
        Core::IIPCServer* _announceHandler;
        Sink _sink;
    };

    class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Core::Options(argumentCount, arguments, _T("h:l:c:C:r:p:s:d:a:m:i:u:g:t:e:x:V:v:P:S:"))
            , Locator(nullptr)
            , ClassName(nullptr)
            , Callsign(nullptr)
            , RemoteChannel(nullptr)
            , InterfaceId(Core::IUnknown::ID)
            , Version(~0)
            , Exchange(0)
            , PersistentPath()
            , SystemPath()
            , DataPath()
            , VolatilePath()
            , AppPath()
            , ProxyStubPath()
            , PostMortemPath()
            , SystemRootPath()
            , User(nullptr)
            , Group(nullptr)
            , Threads(1)
        {
            Parse();
        }
        ~ConsoleOptions()
        {
        }

    public:
        const TCHAR* Locator;
        const TCHAR* ClassName;
        const TCHAR* Callsign;
        const TCHAR* RemoteChannel;
        uint32_t InterfaceId;
        uint32_t Version;
        uint32_t Exchange;
        string PersistentPath;
        string SystemPath;
        string DataPath;
        string VolatilePath;
        string AppPath;
        string ProxyStubPath;
        string PostMortemPath;
        string SystemRootPath;
        const TCHAR* User;
        const TCHAR* Group;
        uint8_t Threads;

    private:
        string Strip(const TCHAR text[]) const
        {
            int length = static_cast<int>(strlen(text));
            if (length > 0) {
                if (*text != '"') {
                    return (string(text, length));
                } else {
                    return (string(&text[1], (length > 2 ? length - 2 : length - 1)));
                }
            }
            return (string());
        }
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
            case 'l':
                Locator = argument;
                break;
            case 'c':
                ClassName = argument;
                break;
            case 'C':
                Callsign = argument;
                break;
            case 'r':
                RemoteChannel = argument;
                break;
            case 'p':
                PersistentPath = Strip(argument);
                break;
            case 's':
                SystemPath = Strip(argument);
                break;
            case 'd':
                DataPath = Strip(argument);
                break;
            case 'P':
                PostMortemPath = Strip(argument);
                break;
            case 'S':
                SystemRootPath = Strip(argument);
                break;
            case 'v':
                VolatilePath = Strip(argument);
                break;
            case 'a':
                AppPath = Strip(argument);
                break;
            case 'm':
                ProxyStubPath = Strip(argument);
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

    static void* CheckInstance(const string& path, const TCHAR locator[], const TCHAR className[], const uint32_t ID, const uint32_t version)
    {
        void* result = nullptr;

        if (path.empty() == false) {
            Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());

            string libraryPath = locator;
            if (libraryPath.empty() || (libraryPath[0] != '/')) {
                // Relative path, prefix with path name.
                string pathName(Core::Directory::Normalize(path));
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

    static void* AcquireInterfaces(ConsoleOptions& options)
    {
        void* result = nullptr;

        if ((options.Locator != nullptr) && (options.ClassName != nullptr)) {
            string path = (!options.SystemRootPath.empty() ? options.SystemRootPath : "") + options.PersistentPath;
            result = CheckInstance(path, options.Locator, options.ClassName, options.InterfaceId, options.Version);

            if (result == nullptr) {
                path = (!options.SystemRootPath.empty() ? options.SystemRootPath : "") + options.SystemPath;
                result = CheckInstance(path, options.Locator, options.ClassName, options.InterfaceId, options.Version);

                if (result == nullptr) {
                    path = (!options.SystemRootPath.empty() ? options.SystemRootPath : "") + options.DataPath;
                    result = CheckInstance(path, options.Locator, options.ClassName, options.InterfaceId, options.Version);

                    if (result == nullptr) {
                        string searchPath(options.AppPath.empty() == false ? Core::Directory::Normalize(options.AppPath) : string());

                        path = (!options.SystemRootPath.empty() ? options.SystemRootPath : "") + searchPath;
                        result = CheckInstance((path + _T("Plugins/")), options.Locator, options.ClassName, options.InterfaceId, options.Version);
                    }
                }
            }
        }

        return (result);
    }

class ProcessFlow {
private:
    class FactoriesImplementation : public PluginHost::IFactories {
    private:
        FactoriesImplementation(const FactoriesImplementation&) = delete;
        FactoriesImplementation& operator=(const FactoriesImplementation&) = delete;

    public:
        FactoriesImplementation()
            : _requestFactory(2)
            , _responseFactory(2)
            , _fileBodyFactory(2)
            , _jsonRPCFactory(2)
        {
        }
        ~FactoriesImplementation() override {
        }

    public:
        Core::ProxyType<Web::Request> Request() override
        {
            return (_requestFactory.Element());
        }
        Core::ProxyType<Web::Response> Response() override
        {
            return (_responseFactory.Element());
        }
        Core::ProxyType<Web::FileBody> FileBody() override
        {
            return (_fileBodyFactory.Element());
        }
        Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> JSONRPC() override
        {
            return (_jsonRPCFactory.Element());
        }

    private:
        Core::ProxyPoolType<Web::Request> _requestFactory;
        Core::ProxyPoolType<Web::Response> _responseFactory;
        Core::ProxyPoolType<Web::FileBody> _fileBodyFactory;
        Core::ProxyPoolType<Web::JSONBodyType<Core::JSONRPC::Message>> _jsonRPCFactory;
    };

    static void UncaughtExceptions () {
        Logging::DumpException(_T("General"));
    }

public:
    ProcessFlow(const ProcessFlow&) = delete;
    ProcessFlow& operator=(const ProcessFlow&) = delete;

    ProcessFlow()
        : _server()
        , _engine()
        , _proxyStubs()
        , _factories()
    {
        _instance = this;

        TRACE_L1("Spawning a new process: %d.", Core::ProcessInfo().Id());
        #ifndef __WINDOWS__
        struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = ExitDaemonHandler;
        sa.sa_flags = 0; // not SA_RESTART!;

        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGQUIT, &sa, nullptr);
        #endif
        std::set_terminate(UncaughtExceptions);
    }
    virtual ~ProcessFlow()
    {
        // Destruct it all !!.
        _lock.Lock();
        _instance = nullptr;
        _lock.Unlock();

        TRACE_L1("Entering Shutdown. Cleaning up process: %d.", Core::ProcessInfo().Id());

        if (_server.IsValid() == true) {

            // We are done, close the channel and unregister all shit we added...
            #ifdef __DEBUG__
            _server->Close(RPC::CommunicationTimeOut);
            #else       
            _server->Close(2 * RPC::CommunicationTimeOut);
            #endif

            _proxyStubs.clear();

            _server.Release();
        }

        // We are going to tear down the stugg. Unregistere the Worker Pool
        Core::IWorkerPool::Assign(nullptr);
        PluginHost::IFactories::Assign(nullptr);

        if (_engine.IsValid() == true) {
            _engine.Release();
        }

        Core::Singleton::Dispose();
        std::set_terminate(nullptr);
        TRACE_L1("Leaving Shutdown. Cleaned up process: %d.", Core::ProcessInfo().Id());
    }

public:
    static void Abort()
    {
        _lock.Lock();

        if ((_instance != nullptr) && (_instance->_engine.IsValid() == true)) {
            _instance->_engine->Stop();
        }

        _lock.Unlock();
    }
    void Startup(const uint8_t threadCount, const Core::NodeId& remoteNode, const string& callsign)
    {
        // Seems like we have enough information, open up the Process communcication Channel.
        _engine = Core::ProxyType<Process::WorkerPoolImplementation>::Create(threadCount, Core::Thread::DefaultStackSize(), 16, callsign);

        // Whenever someone is looking for a WorkerPool, here it is, register it..
        Core::IWorkerPool::Assign(&(*_engine));

        // Some generic object that require instantiation could come form a generic factory.
        PluginHost::IFactories::Assign(&_factories);

        _server = (Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, Core::ProxyType<Core::IIPCServer>(_engine)));
        _engine->Announcements(_server->Announcement());
    }
    void Run(const string& pathName, const uint32_t interfaceId, void* base, const uint32_t sequenceId)
    {
        uint32_t result;
        uint32_t waitTime (RPC::CommunicationTimeOut != Core::infinite ? 2 * RPC::CommunicationTimeOut : RPC::CommunicationTimeOut);

        TRACE_L1("Loading ProxyStubs from %s", (pathName.empty() == false ? pathName.c_str() : _T("<< No Proxy Stubs Loaded >>")));

        if (pathName.empty() == false) {
            Core::Directory index(pathName.c_str(), _T("*.so"));

            while (index.Next() == true) {
                Core::Library library(index.Current().c_str());

                if (library.IsLoaded() == true) {
                    _proxyStubs.push_back(library);
                }
            }
        }
 
        if ((result = _server->Open(waitTime, interfaceId, base, sequenceId)) == Core::ERROR_NONE) {
            TRACE_L1("Process up and running: %d.", Core::ProcessInfo().Id());
            _engine->Run();
        } else {
            TRACE_L1("Could not open the connection, error (%d)", result);
        }
    }

private:
    #ifndef __WINDOWS__
    static void ExitDaemonHandler(int signo)
    {
        TRACE_L1("Signal received %d.", signo);
        syslog(LOG_NOTICE, "Signal received %d.", signo);

        if ((signo == SIGTERM) || (signo == SIGQUIT)) {

            ProcessFlow::Abort();

        } else if (signo == SIGSEGV) {
            Logging::DumpException(_T("SEIGSEGV"));
            // now invoke the default segfault handler
            signal(signo, SIG_DFL);
            kill(getpid(), signo);
        }
    }
    #endif

private:
    Core::ProxyType<RPC::CommunicatorClient> _server;
    Core::ProxyType<WorkerPoolImplementation> _engine;
    std::list<Core::Library> _proxyStubs;
    FactoriesImplementation _factories;

    static Core::CriticalSection _lock;
    static ProcessFlow* _instance;
};

/* static */ Core::CriticalSection  ProcessFlow::_lock;
/* static */ ProcessFlow*           ProcessFlow::_instance = nullptr;

} // Process

} // WPEFramework

using namespace WPEFramework;

#ifdef __WINDOWS__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
    // Give the debugger time to attach to this process..
    // printf("Starting to sleep so you can attach a debugger\n");
    // Sleep(20000);
    // printf("Continueing, I hope you have attached the debugger\n");

    Process::ConsoleOptions options(argc, argv);

    if ((options.RequestUsage() == true) || (options.Locator == nullptr) || (options.ClassName == nullptr) || (options.RemoteChannel == nullptr) || (options.Exchange == 0)) {
        printf("Process [-h] \n");
        printf("         -l <locator>\n");
        printf("         -c <classname>\n");
        printf("         -C <callsign>\n");
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
        printf("        [-P <post mortem path>]\n\n");
        printf("        [-S <system root path>]\n\n");
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
        string callsign;
        if (options.Callsign != nullptr) {
            Core::ProcessInfo hostProcess;
            const TCHAR* local = options.Callsign;
            const TCHAR* lastEntry = ::strrchr(local, '.');
            if (lastEntry != nullptr) {
                local = &(lastEntry[1]);
            }
  
            hostProcess.Name(local);
            callsign = local;
        }

        // set for the main thread
        WARNING_REPORTING_THREAD_SETCALLSIGN_GUARD(callsign.c_str());

        #ifdef USE_BREAKPAD
        google_breakpad::MinidumpDescriptor descriptor(options.PostMortemPath);
        google_breakpad::ExceptionHandler eh(descriptor, NULL,
            [](const google_breakpad::MinidumpDescriptor&, void*, bool succeeded)
                { return succeeded; },
            NULL, true, -1);
        #endif

        // Any remote connection that will be spawned from here, will have this ExchangeId as its parent ID.
        string parentInfo(Core::NumberType<uint32_t>(options.Exchange).Text());

        if (callsign.empty() == false) {
            parentInfo += ("," + callsign);
        }

        Core::SystemInfo::SetEnvironment(_T("COM_PARENT_INFO"), parentInfo);

        Process::ProcessFlow process;

        Core::NodeId remoteNode(options.RemoteChannel);

        TRACE_L1("Opening a message file with ID: [%d].", options.Exchange);

        // Due to the LXC container support all ID's get mapped. For the MessageBuffer, use the host given ID.
        Messaging::MessageUnit::Instance().Open(options.Exchange);

#ifdef __CORE_WARNING_REPORTING__
        WarningReporting::WarningReportingUnit::Instance().Open(options.Exchange);
#endif

        if (remoteNode.IsValid()) {
            void* base = nullptr;

            TRACE_L1("Spawning a new plugin %s.", options.ClassName);

            // Firts make sure we apply the correct rights to our selves..
            if (options.Group != nullptr) {
                Core::ProcessCurrent().Group(string(options.Group));
            }

            if (options.User != nullptr) {
                Core::ProcessCurrent().User(string(options.User));
            }

            process.Startup(options.Threads, remoteNode, callsign);

            // Register an interface to handle incoming requests for interfaces.
            if ((base = Process::AcquireInterfaces(options)) != nullptr) {

                TRACE_L1("Allright time to start running");
                process.Run(options.ProxyStubPath, options.InterfaceId, base, options.Exchange);
            }
        }

        //close messaging unit before singletons are cleared
        Messaging::MessageUnit::Instance().Close();
    }

    TRACE_L1("End of Process!!!!");
    return 0;
}
