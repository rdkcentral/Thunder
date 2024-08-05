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

#pragma once

#include "Module.h"
#include "Config.h"
#include "IPlugin.h"
#include "IShell.h"
#include "ISubSystem.h"
#include "IController.h"

namespace Thunder {
namespace Plugin {
    /**
     * IMPORTANT: If updating this class to add/remove/modify configuration options, ensure
     * the documentation in docs/plugin/config.md is updated to reflect the changes!
    */
    class EXTERNAL Config : public Core::JSON::Container {
    public:
        class EXTERNAL RootConfig : public Core::JSON::Container {
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

            RootConfig()
                : Core::JSON::Container()
                , Locator()
                , User()
                , Group()
                , Threads(1)
                , Priority(0)
                , OutOfProcess(false)
                , Mode(ModeType::LOCAL)
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
                Add(_T("remoteaddress"), &RemoteAddress);
                Add(_T("configuration"), &Configuration);
            }
            RootConfig(const PluginHost::IShell* info)
                : Core::JSON::Container()
                , Locator()
                , User()
                , Group()
                , Threads()
                , Priority(0)
                , OutOfProcess(false)
                , Mode(ModeType::LOCAL)
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
                    RootConfig settings;
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
            RootConfig(const RootConfig& copy)
                : Core::JSON::Container()
                , Locator(copy.Locator)
                , User(copy.User)
                , Group(copy.Group)
                , Threads(copy.Threads)
                , Priority(copy.Priority)
                , OutOfProcess(true)
                , Mode(copy.Mode)
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
                Add(_T("remoteaddress"), &RemoteAddress);
                Add(_T("configuration"), &Configuration);
            }
            RootConfig(RootConfig&& move) noexcept
                : Core::JSON::Container()
                , Locator(std::move(move.Locator))
                , User(std::move(move.User))
                , Group(std::move(move.Group))
                , Threads(std::move(move.Threads))
                , Priority(std::move(move.Priority))
                , OutOfProcess(std::move(move.OutOfProcess))
                , Mode(std::move(move.Mode))
                , RemoteAddress(std::move(move.RemoteAddress))
                , Configuration(std::move(move.Configuration))
            {
                Add(_T("locator"), &Locator);
                Add(_T("user"), &User);
                Add(_T("group"), &Group);
                Add(_T("threads"), &Threads);
                Add(_T("priority"), &Priority);
                Add(_T("outofprocess"), &OutOfProcess);
                Add(_T("mode"), &Mode);
                Add(_T("remoteaddress"), &RemoteAddress);
                Add(_T("configuration"), &Configuration);
            }

            ~RootConfig() override = default;

            RootConfig& operator=(const RootConfig& RHS)
            {
                Locator = RHS.Locator;
                User = RHS.User;
                Group = RHS.Group;
                Threads = RHS.Threads;
                Priority = RHS.Priority;
                OutOfProcess = RHS.OutOfProcess;
                Mode = RHS.Mode;
                RemoteAddress = RHS.RemoteAddress;
                Configuration = RHS.Configuration;

                return (*this);
            }

            RootConfig& operator=(RootConfig&& move) noexcept
            {
                if (this != &move) {
                    Locator = std::move(move.Locator);
                    User = std::move(move.User);
                    Group = std::move(move.Group);
                    Threads = std::move(move.Threads);
                    Priority = std::move(move.Priority);
                    OutOfProcess = std::move(move.OutOfProcess);
                    Mode = std::move(move.Mode);
                    RemoteAddress = std::move(move.RemoteAddress);
                    Configuration = std::move(move.Configuration);
                }

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
            Core::JSON::String RemoteAddress;
            Core::JSON::String Configuration;
        };

