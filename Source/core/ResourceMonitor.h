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

#include "Module.h"
#include "Portability.h"
#include "Singleton.h"
#include "Thread.h"
#include "Trace.h"
#include "Timer.h"
#include "NodeId.h"

#ifdef __WINDOWS__
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef __UNIX__
#define SOCKET signed int
#define SOCKET_ERROR static_cast<signed int>(-1)
#define INVALID_SOCKET static_cast<SOCKET>(-1)
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace Thunder {

namespace Core {

    struct EXTERNAL IResource {
        virtual ~IResource() = default;

        using handle = signed int;
        static constexpr handle INVALID = -1;

        virtual handle Descriptor() const = 0;
        virtual uint16_t Events() = 0;
        virtual void Handle(const uint16_t events) = 0;
    };

    template <typename RESOURCE, typename WATCHDOG, const uint32_t STACK_SIZE, const uint8_t RESOURCE_SLOTS>
    class ResourceMonitorType {
    private:
        using Parent = ResourceMonitorType<RESOURCE, WATCHDOG, STACK_SIZE, RESOURCE_SLOTS>;
        using Resources = std::vector<RESOURCE*>;

        class MonitorWorker : public Core::Thread {
        public:
            MonitorWorker() = delete;
            MonitorWorker(MonitorWorker&&) = delete;
            MonitorWorker(const MonitorWorker&) = delete;
            MonitorWorker& operator=(MonitorWorker&&) = delete;
            MonitorWorker& operator=(const MonitorWorker&) = delete;

            MonitorWorker(Parent& parent)
                : Core::Thread(STACK_SIZE == 0 ? Thread::DefaultStackSize() : STACK_SIZE, parent.Name())
                , _parent(parent)
            {
                Thread::Init();
            }
            ~MonitorWorker() override
            {
                Stop();
                _parent.Break();
                Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
            }

        public:
            #ifdef __LINUX__
            uint32_t Initialize() override
            {
                return ((Thread::Initialize() == Core::ERROR_NONE) && (_parent.Initialize() == Core::ERROR_NONE) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
            }
            #endif
            uint32_t Worker() override
            {
                return (_parent.Worker());
            }

        private:
            Parent& _parent;
        };

    public:
        struct Metadata {
            Core::IResource::handle descriptor;
            uint16_t monitor;
            uint16_t events;
            const char* classname;
            char filename[128];
        };

    public:
        ResourceMonitorType(ResourceMonitorType&&) = delete;
        ResourceMonitorType(const ResourceMonitorType&) = delete;
        ResourceMonitorType& operator=(ResourceMonitorType&&) = delete;
        ResourceMonitorType& operator=(const ResourceMonitorType&) = delete;

        ResourceMonitorType()
            : _monitor(nullptr)
            , _adminLock()
            , _resources()
            , _monitorRuns(0)
            , _name(_T("Monitor::") + ClassNameOnly(typeid(RESOURCE).name()).Text())
            , _watchDog(1024 * 512, _name.c_str())
            #ifdef __WINDOWS__
            , _action(WSACreateEvent())
            #else
            , _descriptorArrayLength(RESOURCE_SLOTS)
            , _descriptorArray(static_cast<struct pollfd*>(::malloc(sizeof(::pollfd) * (RESOURCE_SLOTS + 1))))
            , _signalDescriptor(-1)
            #endif
        {
        }

        ~ResourceMonitorType()
        {
            #ifdef __DEBUG__
            // All resources should be gone !!!
            for (const auto& resource : _resources) {
                TRACE_L1("Resource name: %s", typeid(resource).name());
                ASSERT(resource == nullptr);
            }
            #endif

            if (_monitor != nullptr) {

                _monitor->Stop();
                Break();
                _monitor->Wait(Thread::STOPPED, Core::infinite);

                _adminLock.Lock();

                _resources.clear();

                _adminLock.Unlock();

                delete _monitor;
            }

            #ifdef __LINUX__
            ::free(_descriptorArray);
            if (_signalDescriptor != -1) {
                ::close(_signalDescriptor);
            }
            #endif
            #ifdef __WINDOWS__
            WSACloseEvent(_action);
            #endif
        }

    public:
        const TCHAR* Name() const
        {
            return (_name.c_str());
        }
        uint32_t Runs() const
        {
            return (_monitorRuns);
        }
        thread_id Id() const
        {
            return (_monitor != nullptr ? _monitor->Id() : 0);
        }
        uint32_t Count() const 
        {
            return (static_cast<uint32_t>(_resources.size()));
        }
        bool Info (const uint32_t position, Metadata& info) const
        {
            uint32_t count = position;

            _adminLock.Lock();

            typename Resources::const_iterator index(_resources.cbegin());
            while ( (count != 0) && (index != _resources.cend()) ) { count--; index++; }

            bool found = (index != _resources.cend());

            if (found == true) {
                info.descriptor = (*index)->Descriptor();
                info.classname  = typeid(*(*index)).name();

                #ifdef __LINUX__
                info.monitor = _descriptorArray[position + 1].events;
                info.events  = _descriptorArray[position + 1].revents;

                char procfn[64];
                snprintf(procfn, sizeof(procfn), "/proc/self/fd/%d", info.descriptor);

                size_t len = readlink(procfn, info.filename, sizeof(info.filename) - 1);
                info.filename[len] = '\0';
                #endif
                #ifdef __WINDOWS__
                info.monitor = 0;
                info.events  = 0;
                info.filename[0] = '\0';
                #endif
            }

            _adminLock.Unlock();

            return (found);
        }
        void Register(RESOURCE& resource)
        {
            _adminLock.Lock();

            // Make sure this entry is only registered once !!!
            if (std::find(_resources.begin(), _resources.end(), &resource) == _resources.end()) {
                _resources.push_back(&resource);
            }

            if (_resources.size() == 1) {
                if (_monitor == nullptr) {
                    _monitor = new MonitorWorker(*this);
                    _monitorRuns = 0;
                    // Wait till we are at least initialized
                    _monitor->Wait(Thread::BLOCKED | Thread::STOPPED);
                }

                _monitor->Run();
            } else {
                Break();
            }

            _adminLock.Unlock();
        }
        void Unregister(RESOURCE& resource)
        {
            _adminLock.Lock();

            // Make sure this entry does not exist, only register resources once !!!
            typename Resources::iterator index(std::find(_resources.begin(), _resources.end(), &resource));

            if (index != _resources.end()) {
                *index = nullptr;
                Break();
            }

            _adminLock.Unlock();
        }
        inline void Break()
        {
            ASSERT(_monitor != nullptr);

            #ifdef __APPLE__
            int data = 0;
            ::sendto(_signalDescriptor,
                    & data,
                sizeof(data), 0,
                static_cast<const NodeId&>(_signalNode),
                _signalNode.Size());
            #elif defined(__LINUX__)
            _monitor->Signal(SIGUSR2);
            #elif defined(__WINDOWS__)
            ::WSASetEvent(_action);
            #endif
        };

    private:
        IS_MEMBER_AVAILABLE(Arm, hasArm);

        template <typename TYPE=WATCHDOG>
        inline typename Core::TypeTraits::enable_if<hasArm<TYPE, void>::value, void>::type
        Arm()
        {
            _watchDog.Arm();
        }

        template <typename TYPE=WATCHDOG>
        inline typename Core::TypeTraits::enable_if<!hasArm<TYPE, void>::value, void>::type
        Arm()
        {
        }

        IS_MEMBER_AVAILABLE(Reset, hasReset);

        template <typename TYPE=WATCHDOG>
        inline typename Core::TypeTraits::enable_if<hasReset<TYPE, void>::value, void>::type
        Reset()
        {
            _watchDog.Reset();
        }

        template <typename TYPE=WATCHDOG>
        inline typename Core::TypeTraits::enable_if<!hasReset<TYPE, void>::value, void>::type
        Reset()
        {
        }

        bool SetNonBlocking(signed int socket)
        {
#ifdef __WINDOWS__
            unsigned long l_Value = 1;
            if (ioctlsocket(socket, FIONBIO, &l_Value) != 0) {
                TRACE_L1("Error on port socket NON_BLOCKING call. Error %d", ::WSAGetLastError());
            }
            else {
                return (true);
            }
#endif

#ifdef __POSIX__
            if (fcntl(socket, F_SETOWN, getpid()) == -1) {
                TRACE_L1("Setting Process ID failed. <%d>", errno);
            }
            else {
                int flags = fcntl(socket, F_GETFL, 0) | O_NONBLOCK;

                if (fcntl(socket, F_SETFL, flags) != 0) {
                    TRACE_L1("Error on port socket F_SETFL call. Error %d", errno);
                }
                else {
                    return (true);
                }
            }
#endif

            return (false);
        }

    public:
#ifdef __LINUX__
        uint32_t Initialize()
        {
            #ifdef __APPLE__

            if ((_signalDescriptor = ::socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
                TRACE_L1("Error on creating socket SOCKET. Error %d", errno);
            } else if (SetNonBlocking(_signalDescriptor) == false) {
                _signalDescriptor = -1;
                TRACE_L1("Error on etting socket to non blocking. Error %d", errno);
            } else {
                char fileNameTemplate[] = "/tmp/ResourceMonitor.XXXXXX";
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)
                char* file = mktemp(fileNameTemplate);
POP_WARNING()
                // Do we need to find something to bind to or is it pre-destined
                _signalNode = Core::NodeId(file);
                if (::bind(_signalDescriptor, static_cast<const NodeId&>(_signalNode), _signalNode.Size()) != 0) {
                    _signalDescriptor = -1;
                }
            }

            #else

            sigset_t sigset;

            /* Create a sigset of all the signals that we're interested in */
            int VARIABLE_IS_NOT_USED err = sigemptyset(&sigset);
            ASSERT(err == 0);
            err = sigaddset(&sigset, SIGUSR2);
            ASSERT(err == 0);

            /* We must block the signals in order for signalfd to receive them */
            err = pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
            assert(err == 0);

            /* Create the signalfd */
            _signalDescriptor = signalfd(-1, &sigset, SFD_CLOEXEC);

            #endif

            ASSERT(_signalDescriptor != -1);

            _descriptorArray[0].fd = _signalDescriptor;
            _descriptorArray[0].events = POLLIN;
            _descriptorArray[0].revents = 0;

            return (_signalDescriptor != -1 ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }

        uint32_t Worker()
        {
            uint32_t delay = 0;

            _monitorRuns++;

            // Add entries not in the Array before we start !!!
            _adminLock.Lock();

            // Do we have enough space to allocate all file descriptors ?
            if ((_resources.size() + 1) > _descriptorArrayLength) {
                _descriptorArrayLength = ((((_resources.size() + 1) / RESOURCE_SLOTS) + 1) * RESOURCE_SLOTS);

                ::free(_descriptorArray);

                // Resize the array to fit..
                _descriptorArray = static_cast<::pollfd*>(::malloc(sizeof(::pollfd) * _descriptorArrayLength));

                _descriptorArray[0].fd = _signalDescriptor;
                _descriptorArray[0].events = POLLIN;
                _descriptorArray[0].revents = 0;
            }

            int filledFileDescriptors = 1;
            typename Resources::iterator index = _resources.begin();

            // Fill in all entries required/updated..
            while (index != _resources.end()) {
                RESOURCE* entry = (*index);

                uint16_t events;

                if ((entry == nullptr) || ((events = entry->Events()) == 0)) {
                    index = _resources.erase(index);
                }
                else {
                    _descriptorArray[filledFileDescriptors].fd = entry->Descriptor();
                    _descriptorArray[filledFileDescriptors].events = events;
                    _descriptorArray[filledFileDescriptors].revents = 0;
                    filledFileDescriptors++;
                    index++;
                }
            }

            if (filledFileDescriptors > 1) {
                _adminLock.Unlock();

                int result = poll(_descriptorArray, filledFileDescriptors, -1);

                _adminLock.Lock();

                if (result == -1) {
                    TRACE_L1("poll failed with error <%d>", errno);
                }
                else if (_descriptorArray[0].revents & POLLIN) {
                    #ifdef __APPLE__
                    int info;
                    #else
                    /* We have a valid signal, read the info from the fd */
                    struct signalfd_siginfo info;
                    #endif
                    uint32_t VARIABLE_IS_NOT_USED bytes = read(_signalDescriptor, &info, sizeof(info));
                    ASSERT(bytes == sizeof(info) || bytes == 0);
                }

                // We are only interested in the filedescriptors that have a corresponding client.
                // We also know that once a file descriptor is not found, we handled them all...
                int fd_index = 1;
                index = _resources.begin();
                size_t capacity = _resources.capacity();

                while (fd_index < filledFileDescriptors) {
                    ASSERT(index != _resources.end());

                    RESOURCE* entry = (*index);

                    // The entry might have been removed from observing in the mean time...
                    if (entry != nullptr) {

                        // As we are the only ones that take out the resources from the list, we
                        // always make sure that the iterator is on the right spot/filedescriptor.
                        ASSERT(entry->Descriptor() == _descriptorArray[fd_index].fd);

                        uint16_t flagsSet = _descriptorArray[fd_index].revents;

                        Arm();

                        // Event if the flagsSet == 0, call handle, maybe a break was issued by this RESOURCE..
                        entry->Handle(flagsSet);

                        Reset();
                    }

                    fd_index++;

                    if (capacity == _resources.capacity()) {
                        index++;
                    }
                    else {
                        // vector's capacity was changed, which means its memory was reallocated and we have to adjust the index
                        index = _resources.begin();
                        int current = fd_index;

                        while ((index != _resources.end()) && (current > 1)) {
                            index++;
                            current--;
                        }
                        capacity = _resources.capacity();
                    }
                }
            }
            else {
                _monitor->Block();
                delay = Core::infinite;
            }

            _adminLock.Unlock();

            return (delay);
        }
        #endif

        #ifdef __WINDOWS__
        uint32_t Worker()
        {
            uint32_t delay = 0;
            typename Resources::iterator index;

            _monitorRuns++;

            _adminLock.Lock();

            // Now iterate over the sockets and determine their states..
            index = _resources.begin();

            while (index != _resources.end()) {

                RESOURCE* entry = (*index);

                uint16_t events;

                if ((entry == nullptr) || ((events = entry->Events()) == 0)) {
                    index = _resources.erase(index);
                } else {
                    if ((events & 0x8000) != 0) {
                        ::WSAEventSelect((*index)->Descriptor(), _action, (events & 0x7FFF));
                    }
                    index++;
                }
            }

            if (_resources.size() > 0) {

                size_t count = _resources.size();

                _adminLock.Unlock();

                WaitForSingleObject(_action, Core::infinite);

                _adminLock.Lock();

                // Find all "pending" sockets and signal them..
                index = _resources.begin();
                size_t capacity = _resources.capacity();
                uint32_t counter = 0;

                ::WSAResetEvent(_action);

                while (count != 0) {
                    count--;

                    ASSERT(index != _resources.end());

                    RESOURCE* entry = (*index);

                    if (entry != nullptr) {

                        WSANETWORKEVENTS networkEvents;

                        // Re-enable monitoring for the next round..
                        ::WSAEnumNetworkEvents(entry->Descriptor(), nullptr, &networkEvents);

                        uint16_t flagsSet = static_cast<uint16_t>(networkEvents.lNetworkEvents);

                        Arm();

                        // Event if the flagsSet == 0, call handle, maybe a break was issued by this RESOURCE..
                        entry->Handle(flagsSet);

                        Reset();
                    }
                    counter++;

                    if (capacity == _resources.capacity()) {
                        index++;
                    }
                    else {
                        uint32_t current = counter;
                        // vector's capacity was changed, which means its memory was reallocated and we have to adjust the index
                        index = _resources.begin();

                        while ((index != _resources.end()) && (current > 0)) {
                            index++;
                            current--;
                        }
                        capacity = _resources.capacity();
                    }
                }
            } else {
                delay = Core::infinite;
                _monitor->Block();
            }

            _adminLock.Unlock();

            return (delay);
        }
        #endif

    private:
        MonitorWorker* _monitor;
        mutable Core::CriticalSection _adminLock;
        Resources _resources;
        uint32_t _monitorRuns;
        string _name;
        WATCHDOG _watchDog;

        #ifdef __LINUX__
        uint32_t _descriptorArrayLength;
        struct ::pollfd* _descriptorArray;
        int _signalDescriptor;
        #endif

        #ifdef __WINDOWS__
        HANDLE _action;
        #endif

        #ifdef __APPLE__
        Core::NodeId _signalNode;
        #endif
    };

    #ifdef WATCHDOG_ENABLED
    class ResourceMonitorHandler {
    public:
        ResourceMonitorHandler(ResourceMonitorHandler&& rhs) = delete;
        ResourceMonitorHandler(const ResourceMonitorHandler& rhs) = delete;
        ResourceMonitorHandler& operator=(ResourceMonitorHandler&& rhs) = delete;
        ResourceMonitorHandler& operator=(const ResourceMonitorHandler& rhs) = delete;

        ResourceMonitorHandler() = default;
        ~ResourceMonitorHandler() = default;

    public:
        uint32_t Expired()
        {
            fprintf(stderr, "===> Resource monitor thread is taking too long.");
            return Core::infinite;
        }
    };

    using ResourceMonitorBase = ResourceMonitorType<IResource, WatchDogType<ResourceMonitorHandler>, 0, 32> ;
    #else
    using ResourceMonitorBase = ResourceMonitorType<IResource, Void, 0, 32>;
    #endif

    class EXTERNAL ResourceMonitor : public ResourceMonitorBase {
    private:
        friend SingletonType<ResourceMonitor>;

        ResourceMonitor() = default;

    public:
        ResourceMonitor(ResourceMonitor&&) = delete;
        ResourceMonitor(const ResourceMonitor&) = delete;
        ResourceMonitor& operator=(ResourceMonitor&&) = delete;
        ResourceMonitor& operator=(const ResourceMonitor&) = delete;

        static ResourceMonitor& Instance();

        ~ResourceMonitor() = default;
    };
}
} // namespace Thunder::Core
