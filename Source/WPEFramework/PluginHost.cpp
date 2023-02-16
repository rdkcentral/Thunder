/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
#include <fstream>

#ifndef __WINDOWS__
#include <dlfcn.h> // for dladdr
#include <syslog.h>
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
    static PluginHost::Config* _config = nullptr;
    static PluginHost::Server* _dispatcher = nullptr;
    static bool _background = false;
    static bool _atExitActive = true;

namespace PluginHost {

    class ConsoleOptions : public Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Core::Options(argumentCount, arguments, _T(":bhc:fF"))
            , configFile(Server::ConfigFile)
            , flushMode(Messaging::MessageUnit::flush::OFF)
        {
            Parse();
        }
        ~ConsoleOptions()
        {
        }

    public:
        const TCHAR* configFile;
        Messaging::MessageUnit::flush flushMode;

    private:
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
            case 'c':
                configFile = argument;
                break;
            case 'f':
                flushMode = Messaging::MessageUnit::flush::FLUSH;
                break;
            case 'F':
                flushMode = Messaging::MessageUnit::flush::FLUSH_ABBREVIATED;
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

    class AdapterObserver : public WPEFramework::Core::AdapterObserver::INotification {
    public:
        static int32_t constexpr WaitTime = 3000; //Just wait for 3 seconds

    public:
        AdapterObserver() = delete;
        AdapterObserver(const AdapterObserver&) = delete;
        AdapterObserver& operator=(const AdapterObserver&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        AdapterObserver(string interface)
            : _signal(false, true)
            , _interface(interface)
            , _observer(this)
        {
            _observer.Open();

            Core::AdapterIterator adapter(interface);

            if (adapter.IsValid() == true) {
                _signal.SetEvent();
            }
        }
POP_WARNING()
        ~AdapterObserver() override
        {
            _observer.Close();
        }

    public:
        virtual void Event(const string& interface) override
        {
            if (interface == _interface) {
                // We need to add this interface, it is currently not present.
                _signal.SetEvent();
            }
        }
        inline uint32_t WaitForCompletion(int32_t waitTime)
        {
            return _signal.Lock(waitTime);
        }
        void Up()
        {
            Core::AdapterIterator adapter(_interface);

            if (adapter.IsValid() == true) {
                adapter.Up(true);
                adapter.Add(Core::IPNode(Core::NodeId("127.0.0.1"), 8));
            }
        }

    private:
        Core::Event _signal;
        string _interface;
        Core::AdapterObserver _observer;
    };

    class ExitHandler : public Core::Thread {
    private:
        ExitHandler() = delete;
        ExitHandler(const ExitHandler&) = delete;
        ExitHandler& operator=(const ExitHandler&) = delete;

        ExitHandler(PluginHost::Server* destructor)
            : Core::Thread(Core::Thread::DefaultStackSize(), nullptr)
            , _destructor(destructor)
        {
            ASSERT(_destructor != nullptr);
        }

    public:
        ~ExitHandler() override
        {
            Stop();
            Wait(Core::Thread::STOPPED, Core::infinite);
        }
        static void DumpMetadata() {
            _adminLock.Lock();
            if (_dispatcher != nullptr) {
                _dispatcher->DumpMetadata();
            }
            _adminLock.Unlock();
        }
        static void StartShutdown() {
            _adminLock.Lock();
            if ((_dispatcher != nullptr) && (_instance == nullptr)) {
                _instance = new WPEFramework::PluginHost::ExitHandler(_dispatcher);
                _dispatcher = nullptr;
                _instance->Run();
            }
            _adminLock.Unlock();
        }
        static void Destruct() {

            _adminLock.Lock();

            if (_instance == nullptr) {

                PluginHost::Server* destructor = _dispatcher;

                _dispatcher = nullptr;

                _adminLock.Unlock();

                if (destructor != nullptr) {
                    CloseDown (destructor);
                }
            }
            else {
                ExitHandler* destructor = _instance;
                _instance = nullptr;
 
                _adminLock.Unlock();

                delete destructor; //It will wait till the worker execution completed
            }
        }

    private:
        virtual uint32_t Worker() override
        {
            CloseDown(_destructor);
            Block();
            return (Core::infinite);
        }
        static void CloseDown(PluginHost::Server* destructor)
        {
#ifndef __WINDOWS__
            if (_background) {
                syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon closing down.");
            } else
#endif
            {
                fprintf(stdout, EXPAND_AND_QUOTE(APPLICATION_NAME) " closing down.\n");
                fflush(stderr);
            }

            destructor->Close();
            delete destructor;
            delete _config;
            _config = nullptr;


#ifndef __WINDOWS__
            closelog();
#endif

            Messaging::MessageUnit::Instance().Close();

#ifdef __CORE_WARNING_REPORTING__
            WarningReporting::WarningReportingUnit::Instance().Close();
#endif

#ifndef __WINDOWS__
            if (_background) {
                syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " closing all created singletons.");
            } else
#endif
            {
                fprintf(stdout, EXPAND_AND_QUOTE(APPLICATION_NAME) " closing all created singletons.\n");
                fflush(stderr);
            }


