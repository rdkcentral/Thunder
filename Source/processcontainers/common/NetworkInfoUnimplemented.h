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

    class UnimplementedNetworkInterfaceIterator : public INetworkInterfaceIterator
    {
    public:
        UnimplementedNetworkInterfaceIterator()
            : _empty("")
        {
        }

        virtual string Name() const
        {
            TRACE_L1("INetworkInterfaceIterator::Name() not implemented");
            return _empty;
        }

        virtual uint32_t NumIPs() const
        {
            TRACE_L1("INetworkInterfaceIterator::NumIPs() not implemented");
            return 0;
        }

        virtual string IP(uint32_t n = 0) const 
        {
            TRACE_L1("INetworkInterfaceIterator::IP() not implemented");
            return _empty;
        }

        virtual bool Next() 
        {
            return false;
        }

        virtual bool Previous() 
        {
            return false;
        }

        virtual void Reset(const uint32_t position) 
        {
        }
        
        virtual bool IsValid() const 
        {
            return false;
        }

        virtual uint32_t Index() const 
        {
            return Core::infinite;
        }

        virtual uint32_t Count() const 
        {
            return 0;
        };

        void AddRef() const 
        {
            
        };

        uint32_t Release() const 
        {
            uint32_t result = Core::ERROR_NONE;
            return result;
        };
    private:
        const string _empty;
    };

    template <typename Mixin> // IContainer Mixin
    class NetworkInfoUnimplemented : public Mixin
    {
    public:
        Core::ProxyType<INetworkInterfaceIterator> NetworkInterfaces() const override
        {
            return Core::ProxyType<INetworkInterfaceIterator>(
                Core::ProxyType<UnimplementedNetworkInterfaceIterator>::Create()
            );
        }
    };

} // ProcessContainers
} // WPEFramework