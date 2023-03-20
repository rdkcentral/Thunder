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

#include "Communicator.h"

#include <limits>
#include <memory>

#ifdef PROCESSCONTAINERS_ENABLED
#include "ProcessInfo.h"
#endif

namespace WPEFramework {
namespace RPC {

    class ProcessShutdown;

    static Core::ProxyPoolType<RPC::AnnounceMessage> AnnounceMessageFactory(2);

    class DynamicLoaderPaths {
    private:
        static constexpr TCHAR LoaderConfig[] = _T("/etc/ld.so.conf");

    public:
        DynamicLoaderPaths(const DynamicLoaderPaths&) = delete;
        DynamicLoaderPaths& operator= (const DynamicLoaderPaths&) = delete;

        DynamicLoaderPaths() 
            : _downloadLists()
        {
            ReadList(LoaderConfig, _downloadLists);
            _downloadLists.emplace_back(_T("/usr/lib/"));
            _downloadLists.emplace_back(_T("/lib/"));
        }
        ~DynamicLoaderPaths() = default;

    public:
        const std::vector<string>& Paths() const
        {
            return (_downloadLists);
        }

    private:
        void ReadList(const string& filename, std::vector<string>& entries)
        {

            string filter(Core::File::FileNameExtended(filename));

            if (filter.find('*') != string::npos) {
                Core::Directory dir(Core::File::PathName(filename).c_str(), filter.c_str());

                while (dir.Next() == true) {
                    ReadList(dir.Current(), entries);
                }
            }
            else {
                // Parse it line, by line...
                Core::DataElementFile bufferFile(filename, Core::File::USER_READ);
                Core::TextReader reader(bufferFile);

                while (reader.EndOfText() == false) {
                    Core::TextFragment line(reader.ReadLine());

                    // Drop the spaces in the begining...
                    line.TrimBegin(" \t");

                    if ((line.IsEmpty() == false) && (line[0] != '#')) {
                        Core::TextSegmentIterator segments(line, true, " \t");

                        if (segments.Next() == true) {

                            // Looks like we have a word, see what the word is...
                            if ((segments.Current() == _T("include")) && (segments.Next() == true)) {
                                // Oke, dive into this entry...
                                ReadList(segments.Remainder().Text(), entries);
                            }
                            else {
                                entries.emplace_back(line.Text());
                            }
                        }
                        else {
                            entries.emplace_back(line.Text());
                        }
                    }
                }
            }
        }

    private:
        std::vector<string> _downloadLists;
    };

    /* static */ constexpr TCHAR DynamicLoaderPaths::LoaderConfig[];
    static DynamicLoaderPaths& _LoaderPaths = Core::SingletonType<DynamicLoaderPaths>::Instance();

    /* static */ Core::CriticalSection Communicator::Process::_ldLibLock ;

    class ProcessShutdown : public Core::Thread {
    private:
        static constexpr uint32_t DestructionStackSize = 64 * 1024;

        using DestructorMap = std::unordered_map<uint32_t, Communicator::MonitorableProcess*>;

    public:
        ProcessShutdown(ProcessShutdown&&) = delete;
        ProcessShutdown(const ProcessShutdown&) = delete;
        ProcessShutdown& operator=(const ProcessShutdown&) = delete;

        ProcessShutdown()
            : Core::Thread(ProcessShutdown::DestructionStackSize, "COMRPCTerminator")
            , _adminLock()
            , _destructors() {
        }
        ~ProcessShutdown() = default;

    public:
        void ForceDestruct(const uint32_t id) {

            Communicator::MonitorableProcess* handler = nullptr;

            _adminLock.Lock();

            DestructorMap::iterator index(_destructors.find(id));

            if (index != _destructors.end()) {
                handler = index->second;
                _destructors.erase(index);
            }

            _adminLock.Unlock();

            if (handler != nullptr) {

                // Forcefully kill this ID.
                handler->Destruct();
                handler->Release();
            }
        }
        void Destruct(const uint32_t id, Communicator::MonitorableProcess& entry)
        {
            _adminLock.Lock();

            if (_destructors.find(id) == _destructors.end()) {
                entry.AddRef();
                _destructors.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(&entry));

                Run();
            }

            _adminLock.Unlock();
        }
        void WaitForCompletion(Communicator::RemoteConnectionMap& parent) {

            Communicator::MonitorableProcess* handler = nullptr;

            _adminLock.Lock();

            DestructorMap::iterator index(_destructors.begin());

            while (index != _destructors.end() ) {
                if (index->second->operator==(parent) == true) {
                    // Forcefully kill this ID.
                    index->second->Destruct();
                    index = _destructors.erase(index);
                }
                else {
                    index++;
                }
            }

            _adminLock.Unlock();
        }

