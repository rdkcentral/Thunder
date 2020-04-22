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

namespace WPEFramework {
namespace ProcessContainers {

    template <typename TContainer, typename Mixin> // IContainerAdministrator Mixin
    class BaseAdministrator : public Mixin {
    public:
        BaseAdministrator() 
            : _adminLock()
            , _containers()
        {

        }

        std::vector<string> Containers() override
        {
            std::vector<string> result;

            _adminLock.Lock();
            for (auto container : _containers) {
                result.push_back(container->Id());
            }
            _adminLock.Unlock();

            return result;
        }

        IContainer* Get(const string& id) override
        {
            IContainer* result = nullptr;

            _adminLock.Lock();
            auto found = std::find_if(_containers.begin(), _containers.end(), [&id](const IContainer* c) {return c->Id() == id;});
            if (found != _containers.end()) {
                result = *found;
                result->AddRef();
            }
            _adminLock.Unlock();

            return result;
        }

        // Call only from TContainer when destructing instance!
        void RemoveContainer(TContainer* container)
        {
            _adminLock.Lock();
            _containers.remove(container);
            _adminLock.Unlock();
        }

    protected:

        Core::CriticalSection _adminLock;
        std::list<TContainer*> _containers;
    };

} // ProcessContainers
} // WPEFramework