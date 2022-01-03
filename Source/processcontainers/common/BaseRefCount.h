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

namespace WPEFramework {
namespace ProcessContainers {

    template <typename CONTEXT>
    class BaseRefCount : public CONTEXT {
    private:
        BaseRefCount<CONTEXT>& operator=(const BaseRefCount<CONTEXT>& rhs) = delete;
    public:
        BaseRefCount()
            : _refCount(1)
        {
        }

        void AddRef() const override
        {
            Core::InterlockedIncrement(_refCount);
        }

        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;
            if (Core::InterlockedDecrement(_refCount) == 0) {
                delete this;

                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            return result;
        }

    private:
        mutable uint32_t _refCount;
    };

} // ProcessContainers
} // WPEFramework
