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
#include "processcontainers/common/BaseRefCount.h"

namespace WPEFramework {
namespace ProcessContainers {

    class BaseContainerIterator : public BaseRefCount<IContainerIterator> {
    public:
        BaseContainerIterator(std::vector<string>&& ids)
            : _ids(std::move(ids))
            , _current(UINT32_MAX)
        {
        }

        ~BaseContainerIterator() override = default;

        virtual bool Next() override
        {
            if (_current == UINT32_MAX)
                _current = 0;
            else
                ++_current;

            return IsValid();
        }

        bool Previous() override
        {
            if (_current == UINT32_MAX)
                _current = _ids.size() - 1;
            else
                --_current;

            return IsValid();
        }

        void Reset(const uint32_t position) override
        {
            (void) position;
            _current = UINT32_MAX;
        }

        bool IsValid() const override
        {
            bool valid = (_current < _ids.size()) && (_current != UINT32_MAX);

            return valid;
        }

        uint32_t Index() const override
        {
            return _current;
        }

        uint32_t Count() const override
        {
            return _ids.size();
        }

        const string& Id() const override
        {
            ASSERT(IsValid() == true);

            return _ids[_current];
        }

    private:
        std::vector<string> _ids;
        uint32_t _current;
    };

} // ProcessContainers
} // WPEFramework
