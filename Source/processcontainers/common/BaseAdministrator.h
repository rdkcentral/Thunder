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

#include "processcontainers/ProcessContainer.h"
#include "processcontainers/common/BaseContainerIterator.h"

namespace WPEFramework {
namespace ProcessContainers {

    template <typename TContainer, typename Mixin> // IContainerAdministrator, Lockable Mixin
    class BaseAdministrator : public Mixin {
    public:
        BaseAdministrator() 
            : _containers()
        {

        }

        virtual ~BaseAdministrator() 
        {
            if (_containers.size() > 0) {
                TRACE_L1("There are still active containers when shutting down administrator!");
                
                while (_containers.size() > 0) {
                    _containers.back()->Release();
                    _containers.pop_back();
                }
            }
        }

        IContainerIterator* Containers() override
        {            
            std::vector<string> containers;
            containers.reserve(_containers.size());

            Mixin::InternalLock();
            for (auto& container : _containers) {
                containers.push_back(container->Id());
            }
            Mixin::InternalUnlock();            

            return new BaseContainerIterator(std::move(containers));
        }

        IContainer* Get(const string& id) override
        {
            IContainer* result = nullptr;

            Mixin::InternalLock();
            auto found = std::find_if(_containers.begin(), _containers.end(), [&id](const IContainer* c) {return c->Id() == id;});
            if (found != _containers.end()) {
                result = *found;
                result->AddRef();
            }
            Mixin::InternalUnlock();

            return result;
        }

        // Call only from TContainer when destructing instance!
        void RemoveContainer(TContainer* container)
        {
            Mixin::InternalLock();
            _containers.remove(container);
            Mixin::InternalUnlock();
        }

    protected:
        // Must be called in internal Lock!
        void InsertContainer(TContainer* container)
        {
            _containers.push_back(container);
        }

    private:
        std::list<TContainer*> _containers;
    };

} // ProcessContainers
} // WPEFramework