            // Now clear all singeltons we created.
            Core::Singleton::Dispose();

#ifndef __WINDOWS__
            if (_background) {
                syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " completely stopped.");
            } else
#endif
            {
                fprintf(stdout, EXPAND_AND_QUOTE(APPLICATION_NAME) " completely stopped.\n");
                fflush(stderr);
            }
            _atExitActive = false;
        }

    private:
        PluginHost::Server* _destructor;

        static ExitHandler* _instance;
        static Core::CriticalSection _adminLock;
    };

    ExitHandler* ExitHandler::_instance = nullptr;
    Core::CriticalSection ExitHandler::_adminLock;

    static string GetDeviceId(PluginHost::Server* dispatcher)
    {
        string deviceId;

        PluginHost::ISubSystem* subSystems = dispatcher->Services().SubSystemsInterface();
        if (subSystems != nullptr) {
            if (subSystems->IsActive(PluginHost::ISubSystem::IDENTIFIER) == true) {
                const PluginHost::ISubSystem::IIdentifier* id(subSystems->Get<PluginHost::ISubSystem::IIdentifier>());
                if (id != nullptr) {
                    uint8_t buffer[64];

                    buffer[0] = static_cast<const PluginHost::ISubSystem::IIdentifier*>(id)
                                ->Identifier(sizeof(buffer) - 1, &(buffer[1]));

                    if (buffer[0] != 0) {
                        deviceId = Core::SystemInfo::Instance().Id(buffer, ~0);
                    }

                    id->Release();
                }
            }
            subSystems->Release();
        }
        return deviceId;
    }

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

            if (_background) {
                syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to a SIGTERM or SIGQUIT signal. Regular shutdown");
            } else {
                fprintf(stderr, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to a SIGTERM or SIGQUIT signal. No regular shutdown.\nErrors to follow are collateral damage errors !!!!!!\n");
                fflush(stderr);
            }

            ExitHandler::StartShutdown();
        }
        else if (signo == SIGUSR1) {
            ExitHandler::DumpMetadata();
        }
    }

