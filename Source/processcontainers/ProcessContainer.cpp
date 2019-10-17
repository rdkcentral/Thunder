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

#include "ProcessContainer.h"
#include <stdint.h>
namespace WPEFramework {
namespace ProcessContainers {

    NetworkInterfaceIterator::NetworkInterfaceIterator() 
        : _current(UINT32_MAX)
        , _count(0)
        , _refCount(1)
    {
        
    }

    void NetworkInterfaceIterator::AddRef()
    {
        _refCount++;
    }

    void NetworkInterfaceIterator::Release()
    {
        if (--_refCount == 0) {
            delete this;
        }
    }

    bool NetworkInterfaceIterator::Next() 
    {
        if (_current == UINT32_MAX)
            _current = 0;
        else
            ++_current;

        return IsValid();
    }

    void NetworkInterfaceIterator::Reset() 
    {
        _current = UINT32_MAX;
    }

    bool NetworkInterfaceIterator::IsValid() const
    {
        return (_current < _count) && (_current != UINT32_MAX);
    }

    uint32_t NetworkInterfaceIterator::Count() const
    {
        return _count;
    }

    IContainer* IContainerAdministrator::Find(const string& id)
    {
        auto iterator = Containers();
        IContainer* result = nullptr;

        while(iterator.Next() == true) {
            if (iterator.Current()->Id() == id) {
                result = &(*iterator.Current());
                break;
            }
        }

        return result;
    }

}
}