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

#include "processcontainers/Messaging.h"
#include "processcontainers/ProcessContainer.h"
#include "processcontainers/common/BaseAdministrator.h"
#include "processcontainers/common/BaseRefCount.h"
#include "processcontainers/common/CGroupContainerInfo.h"

namespace Thunder {
namespace ProcessContainers {

    namespace runc {
        class Runner;
    }

    class RunCContainer : public BaseRefCount<IContainer> {
    private:
        friend class RunCContainerAdministrator;
        RunCContainer(const string& id, const string& path, const string& logPath);

    public:
        RunCContainer(const RunCContainer&) = delete;
        RunCContainer& operator=(const RunCContainer&) = delete;
        RunCContainer(RunCContainer&&) = delete;
        RunCContainer& operator=(RunCContainer&&) = delete;
        ~RunCContainer() override;

    public:
        // IContainer methods
        const string& Id() const override;
        uint32_t Pid() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /* ms */) override;
        IMemoryInfo* Memory() const override;
        IProcessorInfo* ProcessorInfo() const override;
        INetworkInterfaceIterator* NetworkInterfaces() const override;

    private:
        uint32_t InternalPurge(const uint32_t timeout = 0);

    private:
        mutable Core::CriticalSection _adminLock;
        string _id;
        string _path;
        mutable uint32_t _pid;
        std::unique_ptr<runc::Runner> _runner;
    };

    class RunCContainerAdministrator : public BaseContainerAdministrator<RunCContainer> {
    private:
        friend class RunCContainer;
        friend class Core::SingletonType<RunCContainerAdministrator>;
        RunCContainerAdministrator() = default;

    public:
        RunCContainerAdministrator(const RunCContainerAdministrator&) = delete;
        RunCContainerAdministrator(RunCContainerAdministrator&&) = delete;
        RunCContainerAdministrator& operator=(const RunCContainerAdministrator&) = delete;
        RunCContainerAdministrator& operator=(RunCContainerAdministrator&&) = delete;
        ~RunCContainerAdministrator() override = default;

    public:
        // IContainerAdministrator methods
        IContainer* Container(const string& id, IStringIterator& searchpaths, const string& logpath, const string& configuration) override;
        void Logging(const string& logDir, const string& loggingOptions) override;
    };

} // namespace ProcessContainers
}
