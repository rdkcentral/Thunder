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

#include "IUnknown.h"
#include "Administrator.h"
#include "Communicator.h"

namespace Thunder {
namespace ProxyStub {
    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    /* virtual */ void UnknownStub::Handle(const uint16_t index,
        Core::ProxyType<Core::IPCChannel>& channel,
        Core::ProxyType<RPC::InvokeMessage>& message)
    {
        Core::instance_id rawIdentifier(message->Parameters().Implementation());

        Core::IUnknown* implementation(Convert(reinterpret_cast<void*>(rawIdentifier)));

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

                // Get the amount of Release we have to do..
                uint32_t dropReleases(reader.Number<uint32_t>());
                uint32_t result;

                ASSERT(dropReleases > 0);

                // This is an external referenced interface that we handed out, so it should
                // be registered. Lets unregister this reference, it is dropped
                // Dropping the ReceoverSey, if applicable
                RPC::Administrator::Instance().UnregisterInterface(channel, implementation, InterfaceId(), dropReleases);

                do {
                   result = implementation->Release();
                   dropReleases--;
                } while ((dropReleases != 0) && ((result == Core::ERROR_NONE) || (result == Core::ERROR_COMPOSIT_OBJECT)));

                ASSERT(dropReleases == 0);

                response.Number<uint32_t>(result);
                break;
            }
            case 2: {
                // QueryInterface
                RPC::Data::Frame::Reader reader(message->Parameters().Reader());
                RPC::Data::Frame::Writer response(message->Response().Writer());
                uint32_t newInterfaceId(reader.Number<uint32_t>());

                void* newInterface = implementation->QueryInterface(newInterfaceId);

                if (newInterface != nullptr) {
                    if (RPC::Administrator::Instance().RegisterInterface(channel, newInterface, newInterfaceId) == false) {
                        Convert(newInterface)->Release();
                        newInterface = nullptr;
                    }
                }

                response.Number<Core::instance_id>(RPC::instance_cast<void*>(newInterface));

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
    const Core::SocketPort* UnknownProxy::Socket() const
    {
        const Core::SocketPort* result = nullptr;

        _adminLock.Lock();
        if (_channel.IsValid() == true) {
            const RPC::Communicator::Client* comchannel = dynamic_cast<const RPC::Communicator::Client*>(_channel.operator->());
            if (comchannel != nullptr) {
                result = &(comchannel->Source());
            }
        }
        _adminLock.Unlock();

        return (result);
    }

    static class UnknownInstantiation {
    public:
        UnknownInstantiation()
        {
            RPC::Administrator::Instance().Announce<Core::IUnknown, UnknownProxyType<Core::IUnknown>, UnknownStub>();
        }
        ~UnknownInstantiation()
        {
            RPC::Administrator::Instance().Recall<Core::IUnknown>();
        }

    } UnknownRegistration;
}
}
