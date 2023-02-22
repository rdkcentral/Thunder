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
#include "processcontainers/ProcessContainer.h"
#include "processcontainers/common/BaseAdministrator.h"
#include "processcontainers/common/BaseRefCount.h"
#include "Module.h"
#include "Tracing.h"
#include <cctype>
#include <lxc/lxccontainer.h>
#include <thread>
#include <utility>
#include <vector>

namespace WPEFramework {
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

    class LXCContainer : public BaseRefCount<IContainer> {
    private:
        class Config : public Core::JSON::Container {
        public:
            class ConfigItem : public Core::JSON::Container {
            public:
                ConfigItem& operator=(const ConfigItem&) = delete;

                ConfigItem(const ConfigItem& rhs);
                ConfigItem();

                ~ConfigItem() override = default;

                Core::JSON::String Key;
                Core::JSON::String Value;
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config();
            ~Config() override = default;

            Core::JSON::String ConsoleLogging;
            Core::JSON::ArrayType<ConfigItem> ConfigItems;
#ifdef __DEBUG__
            Core::JSON::Boolean Attach;
#endif
        };

    private:
        static constexpr uint32_t defaultTimeOutInMSec = 2000;

        friend class LXCContainerAdministrator;
        LXCContainer(const string& name, LxcContainerType* lxcContainer, const string& containerLogDir, const string& configuration, const string& lxcPath);
    public:
        LXCContainer(const LXCContainer&) = delete;
        ~LXCContainer() override;

        LXCContainer& operator=(const LXCContainer&) = delete;

        const string& Id() const override;
        uint32_t Pid() const override;
        IMemoryInfo* Memory() const override;
        IProcessorInfo* ProcessorInfo() const override;
        INetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;

        bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override;
        uint32_t Release() const override;

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

    class LXCContainerAdministrator : public BaseContainerAdministrator<LXCContainer> {
        friend class LXCContainer;
        friend class Core::SingletonType<LXCContainerAdministrator>;

    private:
        static constexpr char const* logFileName = "lxclogging.log";
        static constexpr char const* configFileName = "config";
        static constexpr uint32_t maxReadSize = 32 * (1 << 10); // 32KiB
    private:
        LXCContainerAdministrator();

    public:
        LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
        LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;
        ~LXCContainerAdministrator() override;

        ProcessContainers::IContainer* Container(const string& name, IStringIterator& searchpaths, const string& containerLogDir, const string& configuration) override;

        void Logging(const string& globalLogDir, const string& loggingOptions) override;

    private:
        string _globalLogDir;
    };

}
}
