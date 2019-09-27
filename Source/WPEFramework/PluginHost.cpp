#include "PluginServer.h"
// #include "Portability.h"

#ifndef __WIN32__
#include <dlfcn.h> // for dladdr
#include <syslog.h>
#endif

#ifdef __LINUX__
#include <execinfo.h>
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace PluginHost {

    static PluginHost::Server* _dispatcher = nullptr;
    static bool _background = false;

    class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Core::Options(argumentCount, arguments, _T(":bhc:"))
            , configFile(Server::ConfigFile)
        {
            Parse();
        }
        ~ConsoleOptions()
        {
        }

    public:
        const TCHAR* configFile;

    private:
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
            case 'c':
                configFile = argument;
                break;
#ifndef __WIN32__
            case 'b':
                _background = true;
                break;
#endif
            case 'h':
            default:
                RequestUsage(true);
                break;
            }
        }
    };
    extern "C" {
    static void CloseDown();
    }
#ifndef __WIN32__
    class SignalHandler : public Core::Thread {
    private:
        SignalHandler(const SignalHandler&) = delete;
        SignalHandler& operator=(const SignalHandler&) = delete;

    public:
        SignalHandler()
            : Core::Thread(Core::Thread::DefaultStackSize(), nullptr)
        {
            sigemptyset(&_signalSet);
            sigaddset(&_signalSet, SIGINT);
            sigaddset(&_signalSet, SIGTERM);
            sigaddset(&_signalSet, SIGSEGV);
            sigprocmask(SIG_BLOCK, &_signalSet, 0);
        }
        virtual ~SignalHandler()
        {
            Stop();
            Wait(Core::Thread::STOPPED, Core::infinite);
        }

    private:
        virtual uint32_t Worker() override
        {
            int status, signal;

            status = sigwait(&_signalSet, &signal);
            if (status == 0) {
                ExitDaemonHandler(signal);
            }
            Stop();
            return (Core::infinite);
        }
        void ExitDaemonHandler(int signo)
        {
            if (_background) {
                syslog(LOG_NOTICE, "Signal received %d.", signo);
            } else {
                fprintf(stderr, "Signal received %d.\n", signo);
            }

            if (signo == SIGTERM) {
                if (Core::WorkerPool::IsAvailable() == true) {
                   CloseDown(); // Do it before stopping all the workers;
                   Core::WorkerPool::Instance().Stop();
                }
            } else if (signo == SIGSEGV) {
                DumpCallStack();
                // now invoke the default segfault handler
                signal(signo, SIG_DFL);
                kill(getpid(), signo);
           }
        }
    private:
        sigset_t _signalSet;
    };