    public:
        Config()
            : Core::JSON::Container()
            , Callsign()
            , Locator()
            , ClassName()
            , Versions()
            , Resumed(false)
            , WebUI()
            , Precondition()
            , Termination()
            , Control()
            , Configuration(false)
            , PersistentPathPostfix()
            , VolatilePathPostfix()
            , SystemRootPath()
            , StartupOrder(50)
            , StartMode(PluginHost::IShell::startmode::ACTIVATED)
            , Communicator()
            , Root()
        {
            Add(_T("callsign"), &Callsign);
            Add(_T("locator"), &Locator);
            Add(_T("classname"), &ClassName);
            Add(_T("versions"), &Versions);
            Add(_T("resumed"), &Resumed);
            Add(_T("webui"), &WebUI);
            Add(_T("precondition"), &Precondition);
            Add(_T("termination"), &Termination);
            Add(_T("control"), &Control);
            Add(_T("configuration"), &Configuration);
            Add(_T("persistentpathpostfix"), &PersistentPathPostfix);
            Add(_T("volatilepathpostfix"), &VolatilePathPostfix);
            Add(_T("systemrootpath"), &SystemRootPath);
            Add(_T("startuporder"), &StartupOrder);
            Add(_T("startmode"), &StartMode);
            Add(_T("communicator"), &Communicator);
            Add(_T("root"), &Root);
        }
        Config(const Config& copy)
            : Core::JSON::Container()
            , Callsign(copy.Callsign)
            , Locator(copy.Locator)
            , ClassName(copy.ClassName)
            , Versions(copy.Versions)
            , Resumed(copy.Resumed)
            , WebUI(copy.WebUI)
            , Precondition(copy.Precondition)
            , Termination(copy.Termination)
            , Control(copy.Control)
            , Configuration(copy.Configuration)
            , PersistentPathPostfix(copy.PersistentPathPostfix)
            , VolatilePathPostfix(copy.VolatilePathPostfix)
            , SystemRootPath(copy.SystemRootPath)
            , StartupOrder(copy.StartupOrder)
            , StartMode(copy.StartMode)
            , Communicator(copy.Communicator)
            , Root(copy.Root)
        {
            Add(_T("callsign"), &Callsign);
            Add(_T("locator"), &Locator);
            Add(_T("classname"), &ClassName);
            Add(_T("versions"), &Versions);
            Add(_T("resumed"), &Resumed);
            Add(_T("webui"), &WebUI);
            Add(_T("precondition"), &Precondition);
            Add(_T("termination"), &Termination);
            Add(_T("control"), &Control);
            Add(_T("configuration"), &Configuration);
            Add(_T("persistentpathpostfix"), &PersistentPathPostfix);
            Add(_T("volatilepathpostfix"), &VolatilePathPostfix);
            Add(_T("systemrootpath"), &SystemRootPath);
            Add(_T("startuporder"), &StartupOrder);
            Add(_T("startmode"), &StartMode);
            Add(_T("communicator"), &Communicator);
            Add(_T("root"), &Root);
        }
        Config(Config&& move) noexcept
            : Core::JSON::Container()
            , Callsign(std::move(move.Callsign))
            , Locator(std::move(move.Locator))
            , ClassName(std::move(move.ClassName))
            , Versions(std::move(move.Versions))
            , Resumed(std::move(move.Resumed))
            , WebUI(std::move(move.WebUI))
            , Precondition(std::move(move.Precondition))
            , Termination(std::move(move.Termination))
            , Control(std::move(move.Control))
            , Configuration(std::move(move.Configuration))
            , PersistentPathPostfix(std::move(move.PersistentPathPostfix))
            , VolatilePathPostfix(std::move(move.VolatilePathPostfix))
            , SystemRootPath(std::move(move.SystemRootPath))
            , StartupOrder(std::move(move.StartupOrder))
            , StartMode(std::move(move.StartMode))
            , Communicator(std::move(move.Communicator))
            , Root(std::move(move.Root))
        {
            Add(_T("callsign"), &Callsign);
            Add(_T("locator"), &Locator);
            Add(_T("classname"), &ClassName);
            Add(_T("versions"), &Versions);
            Add(_T("resumed"), &Resumed);
            Add(_T("webui"), &WebUI);
            Add(_T("precondition"), &Precondition);
            Add(_T("termination"), &Termination);
            Add(_T("control"), &Control);
            Add(_T("configuration"), &Configuration);
            Add(_T("persistentpathpostfix"), &PersistentPathPostfix);
            Add(_T("volatilepathpostfix"), &VolatilePathPostfix);
            Add(_T("systemrootpath"), &SystemRootPath);
            Add(_T("startuporder"), &StartupOrder);
            Add(_T("startmode"), &StartMode);
            Add(_T("communicator"), &Communicator);
            Add(_T("root"), &Root);
        }

        ~Config() override = default;

        Config& operator=(const Config& RHS)
        {
            Callsign = RHS.Callsign;
            Locator = RHS.Locator;
            ClassName = RHS.ClassName;
            Versions = RHS.Versions;
            Resumed = RHS.Resumed;
            WebUI = RHS.WebUI;
            Configuration = RHS.Configuration;
            Precondition = RHS.Precondition;
            Termination = RHS.Termination;
            Control = RHS.Control;
            PersistentPathPostfix = RHS.PersistentPathPostfix;
            VolatilePathPostfix = RHS.VolatilePathPostfix;
            SystemRootPath = RHS.SystemRootPath;
            StartupOrder = RHS.StartupOrder;
            StartMode = RHS.StartMode;
            Communicator = RHS.Communicator;
            Root = RHS.Root;

            return (*this);
        }

