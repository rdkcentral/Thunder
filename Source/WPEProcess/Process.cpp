// process.cpp : Defines the entry point for the console application.
//

#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Process {
    class InvokeServer : public RPC::IHandler {
    private:
        InvokeServer(const InvokeServer&) = delete;
        InvokeServer& operator=(const InvokeServer&) = delete;

        struct Info {
            Core::ProxyType<Core::IIPC> message;
            Core::ProxyType<Core::IPCChannel> channel;
        };
        typedef Core::QueueType<Info> MessageQueue;

        class InvokeHandlerImplementation : public Core::IPCServerType<RPC::InvokeMessage> {
        private:
            InvokeHandlerImplementation() = delete;
            InvokeHandlerImplementation(const InvokeHandlerImplementation&) = delete;
            InvokeHandlerImplementation& operator=(const InvokeHandlerImplementation&) = delete;

        public:
            InvokeHandlerImplementation (MessageQueue* queue) : _handleQueue(*queue) {
            }
            virtual ~InvokeHandlerImplementation() {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::InvokeMessage>& data)
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
        class AnnounceHandlerImplementation : public Core::IPCServerType<RPC::AnnounceMessage> {
        private:
            AnnounceHandlerImplementation() = delete;
            AnnounceHandlerImplementation(const AnnounceHandlerImplementation&) = delete;
            AnnounceHandlerImplementation& operator=(const AnnounceHandlerImplementation&) = delete;

        public:
            AnnounceHandlerImplementation(MessageQueue* queue) : _handleQueue(*queue) {
            }
            virtual ~AnnounceHandlerImplementation() {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::AnnounceMessage>& data)
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
        InvokeServer()
            : _handleQueue(64)
            , _handler(RPC::Administrator::Instance())
            , _invokeHandler(Core::ProxyType<InvokeHandlerImplementation>::Create(&_handleQueue))
            , _announceHandler(Core::ProxyType<AnnounceHandlerImplementation>::Create(&_handleQueue))
            , _announcements(nullptr)
        {
        }

        ~InvokeServer()
        {
        }

        virtual Core::ProxyType<Core::IIPCServer> InvokeHandler() override {
            return (_invokeHandler);
        }
        virtual Core::ProxyType<Core::IIPCServer> AnnounceHandler() override {
            return (_announceHandler);
        }
        virtual void AnnounceHandler(Core::IPCServerType<RPC::AnnounceMessage>* handler) override {

            // Concurrency aspect is out of scope as the implementation of this interface is currently limited
            // to the RPC::COmmunicator and RPC::CommunicatorClient. Both of these implementations will first
            // set this callback before any communication is happeing (Open happens after this)
            // Also the announce handler will not be removed until the line is closed and the server (or client)
            // is destructed!!!
            ASSERT ((handler == nullptr) ^ (_announcements == nullptr));

            _announcements = handler;
        }
        void ProcessProcedures()
        {
            Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
            Info newRequest;

            while ((admin.Instances() > 0) && (_handleQueue.Extract(newRequest, Core::infinite) == true)) {

               if (newRequest.message->Label() == RPC::InvokeMessage::Id()) {
                    Core::ProxyType<RPC::InvokeMessage> message (Core::proxy_cast<RPC::InvokeMessage>(newRequest.message));

                    _handler.Invoke(newRequest.channel, message);
                    newRequest.channel->ReportResponse(newRequest.message);
                }
                else {
                    ASSERT (newRequest.message->Label() == RPC::AnnounceMessage::Id());

                    Core::ProxyType<RPC::AnnounceMessage> message (Core::proxy_cast<RPC::AnnounceMessage>(newRequest.message));

                     _announcements->Procedure(*(newRequest.channel), message);
                }

                // This call might have killed the last living object in our process, if so, commit HaraKiri :-)
                admin.FlushLibraries();
            }
        }
    private:
        MessageQueue _handleQueue;
        RPC::Administrator& _handler;
        Core::ProxyType<Core::IPCServerType<RPC::InvokeMessage> > _invokeHandler;
        Core::ProxyType<Core::IPCServerType<RPC::AnnounceMessage> > _announceHandler;
        Core::IPCServerType<RPC::AnnounceMessage>* _announcements;
    };

    class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Core::Options(argumentCount, arguments, _T("h:l:c:r:p:s:d:a:m:i:u:g:"))
            , Locator(nullptr)
            , ClassName(nullptr)
            , RemoteChannel(nullptr)
            , InterfaceId(Core::IUnknown::ID)
            , Version(~0)
            , PersistentPath(nullptr)
            , SystemPath(nullptr)
            , DataPath(nullptr)
            , AppPath(nullptr)
            , ProxyStubPath(nullptr)
            , User(nullptr)
            , Group(nullptr)
        {
            Parse();
        }
        ~ConsoleOptions()
        {
        }

    public:
        const TCHAR* Locator;
        const TCHAR* ClassName;
        const TCHAR* RemoteChannel;
        uint32_t InterfaceId;
        uint32_t Version;
        const TCHAR* PersistentPath;
        const TCHAR* SystemPath;
        const TCHAR* DataPath;
        const TCHAR* AppPath;
        const TCHAR* ProxyStubPath;
        const TCHAR* User;
        const TCHAR* Group;

    private:
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
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
            case 'v':
                Version = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
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
    }
    else {
        TRACE_L1("Spawning a new process: %d.", Core::ProcessInfo().Id());
    }

    Process::ConsoleOptions options(argc, argv);

    if ((options.RequestUsage() == true) || (options.Locator == nullptr) || (options.ClassName == nullptr) || (options.RemoteChannel == nullptr)) {
        printf("Process [-h] \n");
        printf("         -l <locator>\n");
        printf("         -c <classname>\n");
        printf("         -r <communication channel>\n");
        printf("        [-i <interface ID>]\n");
        printf("        [-v <version>]\n");
        printf("        [-u <user>]\n");
        printf("        [-g <group>]\n");
        printf("        [-p <persistent path>]\n");
        printf("        [-s <system path>]\n");
        printf("        [-d <data path>]\n");
        printf("        [-a <app path>]\n\n");
        printf("        [-m <proxy stub library path>]\n\n");
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
    }
    else {
        Core::NodeId remoteNode(options.RemoteChannel);

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
            _invokeServer = Core::ProxyType<Process::InvokeServer>::Create();
            _server = (Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, _invokeServer));

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
				if ((result = _server->Open((RPC::CommunicationTimeOut != Core::infinite ? 2 * RPC::CommunicationTimeOut : RPC::CommunicationTimeOut), options.InterfaceId, base)) == Core::ERROR_NONE) {
                    TRACE_L1("Process up and running: %d.", Core::ProcessInfo().Id());
                    _invokeServer->ProcessProcedures();

					_server->Close(Core::infinite);
				}
                else {
                    TRACE_L1("Could not open the connection, error (%d)", result);
                }
            }
        }
    }

    return 0;
}
