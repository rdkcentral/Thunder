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
#include "processcontainers/common/CGroupContainerInfo.h"
#include "processcontainers/common/Lockable.h"
#include "processcontainers/common/NetworkInfoUnimplemented.h"

namespace WPEFramework {
namespace ProcessContainers {
    using RunCContainerMixins = CGroupContainerInfo<NetworkInfoUnimplemented<BaseRefCount<Lockable<IContainer>>>>;

    class RunCContainer : public RunCContainerMixins {
    private:
        friend class RunCContainerAdministrator;

        RunCContainer(const string& name, const string& path, const string& logPath);

    public:
        RunCContainer(const RunCContainer&) = delete;
        RunCContainer& operator=(const RunCContainer&) = delete;
        ~RunCContainer() override;

        // IContainerMethods
        const string& Id() const override;
        uint32_t Pid() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

    private:
        mutable uint32_t _refCount;
        string _name;
        string _path;
        string _logPath;
        mutable Core::OptionalType<uint32_t> _pid;
    };

    class RunCContainerAdministrator : public BaseAdministrator<RunCContainer, Lockable<IContainerAdministrator>> {
    private:
        friend class RunCContainer;
        friend class Core::SingletonType<RunCContainerAdministrator>;

        RunCContainerAdministrator();
        RunCContainerAdministrator(const RunCContainerAdministrator&) = delete;
        RunCContainerAdministrator& operator=(const RunCContainerAdministrator&) = delete;

    public:
        ~RunCContainerAdministrator() override;

        IContainer* Container(const string& id,
            IStringIterator& searchpaths,
            const string& logpath,
            const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;

    protected:
        void DestroyContainer(const string& name); // make sure that no leftovers from previous launch will cause crash
        bool ContainerNameTaken(const string& name);
    };
}
}
