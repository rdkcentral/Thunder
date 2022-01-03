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
#include "processcontainers/common/BaseContainerIterator.h"

namespace WPEFramework {
namespace ProcessContainers {

    template <typename CONTAINER>
    class BaseContainerAdministrator : public IContainerAdministrator {
    public:
        BaseContainerAdministrator()
            : _containers()
            , _adminLock()
        {
        }

        ~BaseContainerAdministrator() override
        {
            if (_containers.size() > 0) {
                TRACE_L1("There are still active containers when shutting down administrator!");

                while (_containers.size() > 0) {
                    _containers.back()->Release();
                    _containers.pop_back();
                }
            }
        }

        IContainerIterator* Containers() const override
        {
            std::vector<string> containers;
            containers.reserve(_containers.size());

            _adminLock.Lock();
            for (auto& container : _containers) {
                containers.push_back(container->Id());
            }
            _adminLock.Unlock();

            return new BaseContainerIterator(std::move(containers));
        }

        IContainer* Get(const string& id) override
        {
            IContainer* result = nullptr;

            _adminLock.Lock();
            auto found = std::find_if(_containers.begin(), _containers.end(), [&id](const IContainer* c) { return c->Id() == id; });
            if (found != _containers.end()) {
                result = *found;
                result->AddRef();
            }
            _adminLock.Unlock();

            return result;
        }

        // Called only from CONTAINER when destructing instance!
        void RemoveContainer(CONTAINER* container)
        {
            _adminLock.Lock();
            _containers.remove(container);
            _adminLock.Unlock();
        }

        void InternalLock() const
        {
            _adminLock.Lock();
        }

        void InternalUnlock() const
        {
            _adminLock.Unlock();
        }

    protected:
        // Must be called in internal Lock!
        void InsertContainer(CONTAINER* container)
        {
            _containers.push_back(container);
        }

    private:
        std::list<CONTAINER*> _containers;
        mutable Core::CriticalSection _adminLock;
    };

} // ProcessContainers
} // WPEFramework
