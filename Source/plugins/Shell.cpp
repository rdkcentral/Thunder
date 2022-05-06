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

#include "Module.h"
#include "IShell.h"
#include "Configuration.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::IShell::state)

    { PluginHost::IShell::UNAVAILABLE, _TXT("Unavailable") },
    { PluginHost::IShell::DEACTIVATED, _TXT("Deactivated") },
    { PluginHost::IShell::DEACTIVATION, _TXT("Deactivation") },
    { PluginHost::IShell::ACTIVATED, _TXT("Activated") },
    { PluginHost::IShell::ACTIVATION, _TXT("Activation") },
    { PluginHost::IShell::PRECONDITION, _TXT("Precondition") },
    { PluginHost::IShell::DESTROYED, _TXT("Destroyed") },

ENUM_CONVERSION_END(PluginHost::IShell::state)

ENUM_CONVERSION_BEGIN(PluginHost::IShell::reason)

    { PluginHost::IShell::REQUESTED, _TXT("Requested") },
    { PluginHost::IShell::AUTOMATIC, _TXT("Automatic") },
    { PluginHost::IShell::FAILURE, _TXT("Failure") },
    { PluginHost::IShell::MEMORY_EXCEEDED, _TXT("MemoryExceeded") },
    { PluginHost::IShell::STARTUP, _TXT("Startup") },
    { PluginHost::IShell::SHUTDOWN, _TXT("Shutdown") },
    { PluginHost::IShell::CONDITIONS, _TXT("Conditions") },
    { PluginHost::IShell::WATCHDOG_EXPIRED, _TXT("WatchdogExpired") },

ENUM_CONVERSION_END(PluginHost::IShell::reason)

ENUM_CONVERSION_BEGIN(Plugin::Config::startup)

    { Plugin::Config::startup::UNAVAILABLE, _TXT("Unavailable") },
    { Plugin::Config::startup::DEACTIVATED, _TXT("Deactivated") },
    { Plugin::Config::startup::SUSPENDED,   _TXT("Suspended")   },
    { Plugin::Config::startup::RESUMED,     _TXT("Resumed")     },
    { Plugin::Config::startup::RESUMED,     _TXT("Activated")   },

ENUM_CONVERSION_END(Plugin::Config::startup)

namespace PluginHost
{
    class EXTERNAL Object : public Core::JSON::Container {
    private:
        class RootObject : public Core::JSON::Container {
        private:
            RootObject(const RootObject&) = delete;
            RootObject& operator=(const RootObject&) = delete;

        public:
            RootObject()
                : Config(false)
            {
                Add(_T("root"), &Config);
            }
            ~RootObject() override = default;

        public:
            Core::JSON::String Config;
        };

    public:

        enum class ModeType {
            OFF,
            LOCAL,
            CONTAINER,
            DISTRIBUTED
        };

        Object()
            : Core::JSON::Container()
            , Locator()
            , User()
            , Group()
            , Threads(1)
            , Priority(0)
            , OutOfProcess(false)
            , Mode(ModeType::LOCAL)
            , LinkLoaderPath()
            , RemoteAddress()
            , Configuration(false)
        {
            Add(_T("locator"), &Locator);
            Add(_T("user"), &User);
            Add(_T("group"), &Group);
            Add(_T("threads"), &Threads);
            Add(_T("priority"), &Priority);
            Add(_T("outofprocess"), &OutOfProcess);
            Add(_T("mode"), &Mode);
            Add(_T("loaderpath"), &LinkLoaderPath);
            Add(_T("remoteaddress"), &RemoteAddress);
            Add(_T("configuration"), &Configuration);
        }
        Object(const IShell* info)
            : Core::JSON::Container()
            , Locator()
            , User()
            , Group()
            , Threads()
            , Priority(0)
            , OutOfProcess(false)
            , Mode(ModeType::LOCAL)
            , LinkLoaderPath()
            , RemoteAddress()
            , Configuration(false)
        {
            Add(_T("locator"), &Locator);
            Add(_T("user"), &User);
            Add(_T("group"), &Group);
            Add(_T("threads"), &Threads);
            Add(_T("priority"), &Priority);
            Add(_T("outofprocess"), &OutOfProcess);
            Add(_T("mode"), &Mode);
            Add(_T("loaderpath"), &LinkLoaderPath);
            Add(_T("remoteaddress"), &RemoteAddress);
            Add(_T("configuration"), &Configuration);

            RootObject config;
            Core::OptionalType<Core::JSON::Error> error;
            config.FromString(info->ConfigLine(), error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
            }

            if (config.Config.IsSet() == true) {
                // Yip we want to go out-of-process
                Object settings;
                Core::OptionalType<Core::JSON::Error> error;
                settings.FromString(config.Config.Value(), error);
                if (error.IsSet() == true) {
                    SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                }
                *this = settings;

                if (Locator.Value().empty() == true) {
                    Locator = info->Locator();
                }
            }
        }
        Object(const Object& copy)
            : Core::JSON::Container()
            , Locator(copy.Locator)
            , User(copy.User)
            , Group(copy.Group)
            , Threads(copy.Threads)
            , Priority(copy.Priority)
            , OutOfProcess(true)
            , Mode(copy.Mode)
            , LinkLoaderPath(copy.LinkLoaderPath)
            , RemoteAddress(copy.RemoteAddress)
            , Configuration(copy.Configuration)
        {
            Add(_T("locator"), &Locator);
            Add(_T("user"), &User);
            Add(_T("group"), &Group);
            Add(_T("threads"), &Threads);
            Add(_T("priority"), &Priority);
            Add(_T("outofprocess"), &OutOfProcess);
            Add(_T("mode"), &Mode);
            Add(_T("loaderpath"), &LinkLoaderPath);
            Add(_T("remoteaddress"), &RemoteAddress);
            Add(_T("configuration"), &Configuration);
        }
        ~Object() override = default;

