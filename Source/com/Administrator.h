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

#ifndef __COM_ADMINISTRATOR_H
#define __COM_ADMINISTRATOR_H

#include "Messages.h"
#include "Module.h"

namespace Thunder {

namespace ProxyStub {

    class UnknownStub;
    class UnknownProxy;
}

namespace RPC {

#ifdef __DEBUG__
    enum { CommunicationTimeOut = Core::infinite }; // Time in ms. Forever
#else
    enum { CommunicationTimeOut = 3000 }; // Time in ms. 3 Seconds
#endif
    enum { CommunicationBufferSize = 8120 }; // 8K :-)

    struct InstanceRecord {
        Core::instance_id instance;
        uint32_t interface;
    };

    class EXTERNAL Administrator {
    private:
        Administrator();

        class RecoverySet {
        public:
            RecoverySet() = delete;
            RecoverySet(RecoverySet&&) = delete;
            RecoverySet(const RecoverySet&) = delete;
            RecoverySet& operator=(RecoverySet&&) = delete;
            RecoverySet& operator=(const RecoverySet&) = delete;

            RecoverySet(const uint32_t id, Core::IUnknown* object)
                : _interfaceId(id)
                , _interface(object)
                , _referenceCount(1 | (object->AddRef() == Core::ERROR_COMPOSIT_OBJECT ? 0x80000000 : 0)) {
                
                // Check if this is a "Composit" object to which the IUnknown points. Composit means 
                // that the object is owned by another object that controls its lifetime and that 
                // object will handle the lifetime of this object. It will be released anyway, 
                // no-recovery needed.
                object->Release();
            }
            ~RecoverySet() = default;

        public:
            bool IsComposit() const {
                return ((_referenceCount & 0x80000000) != 0);
            }
            inline uint32_t Id() const {
                return (_interfaceId);
            }
            inline Core::IUnknown* Unknown() const {
                return (_interface);
            }
            inline void Increment() {
                _referenceCount = ((_referenceCount & 0x7FFFFFFF) + 1) | (_referenceCount & 0x80000000);
            }
            inline bool Decrement(const uint32_t dropCount) {
                ASSERT((_referenceCount & 0x7FFFFFFF) >= dropCount);
                _referenceCount = ((_referenceCount & 0x7FFFFFFF) - dropCount) | (_referenceCount & 0x80000000);
                return((_referenceCount & 0x7FFFFFFF) > 0);
            }
#ifdef __DEBUG__
            bool Flushed() const {
                return (((_referenceCount & 0x7FFFFFFF) == 0) || (IsComposit()));
            }
#endif

        private:
            uint32_t _interfaceId;
            Core::IUnknown* _interface;
            uint32_t _referenceCount;
        };

        struct EXTERNAL IMetadata {
            virtual ~IMetadata() = default;

            virtual ProxyStub::UnknownProxy* CreateProxy(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& implementation, const bool remoteRefCounted) = 0;
        };

        template <typename PROXY>
        class ProxyType : public IMetadata {
        public:
            ProxyType(ProxyType<PROXY>&& ) = delete;
            ProxyType(const ProxyType<PROXY>&) = delete;
            ProxyType<PROXY>& operator=(ProxyType<PROXY>&&) = delete;
            ProxyType<PROXY>& operator=(const ProxyType<PROXY>&) = delete;

            ProxyType() = default;
            ~ProxyType() override = default;

            ProxyStub::UnknownProxy* CreateProxy(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& implementation, const bool remoteRefCounted) override
            {
                return (new PROXY(channel, implementation, remoteRefCounted))->Administration();
            }
        };

    public:
        using Proxies = std::vector<ProxyStub::UnknownProxy*>;
        using ChannelMap = std::unordered_map<uintptr_t, Proxies>;
        using ReferenceMap = std::unordered_map<uintptr_t, std::list< RecoverySet > >;
        using Stubs = std::unordered_map<uint32_t, ProxyStub::UnknownStub*>;
        using Factories = std::unordered_map<uint32_t, IMetadata*>;

