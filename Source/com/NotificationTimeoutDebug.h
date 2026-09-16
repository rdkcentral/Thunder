/*
 * SPECIAL, NON-UPSTREAM DEBUG INSTRUMENTATION.
 *
 * This header is NOT part of stock Thunder and must not be pushed upstream. It exists only to support a
 * diagnostic RDK build whose purpose is to intentionally crash Thunder (see INTENTIONAL_CRASH in
 * core/Trace.h) at the exact moment an IPC timeout occurs while Thunder is walking its list of registered
 * PluginHost::IPlugin::INotification observers (Source/WPEFramework/PluginServer.h, ServiceMap::Activated /
 * Deactivated / Unavailable), so that the resulting crash report captures live process state at that instant.
 *
 * All clients that observe plugin activation/deactivation (e.g. via RPC::SmartInterfaceType or
 * WPEFramework::SmartLinkType) register a IPlugin::INotification sink with Thunder's ServiceMap. Internally,
 * a remote/out-of-process sink is represented in Thunder's process as a COM-RPC proxy
 * (ProxyStub::UnknownProxyType<PluginHost::IPlugin::INotification>). Every COM-RPC method invocation
 * (including Activated/Deactivated/Unavailable calls to that proxy) funnels through the single generic
 * ProxyStub::UnknownProxy::Invoke() (com/IUnknown.cpp), which already detects IPC timeouts
 * (Core::ERROR_TIMEDOUT). The thread_local state declared here lets ServiceMap's notification loop tell
 * Invoke() "which client (by registration order / iterator position) is currently being called on this
 * thread", without changing any interface/ABI - it is pure out-of-band, same-process bookkeeping.
 *
 * Enable by defining THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION for the whole build. Leaving it undefined
 * compiles this feature out entirely (zero footprint), which is the default/upstream state.
 */
#pragma once

#ifdef THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION

#include "Module.h"

namespace WPEFramework {
namespace RPC {
namespace NotificationTimeoutDebug {

    // 0-based position, within Thunder's ServiceMap::_notifiers list at the time of the call, of the
    // IPlugin::INotification client currently being invoked (Activated/Deactivated/Unavailable) on this
    // thread. Set/cleared by ServiceMap's notification loop; ~0 when no notification call is in flight on
    // this thread. NOTE: because _notifiers is a std::vector that can shrink (Unregister), this position is
    // only guaranteed to match the registration-order index logged by ServiceMap::Register() if no client
    // unregistered in between - treat it as a strong correlation hint, not an absolute guarantee.
    extern thread_local uint32_t CurrentIteratorIndex;

    // Raw sink pointer currently being invoked. Diagnostic only - never dereferenced by the reader.
    extern thread_local const void* CurrentSinkPointer;

    // Records the PID a channel announced (via the existing, unchanged 4.x AnnounceMessage/Data::Init::Id()
    // wire field) keyed by that channel's Core::IPCChannel::LinkId(), so it can be recovered later from any
    // proxy created over the same channel (e.g. a registered IPlugin::INotification observer).
    // Deliberately NOT stored via Core::IPCChannel::CustomData() - that slot is already used by real Thunder
    // code (Administrator::IsValid, CommunicatorClient::Offer/Revoke) for security-sensitive instance
    // validation and must not be repurposed.
    void RecordChannelPid(uintptr_t linkId, uint32_t pid);
    uint32_t LookupChannelPid(uintptr_t linkId);

} // namespace NotificationTimeoutDebug
} // namespace RPC
} // namespace WPEFramework

#endif // THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION

