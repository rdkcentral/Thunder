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

#include "IUnknown.h"
#include "Administrator.h"

namespace WPEFramework {
namespace ProxyStub {
    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    UnknownStub::UnknownStub()
    {
    }

    /* virtual */ UnknownStub::~UnknownStub()
    {
    }

    /* virtual */ void UnknownStub::Handle(const uint16_t index,
        Core::ProxyType<Core::IPCChannel>& channel,
        Core::ProxyType<RPC::InvokeMessage>& message)
    {
        void* rawIdentifier(message->Parameters().Implementation<void*>());
        Core::IUnknown* implementation(Convert(rawIdentifier));

        ASSERT(implementation != nullptr);

        if (implementation != nullptr) {
            switch (index) {
            case 0: {
                // AddRef
                implementation->AddRef();
                break;
            }
            case 1: {
                // Release
                RPC::Data::Frame::Writer response(message->Response().Writer());
                RPC::Data::Frame::Reader reader(message->Parameters().Reader());
                uint32_t dropCount(reader.Number<uint32_t>());
                uint32_t result = Core::ERROR_NONE;
                uint32_t index = dropCount;

                while (index-- != 0) { result = implementation->Release(); }
                
                // This is an external referenced interface that we handed out, so it should
                // be registered. Lets unregister this reference, it is dropped
                RPC::Administrator::Instance().UnregisterInterface(channel, rawIdentifier, InterfaceId(), dropCount);
                response.Number<uint32_t>(result);
                break;
            }
            case 2: {
                // QueryInterface
                RPC::Data::Frame::Reader reader(message->Parameters().Reader());
                RPC::Data::Frame::Writer response(message->Response().Writer());
                uint32_t newInterfaceId(reader.Number<uint32_t>());

                void* newInterface = implementation->QueryInterface(newInterfaceId);
                response.Number<void*>(newInterface);

                RPC::Administrator::Instance().RegisterInterface(channel, newInterface, newInterfaceId);
                break;
            }
            default: {
                TRACE_L1("Method ID [%d] not existing.\n", index);
                break;
            }
            }
        }
    }

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    static class UnknownInstantiation {
    public:
        UnknownInstantiation()
        {
            RPC::Administrator::Instance().Announce<Core::IUnknown, UnknownProxyType<Core::IUnknown>, UnknownStub>();
        }
        ~UnknownInstantiation()
        {
        }

    } UnknownRegistration;
}
}