#endif
    extern "C" {

    static void CloseDown()
    {
        TRACE_L1("Entering @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());

        if (_dispatcher != nullptr) {
            PluginHost::Server* destructor = _dispatcher;
            destructor->Close();
            _dispatcher = nullptr;
            delete destructor;

#ifndef __WIN32__
            if (_background) {
                syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon closed down.");
            } else
#endif
            {
                fprintf(stdout, EXPAND_AND_QUOTE(APPLICATION_NAME) " closed down.\n");
            }

#ifndef __WIN32__
            closelog();
#endif
            // Now clear all singeltons we created.
            Core::Singleton::Dispose();
        }

        TRACE_L1("Leaving @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());
    }

    void LoadPlugins(const string& name, Server::Config& config)
    {
        Core::Directory pluginDirectory(name.c_str(), _T("*.json"));

        while (pluginDirectory.Next() == true) {

            Core::File file(pluginDirectory.Current(), true);

            if (file.Exists()) {
                if (file.IsDirectory()) {
                    if ((file.FileName() != ".") && (file.FileName() != "..")) {
                        LoadPlugins(file.Name(), config);
                    }
                } else if (file.Open(true) == false) {
                    SYSLOG(Logging::Startup, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
                } else {
                    Plugin::Config pluginConfig;
                    pluginConfig.FromFile(file);
                    file.Close();

                    if ((pluginConfig.ClassName.Value().empty() == true) || (pluginConfig.Locator.Value().empty() == true)) {
                        SYSLOG(Logging::Startup, (_T("Plugin config file [%s] does not contain classname or locator."), file.Name().c_str()));
                    } else {
                        if (pluginConfig.Callsign.Value().empty() == true) {
                            pluginConfig.Callsign = Core::File::FileName(file.FileName());
                        }

                        Core::JSON::ArrayType<Plugin::Config>::Iterator index(config.Plugins.Elements());

                        // Check if there is already a plugin config with this callsign
                        while ((index.Next() == true) && (index.Current().Callsign.Value() != pluginConfig.Callsign.Value()))
                            ;

                        if (index.IsValid() == true) {
                            SYSLOG(Logging::Startup, (_T("Plugin config file [%s] can not be reconfigured."), file.Name().c_str()));
                        } else {
                            config.Plugins.Add(pluginConfig);
                        }
                    }
                }
            }
        }
    }

#ifndef __WIN32__
    void StartLoopbackInterface()
    {
        Core::AdapterIterator adapter;
        uint8_t retries = 8;
        // Some interfaces take some time, to be available. Wait a certain amount
        // of time in which the interface should come up.
        do {
            adapter = Core::AdapterIterator(_T("lo"));

            if (adapter.IsValid() == false) {
                Core::AdapterIterator::Flush();
                SleepMs(500);
            }

        } while ((retries-- != 0) && (adapter.IsValid() == false));

        if (adapter.IsValid() == false) {
            SYSLOG(Logging::Startup, (_T("Interface [lo], not available")));
        } else {

            adapter.Up(true);
            adapter.Add(Core::IPNode(Core::NodeId("127.0.0.1"), 8));

            retries = 40;
            Core::NodeId nodeId;

            // Last thing we need to wait for is the resolve of localhost to work.
            do {
                nodeId = Core::NodeId(_T("localhost"));

                if (nodeId.IsValid() == false) {
                    SleepMs(100);
                }

            } while ((retries-- != 0) && (nodeId.IsValid() == false));

            if (retries != 0) {
                SYSLOG(Logging::Startup, (string(_T("Interface [lo], fully functional"))));
            } else {
                SYSLOG(Logging::Startup, (string(_T("Interface [lo], partly functional (no name resolving)"))));
            }
        }
    }
#endif

    static void PublishCallstack(const ::ThreadId threadId)
    {
#ifndef __WIN32__
        void* callstack[32];
        uint32_t entries = ::GetCallStack(threadId, callstack, (sizeof(callstack) / sizeof(void*)));
        char** symbols = backtrace_symbols(callstack, entries);

        for (uint32_t i = 0; i < entries; i++) {
            Dl_info info;
            if (dladdr(callstack[i], &info) && info.dli_sname) {
                char* demangled = NULL;
                int status = -1;
                if (info.dli_sname[0] == '_')
                    demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
                fprintf(stdout, "%-3d %*p %s + %zd\n", i, int(2 + sizeof(void*) * 2), callstack[i],
                    status == 0 ? demangled : info.dli_sname == 0 ? symbols[i] : info.dli_sname,
                    (char*)callstack[i] - (char*)info.dli_saddr);
                free(demangled);
            } else {
                fprintf(stdout, "%-3d %*p %s\n",
                    i, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
            }
        }
        free(symbols);
#endif
    }

#ifdef __WIN32__
    int _tmain(int argc, _TCHAR* argv[])
#else
    int main(int argc, char** argv)
#endif
    {
#ifndef __WIN32__
        //Set our Logging Mask and open the Log
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog(argv[0], LOG_PID, LOG_USER);

        setsid();
#endif

        ConsoleOptions options(argc, argv);

        if (atexit(CloseDown) != 0) {
            TRACE_L1("Could not register @exit handler. Argc %d.", argc);
            CloseDown();
            exit(EXIT_FAILURE);
        } else if (options.RequestUsage()) {
#ifndef __WIN32__
            syslog(LOG_ERR, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon failed to start. Incorrect Options.");
#endif
            if ((_background == false) && (options.RequestUsage())) {
                fprintf(stderr, "Usage: " EXPAND_AND_QUOTE(APPLICATION_NAME) " [-c <config file>] -b\n");
                fprintf(stderr, "       -c <config file>  Define the configuration file to use.\n");
                fprintf(stderr, "       -b                Run " EXPAND_AND_QUOTE(APPLICATION_NAME) " in the background.\n");
            }
            exit(EXIT_FAILURE);
            ;
        }
#ifndef __WIN32__
        if (_background == true) {
            //Close Standard File Descriptors
            // close(STDIN_FILENO);
            // close(STDOUT_FILENO);
            // close(STDERR_FILENO);
            syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon starting");
        } else
#endif

            Logging::SysLog(!_background);

        // Read the config file, to instantiate the proper plugins and for us to open up the right listening ear.
        Core::File configFile(string(options.configFile), false);
        Server::Config serviceConfig;

        if (configFile.Open(true) == true) {
            serviceConfig.FromFile(configFile);

            configFile.Close();
        } else {
#ifndef __WIN32__
            if (_background == true) {
                syslog(LOG_WARNING, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon failed to start. Incorrect Config file.");
            } else
#endif
            {
                fprintf(stdout, "Config file [%s] could not be opened.\n", options.configFile);
            }
        }

        if (serviceConfig.Process.IsSet() == true) {

            Core::ProcessInfo myself;

            if (serviceConfig.Process.OOMAdjust.IsSet() == true) {
                myself.Priority(serviceConfig.Process.OOMAdjust.Value());
            }

            if (serviceConfig.Process.Priority.IsSet()) {
                myself.Priority(serviceConfig.Process.Priority.Value());
            }

            if (serviceConfig.Process.Policy.IsSet()) {
                myself.Policy(serviceConfig.Process.Policy.Value());
            }

            if (serviceConfig.Process.User.IsSet()) {
                Core::ProcessInfo::User(serviceConfig.Process.User.Value());
            }

            if (serviceConfig.Process.Group.IsSet()) {
                myself.Group(serviceConfig.Process.Group.Value());
            }

            if (serviceConfig.Process.StackSize.IsSet() == true) {
                Core::Thread::DefaultStackSize(serviceConfig.Process.StackSize.Value());
            }
        }

#ifndef __WIN32__
        ::umask(serviceConfig.Process.Umask.Value());
#endif
        // Time to start loading the config of the plugins.
        string pluginPath(serviceConfig.Configs.Value());

        if (pluginPath.empty() == true) {
            pluginPath = Core::Directory::Normalize(Core::File::PathName(options.configFile));
            pluginPath += Server::PluginConfigDirectory;
        }
        else {
            pluginPath = Core::Directory::Normalize(pluginPath);
        }

        string traceSettings (options.configFile);
 
        // Time to open up, the trace buffer for this process and define it for the out-of-proccess systems
        // Define the environment variable for Tracing files, if it is not already set.
        Trace::TraceUnit::Instance().Open(serviceConfig.VolatilePath.Value(), 0);

        if (serviceConfig.DefaultTraceCategories.IsQuoted() == true) {

            traceSettings = Core::Directory::Normalize(Core::File::PathName(options.configFile)) + serviceConfig.DefaultTraceCategories.Value();

            Core::File input (traceSettings, true);

            if (input.Open(true)) {
                Trace::TraceUnit::Instance().Defaults(input);
            }
        }
        else {
            Trace::TraceUnit::Instance().Defaults(serviceConfig.DefaultTraceCategories.Value());
        }

        SYSLOG(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME))));
        SYSLOG(Logging::Startup, (_T("Starting time: %s"), Core::Time::Now().ToRFC1123(false).c_str()));
        SYSLOG(Logging::Startup, (_T("Process Id:    %d"), Core::ProcessInfo().Id()));
        SYSLOG(Logging::Startup, (_T("SystemId:      %s"), Core::SystemInfo::Instance().Id(Core::SystemInfo::Instance().RawDeviceId(), ~0).c_str()));
        SYSLOG(Logging::Startup, (_T("Tree ref:      " _T(EXPAND_AND_QUOTE(TREE_REFERENCE)))));
        SYSLOG(Logging::Startup, (_T("Build ref:     " _T(EXPAND_AND_QUOTE(BUILD_REFERENCE)))));
        SYSLOG(Logging::Startup, (_T("Version:       %s"), serviceConfig.Version.Value().c_str()));
        SYSLOG(Logging::Startup, (_T("Traces:        %s"), traceSettings.c_str()));

#ifndef __WIN32__
        // We need at least the loopback interface before we continue...
        StartLoopbackInterface();
#endif

        // Before we do any translation of IP, make sure we have the right network info...
        if (serviceConfig.IPV6.Value() == false) {
            SYSLOG(Logging::Startup, (_T("Forcing the network to IPv4 only.")));
            Core::NodeId::ClearIPV6Enabled();
        }

        // Load plugin configs from a directory.
        LoadPlugins(pluginPath, serviceConfig);

        // Startup/load/initialize what we found in the configuration.
        _dispatcher = new PluginHost::Server(serviceConfig, _background);

        SYSLOG(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME) " actively listening.")));

        // If we have handlers open up the gates to analyze...
        _dispatcher->Open();

#ifndef __WIN32__
        if (_background == true) {
            SignalHandler signalHandler;
            signalHandler.Run();
            Core::WorkerPool::Instance().Join();
        } else
#endif
        {

            char keyPress;

            do {
                keyPress = toupper(getchar());

                switch (keyPress) {
                case 'C': {
                    Core::JSON::ArrayType<MetaData::Channel> metaData;
                    _dispatcher->Dispatcher().GetMetaData(metaData);
                    Core::JSON::ArrayType<MetaData::Channel>::Iterator index(metaData.Elements());

                    printf("\nChannels:\n");
                    printf("============================================================\n");
                    while (index.Next() == true) {
                        printf("ID:         %d\n", index.Current().ID.Value());
                        printf("State:      %s\n", index.Current().JSONState.Data().c_str());
                        printf("Active:     %s\n", (index.Current().Activity.Value() == true ? _T("true") : _T("false")));
                        printf("Remote:     %s\n", (index.Current().Remote.Value().c_str()));
                        printf("Name:       %s\n\n", (index.Current().Name.Value().c_str()));
                    }
                    break;
                }
                case 'P': {
                    Core::JSON::ArrayType<MetaData::Service> metaData;
                    _dispatcher->Services().GetMetaData(metaData);
                    Core::JSON::ArrayType<MetaData::Service>::Iterator index(metaData.Elements());

                    printf("\nPlugins:\n");
                    printf("============================================================\n");
                    while (index.Next() == true) {
                        printf("Callsign:   %s\n", index.Current().Callsign.Value().c_str());
                        printf("State:      %s\n", index.Current().JSONState.Data().c_str());
                        printf("Locator:    %s\n", index.Current().Locator.Value().c_str());
                        printf("Classname:  %s\n", index.Current().ClassName.Value().c_str());
                        printf("Autostart:  %s\n", (index.Current().AutoStart.Value() == true ? _T("true") : _T("false")));
#ifdef RESTFULL_API

                        printf("Observers:  %d\n", index.Current().Observers.Value());
#endif
                        printf("Requests:   %d\n", index.Current().ProcessedRequests.Value());
                        printf("JSON:       %d\n\n", index.Current().ProcessedObjects.Value());
                    }
                    break;
                }
                case 'S': {
                    const Core::WorkerPool::Metadata metaData = Core::WorkerPool::Instance().Snapshot();
                    PluginHost::ISubSystem* status(_dispatcher->Services().SubSystemsInterface());

                    printf("\nServer statistics:\n");
                    printf("============================================================\n");
#ifdef SOCKET_TEST_VECTORS
                    printf("Monitorruns: %d\n", Core::ResourceMonitor::Instance().Runs());
#endif
                    if (status != nullptr) {
                        uint8_t buffer[64] = {};

                        const PluginHost::ISubSystem::IIdentifier* id(status->Get<PluginHost::ISubSystem::IIdentifier>());
                        const PluginHost::ISubSystem::IInternet* internet(status->Get<PluginHost::ISubSystem::IInternet>());
                        const PluginHost::ISubSystem::ILocation* location(status->Get<PluginHost::ISubSystem::ILocation>());
                        const PluginHost::ISubSystem::ITime* time(status->Get<PluginHost::ISubSystem::ITime>());

                        if (id != nullptr) {
                            buffer[0] = static_cast<const ISubSystem::IIdentifier*>(id)
                                            ->Identifier(sizeof(buffer) - 1, &(buffer[1]));
                        }

                        string identifier = Core::SystemInfo::Instance().Id(buffer, ~0);

                        printf("------------------------------------------------------------\n");
                        printf("State:\n");
                        printf("Platform:     %s\n",
                            (status->IsActive(PluginHost::ISubSystem::PLATFORM) == true) ? "Available"
                                                                                         : "Unavailable");
                        printf("Security:     %s\n",
                            (status->IsActive(PluginHost::ISubSystem::SECURITY) == true) ? "Available"
                                                                                         : "Unavailable");
                        printf("Network:      %s\n",
                            (status->IsActive(PluginHost::ISubSystem::NETWORK) == true) ? "Available"
                                                                                        : "Unavailable");
                        printf("Identifier:   %s\n",
                            (status->IsActive(PluginHost::ISubSystem::IDENTIFIER) == true) ? "Available"
                                                                                           : "Unavailable");
                        printf("Internet  :   %s\n",
                            (status->IsActive(PluginHost::ISubSystem::INTERNET) == true) ? "Available"
                                                                                         : "Unavailable");
                        printf("Graphics:     %s\n",
                            (status->IsActive(PluginHost::ISubSystem::GRAPHICS) == true) ? "Available"
                                                                                         : "Unavailable");
                        printf("Location:     %s\n",
                            (status->IsActive(PluginHost::ISubSystem::LOCATION) == true) ? "Available"
                                                                                         : "Unavailable");
                        printf("Time:         %s\n",
                            (status->IsActive(PluginHost::ISubSystem::TIME) == true) ? "Available"
                                                                                     : "Unavailable");
                        printf("Provisioning: %s\n",
                            (status->IsActive(PluginHost::ISubSystem::PROVISIONING) == true) ? "Available"
                                                                                             : "Unavailable");
                        printf("Decryption:   %s\n",
                            (status->IsActive(PluginHost::ISubSystem::DECRYPTION) == true) ? "Available"
                                                                                           : "Unavailable");
                        printf("WebSource:    %s\n",
                            (status->IsActive(PluginHost::ISubSystem::WEBSOURCE) == true) ? "Available"
                                                                                          : "Unavailable");
                        printf("Streaming:    %s\n",
                            (status->IsActive(PluginHost::ISubSystem::STREAMING) == true) ? "Available"
                                                                                          : "Unavailable");
                        printf("Bluetooth:    %s\n",
                            (status->IsActive(PluginHost::ISubSystem::BLUETOOTH) == true) ? "Available"
                                                                                          : "Unavailable");
                        printf("------------------------------------------------------------\n");
                        if (status->IsActive(PluginHost::ISubSystem::INTERNET) == true) {
                            printf("Network Type: %s\n",
                                (internet->NetworkType() == PluginHost::ISubSystem::IInternet::UNKNOWN ? "Unknown" : (internet->NetworkType() == PluginHost::ISubSystem::IInternet::IPV6 ? "IPv6" : "IPv4")));
                            printf("Public IP:   %s\n", internet->PublicIPAddress().c_str());
                        }

                        printf("------------------------------------------------------------\n");
                        if (status->IsActive(PluginHost::ISubSystem::LOCATION) == true) {
                            printf("TimeZone:    %s\n", location->TimeZone().c_str());
                            printf("Country:     %s\n", location->Country().c_str());
                            printf("Region:      %s\n", location->Region().c_str());
                            printf("City:        %s\n", location->City().c_str());
                        }

                        printf("------------------------------------------------------------\n");
                        if (status->IsActive(PluginHost::ISubSystem::TIME) == true) {
                            uint64_t lastSync = time->TimeSync();

                            if (lastSync != 0) {
                                printf("Time sync:   %s\n", Core::Time(lastSync).ToRFC1123(false).c_str());
                            } else {
                                printf("Time sync:   NOT SYNCHRONISED\n");
                            }
                        }

                        printf("------------------------------------------------------------\n");
                        if (status->IsActive(PluginHost::ISubSystem::IDENTIFIER) == true) {
                            printf("Identifier:  %s\n", identifier.c_str());
                        }

                        printf("------------------------------------------------------------\n");
                        status->Release();
                    } else {
                        printf("------------------------------------------------------------\n");
                        printf("SystemState: UNKNOWN\n");
                        printf("------------------------------------------------------------\n");
                    }
                    printf("Pending:     %d\n", metaData.Pending);
                    printf("Occupation:  %d\n", metaData.Occupation);
                    printf("Poolruns:\n");
                    for (uint8_t index = 0; index < metaData.Slots; index++) {
                        printf("  Thread%02d:  %d\n", (index + 1), metaData.Slot[index]);
                    }
                    status->Release();
                    break;
                }
#if !defined(__WIN32__) && !defined(__APPLE__)
                case 'M': {
                    printf("\nMonitor callstack:\n");
                    printf("============================================================\n");
                    PublishCallstack(Core::ResourceMonitor::Instance().Id());
                    break;
                }
                case 'Q':
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8': {
                    uint32_t threadId = _dispatcher->WorkerPool().ThreadId(keyPress - '0');
                    printf("\nThreadPool thread[%c,%d] callstack:\n", keyPress, threadId);
                    printf("============================================================\n");
                    PublishCallstack(threadId);
                    break;
                }
#endif

                case '?':
                    printf("\nOptions are: [P]lugins, [C]hannels, [S]erver stats, [M] Socket monitor stack, [0..8] Workerpool stack and [Q]uit\n\n");
                    break;

                default:
                    break;
                }

            } while (keyPress != 'Q');
        }
        CloseDown();

        return 0;

    } // End main.
    } // extern "C"
}
}