        Config& operator=(Config&& move) noexcept
        {
            Callsign = std::move(move.Callsign);
            Locator = std::move(move.Locator);
            ClassName = std::move(move.ClassName);
            Versions = std::move(move.Versions);
            Resumed = std::move(move.Resumed);
            WebUI = std::move(move.WebUI);
            Configuration = std::move(move.Configuration);
            Precondition = std::move(move.Precondition);
            Termination = std::move(move.Termination);
            Control = std::move(move.Control);
            PersistentPathPostfix = std::move(move.PersistentPathPostfix);
            VolatilePathPostfix = std::move(move.VolatilePathPostfix);
            SystemRootPath = std::move(move.SystemRootPath);
            StartupOrder = std::move(move.StartupOrder);
            StartMode = std::move(move.StartMode);
            Communicator = std::move(move.Communicator);
            Root = std::move(move.Root);

            return (*this);
        }

        string DataPath(const string& basePath) const {
            return (basePath + ClassName.Value() + '/');
        }
        string PersistentPath(const string& basePath) const {
            string postfixPath = ((PersistentPathPostfix.IsSet() == true) && (PersistentPathPostfix.Value().empty() == false)) ? PersistentPathPostfix.Value(): Callsign.Value();
            return (basePath + postfixPath + '/');
        }
        string VolatilePath(const string& basePath) const {
            string postfixPath = ((VolatilePathPostfix.IsSet() == true) && (VolatilePathPostfix.Value().empty() == false)) ? VolatilePathPostfix.Value(): Callsign.Value();
            return (basePath + postfixPath + '/');
        }

        explicit operator Exchange::Controller::IMetadata::Data::Service() const { 
            Exchange::Controller::IMetadata::Data::Service result;

            result.Callsign = Callsign;
            result.Locator = Locator;
            result.ClassName = ClassName;
            result.StartMode = StartMode;
            result.Configuration = Configuration;

            if (Communicator.IsSet() == true) {
                result.Communicator = Communicator;
            }
            if (PersistentPathPostfix.IsSet() == true) {
                result.PersistentPathPostfix = PersistentPathPostfix;
            }
            if (VolatilePathPostfix.IsSet() == true) {
                result.VolatilePathPostfix = VolatilePathPostfix;
            }
            if (SystemRootPath.IsSet() == true) {
                result.SystemRootPath = SystemRootPath;
            }
            if (Precondition.IsSet() == true) {
                result.Precondition = Precondition;
            }
            if (Termination.IsSet() == true) {
                result.Termination = Termination;
            }
            if (Control.IsSet() == true) {
                result.Control = Control;
            }

            return result; 
        }

    public:
        Core::JSON::String Callsign;
        Core::JSON::String Locator;
        Core::JSON::String ClassName;
        Core::JSON::String Versions;
        Core::JSON::Boolean Resumed;
        Core::JSON::String WebUI;
        Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> Precondition;
        Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> Termination;
        Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> Control;
        Core::JSON::String Configuration;
        Core::JSON::String PersistentPathPostfix;
        Core::JSON::String VolatilePathPostfix;
        Core::JSON::String SystemRootPath;
        Core::JSON::DecUInt32 StartupOrder;
        Core::JSON::EnumType<PluginHost::IShell::startmode> StartMode;
        Core::JSON::String Communicator;
        RootConfig Root;

        static Core::NodeId IPV4UnicastNode(const string& ifname);

    public:
        inline bool IsSameInstance(const Config& RHS) const
        {
            return ((Locator.Value() == RHS.Locator.Value()) && (ClassName.Value() == RHS.ClassName.Value()));
        }
    };

    class EXTERNAL Setting : public Trace::Text {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Setting(const Setting& a_Copy) = delete;
        Setting& operator=(const Setting& a_RHS) = delete;

    public:
        Setting(const string& key, const bool value)
            : Trace::Text()
        {
            Trace::Text::Set(_T("Setting: [") + key + _T("] set to value [") + (value ? _T("TRUE]") : _T("FALSE]")));
        }
        Setting(const string& key, const string& value)
            : Trace::Text()
        {
            Trace::Text::Set(_T("Setting: [") + key + _T("] set to value [") + value + _T("]"));
        }
        template <typename NUMBERTYPE, const bool SIGNED, const NumberBase BASETYPE>
        Setting(const string& key, const NUMBERTYPE value)
            : Trace::Text()
        {
            Core::NumberType<NUMBERTYPE, SIGNED, BASETYPE> number(value);
            Trace::Text::Set(_T("Setting: [") + key + _T("] set to value [") + number.Text() + _T("]"));
        }
        ~Setting()
        {
        }
    };
}
}
