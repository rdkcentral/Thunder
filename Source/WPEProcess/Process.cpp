// process.cpp : Defines the entry point for the console application.
//

#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Process {

    class InvokeServer {
    private:
        InvokeServer() = delete;
        InvokeServer(const InvokeServer&) = delete;
        InvokeServer& operator=(const InvokeServer&) = delete;

        struct Info {
            Core::ProxyType<Core::IIPC> message;
            Core::ProxyType<Core::IPCChannel> channel;
        };
        typedef Core::QueueType<Info> MessageQueue;

        class Minion : public Core::Thread {
        private:
            Minion() = delete;
            Minion(const Minion&) = delete;
            Minion& operator=(const Minion&) = delete;

        public:
            Minion(InvokeServer& parent)
                : _parent(parent)
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
            InvokeServer& _parent;
        };

        class InvokeHandlerImplementation : public RPC::ServerType<RPC::InvokeMessage> {
        private:
            InvokeHandlerImplementation() = delete;
            InvokeHandlerImplementation(const InvokeHandlerImplementation&) = delete;
            InvokeHandlerImplementation& operator=(const InvokeHandlerImplementation&) = delete;

        public:
            InvokeHandlerImplementation(MessageQueue* queue, std::atomic<uint8_t>* runningFree)
                : _handleQueue(*queue)
                , _runningFree(*runningFree)
            {
            }
            virtual ~InvokeHandlerImplementation()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::InvokeMessage>& data)
            {
                // Oke, see if we can reference count the IPCChannel
                Info newElement;
                newElement.channel = Core::ProxyType<Core::IPCChannel>(channel);
                newElement.message = data;

                ASSERT(newElement.channel.IsValid() == true);

                if (_runningFree == 0) {
                    SYSLOG(Logging::Notification, ("Possible DEADLOCK in the hosting"));
                }

                _handleQueue.Insert(newElement, Core::infinite);
            }

        private:
            MessageQueue& _handleQueue;
            std::atomic<uint8_t>& _runningFree;
        };
        class AnnounceHandlerImplementation : public RPC::ServerType<RPC::AnnounceMessage> {
        private:
            AnnounceHandlerImplementation() = delete;
            AnnounceHandlerImplementation(const AnnounceHandlerImplementation&) = delete;
            AnnounceHandlerImplementation& operator=(const AnnounceHandlerImplementation&) = delete;

        public:
            AnnounceHandlerImplementation(MessageQueue* queue)
                : _handleQueue(*queue)
            {
            }
            virtual ~AnnounceHandlerImplementation()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::AnnounceMessage>& data) override
            {
                // Oke, see if we can reference count the IPCChannel
                Info newElement;
                newElement.channel = Core::ProxyType<Core::IPCChannel>(channel);
                newElement.message = data;

                ASSERT(newElement.channel.IsValid() == true);

                _handleQueue.Insert(newElement, Core::infinite);
            }

        private:
            MessageQueue& _handleQueue;
        };

    public:
        InvokeServer(const uint8_t threads)
            : _handleQueue(64)
            , _runningFree(threads)
            , _invokeHandler(Core::ProxyType<InvokeHandlerImplementation>::Create(&_handleQueue, &_runningFree))
            , _announceHandler(Core::ProxyType<AnnounceHandlerImplementation>::Create(&_handleQueue))
            , _minions()
        {
            for (uint8_t index = 1; index < threads; index++) {
                _minions.emplace_back(*this);
            }
            if (threads > 1) {
                SYSLOG(Logging::Notification, ("Spawned: %d additional minions.", threads - 1));
            }
        }
        ~InvokeServer()
        {
            _handleQueue.Disable();
            _minions.clear();
        }
        inline const Core::ProxyType<RPC::ServerType<RPC::InvokeMessage> >& InvokeHandler()
        {
            return (_invokeHandler);
        }
        inline const Core::ProxyType<RPC::ServerType<RPC::AnnounceMessage> >& AnnouncementHandler()
        {
            return (_announceHandler);
        }
        void ProcessProcedures()
        {
            Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
            Info newRequest;

            while ((admin.Instances() > 0) && (_handleQueue.Extract(newRequest, Core::infinite) == true)) {

                _runningFree--;

                if (newRequest.message->Label() == RPC::InvokeMessage::Id()) {

                    Core::ProxyType<RPC::InvokeMessage> message(Core::proxy_cast<RPC::InvokeMessage>(newRequest.message));

                    _invokeHandler->Handle(*(newRequest.channel), message);
                } else {
                    ASSERT(newRequest.message->Label() == RPC::AnnounceMessage::Id());

                    Core::ProxyType<RPC::AnnounceMessage> message(Core::proxy_cast<RPC::AnnounceMessage>(newRequest.message));

                    _announceHandler->Handle(*(newRequest.channel), message);
                }

                _runningFree++;

                // This call might have killed the last living object in our process, if so, commit HaraKiri :-)
                Core::ServiceAdministrator::Instance().FlushLibraries();
            }

            _handleQueue.Disable();
        }

    private:
        MessageQueue _handleQueue;
        std::atomic<uint8_t> _runningFree;
        Core::ProxyType<RPC::ServerType<RPC::InvokeMessage>> _invokeHandler;
        Core::ProxyType<RPC::ServerType<RPC::AnnounceMessage>> _announceHandler;
        std::list<Minion> _minions;
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
static Core::ProxyType<Process::InvokeServer> _invokeServer;
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

    if ((options.RequestUsage() == true) || (options.Locator == nullptr) || (options.ClassName == nullptr) || (options.RemoteChannel == nullptr) || (options.Exchange == 0) ) {
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
            _invokeServer = Core::ProxyType<Process::InvokeServer>::Create(options.Threads);
            _server = (Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, _invokeServer->InvokeHandler(), _invokeServer->AnnouncementHandler()));

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