    public:
        Administrator(Administrator&&) = delete;
        Administrator(const Administrator&) = delete;
        Administrator& operator=(Administrator&&) = delete;
        Administrator& operator=(const Administrator&) = delete;

        virtual ~Administrator();

        static Administrator& Instance();

    public:
        bool DelegatedReleases() const {
            return (_delegatedReleases);
        }
        void DelegatedReleases(const bool enabled) {
            _delegatedReleases = enabled;
        }
        // void action(const Client& client)
        template<typename ACTION>
        void Visit(ACTION&& action) const {
            _adminLock.Lock();

            for (const auto& entry : _channelProxyMap) {
                if (entry.second.empty() == false) {
                    action(entry.second);
                }
            }

            if (_danglingProxies.empty() == false) {
                action(_danglingProxies);
            }
            _adminLock.Unlock();
        }
        template <typename ACTUALINTERFACE, typename PROXY, typename STUB>
        void Announce()
        {
            _adminLock.Lock();

#ifdef __DEBUG__
            if (_stubs.find(ACTUALINTERFACE::ID) != _stubs.end()) {
                TRACE_L1("Interface (stub) %d, gets registered multiple times !!!", ACTUALINTERFACE::ID);
            }
            else if (_proxy.find(ACTUALINTERFACE::ID) != _proxy.end()) {
                TRACE_L1("Interface (proxy) %d, gets registered multiple times !!!", ACTUALINTERFACE::ID);
            }
#endif
            _stubs.insert(std::pair<uint32_t, ProxyStub::UnknownStub*>(ACTUALINTERFACE::ID, new STUB()));
            _proxy.insert(std::pair<uint32_t, IMetadata*>(ACTUALINTERFACE::ID, new ProxyType<PROXY>()));

            _adminLock.Unlock();
        }

        template <typename ACTUALINTERFACE>
        void Recall()
        {
            _adminLock.Lock();

            Stubs::iterator stub(_stubs.find(ACTUALINTERFACE::ID));
            if (stub != _stubs.end()) {
PUSH_WARNING(DISABLE_WARNING_DELETE_INCOMPLETE)
                delete stub->second;
POP_WARNING()
                _stubs.erase(ACTUALINTERFACE::ID);
            } else {
                TRACE_L1("Failed to find a Stub for %d.", ACTUALINTERFACE::ID);
            }

            Factories::iterator proxy(_proxy.find(ACTUALINTERFACE::ID));
            if (proxy != _proxy.end()) {
                delete proxy->second;
                _proxy.erase(ACTUALINTERFACE::ID);
            } else {
                TRACE_L1("Failed to find a Proxy for %d.", ACTUALINTERFACE::ID);
            }

            _adminLock.Unlock();
        }

        Core::ProxyType<InvokeMessage> Message()
        {
            return (_factory.Element());
        }

        void DeleteChannel(const Core::ProxyType<Core::IPCChannel>& channel, Proxies& pendingProxies);

        template <typename ACTUALINTERFACE>
        ACTUALINTERFACE* ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl)
        {
            ACTUALINTERFACE* result = nullptr;
            ProxyFind(channel, impl, ACTUALINTERFACE::ID, result);
            return (result);
        }
        ProxyStub::UnknownProxy* ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const uint32_t id, void*& interface);

