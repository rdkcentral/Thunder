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

    class UnimplementedNetworkInterfaceIterator : public BaseRefCount<INetworkInterfaceIterator> {
    public:
        UnimplementedNetworkInterfaceIterator()
            : _empty("")
        {
        }

        string Name() const override
        {
            TRACE_L1("INetworkInterfaceIterator::Name() not implemented");
            return _empty;
        }

        uint16_t NumAddresses() const override
        {
            TRACE_L1("INetworkInterfaceIterator::NumAddresses() not implemented");
            return 0;
        }

        string Address(uint16_t n = 0) const override
        {
            TRACE_L1("INetworkInterfaceIterator::IP() not implemented");
            return _empty;
        }

        bool Next() override
        {
            return false;
        }

        bool Previous() override
        {
            return false;
        }

        void Reset(const uint32_t position) override
        {
        }

        bool IsValid() const override
        {
            return false;
        }

        uint32_t Index() const override
        {
            return Core::infinite;
        }

        uint32_t Count() const override
        {
            return 0;
        };


    private:
        const string _empty;
    };

    //Helper class used for Filling default values for unimplemented network Intefrace
    class NetworkInfoUnimplemented {
    public:
        INetworkInterfaceIterator* NetworkInterfaces() const
        {
            return new UnimplementedNetworkInterfaceIterator;
        }
    };

} // ProcessContainers
} // WPEFramework