#endif

    void LoadPlugins(const string& name, PluginHost::Config& config)
    {
        Core::Directory pluginDirectory(name.c_str(), _T("*.json"));

        while (pluginDirectory.Next() == true) {

            Core::File file(pluginDirectory.Current());

            if (file.Exists()) {
                if (file.IsDirectory()) {
                    if ((file.FileName() != ".") && (file.FileName() != "..")) {
                        LoadPlugins(file.Name(), config);
                    }
                } else if (file.Open(true) == false) {
                    SYSLOG_GLOBAL(Logging::Startup, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
                } else {
                    Plugin::Config pluginConfig;
                    Core::OptionalType<Core::JSON::Error> error;
                    pluginConfig.IElement::FromFile(file, error);
                    if (error.IsSet() == true) {
                        SYSLOG_GLOBAL(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                    }
                    file.Close();

                    if ((pluginConfig.ClassName.Value().empty() == true) || (pluginConfig.Locator.Value().empty() == true)) {
                        SYSLOG_GLOBAL(Logging::Startup, (_T("Plugin config file [%s] does not contain classname or locator."), file.Name().c_str()));
                    } else {
                        if (pluginConfig.Callsign.Value().empty() == true) {
                            pluginConfig.Callsign = Core::File::FileName(file.FileName());
                        }

                        if (config.Add(pluginConfig) == false) {
                            SYSLOG_GLOBAL(Logging::Startup, (_T("Plugin config file [%s] can not be reconfigured."), file.Name().c_str()));
                        }
                    }
                }
            }
        }
    }

#ifndef __WINDOWS__
    void StartLoopbackInterface()
    {
        AdapterObserver observer(_T("lo"));

        if (observer.WaitForCompletion(AdapterObserver::WaitTime) == Core::ERROR_NONE) {
            observer.Up();
            SYSLOG_GLOBAL(Logging::Startup, (string(_T("Interface [lo], fully functional"))));
        } else {
            SYSLOG_GLOBAL(Logging::Startup, (string(_T("Interface [lo], partly functional (no name resolving)"))));
        }
    }
#endif

    static void ForcedExit() {
        if (_atExitActive == true) {
#ifndef __WINDOWS__
            if (_background) {
                syslog(LOG_ERR, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to an atexit request. No regular shutdown. Errors to follow are collateral damage errors !!!!!!");
            } else 
#endif
            {
                fprintf(stderr, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to an atexit request.\nNo regular shutdown.\nErrors to follow are collateral damage errors !!!!!!\n");
                fflush(stderr);
            }
            ExitHandler::Destruct();
        }
    }

    static void UncaughtExceptions () {
#ifndef __WINDOWS__
        if (_background) {
            syslog(LOG_ERR, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to an uncaught exception. No regular shutdown. Errors to follow are collateral damage errors !!!!!!");
        } else
#endif
        {
            fprintf(stderr, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to an uncaught exception.\nNo regular shutdown.\nErrors to follow are collateral damage errors !!!!!!\n");
            fflush(stderr);
        }

        Logging::DumpException(_T("General"));

        ExitHandler::Destruct();
    }

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

        if (options.RequestUsage()) {
#ifndef __WINDOWS__
            syslog(LOG_ERR, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon failed to start. Incorrect Options.");
#endif
            if ((_background == false) && (options.RequestUsage())) {
                fprintf(stderr, "Usage: " EXPAND_AND_QUOTE(APPLICATION_NAME) " [-c <config file>] [-b] [-fF]\n");
                fprintf(stderr, "       -c <config file>  Define the configuration file to use.\n");
                fprintf(stderr, "       -b                Run " EXPAND_AND_QUOTE(APPLICATION_NAME) " in the background.\n");
                fprintf(stderr, "       -f                Flush messaging information also to syslog/console, none abbreviated\n");
                fprintf(stderr, "       -F                Flush messaging information also to syslog/console, abbreviated\n");
            }
            exit(EXIT_FAILURE);
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
            sigaction(SIGUSR1, &sa, nullptr);
        }

        if (atexit(ForcedExit) != 0) {
            TRACE_L1("Could not register @exit handler. Argc %d.", argc);
            ExitHandler::Destruct();
            exit(EXIT_FAILURE);
        } 

        if (_background == true) {
            //Close Standard File Descriptors
            // close(STDIN_FILENO);
            // close(STDOUT_FILENO);
            // close(STDERR_FILENO);
            syslog(LOG_NOTICE, EXPAND_AND_QUOTE(APPLICATION_NAME) " Daemon starting");
        } else
#endif

        std::set_terminate(UncaughtExceptions);

        // Read the config file, to instantiate the proper plugins and for us to open up the right listening ear.
        Core::File configFile(string(options.configFile));
        if (configFile.Open(true) == true) {
            Core::OptionalType<Core::JSON::Error> error;
            _config = new Config(configFile, _background, error);

            if (error.IsSet() == true) {
                SYSLOG_GLOBAL(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
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

                Core::ProcessCurrent myself;

                if (_config->Process().OOMAdjust() != 0) {
                    myself.OOMAdjust(_config->Process().OOMAdjust());
                }
                if (_config->Process().Priority() != 0) {
                    myself.Priority(_config->Process().Priority());
                }

                if (_config->Process().Group().empty() == false) {
                    myself.Group(_config->Process().Group());
                }

                if (_config->Process().User().empty() == false) {
                    myself.User(_config->Process().User());
                }

                if (_config->StackSize() != 0) {
                    Core::Thread::DefaultStackSize(_config->StackSize()); 
                }

#ifndef __WINDOWS__
                if (_config->Process().UMask().IsSet() == true) {
                    ::umask(_config->Process().UMask().Value());
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

            string messagingSettings (options.configFile);
 
            // Create PostMortem path
            Core::Directory postMortemPath(_config->PostMortemPath().c_str());
            if (postMortemPath.Next() != true) {
                postMortemPath.CreatePath();
            }

            if (_config->MessagingCategoriesFile()) {

                messagingSettings = Core::Directory::Normalize(Core::File::PathName(options.configFile)) + _config->MessagingCategories();

                std::ifstream inputFile (messagingSettings, std::ifstream::in);
                std::stringstream buffer;
                buffer << inputFile.rdbuf();
                messagingSettings = buffer.str();
            }
            else {
                messagingSettings = _config->MessagingCategories();
            }

            // Time to open up, the message buffer for this process and define it for the out-of-proccess systems
            // Define the environment variable for Messaging files, if it is not already set.
            uint32_t messagingErrorCode = Core::ERROR_GENERAL;
            messagingErrorCode = Messaging::MessageUnit::Instance().Open(_config->VolatilePath(), _config->MessagingPort(), messagingSettings, _background, options.flushMode);

            if ( messagingErrorCode != Core::ERROR_NONE){
#ifndef __WINDOWS__
                if (_background == true) {
                    syslog(LOG_WARNING, EXPAND_AND_QUOTE(APPLICATION_NAME) " Could not enable messaging/tracing functionality!");
                } else
#endif
                {
                    fprintf(stdout, "Could not enable messaging/tracing functionality!\n");
                }
            }
            
#ifdef __CORE_WARNING_REPORTING__
            if (WarningReporting::WarningReportingUnit::Instance().Open(_config->VolatilePath()) != Core::ERROR_NONE) {
                SYSLOG_GLOBAL(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME) " Could not enable issue reporting functionality!")));
            }

            WarningReporting::WarningReportingUnit::Instance().Defaults(_config->WarningReportingCategories());
#endif

            SYSLOG_GLOBAL(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME))));
            SYSLOG_GLOBAL(Logging::Startup, (_T("Starting time: %s"), Core::Time::Now().ToRFC1123(false).c_str()));
            SYSLOG_GLOBAL(Logging::Startup, (_T("Process Id:    %d"), Core::ProcessInfo().Id()));
            SYSLOG_GLOBAL(Logging::Startup, (_T("Tree ref:      " _T(EXPAND_AND_QUOTE(TREE_REFERENCE)))));
            SYSLOG_GLOBAL(Logging::Startup, (_T("Build ref:     " _T(EXPAND_AND_QUOTE(BUILD_REFERENCE)))));
            SYSLOG_GLOBAL(Logging::Startup, (_T("Version:       %d:%d:%d"), PluginHost::Major, PluginHost::Minor, PluginHost::Minor));
            SYSLOG_GLOBAL(Logging::Startup, (_T("Messages:        %s"), messagingSettings.c_str()));

            // Before we do any translation of IP, make sure we have the right network info...
            if (_config->IPv6() == false) {
                SYSLOG_GLOBAL(Logging::Startup, (_T("Forcing the network to IPv4 only.")));
                Core::NodeId::ClearIPV6Enabled();
            }

            // Load plugin configs from a directory.
            LoadPlugins(pluginPath, *_config);

#ifndef __WINDOWS__
            // We need at least the loopback interface before we continue...
            StartLoopbackInterface();
#endif

            // Startup/load/initialize what we found in the configuration.
            _dispatcher = new PluginHost::Server(*_config, _background);

            SYSLOG_GLOBAL(Logging::Startup, (_T(EXPAND_AND_QUOTE(APPLICATION_NAME) " actively listening.")));

            // If we have handlers open up the gates to analyze...
            _dispatcher->Open();

            string id = GetDeviceId(_dispatcher);
            if (id.empty() == false) {
                SYSLOG_GLOBAL(Logging::Startup, (_T("SystemId:      %s"), id.c_str()));
            }

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
                    case 'A': {
                        Core::JSON::ArrayType<MetaData::COMRPC> proxyChannels;
                        RPC::Administrator::Instance().Visit([&](const Core::IPCChannel& channel, const RPC::Administrator::Proxies& proxies) {
                                MetaData::COMRPC& entry(proxyChannels.Add());
                                const RPC::Communicator::Client* comchannel = dynamic_cast<const RPC::Communicator::Client*>(&channel);

                                if (comchannel != nullptr) {
                                    string identifier = PluginHost::ChannelIdentifier(comchannel->Source());

                                    if (identifier.empty() == false) {
                                        entry.Remote = identifier;
                                    }
                                }

                                for (const auto& proxy : proxies) {
                                    MetaData::COMRPC::Proxy& info(entry.Proxies.Add());
                                    info.InstanceId = proxy->Implementation();
                                    info.InterfaceId = proxy->InterfaceId();
                                    info.RefCount = proxy->ReferenceCount();
                                }
                            }
                        );
                        Core::JSON::ArrayType<MetaData::COMRPC>::Iterator index(proxyChannels.Elements());

                        printf("COMRPC Links:\n");
                        printf("============================================================\n");
                        while (index.Next() == true) {
                            printf("Link: %s\n", index.Current().Remote.Value().c_str());
                            printf("------------------------------------------------------------\n");

                            Core::JSON::ArrayType<MetaData::COMRPC::Proxy>::Iterator loop(index.Current().Proxies.Elements());

                            while (loop.Next() == true) {
                                uint64_t instanceId = loop.Current().InstanceId.Value();
                                printf("InstanceId: 0x%" PRIx64 ", RefCount: %d, InterfaceId %d [0x%X]\n", instanceId, loop.Current().RefCount.Value(), loop.Current().InterfaceId.Value(), loop.Current().InterfaceId.Value());
                            }
                            printf("\n");
                        }
                        break;
                    }
                    case 'C': {
                        Core::JSON::ArrayType<MetaData::Channel> metaData;
                        _dispatcher->Dispatcher().GetMetaData(metaData);
                        _dispatcher->Services().GetMetaData(metaData);
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
                    case 'E': {
                        uint32_t requests, responses, filebodies, jsonrequests;
                        _dispatcher->Statistics(requests, responses, filebodies, jsonrequests);
                        printf("\nProxyPool Elements:\n");
                        printf("============================================================\n");
                        printf("HTTP requests:    %d\n", requests);
                        printf("HTTP responses:   %d\n", responses);
                        printf("HTTP Files:       %d\n", filebodies);
                        printf("JSONRPC messages: %d\n", jsonrequests);

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
#if THUNDER_RESTFULL_API

                            printf("Observers:  %d\n", index.Current().Observers.Value());
#endif

#if THUNDER_RUNTIME_STATISTICS
                            printf("Requests:   %d\n", index.Current().ProcessedRequests.Value());
                            printf("JSON:       %d\n", index.Current().ProcessedObjects.Value());
#endif
                            printf("\n");
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
                                printf("Latitude:    %u\n", location->Latitude());
                                printf("Longitude:   %u\n", location->Longitude());
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
                        printf("Pending:     %d\n", static_cast<uint32_t>(metaData.Pending.size()));
                        printf("Poolruns:\n");
                        for (uint8_t index = 0; index < metaData.Slots; index++) {
                           printf("  Thread%02d|0x%08X: %10d", (index + 1), (uint32_t) metaData.Slot[index].WorkerId, metaData.Slot[index].Runs);
                            if (metaData.Slot[index].Job.IsSet() == false) {
                                printf("\n");
                            }
                            else {
                                printf(" [%s]\n", metaData.Slot[index].Job.Value().c_str());
                            }
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
                      
                            printf ("%6d %s[%s]: %s\n", info.descriptor, info.filename, flags, Core::ClassNameOnly(info.classname).Text().c_str());
                            index++;
                        }
                        break;
                    }
                    case 'Q':
                        break;
                    case 'R': {
                        printf("\nMonitor callstack:\n");
                        printf("============================================================\n");
                        uint8_t counter = 0;
                        std::list<Core::callstack_info> stackList;
                        ::DumpCallStack(Core::ResourceMonitor::Instance().Id(), stackList);
                        for (const Core::callstack_info& entry : stackList) {
                            printf("[%03d] [%p] %.30s %s", counter, entry.address, entry.module.c_str(), entry.function.c_str());
                            if (entry.line != static_cast<uint32_t>(~0)) {
                                    printf(" [%d]\n", entry.line);
                            }
                            else {
                                    printf("\n");
                            }
                            counter++;
                        }
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
                        ThreadId threadId = _dispatcher->WorkerPool().Id(keyPress - '0');
                        printf("\nThreadPool thread[%c] callstack:\n", keyPress);
                        printf("============================================================\n");
                        if (threadId != (ThreadId)(~0)) {
                            uint8_t counter = 0;
                            std::list<Core::callstack_info> stackList;
                            ::DumpCallStack(threadId, stackList);
                            for (const Core::callstack_info& entry : stackList) {
                                printf("[%03d] [%p] %.30s %s", counter, entry.address, entry.module.c_str(), entry.function.c_str());
                                if (entry.line != static_cast<uint32_t>(~0)) {
                                    printf(" [%d]\n", entry.line);
                                }
                                else {
                                    printf("\n");
                                }
                                counter++;
                            }
                        } else {
                           printf("The given Thread ID is not in a valid range, please give thread id between 0 and %d\n", THREADPOOL_COUNT);
                        }

                        break;
                    }
                    case '?':
                        printf("\nOptions are: \n");
                        printf("  [A]ctive Proxy list\n");
                        printf("  [P]lugins\n");
                        printf("  [C]hannels\n");
                        printf("  [S]erver stats\n");
                        printf("  [E]lements in the ProxyPools\n");
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

        if (_background == false) {
            fprintf(stderr, EXPAND_AND_QUOTE(APPLICATION_NAME) " shutting down due to a 'Q' press in the terminal. Regular shutdown\n");
            fflush(stderr);
        }
 
        ExitHandler::Destruct();
        std::set_terminate(nullptr);
        _atExitActive = false;
        return 0;

    } // End main.
    } // extern "C"
}
}
