// process.cpp : Defines the entry point for the console application.
//

#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Process {

    class WorkerPoolImplementation : public Core::IIPCServer, public RPC::WorkerPool {
    private:
        WorkerPoolImplementation() = delete;
        WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
        WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

        class Info {
        public:
            Info()
                : _message()
                , _channel()
                , _job()
            {
            }
            Info(const Core::ProxyType<Core::IPCChannel> channel, const Core::ProxyType<Core::IIPC>& message)
                : _message(message)
                , _channel(channel)
                , _job()
            {
            }
			Info(const Info& copy)
                : _message(copy._message)
                , _channel(copy._channel)
                , _job(copy._job)
            {
			}
            Info(const Core::ProxyType<Core::IDispatch>& job)
                : _message()
                , _channel()
                , _job(job)
            {
            }
            ~Info()
            {
            }

			Info& operator=(const Info& RHS) 
			{
                _message = RHS._message;
                _channel = RHS._channel;
                _job = RHS._job;
 
				return (*this);
			}

        public:
            bool operator==(const Info& RHS) const
            {
                return (_channel.IsValid() ? ((_channel == RHS._channel) && (_message == RHS._message)) : _job == RHS._job);
            }
            bool operator!=(const Info& RHS) const
            {
                return (!operator==(RHS));
            }

            void Dispatch(Core::IIPCServer* announceHandler)
            {
                if (_channel.IsValid() == true) {
                    if (_message->Label() == RPC::InvokeMessage::Id()) {

                        RPC::Job::Invoke(_channel, _message);
                    } else {
                        ASSERT(_message->Label() == RPC::AnnounceMessage::Id());

                        announceHandler->Procedure(*(_channel), _message);
                    }
                    _channel.Release();
                    _message.Release();
                } else {
                    _job->Dispatch();
                    _job.Release();
                }
            }

        private:
            Core::ProxyType<Core::IIPC> _message;
            Core::ProxyType<Core::IPCChannel> _channel;
            Core::ProxyType<Core::IDispatch> _job;
        };
        class Minion : public Core::Thread {
        private:
            Minion() = delete;
            Minion(const Minion&) = delete;
            Minion& operator=(const Minion&) = delete;

        public:
            Minion(WorkerPoolImplementation& parent, const uint32_t stackSize)
                : Core::Thread(stackSize, nullptr)
                , _parent(parent)
            {
            }
            virtual ~Minion()
            {
                Stop();
                Wait(Core::Thread::STOPPED, Core::infinite);
            }

        public:
            virtual uint32_t Worker() override
            {
                _parent.ProcessProcedures();
                Block();
                return (Core::infinite);
            }

        private:
            WorkerPoolImplementation& _parent;
        };
        class TimedJob {
        public:
            TimedJob()
                : _job()
            {
            }
            TimedJob(const Core::ProxyType<Core::IDispatch>& job)
                : _job(job)
            {
            }
            TimedJob(const TimedJob& copy)
                : _job(copy._job)
            {
            }
            ~TimedJob()
            {
            }

            TimedJob& operator=(const TimedJob& RHS)
            {
                _job = RHS._job;
                return (*this);
            }
            bool operator==(const TimedJob& RHS) const
            {
                return (_job == RHS._job);
            }
            bool operator!=(const TimedJob& RHS) const
            {
                return (_job != RHS._job);
            }

        public:
            uint64_t Timed(const uint64_t /* scheduledTime */)
            {
                WorkerPoolImplementation::Instance().Submit(_job);
                _job.Release();

                // No need to reschedule, just drop it..
                return (0);
            }

        private:
            Core::ProxyType<Core::IDispatchType<void>> _job;
        };

        typedef Core::QueueType<Info> MessageQueue;

    public:
        WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize)
            : _handleQueue(64)
            , _runningFree(threads)
            , _minions()
            , _timer(stackSize, _T("WorkerPool::Timer"))
            , _announceHandler(nullptr)
        {
            for (uint8_t index = 1; index < threads; index++) {
                _minions.emplace_back(*this, stackSize);
            }
            if (threads > 1) {
                SYSLOG(Logging::Notification, ("Spawned: %d additional minions.", threads - 1));
            }

            // Register tis as being the worker pool.
            RPC::WorkerPool::Instance(*this);
        }
        ~WorkerPoolImplementation()
        {
            _handleQueue.Disable();
            _minions.clear();
        }
        void ProcessProcedures()
        {
            Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
            Info newRequest;

            while ((admin.Instances() > 0) && (_handleQueue.Extract(newRequest, Core::infinite) == true)) {

                _runningFree--;

                newRequest.Dispatch(_announceHandler);

                _runningFree++;

                // This call might have killed the last living object in our process, if so, commit HaraKiri :-)
                Core::ServiceAdministrator::Instance().FlushLibraries();
            }

            _handleQueue.Disable();
        }

        void Announcements(Core::IIPCServer* announces)
        {
            ASSERT((announces != nullptr) ^ (_announceHandler != nullptr));
            _announceHandler = announces;
        }

        virtual void Submit(const Core::ProxyType<Core::IDispatch>& job) override
        {
            _handleQueue.Insert(Info(job), Core::infinite);
        }
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) override
        {
            _timer.Schedule(time, TimedJob(job));
        }
        virtual uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite) override
        {
            return (_handleQueue.Remove(Info(job)) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        virtual const RPC::WorkerPool::Metadata& Snapshot() const
        {
            return (_metadata);
        }

    private:
        virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
        {
            // Oke, see if we can reference count the IPCChannel
            Core::ProxyType<Core::IPCChannel> newrefChannel(channel);

            ASSERT(newrefChannel.IsValid() == true);

            if (_runningFree == 0) {
                SYSLOG(Logging::Notification, ("Possible DEADLOCK in the hosting"));
            }

            _handleQueue.Insert(Info(newrefChannel, data), Core::infinite);
        }

    private:
        MessageQueue _handleQueue;
        std::atomic<uint8_t> _runningFree;
        std::list<Minion> _minions;
        Core::TimerType<TimedJob> _timer;
        Core::IIPCServer* _announceHandler;
        RPC::WorkerPool::Metadata _metadata;
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

static std::list<Core::Library> _proxyStubs;
static Core::ProxyType<Process::WorkerPoolImplementation> _invokeServer;
static Core::ProxyType<RPC::CommunicatorClient> _server;

//
// It as allowed to call exit from anywhere in the process by any code loaded. This is like using
// a goto in disguise (bad programming). However we need to be robust against mallicious program
// activities.
// To ovecome this jumping and skipping all kinds of code, we install an @exit handler. This should
// one exit and on leaving main always be Called. In this code, we cleanup.
void CloseDown()
{
    TRACE_L1("Entering @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());

    if (_server.IsValid() == true) {
        ASSERT(_invokeServer.IsValid() == true);

        // We are done, close the channel and unregister all shit we added...
        _server->Close(2 * RPC::CommunicationTimeOut);

        _proxyStubs.clear();
    }

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
    // Give the debugger time to attach to this process..
    // Sleep(20000);

    if (atexit(CloseDown) != 0) {
        TRACE_L1("Could not register @exit handler. Argc %d.", argc);
        CloseDown();
        exit(EXIT_FAILURE);
    } else {
        TRACE_L1("Spawning a new process: %d.", Core::ProcessInfo().Id());
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
            _invokeServer = Core::ProxyType<Process::WorkerPoolImplementation>::Create(options.Threads, Core::Thread::DefaultStackSize());
            _server = (Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, Core::ProxyType<Core::IIPCServer>(_invokeServer)));
            _invokeServer->Announcements(_server->Announcement());

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
                    _invokeServer->ProcessProcedures();

                    _server->Close(Core::infinite);
                } else {
                    TRACE_L1("Could not open the connection, error (%d)", result);
                }
            }
        }
    }

    return 0;
}