        template <typename ACTUALINTERFACE>
        ProxyStub::UnknownProxy* ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const bool outbound, ACTUALINTERFACE*& base)
        {
            void* proxyInterface;
            ProxyStub::UnknownProxy* result = ProxyInstance(channel, impl, outbound, ACTUALINTERFACE::ID, proxyInterface);
            base = reinterpret_cast<ACTUALINTERFACE*>(proxyInterface);
            return (result);
        }
        ProxyStub::UnknownProxy* ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const bool outbound, const uint32_t id, void*& interface);

        bool IsValid(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const uint32_t id) const;

        // ----------------------------------------------------------------------------------------------------
        // Methods for the Proxy Environment
        // ----------------------------------------------------------------------------------------------------
        void AddRef(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t interfaceId);
        void Release(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t interfaceId, const uint32_t dropCount);

        // ----------------------------------------------------------------------------------------------------
        // Methods for the Stub Environment
        // ----------------------------------------------------------------------------------------------------
        void Release(ProxyStub::UnknownProxy* proxy, Data::Output& response);
        void Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message);

        // ----------------------------------------------------------------------------------------------------
        // Methods for the Administration
        // ----------------------------------------------------------------------------------------------------
        // Stub method for entries that the Stub returns to the callee
        template <typename ACTUALINTERFACE>
        bool RegisterInterface(const Core::ProxyType<Core::IPCChannel>& channel, ACTUALINTERFACE* reference)
        {
            return (RegisterInterface(channel, reference, ACTUALINTERFACE::ID));
        }
        bool RegisterInterface(const Core::ProxyType<Core::IPCChannel>& channel, const void* source, const uint32_t id)
        {
            bool result = false;

            Core::IUnknown* converted = Convert(const_cast<void*>(source), id);

            if (converted != nullptr) {
                _adminLock.Lock();
                if (channel.IsValid() == true) {
                    RegisterUnknown(channel, converted, id);
                    result = true;
                }
                _adminLock.Unlock();
            }
            else {
                TRACE_L1("Failed to find a Stub for interface 0x%08x!", id);
            }

            return (result);
        }
        void UnregisterInterface(const Core::ProxyType<Core::IPCChannel>& channel, const Core::IUnknown* source, const uint32_t interfaceId, const uint32_t dropCount)
        {
            _adminLock.Lock();

            if (channel.IsValid() == true) {
                UnregisterUnknown(channel, source, interfaceId, dropCount);
            }

            _adminLock.Unlock();
        }
        bool UnregisterUnknownProxy(const ProxyStub::UnknownProxy& proxy);

   private:
        // ----------------------------------------------------------------------------------------------------
        // Methods for the Stub Environment
        // ----------------------------------------------------------------------------------------------------
        Core::IUnknown* Convert(void* rawImplementation, const uint32_t id);
        const Core::IUnknown* Convert(void* rawImplementation, const uint32_t id) const;
        void RegisterUnknown(const Core::ProxyType<Core::IPCChannel>& channel, Core::IUnknown* source, const uint32_t id);
        void UnregisterUnknown(const Core::ProxyType<Core::IPCChannel>& channel, const Core::IUnknown* source, const uint32_t interfaceId, const uint32_t dropCount);

    private:
        // Seems like we have enough information, open up the Process communcication Channel.
        mutable Core::CriticalSection _adminLock;
        Stubs _stubs;
        Factories _proxy;
        Core::ProxyPoolType<InvokeMessage> _factory;
        ChannelMap _channelProxyMap;
        ReferenceMap _channelReferenceMap;
        Proxies _danglingProxies;

        // Delegated release, if enabled, will release references held by connections 
        // that close but still have references on objects in this process space.
        // Whenever an interface is exposed to the otherside over the channel, it is
        // recorded with this channel, if we received an Addref from the other side
        // to keep the object here alive.
        // If the connection dies and the object is still in this recorded set, it 
        // means it will never receive the Release for it. Effectively that will mean 
        // we have a leak on this object, since the Addref was done on behalf of the 
        // other side of the link but the Release will never follow...... Ooops...
        // Since we have this administartion and we know the channels is closed, we 
        // could potentially do the Release for the otherside since we knwo the channel
        // went done. This is a very welcome feature but it was only introduced in
        // Thunder R3.X. 
        // One of the selling reasons of having garbage collected languages is that you 
        // do not need to be aware of the lifetime of object. No need to pair Addrefs 
        // with Releases and Mallocs with Free's or new's with deletes. 
        // C++ languages however expect you to know the lifetime and we know that this
        // is one of the most made mistakes. 
        // As we value stability of software and hate debuging crashes if we move to
        // a new release this feature of automatic cleanup can backfire. If the software
        // (read plugins) are poorly designed/coded and have an incorrect lifetime 
        // management or the plugin is *not* coded/designed for non-happy day scenarios,
        // these plugins might have been running succesfully be the lack of this feature 
        // (at the cost of a memory leak but that is not so visible). Turning on this 
        // feature might correctly cleanup object that in the past where leaking and will
        // now lead to segmentation faults.
        // Typically the first suspect is than the *new* release. As we have seen very 
        // creative plugin implementations and from a Thunder team we have a limit amount
        // of resources to investigate all these creative ways of implementing the plugin
        // we allow for a flag to turn this *very* usefull feature off. 
        // Whenever, after an upgrade to this new release, unexpected crashes are reported
        // with an out-of-process plugins, we will ask the plugin developper to retest but 
        // than with this feature off (flag in the config, no rebuild required). If the
        // crash can than nolonger be reproduced we suggest the plugin developper to 
        // check the lifetime handling on all object in his plugin (AddRef/Releases) and
        // check the code for cleaning up in case of unexpected out-of-process plugin part
        // shutdown.
        // If the same crash continues, please reach out to the Thunder team for assitance!  
        bool _delegatedReleases;
    };

    class EXTERNAL Job : public Core::IDispatch {
    public:
        Job()
            : _message()
            , _channel()
        {
        }
        Job(Core::IPCChannel& channel, const Core::ProxyType<Core::IIPC>& message)
            : _message(message)
            , _channel(channel)
        {
        }
        Job(Job&& move) noexcept
            : _message(std::move(move._message))
            , _channel(std::move(move._channel))
        {
        }
        Job(const Job& copy)
            : _message(copy._message)
            , _channel(copy._channel)
        {
        }
        ~Job() override = default;

        Job& operator=(Job&& rhs) noexcept {
            _message = std::move(rhs._message);
            _channel = std::move(rhs._channel);

            return (*this);
        }
        Job& operator=(const Job& rhs) {
            _message = rhs._message;
            _channel = rhs._channel;

            return (*this);
        }

    public:
        static Core::ProxyType<Job> Instance()
        {
            return (_factory.Element());
        }
        void Clear()
        {
            _message.Release();
            _channel.Release();
        }
        void Set(Core::IPCChannel& channel, const Core::ProxyType<Core::IIPC>& message)
        {
            _message = message;
            _channel = Core::ProxyType<Core::IPCChannel>(channel);
        }
        string Identifier() const override {
            string identifier;
            Core::ProxyType<InvokeMessage> message(_message);
            if (message.IsValid() == false) {
                identifier = _T("{ \"type\": \"COMRPC\" }");
            }
            else {
                identifier = Core::Format(_T("{ \"type\": \"COMRPC\", \"interface\": %d, \"method\": %d }"), message->Parameters().InterfaceId(), message->Parameters().MethodId());
            }
            return (identifier);
        }
        void Dispatch() override
        {
            ASSERT(_message->Label() == InvokeMessage::Id());

            Invoke(_channel, _message);
        }

        static void Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<Core::IIPC>& data)
        {
            Core::ProxyType<InvokeMessage> message(data);
            ASSERT(message.IsValid() == true);
            if (message->Parameters().IsValid() == false) {
                SYSLOG(Logging::Error, (_T("COMRPC Announce message incorrectly formatted!")));
            }
            else {
                _administrator.Invoke(channel, message);
                channel->ReportResponse(data);
            }
        }

    private:
        Core::ProxyType<Core::IIPC> _message;
        Core::ProxyType<Core::IPCChannel> _channel;

        static Core::ProxyPoolType<Job> _factory;
        static Administrator& _administrator;
    };

    struct EXTERNAL IIPCServer : public Core::IIPCServer {
        ~IIPCServer() override = default;

        virtual void Submit(const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual void Revoke(const Core::ProxyType<Core::IDispatch>& job) = 0;
    };

    class EXTERNAL InvokeServer : public IIPCServer {
    public:
        InvokeServer(InvokeServer&&) = delete;
        InvokeServer(const InvokeServer&) = delete;
        InvokeServer& operator=(InvokeServer&&) = delete;
        InvokeServer& operator=(const InvokeServer&) = delete;

        explicit InvokeServer(Core::IWorkerPool* workers)
            : _threadPoolEngine(*workers)
        {
            ASSERT(workers != nullptr);
        }
        ~InvokeServer() override = default;

    public:
        void Submit(const Core::ProxyType<Core::IDispatch>& job) override {
            _threadPoolEngine.Submit(job);
        }
        void Revoke(const Core::ProxyType<Core::IDispatch>& job) override {
            _threadPoolEngine.Revoke(job);
        }

    private:
        void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message) override
        {
            Core::ProxyType<Job> job(Job::Instance());

            job->Set(source, message);
            _threadPoolEngine.Submit(Core::ProxyType<Core::IDispatch>(job));
        }

    private:
        Core::IWorkerPool& _threadPoolEngine;
    };

    template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
    class InvokeServerType : public IIPCServer {
    private:
        class Dispatcher : public Core::ThreadPool::IDispatcher {
        public:
            Dispatcher(Dispatcher&&) = delete;
            Dispatcher(const Dispatcher&) = delete;
            Dispatcher& operator=(Dispatcher&&) = delete;
            Dispatcher& operator=(const Dispatcher&) = delete;

            Dispatcher() = default;
            ~Dispatcher() override = default;

        private:
            void Initialize() override {
            }
            void Deinitialize() override {
            }
            void Dispatch(Core::IDispatch* job) override {
                job->Dispatch();
            }
        };

    public:
        InvokeServerType(InvokeServerType<THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS>&&) = delete;
        InvokeServerType(const InvokeServerType<THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS>&) = delete;
        InvokeServerType<THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS>& operator = (InvokeServerType<THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS>&&) = delete;
        InvokeServerType<THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS>& operator = (const InvokeServerType<THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS>&) = delete;

        InvokeServerType()
            : _dispatcher()
            , _threadPoolEngine(THREADPOOLCOUNT,STACKSIZE,MESSAGESLOTS, &_dispatcher, nullptr, nullptr, nullptr)
        {
            _threadPoolEngine.Run();
        }
        ~InvokeServerType() override
        {
            _threadPoolEngine.Stop();
        }
        void Submit(const Core::ProxyType<Core::IDispatch>& job) override {
            _threadPoolEngine.Submit(job, Core::infinite);
        }
        void Revoke(const Core::ProxyType<Core::IDispatch>& job) override {
            _threadPoolEngine.Revoke(job, Core::infinite);
        }
        void Run() {
            _threadPoolEngine.Run();
        }

        void Stop() {
             _threadPoolEngine.Stop();
        }

    private:
        void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message) override
        {
            if (_threadPoolEngine.Pending() >= ((MESSAGESLOTS * 80) / 100)) {
                TRACE_L1("_threadPoolEngine.Pending() == %d", _threadPoolEngine.Pending());
            }

            ASSERT(message->Label() == InvokeMessage::Id());

            Core::ProxyType<RPC::Job> job(Job::Instance());

            job->Set(source, message);
            _threadPoolEngine.Submit(Core::ProxyType<Core::IDispatch>(job), Core::infinite);
        }

    private:
        Dispatcher _dispatcher;
        Core::ThreadPool _threadPoolEngine;
    };
}

} // namespace RPC

#endif // __COM_ADMINISTRATOR_H