    private:
        uint32_t Worker() override {
            uint32_t delay(Core::infinite);
            uint64_t now(Core::Time::Now().Ticks());
            uint64_t nextSlot(~0);

            _adminLock.Lock();

            DestructorMap::iterator index = _destructors.begin();

            while (index != _destructors.end()) {
                if (index->second->Destruct(now) == false) {
                    const uint64_t& newTime = index->second->Time();
                    if (nextSlot > newTime) {
                        nextSlot = newTime;
                    }
                    index++;
                }
                else {
                    index->second->Release();
                    index = _destructors.erase(index);
                }
            }

            _adminLock.Unlock();

            if (nextSlot != static_cast<uint64_t>(~0)) {
                delay = (now > nextSlot ? 0 : static_cast<uint32_t>((nextSlot - now) / Core::Time::TicksPerMillisecond));
            }

            if (delay != 0) {
                Core::Thread::Block();
            }

            return (delay);
        }

    private:
        Core::CriticalSection _adminLock;
        DestructorMap _destructors;
    };

    static ProcessShutdown& g_destructor = Core::SingletonType<ProcessShutdown>::Instance();

    /* static */ std::atomic<uint32_t> Communicator::RemoteConnection::_sequenceId(1);

    static void LoadProxyStubs(const string& pathName)
    {
        static std::list<Core::Library> processProxyStubs;

        Core::TextSegmentIterator places(Core::TextFragment(pathName), false, '|');

        while (places.Next() == true) {
            Core::Directory index(places.Current().Text().c_str(), _T("*.so"));

            while (index.Next() == true) {
                // Check if this ProxySTub file is already loaded in this process space..
                std::list<Core::Library>::const_iterator loop(processProxyStubs.begin());
                while ((loop != processProxyStubs.end()) && (loop->Name() != index.Current())) {
                    loop++;
                }

                if (loop == processProxyStubs.end()) {
                    Core::Library library(index.Current().c_str());

                    if (library.IsLoaded() == true) {
                        processProxyStubs.push_back(library);
                    }
                }
            }
        }
    }

    /* virtual */ uint32_t Communicator::RemoteConnection::Id() const
    {
        return (_id);
    }

    /* virtual */ void* Communicator::RemoteConnection::Acquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
    {
        void* result(nullptr);

        if (_channel.IsValid() == true) {
            Core::ProxyType<RPC::AnnounceMessage> message(AnnounceMessageFactory.Element());

            TRACE_L1("Aquiring object through RPC: %s, 0x%04X [%d]", className.c_str(), interfaceId, RemoteId());

            message->Parameters().Set(_id, className, interfaceId, version);

            uint32_t feedback = _channel->Invoke(message, waitTime);

            if (feedback == Core::ERROR_NONE) {
                Core::instance_id implementation = message->Response().Implementation();

                if (implementation) {
                    // From what is returned, we need to create a proxy
                    RPC::Administrator::Instance().ProxyInstance(Core::ProxyType<Core::IPCChannel>(_channel), implementation, true, interfaceId, result);
                }
            }
        }

        return (result);
    }

    /* virtual */ void Communicator::RemoteConnection::Terminate()
    {
        Close();
    }

    /* virtual */ uint32_t Communicator::RemoteConnection::RemoteId() const
    {
        return (_remoteId);
    }

    /* virtual */ void Communicator::LocalProcess::Terminate()
    {
        // Do not yet call the close on the connection, the otherside might close down decently and release all opened interfaces..
        // Just submit our selves for destruction !!!!

        // Time to shoot the application, it will trigger a close by definition of the channel, if it is still standing..
        g_destructor.Destruct(Id(), *this);
    }

    uint32_t Communicator::LocalProcess::RemoteId() const
    {
        return (_id);
    }

    void Communicator::LocalProcess::PostMortem() /* override */
    {
        if (_id != 0) {
            Core::ProcessInfo process(_id);
            process.Dump();
        }
    }


#ifdef PROCESSCONTAINERS_ENABLED

    void Communicator::ContainerProcess::Terminate() /* override */
    {
        ASSERT(_container != nullptr);
        g_destructor.Destruct(Id(), *this);
    }

    void Communicator::ContainerProcess::PostMortem() /* override */
    {
        Core::process_t pid;
        if ( (_container != nullptr) && ((pid = static_cast<Core::process_t>(_container->Pid())) != 0) ) {
            Core::ProcessInfo process(pid);
            process.Dump();
        }
    }

#endif
    // Definitions of static members
    uint8_t Communicator::_softKillCheckWaitTime = 10;
    uint8_t Communicator::_hardKillCheckWaitTime = 4;;

    PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST);

    Communicator::Communicator(const Core::NodeId& node, const string& proxyStubPath)
        : _connectionMap(*this)
        , _ipcServer(node, _connectionMap, proxyStubPath) {
        if (proxyStubPath.empty() == false) {
            RPC::LoadProxyStubs(proxyStubPath);
        }
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
        _ipcServer.CreateFactory<InvokeMessage>(3);
    }

    Communicator::Communicator(
        const Core::NodeId& node,
        const string& proxyStubPath,
        const Core::ProxyType<Core::IIPCServer>& handler)
        : _connectionMap(*this)
        , _ipcServer(node, _connectionMap, proxyStubPath, handler) {
        if (proxyStubPath.empty() == false) {
            RPC::LoadProxyStubs(proxyStubPath);
        }
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
        _ipcServer.CreateFactory<InvokeMessage>(3);
    }

    /* virtual */ Communicator::~Communicator()
    {
        // Make sure any closed channel is cleared before we start validating the end result :-)
        _ipcServer.Cleanup();

        // Close all communication paths...
        _ipcServer.Close(Core::infinite);

        // Warn but we need to clos up existing connections..
        _connectionMap.Destroy();

        // Now there are no more connections pending. Remove my pending IMonitorable::ICallback settings.
        g_destructor.WaitForCompletion(_connectionMap);
    }

    void Communicator::Destroy(const uint32_t id)
    {
        // This is a forceull call, blocking, to kill that specific connection
        g_destructor.ForceDestruct(id);
    }

    void Communicator::LoadProxyStubs(const string& pathName)
    {
        RPC::LoadProxyStubs(pathName);
    }

    const std::vector<string>& Communicator::Process::DynamicLoaderPaths() const
    {
        return _LoaderPaths.Paths();
    }
    CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode)
        : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
        , _handler(this)
        , _connectionId(~0)
    {
        CreateFactory<RPC::AnnounceMessage>(1);
        CreateFactory<RPC::InvokeMessage>(2);

        Register(RPC::InvokeMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<InvokeHandlerImplementation>::Create()));
        Register(RPC::AnnounceMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<AnnounceHandlerImplementation>::Create(this)));
    }

    CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode,
        const Core::ProxyType<Core::IIPCServer>& handler)
        : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
        , _handler(this)
        , _connectionId(~0)
    {
        CreateFactory<RPC::AnnounceMessage>(1);
        CreateFactory<RPC::InvokeMessage>(2);

        BaseClass::Register(RPC::InvokeMessage::Id(), handler);
        BaseClass::Register(RPC::AnnounceMessage::Id(), handler);
    }
POP_WARNING()

    CommunicatorClient::~CommunicatorClient()
    {
        BaseClass::Close(Core::infinite);

        BaseClass::Unregister(RPC::InvokeMessage::Id());
        BaseClass::Unregister(RPC::AnnounceMessage::Id());

        DestroyFactory<RPC::InvokeMessage>();
        DestroyFactory<RPC::AnnounceMessage>();
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        //do not set announce parameters, we do not know what side will offer the interface
        _announceMessage->Parameters().Set(Core::ProcessInfo().Id());

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), className, interfaceId, version);

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation, const uint32_t exchangeId)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        Core::instance_id impl = instance_cast<void*>(implementation);

        _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), interfaceId, impl, exchangeId);

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Close(const uint32_t waitTime)
    {
        return (BaseClass::Close(waitTime));
    }

    void CommunicatorClient::StateChange() /* override */ {
        BaseClass::StateChange();

        if (BaseClass::Source().IsOpen()) {
            TRACE_L1("Invoking the Announce message to the server. %d", __LINE__);
            uint32_t result = Invoke<RPC::AnnounceMessage>(_announceMessage, this);

            if (result != Core::ERROR_NONE) {
                TRACE_L1("Error during invoke of AnnounceMessage: %d", result);
            } else {
                RPC::Data::Init& setupFrame(_announceMessage->Parameters());

                if (setupFrame.IsRequested() == true) {
                    Core::ProxyType<Core::IPCChannel> refChannel(*this);

                    ASSERT(refChannel.IsValid());

                    // Register the interface we are passing to the otherside:
                    RPC::Administrator::Instance().RegisterInterface(refChannel, reinterpret_cast<void*>(setupFrame.Implementation()), setupFrame.InterfaceId());
                }
            }
        } else {
            TRACE_L1("Connection to the server is down");
        }
    }

    /* virtual */ void CommunicatorClient::Dispatch(Core::IIPC& element)
    {
        // Message delivered and responded on....
        RPC::AnnounceMessage* announceMessage = static_cast<RPC::AnnounceMessage*>(&element);

        ASSERT(dynamic_cast<RPC::AnnounceMessage*>(&element) != nullptr);

        if (announceMessage->Response().IsSet() == true) {
            string jsonMessagingCategories(announceMessage->Response().MessagingCategories());

#if defined(WARNING_REPORTING_ENABLED)
            string jsonDefaultWarningCategories(announceMessage->Response().WarningReportingCategories());
            if(jsonDefaultWarningCategories.empty() == false){
                WarningReporting::WarningReportingUnit::Instance().Defaults(jsonDefaultWarningCategories);
            }
#endif
            _connectionId = announceMessage->Response().SequenceNumber();

            string proxyStubPath(announceMessage->Response().ProxyStubPath());
            if (proxyStubPath.empty() == false) {
                // Also load the ProxyStubs before we do anything else
                RPC::LoadProxyStubs(proxyStubPath);
            }
        }

        // Set event so WaitForCompletion() can continue.
        _announceEvent.SetEvent();
    }

    //We may eliminate the following statement when switched to C++17 compiler
    constexpr uint32_t RPC::ProcessShutdown::DestructionStackSize;

}
}
