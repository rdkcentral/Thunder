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

#include <cstdint>


namespace Implementation {

namespace Platform {

    class TEE  {
    public:
        TEE(const TEE&) = delete;
        TEE& operator=(const TEE&) = delete;

        TEE()
            : _moduleHandle(nullptr)
            , _drmHandle(0)
            , _drmVersion(0)
        {
        }

        virtual ~TEE() = default;

    public:
        virtual void* ModuleHandle() const
        {
            return _moduleHandle;
        }

        virtual uint32_t DRMHandle() const
        {
            return _drmHandle;
        }

        virtual uint32_t DRMVersion() const
        {
            return _drmVersion;
        }

    protected:
        virtual void ModuleHandle(void* handle)
        {
            _moduleHandle = handle;
        }

        virtual void DRMHandle(const uint32_t handle)
        {
            _drmHandle = handle;
        }

        virtual void DRMVersion(const uint32_t version)
        {
            _drmVersion = version;
        }

    private:
        void* _moduleHandle;
        uint32_t _drmHandle;
        uint32_t _drmVersion;
    };

} // namespace Platform

} // namespace Implementation

