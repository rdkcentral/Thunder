/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PluginServer.h"

#ifndef __WINDOWS__
#include <dlfcn.h> // for dladdr
#include <syslog.h>
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
    static PluginHost::Config* _config = nullptr;
    static PluginHost::Server* _dispatcher = nullptr;
    static bool _background = false;

namespace PluginHost {

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
#ifndef __WINDOWS__
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

        static void Construct() {
            _adminLock.Lock();
            if (_instance == nullptr) {
                _instance = new WPEFramework::PluginHost::ExitHandler();
                _instance->Run();
            }
            _adminLock.Unlock();
        }
        static void Destruct() {
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

            if (_dispatcher != nullptr) {
                PluginHost::Server* destructor = _dispatcher;
                destructor->Close();
                _dispatcher = nullptr;
                delete destructor;

                delete _config;
                _config = nullptr;

#ifndef __WINDOWS__
                if (_background) {
                    syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon closed down.");
                } else
#endif
                {
                   fprintf(stdout, EXPAND_AND_QUOTE(APPLICATION_NAME) " closed down.\n");
                }

#ifndef __WINDOWS__
                closelog();
#endif
                // Now clear all singeltons we created.
                Core::Singleton::Dispose();
            }

            TRACE_L1("Leaving @Exit. Cleaning up process: %d.", Core::ProcessInfo().Id());
        }
        private:
            static ExitHandler* _instance;
            static Core::CriticalSection _adminLock;
    };

    ExitHandler* ExitHandler::_instance = nullptr;
    Core::CriticalSection ExitHandler::_adminLock;

    extern "C" {

#ifndef __WINDOWS__

    void ExitDaemonHandler(int signo)
    {
        if (_background) {
            syslog(LOG_NOTICE, "Signal received %d. in process [%d]", signo, getpid());
        } else {
            fprintf(stderr, "Signal received %d. in process [%d]\n", signo, getpid());
        }

        if ((signo == SIGTERM) || (signo == SIGQUIT)) {
            ExitHandler::Construct();
        }
    }

#endif

    void LoadPlugins(const string& name, PluginHost::Config& config)
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
                    Core::OptionalType<Core::JSON::Error> error;
                    pluginConfig.IElement::FromFile(file, error);
                    if (error.IsSet() == true) {
                        SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                    }
                    file.Close();

                    if ((pluginConfig.ClassName.Value().empty() == true) || (pluginConfig.Locator.Value().empty() == true)) {
                        SYSLOG(Logging::Startup, (_T("Plugin config file [%s] does not contain classname or locator."), file.Name().c_str()));
                    } else {
                        if (pluginConfig.Callsign.Value().empty() == true) {
                            pluginConfig.Callsign = Core::File::FileName(file.FileName());
                        }

                        if (config.Add(pluginConfig) == false) {
                            SYSLOG(Logging::Startup, (_T("Plugin config file [%s] can not be reconfigured."), file.Name().c_str()));
                        }
                    }
                }
            }
        }
    }

#ifndef __WINDOWS__
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

#ifdef __WINDOWS__
    int _tmain(int argc, _TCHAR* argv[])
#else
    int main(int argc, char** argv)
