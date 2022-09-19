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

namespace WPEFramework {
namespace Plugin {
    class EXTERNAL Config : public Core::JSON::Container {
    public:
        enum startup : uint8_t {
            UNAVAILABLE,
            DEACTIVATED,
            SUSPENDED,
            RESUMED
        };

    public:
        Config()
            : Core::JSON::Container()
            , Callsign()
            , Locator()
            , ClassName()
            , Versions()
            , AutoStart(true)
            , Resumed(false)
            , WebUI()
            , Precondition()
            , Termination()
            , Configuration(false)
            , PersistentPathPostfix()
            , VolatilePathPostfix()
            , SystemRootPath()
            , StartupOrder(50)
            , Startup(startup::DEACTIVATED)
        {
            Add(_T("callsign"), &Callsign);
            Add(_T("locator"), &Locator);
            Add(_T("classname"), &ClassName);
            Add(_T("versions"), &Versions);
            Add(_T("autostart"), &AutoStart);
            Add(_T("resumed"), &Resumed);
            Add(_T("webui"), &WebUI);
            Add(_T("precondition"), &Precondition);
            Add(_T("termination"), &Termination);
            Add(_T("configuration"), &Configuration);
            Add(_T("persistentpathpostfix"), &PersistentPathPostfix);
            Add(_T("volatilepathpostfix"), &VolatilePathPostfix);
            Add(_T("systemrootpath"), &SystemRootPath);
            Add(_T("startuporder"), &StartupOrder);
            Add(_T("startmode"), &Startup);
        }
        Config(const Config& copy)
            : Core::JSON::Container()
            , Callsign(copy.Callsign)
            , Locator(copy.Locator)
            , ClassName(copy.ClassName)
            , Versions(copy.Versions)
            , AutoStart(copy.AutoStart)
            , Resumed(copy.Resumed)
            , WebUI(copy.WebUI)
            , Precondition(copy.Precondition)
            , Termination(copy.Termination)
            , Configuration(copy.Configuration)
            , PersistentPathPostfix(copy.PersistentPathPostfix)
            , VolatilePathPostfix(copy.VolatilePathPostfix)
            , SystemRootPath(copy.SystemRootPath)
            , StartupOrder(copy.StartupOrder)
            , Startup(copy.Startup)
        {
            Add(_T("callsign"), &Callsign);
            Add(_T("locator"), &Locator);
            Add(_T("classname"), &ClassName);
            Add(_T("versions"), &Versions);
            Add(_T("autostart"), &AutoStart);
            Add(_T("resumed"), &Resumed);
            Add(_T("webui"), &WebUI);
            Add(_T("precondition"), &Precondition);
            Add(_T("termination"), &Termination);
            Add(_T("configuration"), &Configuration);
            Add(_T("persistentpathpostfix"), &PersistentPathPostfix);
            Add(_T("volatilepathpostfix"), &VolatilePathPostfix);
            Add(_T("systemrootpath"), &SystemRootPath);
            Add(_T("startuporder"), &StartupOrder);
            Add(_T("startmode"), &Startup);
        }
        ~Config() override = default;

        Config& operator=(const Config& RHS)
        {
            Callsign = RHS.Callsign;
            Locator = RHS.Locator;
            ClassName = RHS.ClassName;
            Versions = RHS.Versions;
            AutoStart = RHS.AutoStart;
            Resumed = RHS.Resumed;
            WebUI = RHS.WebUI;
            Configuration = RHS.Configuration;
            Precondition = RHS.Precondition;
            Termination = RHS.Termination;
            PersistentPathPostfix = RHS.PersistentPathPostfix;
            VolatilePathPostfix = RHS.VolatilePathPostfix;
            SystemRootPath = RHS.SystemRootPath;
            StartupOrder = RHS.StartupOrder;
            Startup = RHS.Startup;

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

    public:
        Core::JSON::String Callsign;
        Core::JSON::String Locator;
        Core::JSON::String ClassName;
        Core::JSON::String Versions;
        Core::JSON::Boolean AutoStart;
        Core::JSON::Boolean Resumed;
        Core::JSON::String WebUI;
        Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> Precondition;
        Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>> Termination;
        Core::JSON::String Configuration;
        Core::JSON::String PersistentPathPostfix;
        Core::JSON::String VolatilePathPostfix;
        Core::JSON::String SystemRootPath;
        Core::JSON::DecUInt32 StartupOrder;
        Core::JSON::EnumType<startup> Startup;

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
        {
            Trace::Text::Set(_T("Setting: [") + key + _T("] set to value [") + (value ? _T("TRUE]") : _T("FALSE]")));
        }
        Setting(const string& key, const string& value)
        {
            Trace::Text::Set(_T("Setting: [") + key + _T("] set to value [") + value + _T("]"));
        }
        template <typename NUMBERTYPE, const bool SIGNED, const NumberBase BASETYPE>
        Setting(const string& key, const NUMBERTYPE value)
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
