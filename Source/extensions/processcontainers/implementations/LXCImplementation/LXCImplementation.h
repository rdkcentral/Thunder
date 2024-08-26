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

#include <lxc/lxccontainer.h>

#include "processcontainers/Messaging.h"
#include "processcontainers/ContainerAdministrator.h"
#include "processcontainers/common/CGroupContainerInfo.h"

namespace Thunder {
namespace ProcessContainers {
    using LxcContainerType = struct lxc_container;

    class LXCNetworkInterfaceIterator : public BaseRefCount<INetworkInterfaceIterator> {
    public:
        LXCNetworkInterfaceIterator(LxcContainerType* lxcContainer);
        ~LXCNetworkInterfaceIterator() override;

        bool Next();
        bool Previous();
        void Reset(const uint32_t position);
        bool IsValid() const;
        uint32_t Index() const;
        uint32_t Count() const;

        string Name() const;
        uint16_t NumAddresses() const;
        string Address(const uint16_t n = 0) const;

    private:
        struct LXCNetInterface {
            char* name;
            char** addresses;
            uint32_t numAddresses;
        };

        uint32_t _current;
        std::vector<LXCNetInterface> _interfaces;
    };

    class LXCContainer : public IContainer {
    private:
        class Config : public Core::JSON::Container {
        public:
            class ConfigItem : public Core::JSON::Container {
            public:
                ConfigItem& operator=(ConfigItem&&) = delete;
                ConfigItem& operator=(const ConfigItem&) = delete;

                ~ConfigItem() override = default;

                ConfigItem()
                    : Core::JSON::Container()
                    , Key()
                    , Value()
                {
                    Init();
                }

                ConfigItem(const ConfigItem& copy)
                    : Core::JSON::Container()
                    , Key(copy.Key)
                    , Value(copy.Value)
                {
                    Init();
                }

                ConfigItem(ConfigItem&& move)
                    : Core::JSON::Container()
                    , Key(std::move(move.Key))
                    , Value(std::move(move.Value))
                {
                    Init();
                }

            private:
                void Init()
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                }

            public:
                Core::JSON::String Key;
                Core::JSON::String Value;
            };

        public:
            Config();
            Config(const Config&) = delete;
            Config(Config&&) = delete;
            Config& operator=(const Config&) = delete;
            Config& operator=(Config&&) = delete;
            ~Config() override = default;

        public:
            Core::JSON::String ConsoleLogging;
            Core::JSON::ArrayType<ConfigItem> ConfigItems;

#ifdef __DEBUG__
            Core::JSON::Boolean Attach;
#endif
        };

    private:
        static constexpr uint32_t defaultTimeOutInMSec = 2000;

    public:
        LXCContainer(const string& name, LxcContainerType* lxcContainer, const string& containerLogDir,
            const string& configuration, const string& lxcPath);

        LXCContainer() = delete;
        ~LXCContainer() override;

        LXCContainer(const LXCContainer&) = delete;
        LXCContainer(LXCContainer&&) = delete;
        LXCContainer& operator=(const LXCContainer&) = delete;
        LXCContainer& operator=(LXCContainer&&) = delete;

    public:
        containertype Type() const override { return IContainer::LXC; }
        const string& Id() const override;
        uint32_t Pid() const override;
        IMemoryInfo* Memory() const override;
        IProcessorInfo* ProcessorInfo() const override;
        INetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;
        bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

    private:
        void InheritRequestedEnvironment();

    private:
        const string _name;
        uint32_t _pid;
        string _lxcPath;
        string _containerLogDir;
        mutable Core::CriticalSection _adminLock;
        mutable LxcContainerType* _lxcContainer;

#ifdef __DEBUG__
        bool _attach;
#endif
    };

    class LXCContainerAdministrator : public IContainerProducer {
    public:
        LXCContainerAdministrator() = default;
        ~LXCContainerAdministrator() override = default;

        LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
        LXCContainerAdministrator(LXCContainerAdministrator&&) = delete;
        LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;
        LXCContainerAdministrator& operator=(LXCContainerAdministrator&&) = delete;

    public:
        uint32_t Initialize(const string& config) override;
        void Deinitialize() override;
        Core::ProxyType<IContainer> Container(const string& name, IStringIterator& searchpaths,
                                        const string& containerLogDir, const string& configuration) override;

    private:
        void Logging(const string& globalLogDir, const string& loggingOptions);
    };

}
}