#endif
    {
#ifndef __WINDOWS__
        //Set our Logging Mask and open the Log
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog(argv[0], LOG_PID, LOG_USER);

        setsid();
#endif

        ConsoleOptions options(argc, argv);

        if (atexit(ExitHandler::Destruct) != 0) {
            TRACE_L1("Could not register @exit handler. Argc %d.", argc);
            ExitHandler::Destruct();
            exit(EXIT_FAILURE);
        } else if (options.RequestUsage()) {
#ifndef __WINDOWS__
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
#ifndef __WINDOWS__
        else {
            struct sigaction sa;
            memset(&sa, 0, sizeof(struct sigaction));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = ExitDaemonHandler;
            sa.sa_flags = 0; // not SA_RESTART!;

            sigaction(SIGINT, &sa, nullptr);
            sigaction(SIGTERM, &sa, nullptr);
            sigaction(SIGQUIT, &sa, nullptr);
        }

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
        if (configFile.Open(true) == true) {
            Core::OptionalType<Core::JSON::Error> error;
            _config = new Config(configFile, _background, error);

            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                delete _config;
                _config = nullptr;
            }

            configFile.Close();
        } else {
#ifndef __WINDOWS__
            if (_background == true) {
                syslog(LOG_WARNING, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon failed to start. Incorrect Config file.");
            } else
#endif
            {
                fprintf(stdout, "Config file [%s] could not be opened.\n", options.configFile);
            }
        }

        if (_config != nullptr) {

            if (_config->Process().IsSet() == true) {

                Core::ProcessInfo myself;

                if (_config->Process().OOMAdjust() != 0) {
                    myself.OOMAdjust(_config->Process().OOMAdjust());
                }
                if (_config->Process().Priority() != 0) {
                    myself.Priority(_config->Process().Priority());
                }
                if (_config->Process().User().empty() == false) {
                    myself.User(_config->Process().User());
                }
                if (_config->Process().Group().empty() == false) {
                    myself.Group(_config->Process().Group());
                }
                if (_config->StackSize() != 0) {
                    Core::Thread::DefaultStackSize(_config->StackSize()); 
                }

#ifndef __WINDOWS__
                if (_config->Process().UMask() != 0) {
                    ::umask(_config->Process().UMask());
                }
#endif
                myself.Policy(_config->Process().Policy());
            }

            // Time to start loading the config of the plugins.
            string pluginPath(_config->ConfigsPath());

            if (pluginPath.empty() == true) {
                pluginPath = Core::Directory::Normalize(Core::File::PathName(options.configFile));
                pluginPath += Server::PluginConfigDirectory;
            }
            else {
                pluginPath = Core::Directory::Normalize(pluginPath);
            }

            string traceSettings (options.configFile);
 
            // Create PostMortem path
            Core::Directory postMortemPath(_config->PostMortemPath().c_str());
            if (postMortemPath.Next() != true) {
                postMortemPath.CreatePath();
            }

            // Time to open up, the trace buffer for this process and define it for the out-of-proccess systems
            // Define the environment variable for Tracing files, if it is not already set.
            if ( Trace::TraceUnit::Instance().Open(_config->VolatilePath()) != Core::ERROR_NONE){
#ifndef __WINDOWS__
                if (_background == true) {
                    syslog(LOG_WARNING, EXPAND_AND_QUOTE(APPLICATION_NAME) " Could not enable trace functionality!");
                } else
#endif
                {
                    fprintf(stdout, "Could not enable trace functionality!\n");
                }
            }

            if (_config->TraceCategoriesFile() == true) {

                traceSettings = Core::Directory::Normalize(Core::File::PathName(options.configFile)) + _config->TraceCategories();

                Core::File input (traceSettings, true);

                if (input.Open(true)) {
                    Trace::TraceUnit::Instance().Defaults(input);
                }
            }
            else {
                Trace::TraceUnit::Instance().Defaults(_config->TraceCategories());
            }

            SYSLOG(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME))));
            SYSLOG(Logging::Startup, (_T("Starting time: %s"), Core::Time::Now().ToRFC1123(false).c_str()));
            SYSLOG(Logging::Startup, (_T("Process Id:    %d"), Core::ProcessInfo().Id()));
            SYSLOG(Logging::Startup, (_T("SystemId:      %s"), Core::SystemInfo::Instance().Id(Core::SystemInfo::Instance().RawDeviceId(), ~0).c_str()));
            SYSLOG(Logging::Startup, (_T("Tree ref:      " _T(EXPAND_AND_QUOTE(TREE_REFERENCE)))));
            SYSLOG(Logging::Startup, (_T("Build ref:     " _T(EXPAND_AND_QUOTE(BUILD_REFERENCE)))));
            SYSLOG(Logging::Startup, (_T("Version:       %s"), _config->Version().c_str()));
            SYSLOG(Logging::Startup, (_T("Traces:        %s"), traceSettings.c_str()));

#ifndef __WINDOWS__
            // We need at least the loopback interface before we continue...
            StartLoopbackInterface();
#endif

            // Before we do any translation of IP, make sure we have the right network info...
            if (_config->IPv6() == false) {
                SYSLOG(Logging::Startup, (_T("Forcing the network to IPv4 only.")));
                Core::NodeId::ClearIPV6Enabled();
            }

            // Load plugin configs from a directory.
            LoadPlugins(pluginPath, *_config);

            // Startup/load/initialize what we found in the configuration.
            _dispatcher = new PluginHost::Server(*_config, _background);

            SYSLOG(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME) " actively listening.")));

            // If we have handlers open up the gates to analyze...
            _dispatcher->Open();

#ifndef __WINDOWS__
            if (_background == true) {
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
                    case 'T': {
                        printf("Triggering the Resource Monitor to do an evaluation.\n");
                        Core::ResourceMonitor::Instance().Break();
                        break;
                    }
                    case 'M': {
                        printf("\nResource Monitor Entry states:\n");
                        printf("============================================================\n");
                        Core::ResourceMonitor& monitor = Core::ResourceMonitor::Instance();
                        printf("Currently monitoring: %d resources\n", monitor.Count());
                        uint32_t index = 0;
                        Core::ResourceMonitor::Metadata info;

                        info.classname = _T("");
                        info.descriptor = ~0;
                        info.events = 0;
                        info.monitor = 0;

                        while (monitor.Info(index, info) == true) {
#ifdef __WINDOWS__
                            TCHAR flags[12];
                            flags[0]  = (info.monitor & FD_CLOSE   ? 'C' : '-');
                            flags[1]  = (info.monitor & FD_READ    ? 'R' : '-');
                            flags[2]  = (info.monitor & FD_WRITE   ? 'W' : '-');
                            flags[3]  = (info.monitor & FD_ACCEPT  ? 'A' : '-');
                            flags[4]  = (info.monitor & FD_CONNECT ? 'O' : '-');
                            flags[5]  = ':';
                            flags[6]  = (info.events & FD_CLOSE   ? 'C' : '-');
                            flags[7]  = (info.events & FD_READ    ? 'R' : '-');
                            flags[8]  = (info.events & FD_WRITE   ? 'W' : '-');
                            flags[9]  = (info.events & FD_ACCEPT  ? 'A' : '-');
                            flags[10] = (info.events & FD_CONNECT ? 'O' : '-');
                            flags[11] = '\0';
#else
                            TCHAR flags[8];
                            flags[0] = (info.monitor & POLLIN     ? 'I' : '-');
                            flags[1] = (info.monitor & POLLOUT    ? 'O' : '-');
                            flags[2] = (info.monitor & POLLHUP    ? 'H' : '-');
                            flags[3]  = ':';
                            flags[4] = (info.events & POLLIN     ? 'I' : '-');
                            flags[5] = (info.events & POLLOUT    ? 'O' : '-');
                            flags[6] = (info.events & POLLHUP    ? 'H' : '-');
                            flags[7] = '\0';
#endif
                      
                            printf ("%6d [%s]: %s\n", info.descriptor, flags, Core::ClassNameOnly(info.classname).Text().c_str());
                            index++;
                        }
                        break;
                    }
                    case 'Q':
                        break;
                    case 'R': {
                        printf("\nMonitor callstack:\n");
                        printf("============================================================\n");
                        ::DumpCallStack(Core::ResourceMonitor::Instance().Id(), stdout);
                        break;
                    }
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8': 
                    case '9': {
                        uint32_t threadId = _dispatcher->WorkerPool().Id(keyPress - '0');
                        printf("\nThreadPool thread[%c] callstack:\n", keyPress);
                        printf("============================================================\n");
                        if (threadId != static_cast<uint32_t>(~0)) {
                            ::DumpCallStack(threadId, stdout);
                        } else {
                           printf("The given Thread ID is not in a valid range, please give thread id between 0 and %d\n", THREADPOOL_COUNT);
                        }

                        break;
                    }
                    case '?':
                        printf("\nOptions are: \n");
                        printf("  [P]lugins\n");
                        printf("  [C]hannels\n");
                        printf("  [S]erver stats\n");
                        printf("  [T]rigger resource monitor\n");
                        printf("  [M]etadata resource monitor\n");
                        printf("  [R]esource monitor stack\n");
                        printf("  [0..%d] Workerpool stacks\n", THREADPOOL_COUNT);
                        printf("  [Q]uit\n\n");
                        break;

                    default:
                        break;
                    }

                } while (keyPress != 'Q');
            }
        }

        ExitHandler::Destruct();
        return 0;

    } // End main.
    } // extern "C"
}
}
