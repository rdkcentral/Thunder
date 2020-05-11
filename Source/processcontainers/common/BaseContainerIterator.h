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

    class BaseContainerIterator : public IContainerIterator {
    public:
        BaseContainerIterator()
            : _ids()
            , _current(UINT32_MAX)
        {
        }

        virtual ~BaseContainerIterator() {};

        virtual bool Next()
        {
            if (_current == UINT32_MAX)
                _current = 0;
            else
                ++_current;

            return IsValid();
        }

        virtual bool Previous()
        {
            if (_current == UINT32_MAX)
                _current = _ids.size() - 1;
            else
                --_current;

            return IsValid();
        }

        virtual void Reset(const uint32_t position)
        {
            _current = UINT32_MAX;
        }

        virtual bool IsValid() const
        {
            bool valid = (_current < _ids.size()) && (_current != UINT32_MAX);
            
            return valid;
        }

        virtual uint32_t Index() const
        {
            return _current;
        }

        virtual uint32_t Count() const
        {
            return _ids.size();
        }
        
        const string& Id()
        {
            return _ids[_current];
        }

        void Set(std::vector<string>&& ids) 
        {
            _ids = std::move(ids);
        }
    private:
        std::vector<string> _ids;
        uint32_t _current;
    };

} // ProcessContainers
} // WPEFramework