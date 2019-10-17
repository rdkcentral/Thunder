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

#pragma once

#include "Module.h"
#include "Tracing.h"
#include <lxc/lxccontainer.h>
#include <vector>
#include <utility>
#include <thread>
#include <cctype>
#include "../../ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {
    using LxcContainerType = struct lxc_container;

    class LXCNetworkInterfaceIterator : public NetworkInterfaceIterator 
    {
    public:
        LXCNetworkInterfaceIterator(LxcContainerType* lxcContainer);
        ~LXCNetworkInterfaceIterator();

        std::string Name() const override;
        uint32_t NumIPs() const override;

        std::string IP(uint32_t id) const override;
    private:
        struct LXCNetInterface {
            char* name;
            char** addresses;
            uint32_t numAddresses;
        };

        std::vector<LXCNetInterface> _interfaces;
    };

    class LXCContainer : public IContainer {
    private:
        class Config : public Core::JSON::Container {
        public:
            class ConfigItem : public Core::JSON::Container {
            public:
                ConfigItem& operator=(const ConfigItem&) = delete;

                ConfigItem(const ConfigItem& rhs);
                ConfigItem();
                
                ~ConfigItem() = default;

                Core::JSON::String Key;
                Core::JSON::String Value;
            };
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config();
            ~Config() = default;

            Core::JSON::String ConsoleLogging;
            Core::JSON::ArrayType<ConfigItem> ConfigItems;
#ifdef __DEBUG__
            Core::JSON::Boolean Attach;
#endif
        };

    public:

        LXCContainer(const LXCContainer&) = delete;
        LXCContainer& operator=(const LXCContainer&) = delete;

        LXCContainer(const string& name, LxcContainerType* lxcContainer, const string& containerLogDir, const string& configuration, const string& lxcPath);

        const string Id() const override;
        uint32_t Pid() const override;
        MemoryInfo Memory() const override;
        CPUInfo Cpu() const override;
        NetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;

        bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override;
        uint32_t Release() override;

    protected:
        void InheritRequestedEnvironment();

    private:
        const string _name;
        uint32_t _pid;
        string _lxcPath;
        string _containerLogDir;
        mutable Core::CriticalSection _adminLock;
        mutable uint32_t _referenceCount;
        LxcContainerType* _lxcContainer;
#ifdef __DEBUG__
        bool _attach;
#endif
    };

    class LXCContainerAdministrator : public ProcessContainers::IContainerAdministrator {
    public:
        friend class LXCContainer;

    private:
        static constexpr char const* logFileName = "lxclogging.log";
        static constexpr char const* configFileName = "config";
        static constexpr uint32_t maxReadSize = 32 * (1 << 10); // 32KiB
    public:
        LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
        LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;

        LXCContainerAdministrator();
        virtual ~LXCContainerAdministrator();

        ProcessContainers::IContainer* Container(const string& name, IStringIterator& searchpaths, const string& containerLogDir, const string& configuration) override;

        void Logging(const string& globalLogDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

        void AddRef() const override;
        uint32_t Release() override;
    protected:
        void RemoveContainer(ProcessContainers::IContainer* container);

    private:
        mutable Core::CriticalSection _lock;
        std::list<IContainer*> _containers;
        string _globalLogDir;
    };

}
}