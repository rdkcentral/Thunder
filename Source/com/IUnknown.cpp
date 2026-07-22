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

#if __has_include("rdk_otlp_instrumentation.h")
#include "rdk_otlp_instrumentation.h"
#include <dlfcn.h>
#include <pthread.h>
#define RDK_OTEL_COM_ENABLED 1

namespace {
    // Function pointer types matching rdk_otlp_instrumentation.h
    using FnGetTraceparent = const char* (*)();
    using FnStartChild     = void (*)(const char*, const char*);
    using FnFinishChild    = void (*)();

    static FnGetTraceparent  s_getTraceparent = nullptr;
    static FnStartChild      s_startChild     = nullptr;
    static FnFinishChild     s_finishChild    = nullptr;
    static pthread_once_t    s_otelOnce       = PTHREAD_ONCE_INIT;

    // Called once per process via pthread_once.
    // RTLD_NOLOAD means: only succeed if librdk_otlp.so is ALREADY loaded in this process.
    // WPEFramework loads it (linked via PluginHost) — so tracing works there.
    // WPEProcess, mfrmgr, IARM daemons do NOT load it — handle is null, tracing silently
    // disabled, zero impact on their normal operation.
    static void resolveOtelSymbols() {
        void* handle = ::dlopen("librdk_otlp.so", RTLD_NOLOAD | RTLD_NOW);
        if (handle == nullptr) return;
        s_getTraceparent = reinterpret_cast<FnGetTraceparent>(::dlsym(handle, "rdk_otlp_get_current_traceparent"));
        s_startChild     = reinterpret_cast<FnStartChild>    (::dlsym(handle, "rdk_otlp_start_child_from_traceparent"));
        s_finishChild    = reinterpret_cast<FnFinishChild>   (::dlsym(handle, "rdk_otlp_finish_child_span"));
        // Intentionally keep handle open — function pointers stay valid for process lifetime
    }

    static inline void ensureOtelResolved() {
        ::pthread_once(&s_otelOnce, resolveOtelSymbols);
    }
}

#else
#define RDK_OTEL_COM_ENABLED 0
#endif

namespace WPEFramework {
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
#if RDK_OTEL_COM_ENABLED
            bool _otelSpanStarted = false;
            ensureOtelResolved();
            if (s_getTraceparent != nullptr) {
                const char* tp = s_getTraceparent();
                if (tp != nullptr) {
                    char spanName[80];
                    snprintf(spanName, sizeof(spanName), "COMRPC.if0x%X.method%u",
                             message->Parameters().InterfaceId(),
                             message->Parameters().MethodId() - 3);
                    s_startChild(tp, spanName);
                    _otelSpanStarted = true;
                }
            }
#endif
            result = channel->Invoke(message, waitTime);
#if RDK_OTEL_COM_ENABLED
            if (_otelSpanStarted) { s_finishChild(); }
#endif

            if (result != Core::ERROR_NONE) {

                if (result == Core::ERROR_TIMEDOUT) {
                    SYSLOG(Logging::Error, (_T("IPC method Invoke failed due to timeout (Interface ID 0x%X, Method ID 0x%X). Execution of code may or may not have happened. Side effects are to be expected after this message"), message->Parameters().InterfaceId(), message->Parameters().MethodId()));
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