        Object& operator=(const Object& RHS)
        {
            Locator = RHS.Locator;
            User = RHS.User;
            Group = RHS.Group;
            Threads = RHS.Threads;
            Priority = RHS.Priority;
            OutOfProcess = RHS.OutOfProcess;
            Mode = RHS.Mode;
            RemoteAddress = RHS.RemoteAddress;
            LinkLoaderPath = RHS.LinkLoaderPath;
            Configuration = RHS.Configuration;

            return (*this);
        }

        RPC::Object::HostType HostType() const {
            RPC::Object::HostType result = RPC::Object::HostType::LOCAL;
            switch( Mode.Value() ) {
                case ModeType::CONTAINER :
                    result = RPC::Object::HostType::CONTAINER;
                    break;
                case ModeType::DISTRIBUTED:
                    result = RPC::Object::HostType::DISTRIBUTED;
                    break;
                default:
                    result = RPC::Object::HostType::LOCAL;
                    break;
            }
            return result;
        }

    public:
        Core::JSON::String Locator;
        Core::JSON::String User;
        Core::JSON::String Group;
        Core::JSON::DecUInt8 Threads;
        Core::JSON::DecSInt8 Priority;
        Core::JSON::Boolean OutOfProcess;
        Core::JSON::EnumType<ModeType> Mode; 
        Core::JSON::String LinkLoaderPath; 
        Core::JSON::String RemoteAddress; 
        Core::JSON::String Configuration;
    };

    void* IShell::Root(uint32_t & pid, const uint32_t waitTime, const string className, const uint32_t interface, const uint32_t version)
    {
        pid = 0;
        void* result = nullptr;
        Object rootObject(this);

        // Note: when both new and old not set this one will revert to the old default which was inprocess 
        //       when both set the Old one is ignored
        if ( (( !rootObject.Mode.IsSet() ) && ( rootObject.OutOfProcess.Value() == false )) ||
             ((  rootObject.Mode.IsSet() ) && ( rootObject.Mode == Object::ModeType::OFF )) ) { 

            string locator(rootObject.Locator.Value());

            if (locator.empty() == true) {
                result = Core::ServiceAdministrator::Instance().Instantiate(Core::Library(), className.c_str(), version, interface);
            } else {
                std::vector<string> all_paths = GetLibrarySearchPaths(locator);
                std::vector<string>::const_iterator index = all_paths.begin();
                while ((result == nullptr) && (index != all_paths.end())) {
                    Core::File file(index->c_str());
                    if (file.Exists())
                    {
                        Core::Library resource(index->c_str());
                        if (resource.IsLoaded())
                            result = Core::ServiceAdministrator::Instance().Instantiate(
                                resource,
                                className.c_str(),
                                version,
                                interface);
                    }
                    index++;
                }
            }
        } else {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                string locator(rootObject.Locator.Value());
                if (locator.empty() == true) {
                    locator = Locator();
                }
                RPC::Object definition(locator,
                    className,
                    Callsign(),
                    interface,
                    version,
                    rootObject.User.Value(),
                    rootObject.Group.Value(),
                    rootObject.Threads.Value(),
                    rootObject.Priority.Value(),
                    rootObject.HostType(), 
                    rootObject.LinkLoaderPath.Value(),
                    rootObject.RemoteAddress.Value(),
                    rootObject.Configuration.Value());

                result = handler->Instantiate(definition, waitTime, pid);
            }
        }

        return (result);
    }
}

ENUM_CONVERSION_BEGIN(PluginHost::Object::ModeType)

    { PluginHost::Object::ModeType::OFF, _TXT("Off") },
    { PluginHost::Object::ModeType::LOCAL, _TXT("Local") },
    { PluginHost::Object::ModeType::CONTAINER, _TXT("Container") },
    { PluginHost::Object::ModeType::DISTRIBUTED, _TXT("Distributed") },

ENUM_CONVERSION_END(PluginHost::Object::ModeType);

} // namespace 



