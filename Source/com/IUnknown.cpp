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
#include "NotificationTimeoutDebug.h"

#ifdef THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION
#include <map>
#endif

namespace WPEFramework {

#ifdef THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION
namespace RPC {
namespace NotificationTimeoutDebug {

    thread_local uint32_t CurrentIteratorIndex = static_cast<uint32_t>(~0);
    thread_local const void* CurrentSinkPointer = nullptr;

    namespace {
        Core::CriticalSection _channelPidLock;
        std::map<uintptr_t, uint32_t> _channelPidMap;
    }

    void RecordChannelPid(uintptr_t linkId, uint32_t pid)
    {
        _channelPidLock.Lock();
        _channelPidMap[linkId] = pid;
        _channelPidLock.Unlock();
    }

    uint32_t LookupChannelPid(uintptr_t linkId)
    {
        uint32_t result = 0;

        _channelPidLock.Lock();
        std::map<uintptr_t, uint32_t>::const_iterator index(_channelPidMap.find(linkId));
        if (index != _channelPidMap.end()) {
            result = index->second;
        }
        _channelPidLock.Unlock();

        return (result);
    }

} // namespace NotificationTimeoutDebug
} // namespace RPC
#endif // THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION

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
                response.Number<Core::instance_id>(RPC::instance_cast<void*>(newInterface));

                if (newInterface != nullptr) {
                    RPC::Administrator::Instance().RegisterInterface(channel, newInterface, newInterfaceId);
                }

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
    uint32_t UnknownProxy::Invoke(Core::ProxyType<RPC::InvokeMessage>& message, const uint32_t waitTime) const
    {
        uint32_t result = Core::ERROR_UNAVAILABLE | COM_ERROR;

        _adminLock.Lock();
	    Core::ProxyType<Core::IPCChannel> channel (_channel);
        _adminLock.Unlock();

        if (channel.IsValid() == true) {
            result = channel->Invoke(message, waitTime);

            if (result != Core::ERROR_NONE) {

                if (result == Core::ERROR_TIMEDOUT) {
                    SYSLOG(Logging::Error, (_T("IPC method Invoke failed due to timeout (Interface ID 0x%X, Method ID 0x%X). Execution of code may or may not have happened. Side effects are to be expected after this message"), message->Parameters().InterfaceId(), message->Parameters().MethodId()));

#ifdef THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION
                    // Special, non-upstream instrumentation: correlate this timeout back to the exact
                    // IPlugin::INotification client ServiceMap's notification loop was calling when it fired,
                    // then intentionally crash so a crash report captures process state at this exact point.
                    if (message->Parameters().InterfaceId() == RPC::ID_PLUGIN_NOTIFICATION) {
                        SYSLOG(Logging::Error, (_T("[NOTIFY-TIMEOUT-DEBUG] IPC timeout notifying an IPlugin::INotification client: registrationOrder(iteratorIndex)=%u, sinkPtr=%p"),
                            RPC::NotificationTimeoutDebug::CurrentIteratorIndex, RPC::NotificationTimeoutDebug::CurrentSinkPointer));

                        INTENTIONAL_CRASH("IPC timeout notifying IPlugin::INotification client at registration order/iterator index %u (sink %p)",
                            RPC::NotificationTimeoutDebug::CurrentIteratorIndex, RPC::NotificationTimeoutDebug::CurrentSinkPointer);
                    }
#endif // THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION
                }

                result |= COM_ERROR;

                // Oops something failed on the communication. Report it.
                TRACE_L1("IPC method invocation failed for 0x%X, error: %d", message->Parameters().InterfaceId(), result);
            }
        }

        return (result);
    }

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
