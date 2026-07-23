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
#include <mutex>
#include <atomic>
#define RDK_OTEL_COM_ENABLED 1

namespace {
    // Function pointer types matching rdk_otlp_instrumentation.h
    using FnGetTraceparent    = const char* (*)();
    using FnStartChild        = void (*)(const char*, const char*);
    using FnFinishChild       = void (*)();
    using FnResumeTraceparent = void (*)(const char*);

    static FnGetTraceparent           s_getTraceparent    = nullptr;
    static FnStartChild               s_startChild        = nullptr;
    static FnFinishChild              s_finishChild       = nullptr;
    static FnResumeTraceparent        s_resumeTraceparent = nullptr;
    static std::atomic<bool>          s_otelResolved{false};
    static std::mutex                 s_otelMutex;

    // Retriable lazy resolution.
    // RTLD_NOLOAD succeeds only when librdk_otlp.so is already mapped into this process.
    // WPEFramework loads it at startup (rdk_otlp_init in PluginHost.cpp).
    // The first UnknownProxy::Invoke() call happens during plugin boot — BEFORE
    // rdk_otlp_init() — so dlopen fails on that first call.  We MUST NOT use
    // pthread_once here: once it fires with a null handle we'd never retry and
    // s_getTraceparent would stay null for the whole process lifetime.
    // Instead we mark resolved only after we successfully obtain the symbols,
    // so every Invoke() call before the library is loaded is a cheap miss, and
    // the first call after rdk_otlp_init() picks up all three function pointers.
    static inline void ensureOtelResolved() {
        // Fast path: already resolved (acquire matches the release store below).
        if (s_otelResolved.load(std::memory_order_acquire)) return;

        std::lock_guard<std::mutex> lk(s_otelMutex);
        if (s_otelResolved.load(std::memory_order_relaxed)) return; // double-check

        void* handle = ::dlopen("librdk_otlp.so", RTLD_NOLOAD | RTLD_NOW);
        if (handle == nullptr) return; // library not loaded yet — retry next call

        s_getTraceparent    = reinterpret_cast<FnGetTraceparent>   (::dlsym(handle, "rdk_otlp_get_current_traceparent"));
        s_startChild        = reinterpret_cast<FnStartChild>       (::dlsym(handle, "rdk_otlp_start_child_from_traceparent"));
        s_finishChild       = reinterpret_cast<FnFinishChild>      (::dlsym(handle, "rdk_otlp_finish_child_span"));
        s_resumeTraceparent = reinterpret_cast<FnResumeTraceparent>(::dlsym(handle, "rdk_otlp_resume_traceparent"));
        // Intentionally keep handle open — function pointers stay valid for process lifetime.
        // Only mark resolved once we have a non-null get-traceparent pointer.
        if (s_getTraceparent != nullptr) {
            s_otelResolved.store(true, std::memory_order_release);
        }
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
            char _otelParentTp[64] = {};   // saved copy of parent traceparent
            ensureOtelResolved();
            if (s_getTraceparent != nullptr) {
                const char* tp = s_getTraceparent();
                if (tp != nullptr) {
                    // Copy the parent traceparent BEFORE starting the child.
                    // s_startChild will overwrite the TLS slot; we need the
                    // original string to restore it afterwards so that the next
                    // Invoke() on the same thread still has the parent context.
                    strncpy(_otelParentTp, tp, sizeof(_otelParentTp) - 1);
                    char spanName[80];
                    snprintf(spanName, sizeof(spanName), "COMRPC.if0x%X.method%u",
                             message->Parameters().InterfaceId(),
                             message->Parameters().MethodId() - 3);
                    s_startChild(_otelParentTp, spanName);
                    _otelSpanStarted = true;
                }
            }
#endif
            result = channel->Invoke(message, waitTime);
#if RDK_OTEL_COM_ENABLED
            if (_otelSpanStarted) {
                s_finishChild();
                // Restore parent context so the next Invoke() on this thread
                // still sees the parent span and generates another child.
                if (s_resumeTraceparent != nullptr) {
                    s_resumeTraceparent(_otelParentTp);
                }
            }
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
